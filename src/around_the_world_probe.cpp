// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <mpi.h>
#include <cstdlib>
#include <iostream>

int main(int argc, char **argv) {
  int provided;
  // MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  MPI_Init(&argc, &argv);

  int num_trips  = atoi(argv[1]);
  int num_trials = atoi(argv[2]);

  int mpi_rank;
  int mpi_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  const char *num_nodes          = std::getenv("SLURM_JOB_NUM_NODES");
  const char *num_tasks_per_node = std::getenv("SLURM_NTASKS_PER_NODE");
  std::string num_nodes_str(num_nodes ? num_nodes : "");
  std::string num_tasks_per_node_str(num_tasks_per_node ? num_tasks_per_node
                                                        : "");

  if (mpi_rank == 0) {
    std::cout << num_nodes_str << ", " << num_tasks_per_node_str << ", "
              << num_trips;
  }

  for (int trial = 0; trial < num_trials; ++trial) {
    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();

    for (int i = 0; i < num_trips; ++i) {
      if (mpi_rank == 0) {
        MPI_Status status;

        MPI_Ssend(NULL, 0, MPI_BYTE, (mpi_rank + 1) % mpi_size, 0,
                  MPI_COMM_WORLD);
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        MPI_Recv(NULL, 0, MPI_BYTE,
                 status.MPI_SOURCE /*(mpi_rank + mpi_size - 1) % mpi_size*/,
                 status.MPI_TAG /*0*/, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      } else {
        MPI_Status status;

        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        MPI_Recv(NULL, 0, MPI_BYTE,
                 status.MPI_SOURCE /*(mpi_rank + mpi_size - 1) % mpi_size*/,
                 status.MPI_TAG /*0*/, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Ssend(NULL, 0, MPI_BYTE, (mpi_rank + 1) % mpi_size, 0,
                  MPI_COMM_WORLD);
      }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double elapsed = MPI_Wtime() - start;

    if (mpi_rank == 0) {
      auto total_hops = num_trips * mpi_size;
      std::cout << ", " << elapsed << ", " << total_hops / elapsed;
    }
  }

  if (mpi_rank == 0) {
    std::cout << std::endl;
  }

  MPI_Finalize();
  return 0;
}
