// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <mpi.h>

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);

  MPI_Finalize();
  return 0;
}
