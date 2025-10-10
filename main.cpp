// This file is distributed under the MIT license.
// See the LICENSE file for details.

/*
  SNOWMAN is currently under active development.
  Features, functionality, and output may change frequently.

  It is created for teaching purposes as part of an HPC (High Performance Computing) course.

  If you encounter any issues feel free to reach out:

  Contact: kmanda@uni-bonn.de.com
*/

#include <mpi.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include "raytracer.hpp"
#include "scene.hpp"

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        if (rank == 0) {
            std::cout << "Usage: " << argv[0] << " <image_size> <num_snowmen>\n";
        }
        MPI_Finalize();
        return 1;
    }

    int image_size = std::stoi(argv[1]);
    int num_snowmen = std::stoi(argv[2]);

    // Scene generation and RayTracer setup
    Scene scene;
    scene.generate_snowmen(num_snowmen);

    RayTracer raytracer(image_size, image_size);
    raytracer.set_scene(&scene);

    std::vector<Color> local_pixels;

    // Local Computation Time using std::chrono
    auto compute_start_time = std::chrono::high_resolution_clock::now();
    raytracer.render(rank, size, local_pixels);
    auto compute_end_time = std::chrono::high_resolution_clock::now();

    // Calculate duration in seconds
    std::chrono::duration<double> local_compute_duration = compute_end_time - compute_start_time;
    double local_compute_time = local_compute_duration.count();

    // Convert local_pixels to local_buf for MPI communication
    std::vector<unsigned char> local_buf(local_pixels.size() * 3);
    for (size_t i = 0; i < local_pixels.size(); ++i) {
        local_buf[3*i + 0] = local_pixels[i].r;
        local_buf[3*i + 1] = local_pixels[i].g;
        local_buf[3*i + 2] = local_pixels[i].b;
    }

    int rows_per_rank = image_size / size;
    int local_rows = (rank == size - 1) ? (image_size - rows_per_rank * (size - 1)) : rows_per_rank;
    int local_count = local_rows * image_size * 3; // 3 bytes per pixel

    std::vector<int> recvcounts(size);
    std::vector<int> displs(size);

    for (int i = 0; i < size; ++i) {
        int rows = (i == size -1) ? (image_size - rows_per_rank * (size - 1)) : rows_per_rank;
        recvcounts[i] = rows * image_size * 3;
        displs[i] = (i == 0) ? 0 : displs[i-1] + recvcounts[i-1];
    }

    std::vector<unsigned char> full_buf;
    if (rank == 0) {
        full_buf.resize(image_size * image_size * 3);
    }

    // MPI_Gatherv communication
    MPI_Gatherv(local_buf.data(), local_count, MPI_UNSIGNED_CHAR,
                full_buf.data(), recvcounts.data(), displs.data(), MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);

    // Image saving
    if (rank == 0) {
        // Convert full buffer to vector<Color>
        std::vector<Color> full_pixels(image_size * image_size);
        for (int i = 0; i < image_size * image_size; ++i) {
            full_pixels[i] = Color(full_buf[3*i], full_buf[3*i + 1], full_buf[3*i + 2]);
        }

        raytracer.save_image("output.ppm", full_pixels);
        std::cout << "Image saved to output.ppm\n";
    }

    // Reporting Performance Metrics for Computation
    double max_local_compute_time;
    MPI_Reduce(&local_compute_time, &max_local_compute_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    double min_local_compute_time;
    MPI_Reduce(&local_compute_time, &min_local_compute_time, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);

    double sum_local_compute_time;
    MPI_Reduce(&local_compute_time, &sum_local_compute_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double avg_local_compute_time = sum_local_compute_time / size;

        std::cout << "\n--- Computational Performance Metrics ---\n";
        std::cout << "Image Size: " << image_size << ", Num Snowmen: " << num_snowmen << ", MPI Processes: " << size << "\n";
        std::cout << "Max Local Computation Time (across all ranks): " << max_local_compute_time << " seconds\n";
        std::cout << "Min Local Computation Time (across all ranks): " << min_local_compute_time << " seconds\n";
        std::cout << "Avg Local Computation Time (across all ranks): " << avg_local_compute_time << " seconds\n";
    }

    MPI_Finalize();
    return 0;
}
