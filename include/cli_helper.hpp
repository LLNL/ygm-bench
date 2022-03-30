// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <string>

struct parameters_t {
  int         ygm_buffer_capacity;
  int         range_size;
  size_t      vertex_count;
  size_t      local_edge_count;
  size_t      compaction_threshold;
  size_t      promotion_threshold;
  int         num_trials;
  uint32_t    seed;
  std::string stats_output_prefix;
};

parameters_t parse_args(int argc, char **argv) {
  parameters_t params{};
  params.ygm_buffer_capacity  = atoi(argv[1]);
  params.range_size           = atoi(argv[2]);
  int log_vertex_count        = atoi(argv[3]);
  params.vertex_count         = ((size_t)1) << log_vertex_count;
  params.local_edge_count     = atoll(argv[4]);
  params.compaction_threshold = atoll(argv[5]);
  params.promotion_threshold  = atoll(argv[6]);
  params.num_trials           = atoi(argv[7]);
  params.seed                 = atoi(argv[8]);
  params.stats_output_prefix  = argv[9];

  return params;
}
