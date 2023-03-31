// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <mpi.h>
#include <cstdlib>
#include <iostream>
#include <utility.hpp>

#include <boost/json/src.hpp>

struct parameters_t {
  int  num_trips;
  int  num_trials;
  bool pretty_print;

  parameters_t() : num_trips(1000), num_trials(5), pretty_print(false) {}
};

void usage(int mpi_rank) {
  if (mpi_rank == 0) {
    std::cerr << "around_the_world_mpi usage:"
              << "\n\t-n <int>\t- Number of trips around the world"
              << "\n\t-t <int>\t- Number of trials"
              << "\n\t-p\t\t- Pretty print output"
              << "\n\t-h\t\t- Print help" << std::endl;
  }
}

parameters_t parse_cmd_line(int argc, char **argv, int mpi_rank) {
  parameters_t params;
  int          c;
  bool         prn_help = false;

  // Suppress error messages from getopt
  extern int opterr;
  opterr = 0;

  while ((c = getopt(argc, argv, "n:t:ph")) != -1) {
    switch (c) {
      case 'h':
        prn_help = true;
        break;
      case 'n':
        params.num_trips = atoi(optarg);
        break;
      case 't':
        params.num_trials = atoi(optarg);
        break;
      case 'p':
        params.pretty_print = true;
        break;
      default:
        if (mpi_rank == 0) {
          std::cerr << "Unrecognized option: " << char(optopt) << std::endl;
        }
        prn_help = true;
        break;
    }
  }

  if (prn_help) {
    usage(mpi_rank);
    exit(-1);
  }

  return params;
}

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);

  int mpi_rank;
  int mpi_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  parameters_t params = parse_cmd_line(argc, argv, mpi_rank);

  const char *num_nodes_cstr          = std::getenv("SLURM_JOB_NUM_NODES");
  const char *num_tasks_per_node_cstr = std::getenv("SLURM_NTASKS_PER_NODE");
  std::string num_nodes_str(num_nodes_cstr ? num_nodes_cstr : "");
  std::string num_tasks_per_node_str(
      num_tasks_per_node_cstr ? num_tasks_per_node_cstr : "");
  int num_nodes = std::stoi(num_nodes_str);
  int num_tasks_per_node;
  if (num_tasks_per_node_str == "") {
    num_tasks_per_node = mpi_size / num_nodes;
  } else {
    num_tasks_per_node = std::stoi(num_tasks_per_node_str);
  }

  boost::json::object output;

  output["NAME"]           = "ATW_MPI";
  output["TIME"]           = boost::json::array();
  output["HOPS_PER_SEC"]   = boost::json::array();
  output["COMM_SIZE"]      = mpi_size;
  output["RANKS_PER_NODE"] = num_tasks_per_node;  //
  output["NUM_NODES"]      = num_nodes;

  auto total_hops      = params.num_trips * mpi_size;
  output["NUM_TRIPS"]  = params.num_trips;
  output["TOTAL_HOPS"] = total_hops;

  for (int trial = 0; trial < params.num_trials; ++trial) {
    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();

    for (int i = 0; i < params.num_trips; ++i) {
      if (mpi_rank == 0) {
        MPI_Ssend(NULL, 0, MPI_BYTE, (mpi_rank + 1) % mpi_size, 0,
                  MPI_COMM_WORLD);
        MPI_Recv(NULL, 0, MPI_BYTE, (mpi_rank + mpi_size - 1) % mpi_size, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      } else {
        MPI_Recv(NULL, 0, MPI_BYTE, (mpi_rank + mpi_size - 1) % mpi_size, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Ssend(NULL, 0, MPI_BYTE, (mpi_rank + 1) % mpi_size, 0,
                  MPI_COMM_WORLD);
      }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double elapsed = MPI_Wtime() - start;

    output["TIME"].as_array().emplace_back(elapsed);
    output["HOPS_PER_SEC"].as_array().emplace_back(total_hops / elapsed);
  }

  if (mpi_rank == 0) {
    if (params.pretty_print) {
      pretty_print(std::cout, output);
      std::cout << "\n";
    } else {
      std::cout << output << std::endl;
    }
  }

  MPI_Finalize();
  return 0;
}
