// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <krowkee/sketch/interface.hpp>
#include <krowkee/stream/interface.hpp>

#include <krowkee/util/cmap_types.hpp>
#include <krowkee/util/sketch_types.hpp>
#include <krowkee/util/ygm_tests.hpp>

#include <utility.hpp>

#include <ygm/comm.hpp>
#include <ygm/detail/ygm_cereal_archive.hpp>
#include <ygm/detail/ygm_ptr.hpp>
#include <ygm/utility.hpp>

#include <random>

using sketch_type_t = krowkee::util::sketch_type_t;
using cmap_type_t   = krowkee::util::cmap_type_t;

using DistributedPromotable32CountSketch_t = krowkee::stream::Distributed<
    krowkee::stream::Summary, krowkee::sketch::Sketch,
    krowkee::transform::CountSketchFunctor, krowkee::sketch::MapPromotable32,
    std::plus, std::uint64_t, std::int32_t, krowkee::hash::MulAddShift>;

struct parameters_t {
  int         range_size;
  size_t      vertex_count;
  size_t      local_edge_count;
  size_t      compaction_threshold;
  size_t      promotion_threshold;
  int         num_trials;
  uint32_t    seed;
  std::string stats_output_prefix;
};

template <typename DistributedType>
void do_analysis(ygm::comm &world, const parameters_t &params) {
  typedef DistributedType              dsk_t;
  typedef typename dsk_t::data_t       data_t;
  typedef typename data_t::sk_t        sk_t;
  typedef typename sk_t::container_t   con_t;
  typedef typename sk_t::sf_t          sf_t;
  typedef typename sk_t::sf_ptr_t      sf_ptr_t;
  typedef make_ygm_ptr_functor_t<sf_t> make_ygm_ptr_t;

  make_ygm_ptr_functor_t make_ygm_ptr = make_ygm_ptr_t();

  sf_ptr_t sf_ptr(make_ygm_ptr(params.vertex_count, params.seed));

  std::mt19937                            gen(world.rank());
  std::uniform_int_distribution<uint64_t> dist(0, params.vertex_count - 1);

  double total_update_time{0.0};

  world.stats_reset();

  for (int trial = 0; trial < params.num_trials; ++trial) {
    std::vector<std::pair<uint64_t, uint64_t>> edges(params.local_edge_count);

    for (int64_t i = 0; i < params.local_edge_count; ++i) {
      edges[i] = {dist(gen), dist(gen)};
    }

    dsk_t dsk(world, sf_ptr, params.compaction_threshold,
              params.promotion_threshold);

    // I don't think that this is really necessary. We can remove it later if
    // desired.
    data_t null_sketch{sf_ptr, params.compaction_threshold,
                       params.promotion_threshold};
    for (const auto out_vertex : vertex_set) {
      dsk.ygm_map().async_insert_if_missing(out_vertex, null_sketch);
    }

    world.barrier();

    world.set_track_stats(true);

    ygm::timer update_timer{};

    for (const auto &edge : edges) {
      dsk.async_update(edge.first, edge.second);
      // dsk.async_insert(edge.second, edge.first);
    }
    dsk.compactify();

    world.barrier();

    world.set_track_stats(false);

    double trial_time = update_timer.elapsed();

    if (world.rank0()) {
      double trial_rate = params.local_edge_count * world.size() / trial_time /
                          (1000 * 1000 * 1000);

      std::cout << ", " << trial_time << ", " << trial_rate;
    }

    total_update_time += trial_time;
  }

  uint64_t global_bytes = world.global_bytes_sent();
  uint64_t global_message_bytes =
      params.local_edge_count * world.size() * params.num_trials * 8;

  if (world.rank0()) {
    double average_rate = params.local_edge_count * world.size() *
                          params.num_trials / total_update_time /
                          (1000 * 1000 * 1000);

    std::cout << std::endl;
  }

  auto experiment_stats = world.stats_snapshot();
  write_stats_files(world, experiment_stats, params.stats_output_prefix);
}

int main(int argc, char **argv) {
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  if (provided != MPI_THREAD_MULTIPLE) {
    throw std::runtime_error(
        "MPI_Init_thread: MPI_THREAD_MULTIPLE not provided.");
  }

  {
    int ygm_buffer_capacity = atoi(argv[1]);

    ygm::comm world(MPI_COMM_WORLD, ygm_buffer_capacity);

    int         range_size(atoi(argv[2]));
    int         log_vertex_count{atoi(argv[3])};
    int         log_local_edge_count{atoi(argv[4])};
    size_t      compaction_threshold(atoll(argv[5]));
    size_t      promotion_threshold(atoll(argv[6]));
    int         num_trials{atoi(argv[7])};
    uint32_t    seed(atoi(argv[8]));
    std::string stats_output_prefix(argv[9]);

    size_t vertex_count     = ((size_t)1) << log_vertex_count;
    size_t local_edge_count = ((size_t)1) << log_local_edge_count;
    // world.cout0("Creating global table with size ", global_table_size);
    if (world.rank0()) {
      std::cout << world.layout().node_size() << ", "
                << world.layout().local_size() << ", " << ygm_buffer_capacity
                << ", " << world.routing_protocol() << ", " << range_size
                << ", " << vertex_count << ", "
                << local_edge_count * world.size() << ", "
                << compaction_threshold << ", " << promotion_threshold << ", "
                << seed;
    }

    parameters_t params{range_size,
                        vertex_count,
                        local_edge_count,
                        compaction_threshold,
                        promotion_threshold,
                        num_trials,
                        seed,
                        stats_output_prefix};

    do_analysis<DistributedPromotable32CountSketch_t>(world, params);
  }

  MPI_Finalize();
}
