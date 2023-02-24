// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <fstream>
#include <string>
#include <ygm/comm.hpp>

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

void write_stats_files(ygm::comm &world, const std::string &experiment_name,
                       std::ostream &os) {
  world.stats_print(experiment_name, os);
}
