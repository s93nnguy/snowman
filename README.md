# SNOWMAN

SNOWMAN is a small educational ray tracer used for the **High Performance Computing** lab.
It renders a snowman scene and is intended for experiments with MPI, OpenMP, scheduling,
tiling, and performance measurement.

The current version supports two MPI scheduling strategies in one executable:

- `static`: fixed row decomposition across MPI ranks.
- `dynamic`: tiled decomposition with a master/worker dynamic scheduler.

Both modes can use OpenMP inside each MPI rank.

## Build

On the HPC system, load MPI and GCC first:

```bash
module load GCC/13.3.0 OpenMPI/5.0.3-GCC-13.3.0
make
```

OpenMP is enabled by default through `-fopenmp`.

To build without OpenMP:

```bash
make OPENMP=0
```

Clean generated object files and executable:

```bash
make clean
```

## Run

The executable writes `output.ppm` and prints performance metrics.

### Static Row Scheduling

```bash
mpirun -np 4 ./snowman 800 3 static
```

This renders an `800 x 800` image with `3` snowmen using 4 MPI ranks. Each rank receives a
contiguous block of rows.

### Dynamic Tile Scheduling

```bash
mpirun -np 4 ./snowman 800 3 dynamic 64
```

This renders the same scene with dynamic tile scheduling and `64 x 64` tiles.

The old dynamic form is still accepted:

```bash
mpirun -np 4 ./snowman 800 3 64
```

## OpenMP Controls

Set the number of OpenMP threads per MPI process with `OMP_NUM_THREADS`:

```bash
OMP_NUM_THREADS=8 mpirun -np 4 ./snowman 800 3 dynamic 64
```

The renderer uses `collapse(2)` for the nested pixel loops by default. Disable it with:

```bash
SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=8 mpirun -np 4 ./snowman 800 3 static
```

Enable it explicitly with:

```bash
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=8 mpirun -np 4 ./snowman 800 3 dynamic 64
```

## Pinning Example

For hybrid MPI + OpenMP runs, pin MPI ranks and OpenMP threads to avoid oversubscription and
thread migration:

```bash
OMP_NUM_THREADS=12 \
OMP_PLACES=cores \
OMP_PROC_BIND=close \
mpirun -np 4 --report-bindings --map-by socket:PE=12 --bind-to core ./snowman 800 3 dynamic 64
```

Adjust `-np`, `OMP_NUM_THREADS`, and `PE` so that:

```text
MPI ranks * OpenMP threads <= allocated CPU cores
```

## Suggested Experiments

Compare:

- `static` vs `dynamic`
- tile sizes such as `16`, `32`, `64`, `128`
- `SNOWMAN_OMP_COLLAPSE=0` vs `SNOWMAN_OMP_COLLAPSE=1`
- different MPI rank counts
- different `OMP_NUM_THREADS` values
- pinned vs unpinned runs

The program reports:

- selected schedule
- image size
- number of snowmen
- MPI process count
- OpenMP thread count
- `collapse(2)` setting
- MPI thread support level
- min, max, and average runtime

## Contact

If you encounter issues or have suggestions, contact:

**Email:** kmanda@uni-bonn.de

## Acknowledgements

This project's foundational ray tracing algorithms, including ray-sphere intersection, basic
camera setup, and image output, are significantly inspired by the "Ray Tracing in One Weekend"
book series:

https://raytracing.github.io/
