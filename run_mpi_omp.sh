#!/bin/bash
#SBATCH --job-name=your_job_name # Job name
#SBATCH --nodes=1                    # Run all processes on a single node	
#SBATCH --ntasks=20                   # number of processes = 20
#SBATCH --cpus-per-task=1            # Number of CPU cores allocated to each process
#SBATCH --partition=Project            # Partition name: Project or Debug (Debug is default)

mpirun -np 4 ./mpi_omp 10 100 4
mpirun -np 8 ./mpi_omp 10 100 4
mpirun -np 12 ./mpi_omp 10 100 4
mpirun -np 16 ./mpi_omp 10 100 4
mpirun -np 20 ./mpi_omp 10 100 4
mpirun -np 24 ./mpi_omp 10 100 4
mpirun -np 28 ./mpi_omp 10 100 4
mpirun -np 32 ./mpi_omp 10 100 4
mpirun -np 36 ./mpi_omp 10 100 4

#mpirun -np 4 ./mpi_omp 100 100 4
#mpirun -np 8 ./mpi_omp 100 100 4
#mpirun -np 12 ./mpi_omp 100 100 4
#mpirun -np 16 ./mpi_omp 100 100 4
#mpirun -np 20 ./mpi_omp 100 100 4
#mpirun -np 24 ./mpi_omp 100 100 4
#mpirun -np 28 ./mpi_omp 100 100 4
#mpirun -np 32 ./mpi_omp 100 100 4
#mpirun -np 36 ./mpi_omp 100 100 4

#mpirun -np 4 ./mpi_omp 1000 100 4
#mpirun -np 8 ./mpi_omp 1000 100 4
#mpirun -np 12 ./mpi_omp 1000 100 4
#mpirun -np 16 ./mpi_omp 1000 100 4
#mpirun -np 20 ./mpi_omp 1000 100 4
#mpirun -np 24 ./mpi_omp 1000 100 4
#mpirun -np 28 ./mpi_omp 1000 100 4
#mpirun -np 32 ./mpi_omp 1000 100 4
#mpirun -np 36 ./mpi_omp 1000 100 4

#mpirun -np 8 ./mpi_omp 1000 100 4
#mpirun -np 8 ./mpi_omp 1000 100 8
#mpirun -np 8 ./mpi_omp 1000 100 12
#mpirun -np 8 ./mpi_omp 1000 100 16
#mpirun -np 8 ./mpi_omp 1000 100 20
#mpirun -np 8 ./mpi_omp 1000 100 24
#mpirun -np 8 ./mpi_omp 1000 100 28
#mpirun -np 8 ./mpi_omp 1000 100 32
#mpirun -np 8 ./mpi_omp 1000 100 36
