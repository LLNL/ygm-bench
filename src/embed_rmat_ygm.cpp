// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <krowkee_helper.hpp>
#include <rmat_edge_generator.hpp>

struct rmat_edge_vec_generator_t {
  std::vector<std::pair<std::uint64_t, std::uint64_t>> operator()(
      ygm::comm &world, const parameters_t &params, const int seed) {
    std::vector<std::pair<uint64_t, uint64_t>> edges;
    edges.reserve(params.local_edge_count);

    distributed_rmat_edge_generator rmat(
        world, params.log_vertex_count, params.local_edge_count * world.size(),
        params.seed + seed, true, false, 0.25, 0.25, 0.25, 0.25);

    rmat.for_all([&edges](const auto first, const auto second) {
      edges.push_back({first, second});
    });

    return edges;
  }
};

int main(int argc, char **argv) {
  do_main<rmat_edge_vec_generator_t>(argc, argv);
  // int provided;
  // MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  // if (provided != MPI_THREAD_MULTIPLE) {
  //   throw std::runtime_error(
  //       "MPI_Init_thread: MPI_THREAD_MULTIPLE not provided.");
  // }

  // {
  //   parameters_t params = parse_args(argc, argv);

  //   ygm::comm world(MPI_COMM_WORLD, params.ygm_buffer_capacity);

  //   if (world.rank0()) {
  //     std::cout << world.layout().node_size() << ", "
  //               << world.layout().local_size() << ", "
  //               << params.ygm_buffer_capacity << ", "
  //               << world.routing_protocol() << ", " << params.range_size <<
  //               ", "
  //               << params.vertex_count << ", "
  //               << params.local_edge_count * world.size() << ", "
  //               << params.compaction_threshold << ", "
  //               << params.promotion_threshold << ", " << params.seed;
  //   }

  //   do_analysis<DistributedPromotable32CountSketch_t,
  //               rmat_edge_vec_generator_t>(world, params);
  // }

  // MPI_Finalize();
}
