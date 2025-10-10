// This file is distributed under the MIT license.
// See the LICENSE file for details.


#ifndef SCENE_HPP
#define SCENE_HPP

#include <vector>
#include "utils.hpp"

// Basic sphere: center, radius, and color
struct Sphere {
    Vec3 center;
    double radius;
    Color color;

    Sphere(const Vec3& c, double r, const Color& col)
        : center(c), radius(r), color(col) {}
};

class Scene {
public:
    std::vector<Sphere> spheres;
    std::vector<Plane> planes;

    // Generate N snowmen evenly spaced in the scene
    void generate_snowmen(int count);
};

#endif

