// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <fstream>
#include <string>
#include <ygm/comm.hpp>
#include <ygm/detail/stats_tracker.hpp>

template <typename T>
std::vector<std::vector<T>> gather_vectors_rank_0(
    ygm::comm &world, const std::vector<T> &local_vec) {
  std::vector<std::vector<T>> to_return;

  if (world.rank0()) {
    to_return.resize(world.size());
  }

  auto to_return_ptr = world.make_ygm_ptr(to_return);

  world.barrier();

  auto gather_lambda = [](const std::vector<T> &local_vec, const int rank,
                          auto global_vec_ptr) {
    (*global_vec_ptr)[rank] = local_vec;
  };

  world.async(0, gather_lambda, local_vec, world.rank(), to_return_ptr);

  world.barrier();

  return to_return;
}

template <typename T>
void write_stats_vec_file(const std::vector<T> &stats_vec,
                          const std::string &   filename) {
  std::ofstream ofs(filename);

  ofs << "Source/Dest";
  for (int i = 0; i < stats_vec.size(); ++i) {
    ofs << "," << i;
  }

  for (int i = 0; i < stats_vec.size(); ++i) {
    ofs << "\n" << i;
    for (const auto &val : stats_vec[i]) {
      ofs << "," << val;
    }
  }
}

void write_stats_files(ygm::comm &                       world,
                       const ygm::detail::stats_tracker &stats,
                       const std::string &               filename_prefix) {
  auto global_destination_message_bytes =
      gather_vectors_rank_0(world, stats.get_destination_message_bytes());
  auto global_destination_header_bytes =
      gather_vectors_rank_0(world, stats.get_destination_header_bytes());
  auto global_mpi_bytes = gather_vectors_rank_0(world, stats.get_mpi_bytes());
  auto global_mpi_sends = gather_vectors_rank_0(world, stats.get_mpi_sends());

  if (world.rank0()) {
    std::string message_bytes_filename = filename_prefix + "_message_bytes";
    write_stats_vec_file(global_destination_message_bytes,
                         message_bytes_filename);

    std::string header_bytes_filename = filename_prefix + "_header_bytes";
    write_stats_vec_file(global_destination_header_bytes,
                         header_bytes_filename);

    std::string mpi_bytes_filename = filename_prefix + "_mpi_bytes";
    write_stats_vec_file(global_mpi_bytes, mpi_bytes_filename);

    std::string mpi_sends_filename = filename_prefix + "_mpi_sends";
    write_stats_vec_file(global_mpi_sends, mpi_sends_filename);
  }
}
