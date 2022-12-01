// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT
#pragma once

#include <utility.hpp>

#include <krowkee/sketch/interface.hpp>
#include <krowkee/stream/interface.hpp>

#include <krowkee/util/cmap_types.hpp>
#include <krowkee/util/sketch_types.hpp>
#include <krowkee/util/ygm_tests.hpp>

#include <ygm/comm.hpp>
#include <ygm/detail/ygm_cereal_archive.hpp>
#include <ygm/detail/ygm_ptr.hpp>
#include <ygm/utility.hpp>

#include <string>

using sketch_type_t = krowkee::util::sketch_type_t;
using cmap_type_t   = krowkee::util::cmap_type_t;

using Dense32CountSketch_t =
    krowkee::sketch::Sketch<krowkee::transform::CountSketchFunctor,
                            krowkee::sketch::Dense, std::plus, std::int32_t,
                            std::shared_ptr, krowkee::hash::MulAddShift>;

struct parameters_t {
  int         ygm_buffer_capacity;
  int         range_size;
  int         log_vertex_count;
  size_t      vertex_count;
  size_t      local_edge_count;
  int         num_trials;
  uint32_t    seed;
  std::string stats_output_prefix;
  bool        embed;
  bool        stream;
  bool        rmat;
};

parameters_t parse_args(int argc, char **argv) {
  std::stringstream ss;
  ss << "\nexpected usage:  " << argv[0] << " ygm_buffer_capacity"
     << " range_size"
     << " log_vertex_count"
     << " local_edge_count"
     << " num_trials"
     << " seed"
     << " stats_output_prefix"
     << " embed_bool"
     << " stream_bool"
     << " rmat_bool" << std::endl;
  if (argc != 11) {
    throw std::range_error(ss.str());
  }
  parameters_t params{};
  params.ygm_buffer_capacity = atoi(argv[1]);
  params.range_size          = atoi(argv[2]);
  params.log_vertex_count    = atoi(argv[3]);
  params.vertex_count        = ((size_t)1) << params.log_vertex_count;
  params.local_edge_count    = atoll(argv[4]);
  params.num_trials          = atoi(argv[5]);
  params.seed                = atoi(argv[6]);
  params.stats_output_prefix = argv[7];
  params.embed               = atoi(argv[8]) == 1;
  params.stream              = atoi(argv[9]) == 1;
  params.rmat                = atoi(argv[10]) == 1;

  return params;
}

template <typename EdgeGeneratorType, typename MapType>
void do_streaming_analysis(ygm::comm &world, MapType &vertex_map,
                           const parameters_t &params) {
  EdgeGeneratorType edge_stream(world, params, 0);
  double            total_update_time{0.0};

  world.stats_reset();

  for (int trial = 0; trial < params.num_trials; ++trial) {
    world.barrier();

    world.set_track_stats(true);

    ygm::timer update_timer{};

    for (int i(0); i < params.local_edge_count; ++i) {
      std::pair<std::uint64_t, std::uint64_t> edge(edge_stream());
      vertex_map.async_visit(
          edge.first,
          [](auto kv_pair, const std::uint64_t dst) {
            kv_pair.second.insert(dst);
          },
          edge.second);
    }

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
}

template <typename EdgeGeneratorType>
void do_main(ygm::comm &world, const parameters_t &params) {
  if (world.rank0()) {
    std::cout << world.layout().node_size() << ", "
              << world.layout().local_size() << ", "
              << params.ygm_buffer_capacity << ", " << world.routing_protocol()
              << ", " << params.range_size << ", " << params.vertex_count
              << ", " << params.local_edge_count * world.size() << ", "
              << params.seed;
  }

  if (params.embed) {
    typedef Dense32CountSketch_t    sk_t;
    typedef typename sk_t::sf_t     sf_t;
    typedef typename sk_t::sf_ptr_t sf_ptr_t;

    sf_ptr_t sf_ptr(std::make_shared<sf_t>(params.range_size, params.seed));
    sk_t     default_vertex(sf_ptr);
    ygm::container::map<std::uint64_t, sk_t> vertex_map(world, default_vertex);

    do_streaming_analysis<EdgeGeneratorType>(world, vertex_map, params);
  } else {
    std::set<std::uint64_t>                                     default_vertex;
    ygm::container::map<std::uint64_t, std::set<std::uint64_t>> vertex_map(
        world, default_vertex);
    do_streaming_analysis<EdgeGeneratorType>(world, vertex_map, params);
  }
  world.cout0("");
}