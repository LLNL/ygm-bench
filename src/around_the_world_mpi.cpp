// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <mpi.h>
#include <cstdlib>
#include <iostream>

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);

  int num_trips = atoi(argv[1]);

  int mpi_rank;
  int mpi_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  MPI_Barrier(MPI_COMM_WORLD);
  double start = MPI_Wtime();

  for (int i = 0; i < num_trips; ++i) {
    if (mpi_rank == 0) {
      MPI_Ssend(NULL, 0, MPI_BYTE, (mpi_rank + 1) % mpi_size, 0,
                MPI_COMM_WORLD);
      // std::cout << "Rank " << mpi_rank << " sent" << std::endl;
      MPI_Recv(NULL, 0, MPI_BYTE, (mpi_rank + mpi_size - 1) % mpi_size, 0,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      // std::cout << "Rank " << mpi_rank << " received" << std::endl;
    } else {
      MPI_Recv(NULL, 0, MPI_BYTE, (mpi_rank + mpi_size - 1) % mpi_size, 0,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      // std::cout << "Rank " << mpi_rank << " received" << std::endl;
      MPI_Ssend(NULL, 0, MPI_BYTE, (mpi_rank + 1) % mpi_size, 0,
                MPI_COMM_WORLD);
      // std::cout << "Rank " << mpi_rank << " sent" << std::endl;
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);
  double elapsed = MPI_Wtime() - start;

  if (mpi_rank == 0) {
    std::cout << "Went around the world " << num_trips << " times in "
              << elapsed
              << " seconds\nAverage trip time: " << elapsed / num_trips
              << std::endl;
  }

  MPI_Finalize();
  return 0;
}
