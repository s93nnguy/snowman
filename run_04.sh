#!/bin/bash

#SBATCH --nodes=1
#SBATCH --partition=intelsr_devel
#SBATCH --account=tmp_hpca_workshop
#SBATCH --ntasks=96
#SBATCH --cpus-per-task=1
#SBATCH --time=01:00:00
#SBATCH --job-name=exercise04_snowman
#SBATCH --output=output/exercise_04/snowman_%j.out

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

export OMP_PLACES=cores
export OMP_PROC_BIND=close

# SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=1 perf-report --output=${OUT}/baseline_static_01p_01t mpirun -n 1 ./snowman 1024 4 static
# SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=1 perf-report --output=${OUT}/baseline_dynamic_01p_01t_064s mpirun -n 1 ./snowman 1024 4 dynamic 64

# static mode: with and without collapse(2)
# SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=8 perf-report --output=${OUT}/static_04p_08t_c0 mpirun -n 4 --map-by slot:PE=8 --bind-to core ./snowman 1024 4 static
# SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=8 perf-report --output=${OUT}/static_04p_08t_c1 mpirun -n 4 --map-by slot:PE=8 --bind-to core ./snowman 1024 4 static

# dynamic mode: with and without collapse(2)
# SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=8 perf-report --output=${OUT}/dynamic_04p_08t_064s_c0 mpirun -n 4 --map-by slot:PE=8 --bind-to core ./snowman 1024 4 dynamic 64
# SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=8 perf-report --output=${OUT}/dynamic_04p_08t_064s_c1 mpirun -n 4 --map-by slot:PE=8 --bind-to core ./snowman 1024 4 dynamic 64

# tile-size comparison
# SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=12 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/dynamic_04p_12t_008s mpirun -n 4 --report-bindings --bind-to numa ./snowman 1024 4 dynamic 8 >> ./logs/ex04/collapse_8.out
# SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=12 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/dynamic_04p_12t_016s mpirun -n 4 --report-bindings --bind-to numa ./snowman 1024 4 dynamic 16 >> ./logs/ex04/collapse_16.out
SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=12 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/cdynamic_04p_12t_032s mpirun -n 4 --report-bindings --bind-to numa ./snowman 1024 4 dynamic 32 >> ./logs/ex04/collapse_32.out
# SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=12 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/dynamic_04p_12t_064s mpirun -n 4 --report-bindings --bind-to numa ./snowman 1024 4 dynamic 64 >> ./logs/ex04/collapse_64.out

# SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=12 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/dynamic_04p_12t_008s mpirun -n 4 --report-bindings --bind-to numa ./snowman 1024 4 dynamic 8 >> ./logs/ex04/no_collapse_08.out
# SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=12 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/dynamic_04p_12t_016s mpirun -n 4 --report-bindings --bind-to numa ./snowman 1024 4 dynamic 16 >> ./logs/ex04/no_collapse_16.out
SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=12 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/dynamic_04p_12t_032s mpirun -n 4 --report-bindings --bind-to numa ./snowman 1024 4 dynamic 32 >> ./logs/ex04/no_collapse_32.out
# SNOWMAN_OMP_COLLAPSE=0 OMP_NUM_THREADS=12 OMP_DISPLAY_AFFINITY=True OMP_PLACES=cores OMP_PROC_BIND=close perf-report --output=${OUT}/dynamic_04p_12t_064s mpirun -n 4 --report-bindings --bind-to numa ./snowman 1024 4 dynamic 64 >> ./logs/ex04/no_collapse_64.out

# MPI/OpenMP balance with 48 total cores
# SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=12 perf-report --output=${OUT}/dynamic_04p_12t_064s mpirun -n 4 --map-by slot:PE=12 --bind-to core ./snowman 1024 4 dynamic 64
# SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=6 perf-report --output=${OUT}/dynamic_08p_06t_064s mpirun -n 8 --map-by slot:PE=6 --bind-to core ./snowman 1024 4 dynamic 64
# SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=4 perf-report --output=${OUT}/dynamic_12p_04t_064s mpirun -n 12 --map-by slot:PE=4 --bind-to core ./snowman 1024 4 dynamic 64
# SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=2 perf-report --output=${OUT}/dynamic_24p_02t_064s mpirun -n 24 --map-by slot:PE=2 --bind-to core ./snowman 1024 4 dynamic 64
# SNOWMAN_OMP_COLLAPSE=1 OMP_NUM_THREADS=1 perf-report --output=${OUT}/dynamic_48p_01t_064s mpirun -n 48 --map-by slot:PE=1 --bind-to core ./snowman 1024 4 dynamic 64
