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
#include <ygm/container/array.hpp>
#include <ygm/detail/ygm_cereal_archive.hpp>
#include <ygm/detail/ygm_ptr.hpp>
#include <ygm/utility.hpp>

#include <boost/json/src.hpp>

#include <string>

using sketch_type_t = krowkee::util::sketch_type_t;
using cmap_type_t   = krowkee::util::cmap_type_t;

using Dense32CountSketch_t =
    krowkee::sketch::Sketch<krowkee::transform::CountSketchFunctor,
                            krowkee::sketch::Dense, std::plus, std::int32_t,
                            std::shared_ptr, krowkee::hash::MulAddShift>;

struct parameters_t {
  enum class container { map, array };

  int       range_size;
  int       log_vertex_count;
  size_t    vertex_count;
  size_t    local_edge_count;
  int       num_trials;
  uint32_t  seed;
  container cont;
  bool      embed;
  bool      stream;
  bool      rmat;
  bool      pretty_print;

  parameters_t()
      : range_size(8),
        log_vertex_count(20),
        local_edge_count(1000000),
        num_trials(5),
        seed(1),
        cont(container::array),
        embed(true),
        stream(false),
        rmat(false),
        pretty_print(false) {}
};

void usage(ygm::comm &comm) {
  comm.cerr0()
      << "embed_ygm usage:"
      << "\n\t-d <int>\t- Range size"
      << "\n\t-v <int>\t- Log_2 of global vertex count"
      << "\n\t-e <int>\t- Number of edges per rank"
      << "\n\t-t <int>\t- Number of trials"
      << "\n\t-s <int>\t- Seed"
      << "\n\t-r\t\t- Flag indicating insertions should use RMAT generator"
      << "\n\t-b\t\t- Flag to embed data"
      << "\n\t-m\t\t- Flag indicating streaming mode"
      << "\n\t-p\t\t- Pretty print output"
      << "\n\t-h\t\t- Print help" << std::endl;
}

parameters_t parse_args(int argc, char **argv, ygm::comm &comm) {
  parameters_t params{};
  int          c;
  bool         prn_help = false;

  // Suppress error messages from getopt
  extern int opterr;
  opterr = 0;

  while ((c = getopt(argc, argv, "d:v:e:t:s:mrbaph")) != -1) {
    switch (c) {
      case 'h':
        prn_help = true;
        break;
      case 'd':
        params.range_size = atoi(optarg);
        break;
      case 'v':
        params.log_vertex_count = atoi(optarg);
        break;
      case 'e':
        params.local_edge_count = atoll(optarg);
        break;
      case 't':
        params.num_trials = atoi(optarg);
        break;
      case 's':
        params.seed = atoi(optarg);
        break;
      case 'm':
        params.cont = parameters_t::container::map;
        break;
      case 'r':
        params.rmat = true;
        break;
      case 'b':
        params.embed = true;
        break;
      case 'a':
        params.stream = true;
        break;
      case 'p':
        params.pretty_print = true;
        break;
      default:
        comm.cerr0() << "Unrecognized option: " << char(optopt) << std::endl;
        prn_help = true;
        break;
    }
  }

  params.vertex_count = ((size_t)1) << params.log_vertex_count;

  if (prn_help) {
    usage(comm);
    exit(-1);
  }

  return params;
}

template <typename EdgeGeneratorType, typename MapType>
void do_streaming_analysis(ygm::comm &world, MapType &vertex_map,
                           const parameters_t  &params,
                           boost::json::object &output) {
  for (int trial = 0; trial < params.num_trials; ++trial) {
    EdgeGeneratorType edge_stream(world, params, 0);

    world.barrier();
    world.stats_reset();

    ygm::timer update_timer{};

    for (int i(0); i < params.local_edge_count; ++i) {
      const std::pair<std::uint64_t, std::uint64_t> edge(edge_stream());
      vertex_map.async_visit(
          edge.first,
          [](const auto &key, auto &value, const std::uint64_t &dst) {
            value.insert(dst);
          },
          edge.second);
    }

    world.barrier();

    double trial_time = update_timer.elapsed();

    output["TIME"].as_array().emplace_back(trial_time);
    double trial_rate = params.local_edge_count * world.size() / trial_time /
                        (1000 * 1000 * 1000);

    output["INSERTS_PER_SECOND(BILLIONS)"].as_array().emplace_back(trial_rate);

    parse_stats(world, output);
  }
}

template <typename EdgeGeneratorType>
void do_main(ygm::comm &world, const parameters_t &params) {
  /*
if (world.rank0()) {
std::cout << world.layout().node_size() << ", "
        << world.layout().local_size() << ", "
        << params.ygm_buffer_capacity << ", " << world.routing_protocol()
        << ", " << params.range_size << ", " << params.vertex_count
        << ", " << params.local_edge_count * world.size() << ", "
        << params.seed;
}
  */

  boost::json::object output;

  output["NAME"]                         = "EMBED_YGM";
  output["TIME"]                         = boost::json::array();
  output["INSERTS_PER_SECOND(BILLIONS)"] = boost::json::array();
  output["GLOBAL_ASYNC_COUNT"]           = boost::json::array();
  output["GLOBAL_ISEND_COUNT"]           = boost::json::array();
  output["GLOBAL_ISEND_BYTES"]           = boost::json::array();
  output["MAX_WAITSOME_ISEND_IRECV"]     = boost::json::array();
  output["MAX_WAITSOME_IALLREDUCE"]      = boost::json::array();
  output["COUNT_IALLREDUCE"]             = boost::json::array();
  output["EMBED"]                        = params.embed;
  output["EMBEDDING_DIMENSION"]          = params.range_size;
  output["VERTICES"]                     = params.vertex_count;
  output["EDGES"]  = params.local_edge_count * world.size();
  output["SEED"]   = params.seed;
  output["STREAM"] = params.stream;
  if (params.rmat) {
    output["GENERATOR"] = "RMAT";
  } else {
    output["GENERATOR"] = "UNIFORM";
  }
  if (params.cont == parameters_t::container::map) {
    output["CONTAINER"] = "MAP";
  } else if (params.cont == parameters_t::container::array) {
    output["CONTAINER"] = "ARRAY";
  } else {
    output["CONTAINER"] = "UNKNOWN";
  }

  parse_welcome(world, output);

  if (params.embed) {
    typedef Dense32CountSketch_t    sk_t;
    typedef typename sk_t::sf_t     sf_t;
    typedef typename sk_t::sf_ptr_t sf_ptr_t;

    sf_ptr_t sf_ptr(std::make_shared<sf_t>(params.range_size, params.seed));
    sk_t     default_vertex(sf_ptr);

    if (params.cont == parameters_t::container::map) {
      ygm::container::map<std::uint64_t, sk_t> vertex_map(world,
                                                          default_vertex);

      do_streaming_analysis<EdgeGeneratorType>(world, vertex_map, params,
                                               output);
    } else if (params.cont == parameters_t::container::array) {
      ygm::container::array<sk_t> vertex_array(world, params.vertex_count,
                                               default_vertex);

      do_streaming_analysis<EdgeGeneratorType>(world, vertex_array, params,
                                               output);
    }
  } else {
    std::set<std::uint64_t> default_vertex;

    if (params.cont == parameters_t::container::map) {
      ygm::container::map<std::uint64_t, std::set<std::uint64_t>> vertex_map(
          world, default_vertex);

      do_streaming_analysis<EdgeGeneratorType>(world, vertex_map, params,
                                               output);
    } else if (params.cont == parameters_t::container::array) {
      ygm::container::array<std::set<std::uint64_t>> vertex_array(
          world, params.vertex_count, default_vertex);

      do_streaming_analysis<EdgeGeneratorType>(world, vertex_array, params,
                                               output);
    }
  }

  if (params.pretty_print) {
    pretty_print(world.cout0(), output);
    world.cout0() << "\n";
  } else {
    world.cout0(output);
  }
}
