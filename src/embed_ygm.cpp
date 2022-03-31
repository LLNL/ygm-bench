// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <krowkee_helper.hpp>

#include <random>

struct uniform_edge_vec_generator_t {
  std::vector<std::pair<std::uint64_t, std::uint64_t>> operator()(
      ygm::comm &world, const parameters_t &params, const int seed) {
    std::mt19937                            gen(params.seed + seed);
    std::uniform_int_distribution<uint64_t> dist(0, params.vertex_count - 1);

    std::vector<std::pair<uint64_t, uint64_t>> edges(params.local_edge_count);

    for (std::int64_t i(0); i < params.local_edge_count; ++i) {
      edges[i] = {dist(gen), dist(gen)};
    }
    return edges;
  }
};

int main(int argc, char **argv) {
  do_main<uniform_edge_vec_generator_t>(argc, argv);
}
