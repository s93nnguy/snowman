# SNOWMAN

This raytracer project is currently under active development.
Functionality and output may change frequently.

It is designed for **educational purposes**, specifically for teaching concepts in **High Performance Computing (HPC)**.

### Contact
If you encounter issues or have suggestions, feel free to reach out:

**Email:** kmanda@uni-bonn.de

### Build Instructions:
- module load GCC/13.3.0 OpenMPI/5.0.3-GCC-13.3.0
- make

### Run
- mpirun -np 4 snowman 800 3
- The above command uses 4 processes and image size of 800 * 800 pixels with 3 snowmen in the snowfall.

### Acknowledgements
This project's foundational ray tracing algorithms, including ray-sphere intersection, basic camera setup, and image output, are significantly inspired by the "Ray Tracing in One Weekend" book series. This invaluable educational resource is freely available under the CC0 license, encouraging broad use and adaptation. https://raytracing.github.io/
