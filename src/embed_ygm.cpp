// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <krowkee_helper.hpp>

#include <ygm/comm.hpp>

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
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  if (provided != MPI_THREAD_MULTIPLE) {
    throw std::runtime_error(
        "MPI_Init_thread: MPI_THREAD_MULTIPLE not provided.");
  }

  {
    parameters_t params = parse_args(argc, argv);

    ygm::comm world(MPI_COMM_WORLD, params.ygm_buffer_capacity);

    if (world.rank0()) {
      std::cout << world.layout().node_size() << ", "
                << world.layout().local_size() << ", "
                << params.ygm_buffer_capacity << ", "
                << world.routing_protocol() << ", " << params.range_size << ", "
                << params.vertex_count << ", "
                << params.local_edge_count * world.size() << ", "
                << params.compaction_threshold << ", "
                << params.promotion_threshold << ", " << params.seed;
    }

    do_analysis<DistributedPromotable32CountSketch_t,
                uniform_edge_vec_generator_t>(world, params);
  }

  MPI_Finalize();
}
