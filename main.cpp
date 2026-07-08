#include <mpi.h>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "raytracer.hpp"
#include "scene.hpp"
#include "utils.hpp"

const int TAG_WORK = 1;
const int TAG_DONE = 2;
const int TAG_STOP = 4;

enum class ScheduleMode {
    StaticRows,
    DynamicTiles
};

bool use_collapse_two() {
    const char* value = std::getenv("SNOWMAN_OMP_COLLAPSE");
    if (!value) {
        return true;
    }

    std::string mode(value);
    return !(mode == "0" || mode == "false" || mode == "FALSE" || mode == "off" || mode == "OFF");
}

int openmp_threads() {
#ifdef _OPENMP
    return omp_get_max_threads();
#else
    return 1;
#endif
}

std::string mpi_thread_support_name(int support) {
    switch (support) {
        case MPI_THREAD_SINGLE:
            return "single";
        case MPI_THREAD_FUNNELED:
            return "funneled";
        case MPI_THREAD_SERIALIZED:
            return "serialized";
        case MPI_THREAD_MULTIPLE:
            return "multiple";
        default:
            return "unknown";
    }
}

bool is_integer(const std::string& value) {
    if (value.empty()) {
        return false;
    }

    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isdigit(c);
    });
}

ScheduleMode parse_schedule_mode(const std::string& value) {
    if (value == "static" || value == "row" || value == "rows") {
        return ScheduleMode::StaticRows;
    }

    if (value == "dynamic" || value == "tile" || value == "tiles") {
        return ScheduleMode::DynamicTiles;
    }

    throw std::invalid_argument("schedule mode must be static or dynamic");
}

std::string schedule_mode_name(ScheduleMode mode) {
    return mode == ScheduleMode::StaticRows ? "static rows" : "dynamic tiles";
}

std::vector<Tile> make_tiles(int width, int height, int tile_size) {
    int tiles_x = (width + tile_size - 1) / tile_size;
    int tiles_y = (height + tile_size - 1) / tile_size;

    std::vector<Tile> tiles(tiles_x * tiles_y);

    #pragma omp parallel for collapse(2)
    for (int ty = 0; ty < tiles_y; ty++) {
        for (int tx = 0; tx < tiles_x; tx++) {
            int x = tx * tile_size;
            int y = ty * tile_size;
            int id = ty * tiles_x + tx;

            tiles[id] = {
                x,
                y,
                std::min(tile_size, width - x),
                std::min(tile_size, height - y),
                id
            };
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
    #pragma omp parallel for collapse(2)
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

    #pragma omp parallel for
    for (size_t i = 0; i < pixels.size(); i++) {
        buf[3 * i + 0] = pixels[i].r;
        buf[3 * i + 1] = pixels[i].g;
        buf[3 * i + 2] = pixels[i].b;
    }

    return buf;
}

std::vector<Color> bytes_to_colors(const std::vector<unsigned char>& buf) {
    std::vector<Color> pixels(buf.size() / 3);

    #pragma omp parallel for
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

    MPI_Send(meta, 5, MPI_INT, dest, TAG_WORK, MPI_COMM_WORLD);
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

    MPI_Send(dummy, 5, MPI_INT, dest, TAG_STOP, MPI_COMM_WORLD);
}

void print_common_report(
    int image_size,
    int num_snowmen,
    int mpi_size,
    int mpi_thread_support
) {
    std::cout << "Image Size: " << image_size << " x " << image_size << "\n";
    std::cout << "Num Snowmen: " << num_snowmen << "\n";
    std::cout << "MPI Processes: " << mpi_size << "\n";
    std::cout << "OpenMP Threads per Process: " << openmp_threads() << "\n";
    std::cout << "OpenMP Collapse(2): " << (use_collapse_two() ? "enabled" : "disabled") << "\n";
    std::cout << "MPI Thread Support: " << mpi_thread_support_name(mpi_thread_support) << "\n";
}

void run_static_rows(
    int rank,
    int size,
    int image_size,
    int num_snowmen,
    int mpi_thread_support,
    RayTracer& raytracer
) {
    std::vector<Color> local_pixels;

    MPI_Barrier(MPI_COMM_WORLD);
    auto start_time = std::chrono::high_resolution_clock::now();

    raytracer.render(rank, size, local_pixels);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;
    double local_time = duration.count();

    std::vector<unsigned char> local_buf = colors_to_bytes(local_pixels);

    int rows_per_rank = image_size / size;
    int local_rows = (rank == size - 1) ? (image_size - rows_per_rank * (size - 1)) : rows_per_rank;
    int local_count = local_rows * image_size * 3;

    std::vector<int> recvcounts(size);
    std::vector<int> displs(size);

    for (int i = 0; i < size; ++i) {
        int rows = (i == size - 1) ? (image_size - rows_per_rank * (size - 1)) : rows_per_rank;
        recvcounts[i] = rows * image_size * 3;
        displs[i] = (i == 0) ? 0 : displs[i - 1] + recvcounts[i - 1];
    }

    std::vector<unsigned char> full_buf;
    if (rank == 0) {
        full_buf.resize(image_size * image_size * 3);
    }

    MPI_Gatherv(
        local_buf.data(),
        local_count,
        MPI_UNSIGNED_CHAR,
        rank == 0 ? full_buf.data() : nullptr,
        recvcounts.data(),
        displs.data(),
        MPI_UNSIGNED_CHAR,
        0,
        MPI_COMM_WORLD
    );

    double max_time;
    double min_time;
    double sum_time;

    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_time, &min_time, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_time, &sum_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::vector<Color> full_pixels = bytes_to_colors(full_buf);

        raytracer.save_image("output.ppm", full_pixels);
        std::cout << "Image saved to output.ppm\n";

        std::cout << "\n--- Static Row Scheduling Performance ---\n";
        print_common_report(image_size, num_snowmen, size, mpi_thread_support);
        std::cout << "Rows per Rank: " << rows_per_rank << " plus remainder on last rank\n";
        std::cout << "Max Time: " << max_time << " seconds\n";
        std::cout << "Min Time: " << min_time << " seconds\n";
        std::cout << "Avg Time: " << sum_time / size << " seconds\n";
    }
}

void run_dynamic_tiles(
    int rank,
    int size,
    int image_size,
    int num_snowmen,
    int tile_size,
    int mpi_thread_support,
    RayTracer& raytracer
) {
    std::vector<Tile> tiles = make_tiles(image_size, image_size, tile_size);
    std::vector<Color> full_pixels(image_size * image_size, Color(0, 0, 0));

    int local_tiles_rendered = 0;

    MPI_Barrier(MPI_COMM_WORLD);
    auto start_time = std::chrono::high_resolution_clock::now();

    if (size == 1) {
        for (const auto& tile : tiles) {
            std::vector<Color> tile_pixels;
            raytracer.render_tile(tile, tile_pixels);
            copy_tile(tile, tile_pixels, full_pixels, image_size);
            local_tiles_rendered++;
        }
    } else if (rank == 0) {
        int next_tile = 0;
        int completed_tiles = 0;

        for (int worker = 1; worker < size; worker++) {
            if (next_tile < static_cast<int>(tiles.size())) {
                send_tile(worker, tiles[next_tile]);
                next_tile++;
            } else {
                send_stop(worker);
            }
        }

        while (completed_tiles < static_cast<int>(tiles.size())) {
            int done;
            MPI_Status status;

            MPI_Recv(&done, 1, MPI_INT, MPI_ANY_SOURCE, TAG_DONE, MPI_COMM_WORLD, &status);

            int worker = status.MPI_SOURCE;
            completed_tiles++;

            if (next_tile < static_cast<int>(tiles.size())) {
                send_tile(worker, tiles[next_tile]);
                next_tile++;
            } else {
                send_stop(worker);
            }
        }
    } else {
        while (true) {
            MPI_Status status;
            Tile tile = recv_tile_from_master(status);

            if (status.MPI_TAG == TAG_STOP) {
                break;
            }

            std::vector<Color> tile_pixels;
            raytracer.render_tile(tile, tile_pixels);

            copy_tile(tile, tile_pixels, full_pixels, image_size);
            local_tiles_rendered++;

            int done = 1;
            MPI_Send(&done, 1, MPI_INT, 0, TAG_DONE, MPI_COMM_WORLD);
        }
    }

    std::vector<unsigned char> local_bytes = colors_to_bytes(full_pixels);
    std::vector<unsigned char> full_bytes;

    if (rank == 0) {
        full_bytes.resize(image_size * image_size * 3);
    }

    MPI_Reduce(
        local_bytes.data(),
        rank == 0 ? full_bytes.data() : nullptr,
        image_size * image_size * 3,
        MPI_UNSIGNED_CHAR,
        MPI_MAX,
        0,
        MPI_COMM_WORLD
    );

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;
    double local_time = duration.count();

    std::vector<double> all_times;
    std::vector<int> all_tiles;

    if (rank == 0) {
        all_times.resize(size);
        all_tiles.resize(size);
    }

    MPI_Gather(&local_time, 1, MPI_DOUBLE, rank == 0 ? all_times.data() : nullptr, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(&local_tiles_rendered, 1, MPI_INT, rank == 0 ? all_tiles.data() : nullptr, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::vector<Color> final_pixels = bytes_to_colors(full_bytes);

        raytracer.save_image("output.ppm", final_pixels);
        std::cout << "Image saved to output.ppm\n";

        double max_time = *std::max_element(all_times.begin(), all_times.end());
        double min_time = *std::min_element(all_times.begin(), all_times.end());

        double sum_time = 0.0;
        int total_tiles_rendered = 0;

        for (int i = 0; i < size; i++) {
            sum_time += all_times[i];
            total_tiles_rendered += all_tiles[i];
        }

        std::cout << "\n--- Dynamic Tile Scheduling Performance ---\n";
        print_common_report(image_size, num_snowmen, size, mpi_thread_support);
        std::cout << "Tile Size: " << tile_size << " x " << tile_size << "\n";
        std::cout << "Total Tiles: " << tiles.size() << "\n";
        std::cout << "Rendered Tiles: " << total_tiles_rendered << "\n";
        std::cout << "Max Time: " << max_time << " seconds\n";
        std::cout << "Min Time: " << min_time << " seconds\n";
        std::cout << "Avg Time: " << sum_time / size << " seconds\n";
    }
}

void print_usage(const char* program) {
    std::cout << "Usage:\n"
              << "  " << program << " <image_size> <num_snowmen> static\n"
              << "  " << program << " <image_size> <num_snowmen> dynamic <tile_size>\n"
              << "  " << program << " <image_size> <num_snowmen> <tile_size>    # legacy dynamic mode\n";
}

int main(int argc, char* argv[]) {
    int mpi_thread_support = MPI_THREAD_SINGLE;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &mpi_thread_support);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 4) {
        if (rank == 0) {
            print_usage(argv[0]);
        }

        MPI_Finalize();
        return 1;
    }

    int image_size = std::stoi(argv[1]);
    int num_snowmen = std::stoi(argv[2]);
    ScheduleMode schedule_mode = ScheduleMode::DynamicTiles;
    int tile_size = 0;

    try {
        std::string mode_arg(argv[3]);
        if (is_integer(mode_arg)) {
            schedule_mode = ScheduleMode::DynamicTiles;
            tile_size = std::stoi(mode_arg);
        } else {
            schedule_mode = parse_schedule_mode(mode_arg);
            if (schedule_mode == ScheduleMode::DynamicTiles) {
                if (argc < 5) {
                    throw std::invalid_argument("dynamic mode requires <tile_size>");
                }
                tile_size = std::stoi(argv[4]);
            }
        }
    } catch (const std::exception& error) {
        if (rank == 0) {
            std::cerr << "Invalid arguments: " << error.what() << "\n";
            print_usage(argv[0]);
        }

        MPI_Finalize();
        return 1;
    }

    if (schedule_mode == ScheduleMode::DynamicTiles && tile_size <= 0) {
        if (rank == 0) {
            std::cerr << "Invalid arguments: tile_size must be positive\n";
            print_usage(argv[0]);
        }

        MPI_Finalize();
        return 1;
    }

    Scene scene;
    scene.generate_snowmen(num_snowmen);

    RayTracer raytracer(image_size, image_size);
    raytracer.set_scene(&scene);

    if (rank == 0) {
        std::cout << "Selected Schedule: " << schedule_mode_name(schedule_mode) << "\n";
    }

    if (schedule_mode == ScheduleMode::StaticRows) {
        run_static_rows(rank, size, image_size, num_snowmen, mpi_thread_support, raytracer);
    } else {
        run_dynamic_tiles(rank, size, image_size, num_snowmen, tile_size, mpi_thread_support, raytracer);
    }

    MPI_Finalize();
    return 0;
}
