#include <mpi.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>

#include "raytracer.hpp"
#include "scene.hpp"
#include "utils.hpp"

const int TAG_WORK = 1;
const int TAG_RESULT_META = 2;
const int TAG_RESULT_PIXELS = 3;
const int TAG_STOP = 4;

std::vector<Tile> make_tiles(int width, int height, int tile_size) {
    std::vector<Tile> tiles;
    int id = 0;

    for (int y = 0; y < height; y += tile_size) {
        for (int x = 0; x < width; x += tile_size) {
            tiles.push_back({
                x,
                y,
                std::min(tile_size, width - x),
                std::min(tile_size, height - y),
                id++
            });
        }
    }

    return tiles;
}

void copy_tile(
    const Tile& tile,
    const std::vector<Color>& tile_pixels,
    std::vector<Color>& full_pixels,
    int image_width
) {
    for (int ty = 0; ty < tile.h; ty++) {
        for (int tx = 0; tx < tile.w; tx++) {
            int tile_idx = ty * tile.w + tx;
            int global_x = tile.x0 + tx;
            int global_y = tile.y0 + ty;
            int full_idx = global_y * image_width + global_x;

            full_pixels[full_idx] = tile_pixels[tile_idx];
        }
    }
}

std::vector<unsigned char> colors_to_bytes(const std::vector<Color>& pixels) {
    std::vector<unsigned char> buf(pixels.size() * 3);

    for (size_t i = 0; i < pixels.size(); i++) {
        buf[3 * i + 0] = pixels[i].r;
        buf[3 * i + 1] = pixels[i].g;
        buf[3 * i + 2] = pixels[i].b;
    }

    return buf;
}

std::vector<Color> bytes_to_colors(const std::vector<unsigned char>& buf) {
    std::vector<Color> pixels(buf.size() / 3);

    for (size_t i = 0; i < pixels.size(); i++) {
        pixels[i] = Color(
            buf[3 * i + 0],
            buf[3 * i + 1],
            buf[3 * i + 2]
        );
    }

    return pixels;
}

void send_tile(int dest, const Tile& tile) {
    int meta[5] = {tile.x0, tile.y0, tile.w, tile.h, tile.id};

    MPI_Send(
        meta,
        5,
        MPI_INT,
        dest,
        TAG_WORK,
        MPI_COMM_WORLD
    );
}

Tile recv_tile_from_master(MPI_Status& status) {
    int meta[5];

    MPI_Recv(
        meta,
        5,
        MPI_INT,
        0,
        MPI_ANY_TAG,
        MPI_COMM_WORLD,
        &status
    );

    return Tile{meta[0], meta[1], meta[2], meta[3], meta[4]};
}

void send_stop(int dest) {
    int dummy[5] = {0, 0, 0, 0, -1};

    MPI_Send(
        dummy,
        5,
        MPI_INT,
        dest,
        TAG_STOP,
        MPI_COMM_WORLD
    );
}

void send_tile_result_to_master(
    const Tile& tile,
    const std::vector<Color>& tile_pixels
) {
    int meta[5] = {tile.x0, tile.y0, tile.w, tile.h, tile.id};

    MPI_Send(
        meta,
        5,
        MPI_INT,
        0,
        TAG_RESULT_META,
        MPI_COMM_WORLD
    );

    std::vector<unsigned char> buf = colors_to_bytes(tile_pixels);

    MPI_Send(
        buf.data(),
        static_cast<int>(buf.size()),
        MPI_UNSIGNED_CHAR,
        0,
        TAG_RESULT_PIXELS,
        MPI_COMM_WORLD
    );
}

Tile recv_tile_result_meta(int source) {
    int meta[5];

    MPI_Recv(
        meta,
        5,
        MPI_INT,
        source,
        TAG_RESULT_META,
        MPI_COMM_WORLD,
        MPI_STATUS_IGNORE
    );

    return Tile{meta[0], meta[1], meta[2], meta[3], meta[4]};
}

std::vector<Color> recv_tile_result_pixels(int source, const Tile& tile) {
    int count = tile.w * tile.h * 3;
    std::vector<unsigned char> buf(count);

    MPI_Recv(
        buf.data(),
        count,
        MPI_UNSIGNED_CHAR,
        source,
        TAG_RESULT_PIXELS,
        MPI_COMM_WORLD,
        MPI_STATUS_IGNORE
    );

    return bytes_to_colors(buf);
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 4) {
        if (rank == 0) {
            std::cout << "Usage: "
                      << argv[0]
                      << " <image_size> <num_snowmen> <tile_size>\n";
        }

        MPI_Finalize();
        return 1;
    }

    int image_size = std::stoi(argv[1]);
    int num_snowmen = std::stoi(argv[2]);
    int tile_size = std::stoi(argv[3]);

    auto tiles = make_tiles(image_size, image_size, tile_size);

    // Scene generation and RayTracer setup
    Scene scene;
    scene.generate_snowmen(num_snowmen);

    RayTracer raytracer(image_size, image_size);
    raytracer.set_scene(&scene);

    MPI_Barrier(MPI_COMM_WORLD);
    auto start_time = std::chrono::high_resolution_clock::now();

    int local_tiles_rendered = 0;

    if (size == 1) {
        std::vector<Color> full_pixels(image_size * image_size);

        for (const auto& tile : tiles) {
            std::vector<Color> tile_pixels;
            raytracer.render_tile(tile, tile_pixels);
            copy_tile(tile, tile_pixels, full_pixels, image_size);
            local_tiles_rendered++;
        }

        raytracer.save_image("output.ppm", full_pixels);
    }
    else if (rank == 0) {
        std::vector<Color> full_pixels(image_size * image_size);

        int next_tile = 0;
        int completed_tiles = 0;
        int active_workers = 0;

        for (int worker = 1; worker < size && next_tile < static_cast<int>(tiles.size()); worker++) {
            send_tile(worker, tiles[next_tile]);
            next_tile++;
            active_workers++;
        }

        while (completed_tiles < static_cast<int>(tiles.size())) {
            MPI_Status status;

            MPI_Probe(
                MPI_ANY_SOURCE,
                TAG_RESULT_META,
                MPI_COMM_WORLD,
                &status
            );

            int worker = status.MPI_SOURCE;

            Tile tile = recv_tile_result_meta(worker);
            std::vector<Color> tile_pixels = recv_tile_result_pixels(worker, tile);

            copy_tile(tile, tile_pixels, full_pixels, image_size);
            completed_tiles++;

            if (next_tile < static_cast<int>(tiles.size())) {
                send_tile(worker, tiles[next_tile]);
                next_tile++;
            } else {
                send_stop(worker);
                active_workers--;
            }
        }

        for (int worker = 1; worker < size; worker++) {
            if (worker >= active_workers + 1) {
                send_stop(worker);
            }
        }

        raytracer.save_image("output.ppm", full_pixels);
        std::cout << "Image saved to output.ppm\n";
    }
    else {
        while (true) {
            MPI_Status status;
            Tile tile = recv_tile_from_master(status);

            if (status.MPI_TAG == TAG_STOP) {
                break;
            }

            std::vector<Color> tile_pixels;
            raytracer.render_tile(tile, tile_pixels);

            send_tile_result_to_master(tile, tile_pixels);
            local_tiles_rendered++;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;
    double local_time = duration.count();

    double max_time;
    double min_time;
    double sum_time;

    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_time, &min_time, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_time, &sum_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    int total_tiles_rendered;
    MPI_Reduce(&local_tiles_rendered, &total_tiles_rendered, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "\n--- Dynamic Tile Scheduling Performance ---\n";
        std::cout << "Image Size: " << image_size << " x " << image_size << "\n";
        std::cout << "Num Snowmen: " << num_snowmen << "\n";
        std::cout << "Tile Size: " << tile_size << " x " << tile_size << "\n";
        std::cout << "MPI Processes: " << size << "\n";
        std::cout << "Total Tiles: " << tiles.size() << "\n";
        std::cout << "Rendered Tiles: " << total_tiles_rendered << "\n";
        std::cout << "Max Time: " << max_time << " seconds\n";
        std::cout << "Min Time: " << min_time << " seconds\n";
        std::cout << "Avg Time: " << sum_time / size << " seconds\n";
    }

    MPI_Finalize();
    return 0;
}