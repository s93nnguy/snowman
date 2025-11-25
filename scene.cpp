// This file is distributed under the MIT license.
// See the LICENSE file for details.

/*
  SNOWMAN is currently under active development.
  Features, functionality, and output may change frequently.

  It is created for teaching purposes as part of an HPC (High Performance Computing) course.

  If you encounter any issues feel free to reach out:

  Contact: kmanda@uni-bonn.de
*/

#include "scene.hpp"
#include <random>
#include <algorithm>
#include <cmath>

void Scene::generate_snowmen(int count) {
    spheres.clear();
    planes.clear(); // Clear existing planes too

    // Dimensions
    double base_radius = 1.2;
    double body_radius = 0.9;
    double head_radius = 0.5;

    double spacing = 4.0;
    double start_x = -((count - 1) * spacing) / 2.0;

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> offset_dist(-0.1, 0.1);

    for (int i = 0; i < count; ++i) {
        double x = start_x + i * spacing;
        double z = -10 + offset_dist(rng);  // Slight Z variation
        double x_offset = offset_dist(rng);

        // Stack: Head (top) → Body → Base
        double y_head = 3.0;
        double y_body = y_head - head_radius - body_radius;
        double y_base = y_body - body_radius - base_radius;

        Vec3 head(x + x_offset, y_head, z);
        Vec3 body(x + x_offset, y_body, z);
        Vec3 base(x + x_offset, y_base, z);

        Color snow(245, 245, 255);
        spheres.emplace_back(base, base_radius, snow);
        spheres.emplace_back(body, body_radius, snow);
        spheres.emplace_back(head, head_radius, snow);

        // Nose (carrot)
        Vec3 nose(head.x, head.y, head.z + head_radius + 0.12);
        spheres.emplace_back(nose, 0.12, Color(255, 128, 0)); // Larger, more visible

        // Eyes
        double eye_offset_x = 0.18;
        double eye_y = head.y + 0.1;
        double eye_z = head.z + head_radius + 0.1;
        spheres.emplace_back(Vec3(head.x - eye_offset_x, eye_y, eye_z), 0.1, Color(0, 0, 0));
        spheres.emplace_back(Vec3(head.x + eye_offset_x, eye_y, eye_z), 0.1, Color(0, 0, 0));

        // Buttons (middle snowball, not base)
        Color button_color(30, 30, 30);
        double button_z = body.z + body_radius + 0.1;
        double button_spacing = 0.25;
        for (int b = 0; b < 3; ++b) {
            double by = body.y + 0.3 - b * button_spacing;
            spheres.emplace_back(Vec3(body.x, by, button_z), 0.12, button_color);
        }

        // Hat (2 dark spheres)
        Vec3 hat_base(head.x, head.y + head_radius + 0.05, head.z);
        Vec3 hat_top(head.x, hat_base.y + 0.2, head.z);
        spheres.emplace_back(hat_base, 0.3, Color(15, 15, 15)); // brim
        spheres.emplace_back(hat_top, 0.2, Color(20, 20, 20));  // top

        // Ground plane (only once)
        if (i == 0) {
            planes.emplace_back(Vec3(0, 0, 0), Vec3(0, 1, 0), Color(245, 245, 245));
        }
    }
}
