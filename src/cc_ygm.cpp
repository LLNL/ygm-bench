// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <random>
#include <rmat_edge_generator.hpp>
#include <utility.hpp>
#include <ygm/comm.hpp>
#include <ygm/container/disjoint_set.hpp>
#include <ygm/utility/timer.hpp>

#include <boost/json/src.hpp>

struct parameters_t {
  enum class generator { rmat, linked_list };

  int       graph_scale;
  int       edgefactor;
  int       num_trials;
  generator gen;
  bool      pretty_print;

  parameters_t()
      : graph_scale(15),
        edgefactor(16),
        num_trials(5),
        gen(generator::rmat),
        pretty_print(false) {}
};

void usage(ygm::comm &comm) {
  comm.cerr0() << "cc_ygm usage:"
               << "\n\t-g <int>\t- Log_2 of global number of vertices"
               << "\n\t-e <int>\t- Edgefactor (half of average vertex degree)"
               << "\n\t-l\t\t- Use linked-list graph"
               << "\n\t-t <int>\t- Number of trials"
               << "\n\t-p\t\t- Pretty print output"
               << "\n\t-h\t\t- Print help" << std::endl;
}

parameters_t parse_cmd_line(int argc, char **argv, ygm::comm &comm) {
  parameters_t params;
  int          c;
  bool         prn_help = false;

  // Suppress error messages from getopt
  extern int opterr;
  opterr = 0;

  while ((c = getopt(argc, argv, "g:e:lt:ph")) != -1) {
    switch (c) {
      case 'h':
        prn_help = true;
        break;
      case 'g':
        params.graph_scale = atoi(optarg);
        break;
      case 'e':
        params.edgefactor = atoi(optarg);
        break;
      case 'l':
        params.gen = parameters_t::generator::linked_list;
        break;
      case 't':
        params.num_trials = atoi(optarg);
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

  if (prn_help) {
    usage(comm);
    exit(-1);
  }

  return params;
}

std::vector<std::pair<uint64_t, uint64_t>> generate_edges(
    ygm::comm &world, const int graph_scale, const int edgefactor,
    const int trial, const parameters_t::generator gen) {
  std::vector<std::pair<uint64_t, uint64_t>> edges;
  uint64_t num_vertices = ((uint64_t)1) << graph_scale;

  if (gen == parameters_t::generator::rmat) {
    uint64_t                        global_edges = num_vertices * edgefactor;
    distributed_rmat_edge_generator rmat(world, graph_scale, global_edges,
                                         trial);

    rmat.for_all([&edges](const auto first, const auto second) {
      edges.push_back(std::make_pair(first, second));
    });
  } else if (gen == parameters_t::generator::linked_list) {
    // Skip last vertex as source (only a sink) by using num_vertices-1
    uint64_t min_block_size = (num_vertices - 1) / world.size();
    uint64_t num_local_sources =
        min_block_size + (world.rank() < (num_vertices - 1) % world.size());
    uint64_t vertex_offset =
        world.rank() * min_block_size +
        std::min<uint64_t>(world.rank(), (num_vertices - 1) % world.size());

    for (uint64_t i = 0; i < num_local_sources; ++i) {
      edges.push_back(std::make_pair(vertex_offset + i, vertex_offset + i + 1));
    }
  } else {
    world.cerr0() << "Unrecognized graph generator" << std::endl;
    exit(-1);
  }

  world.barrier();

  return edges;
}

void run_cc(ygm::comm                                        &world,
            const std::vector<std::pair<uint64_t, uint64_t>> &edges,
            ygm::container::disjoint_set<uint64_t>           &dset) {
  for (const auto &edge : edges) {
    dset.async_union(edge.first, edge.second);
  }

  world.barrier();
}

int main(int argc, char **argv) {
  {
    ygm::comm world(&argc, &argv);

    parameters_t params = parse_cmd_line(argc, argv, world);

    uint64_t num_vertices = ((uint64_t)1) << params.graph_scale;

    boost::json::object output;

    output["NAME"]                        = "CC_YGM";
    output["TIME"]                        = boost::json::array();
    output["UNIONS_PER_SECOND(MILLIONS)"] = boost::json::array();
    output["GLOBAL_ASYNC_COUNT"]          = boost::json::array();
    output["GLOBAL_ISEND_COUNT"]          = boost::json::array();
    output["GLOBAL_ISEND_BYTES"]          = boost::json::array();
    output["MAX_WAITSOME_ISEND_IRECV"]    = boost::json::array();
    output["MAX_WAITSOME_IALLREDUCE"]     = boost::json::array();
    output["COUNT_IALLREDUCE"]            = boost::json::array();
    output["GRAPH_SCALE"]                 = params.graph_scale;
    output["VERTICES"]                    = num_vertices;
    if (params.gen == parameters_t::generator::rmat) {
      output["GENERATOR"] = "RMAT";
    } else if (params.gen == parameters_t::generator::linked_list) {
      output["GENERATOR"] = "LINKED_LIST";
    } else {
      output["GENERATOR"] = "UNKNOWN";
    }

    parse_welcome(world, output);

    for (int trial = 0; trial < params.num_trials; ++trial) {
      ygm::container::disjoint_set<uint64_t> dset(world);
      world.stats_reset();

      std::vector<std::pair<uint64_t, uint64_t>> edges = generate_edges(
          world, params.graph_scale, params.edgefactor, trial, params.gen);

      uint64_t num_edges = ygm::sum(edges.size(), world);

      double trial_time;
      double trial_rate;
      world.barrier();

      ygm::utility::timer update_timer{};

      run_cc(world, edges, dset);

      trial_time = update_timer.elapsed();
      trial_rate = num_edges / trial_time / (1000 * 1000);

      output["TIME"].as_array().emplace_back(trial_time);
      output["UNIONS_PER_SECOND(MILLIONS)"].as_array().emplace_back(trial_rate);
      output["EDGES"] = num_edges;

      parse_stats(world, output);
    }

    if (params.pretty_print) {
      pretty_print(world.cout0(), output);
      world.cout0() << "\n";
    } else {
      world.cout0(output);
    }
  }

  return 0;
}
