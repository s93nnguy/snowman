#!/bin/bash

#SBATCH --nodes=1
#SBATCH --partition=intelsr_devel
#SBATCH --account=tmp_hpca_workshop
#SBATCH --ntasks=96
#SBATCH --cpus-per-task=1
#SBATCH --time=01:00:00
#SBATCH --job-name=exercise04_snowman
#SBATCH --output=output/exercise_04/snowman_%j_%t.out

unset SLURM_EXPORT_ENV

# load modules
module purge
module load GCC/13.2.0 OpenMPI/4.1.6-GCC-13.2.0 LinaroForge/25.0.3-GCCcore-13.2.0-linux-x86_64

# build
make clean
make

OUT=/home/s93nnguy_hpc/exercise/snowman/output/exercise_04
mkdir -p ${OUT}

# perf-report --output=${OUT}/static_01p mpirun -n 1  --map-by slot:PE=1 --bind-to core ./snowman 1024 4 static
# perf-report --output=${OUT}/static_02p mpirun -n 2  --map-by slot:PE=1 --bind-to core ./snowman 1024 4 static
# perf-report --output=${OUT}/static_04p mpirun -n 4  --map-by slot:PE=1 --bind-to core ./snowman 1024 4 static
# perf-report --output=${OUT}/static_08p mpirun -n 8  --map-by slot:PE=1 --bind-to core ./snowman 1024 4 static
# perf-report --output=${OUT}/static_16p mpirun -n 16 --map-by slot:PE=1 --bind-to core ./snowman 1024 4 static
# perf-report --output=${OUT}/static_32p mpirun -n 32 --map-by slot:PE=1 --bind-to core ./snowman 1024 4 static
# perf-report --output=${OUT}/static_48p mpirun -n 48 --map-by slot:PE=1 --bind-to core ./snowman 1024 4 static
# perf-report --output=${OUT}/static_96p mpirun -n 96 --map-by slot:PE=1 --bind-to core ./snowman 1024 4 static

SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=4  OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp04_dynamic_12p_04t_032s mpirun -n 12 --report-bindings --map-by node:PE=4 --bind-to core ./snowman 1024 4 dynamic 32
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=4  OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp04_static_12p_04t_000s mpirun -n 12 --report-bindings  --map-by node:PE=4 --bind-to core ./snowman 1024 4 static
# MPI only
# static origin
perf-report --output=${OUT}/exp00_static_48p_01t_000s_1c mpirun -n 48 ./snowman 1024 4 static

# dynamic + tile 
perf-report --output=${OUT}/exp00_dynamic_48p_01t_032s_1c mpirun -n 48 ./snowman 1024 4 dynamic 32

# hybrid 
# static + collapse(1)
SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=24 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_static_02p_24t_000s_1c mpirun -n 2  --report-bindings  --bind-to numa ./snowman 1024 4 static
SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=12 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_static_04p_12t_000s_1c mpirun -n 4  --report-bindings  --bind-to numa ./snowman 1024 4 static
SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=6  OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_static_08p_06t_000s_1c mpirun -n 8  --report-bindings  --map-by node:PE=6 --bind-to core ./snowman 1024 4 static
SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=4  OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_static_12p_04t_000s_1c mpirun -n 12 --report-bindings  --map-by node:PE=4 --bind-to core ./snowman 1024 4 static

# dynamic + collapse(1)
SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=24 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_02p_24t_000s_1c mpirun -n 2  --report-bindings  --bind-to numa ./snowman 1024 4 dynamic 32
SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=12 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_04p_12t_000s_1c mpirun -n 4  --report-bindings  --bind-to numa ./snowman 1024 4 dynamic 32
SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=6  OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_08p_06t_000s_1c mpirun -n 8  --report-bindings  --map-by node:PE=6 --bind-to core ./snowman 1024 4 dynamic 32
SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=4  OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_12p_04t_000s_1c mpirun -n 12 --report-bindings  --map-by node:PE=4 --bind-to core ./snowman 1024 4 dynamic 32

# static + collapse(2)
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=24 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_static_02p_24t_000s_2c mpirun -n 2  --report-bindings  --bind-to numa ./snowman 1024 4 static
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=12 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_static_04p_12t_000s_2c mpirun -n 4  --report-bindings  --bind-to numa ./snowman 1024 4 static
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=6  OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_static_08p_06t_000s_2c mpirun -n 8  --report-bindings  --map-by node:PE=6 --bind-to core ./snowman 1024 4 static
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=4  OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_static_12p_04t_000s_2c mpirun -n 12 --report-bindings  --map-by node:PE=4 --bind-to core ./snowman 1024 4 static

# dynamic + collapse(2)
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=24 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_02p_24t_000s_2c mpirun -n 2  --report-bindings  --bind-to numa ./snowman 1024 4 dynamic 32
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=12 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_04p_12t_000s_2c mpirun -n 4  --report-bindings  --bind-to numa ./snowman 1024 4 dynamic 32
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=6  OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_08p_06t_000s_2c mpirun -n 8  --report-bindings  --map-by node:PE=6 --bind-to core ./snowman 1024 4 dynamic 32
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=4  OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_12p_04t_000s_2c mpirun -n 12 --report-bindings  --map-by node:PE=4 --bind-to core ./snowman 1024 4 dynamic 32


# additional dynamic with 1 process for rank 0
# dynamic + collapse(1)
SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=24 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_03p_24t_000s_1c mpirun -n 3  --report-bindings  --bind-to numa ./snowman 1024 4 dynamic 32
SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=12 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_05p_12t_000s_1c mpirun -n 5  --report-bindings  --map-by node:PE=12 --bind-to core ./snowman 1024 4 dynamic 32
SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=6  OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_09p_06t_000s_1c mpirun -n 9  --report-bindings  --map-by node:PE=6  --bind-to core ./snowman 1024 4 dynamic 32
SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=4  OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_13p_04t_000s_1c mpirun -n 13 --report-bindings  --map-by node:PE=4  --bind-to core ./snowman 1024 4 dynamic 32

# dynamic + collapse(2)
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=24 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_03p_24t_000s_2c mpirun -n 3  --report-bindings  --bind-to numa ./snowman 1024 4 dynamic 32
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=12 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_05p_12t_000s_2c mpirun -n 5  --report-bindings  --map-by node:PE=12 --bind-to core ./snowman 1024 4 dynamic 32
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=6  OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_09p_06t_000s_2c mpirun -n 9  --report-bindings  --map-by node:PE=6  --bind-to core ./snowman 1024 4 dynamic 32
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=4  OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/exp00_dynamic_13p_04t_000s_2c mpirun -n 13 --report-bindings  --map-by node:PE=4  --bind-to core ./snowman 1024 4 dynamic 32