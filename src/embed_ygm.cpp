// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <krowkee_helper.hpp>
#include <rmat_edge_generator.hpp>
#include <utility.hpp>

#include <boost/json/src.hpp>

#include <random>

std::uint32_t make_seed(const std::uint32_t seed, int size, const int rank,
                        const int trial) {
  return seed + size * trial + rank;
}

struct uniform_edge_vec_generator_t {
  uniform_edge_vec_generator_t(ygm::comm &world, const parameters_t &params,
                               const int trial)
      : _edges(params.local_edge_count),
        _i(0),
        _local_edge_count(params.local_edge_count) {
    std::mt19937 gen(make_seed(params.seed, world.size(), world.rank(), trial));
    std::uniform_int_distribution<uint64_t> dist(0, params.vertex_count - 1);

    for (std::int64_t i(0); i < _local_edge_count; ++i) {
      _edges[i] = {dist(gen), dist(gen)};
    }
  }

  std::pair<std::uint64_t, std::uint64_t> operator()() {
    if (_i >= _local_edge_count) {
      throw std::out_of_range(
          "calling uniform_edge_vec_generator too many times!");
    }
    return _edges[_i++];
  }

 private:
  std::vector<std::pair<std::uint64_t, std::uint64_t>> _edges;
  std::uint64_t                                        _i;
  std::uint64_t                                        _local_edge_count;
};

struct rmat_edge_vec_generator_t {
  rmat_edge_vec_generator_t(ygm::comm &world, const parameters_t &params,
                            const int trial)
      : _edges(params.local_edge_count),
        _i(0),
        _local_edge_count(params.local_edge_count) {
    std::uint32_t seed(
        make_seed(params.seed, world.size(), world.rank(), trial));
    rmat_edge_generator rmat(params.log_vertex_count, params.local_edge_count,
                             seed);
    for (int i(0); i < _local_edge_count; ++i) {
      _edges[i] = rmat.generate_single_edge();
    }
  }

  std::pair<std::uint64_t, std::uint64_t> operator()() {
    if (_i >= _local_edge_count) {
      throw std::out_of_range(
          "calling rmat_edge_vec_generator too many times!");
    }
    return _edges[_i++];
  }

 private:
  std::vector<std::pair<std::uint64_t, std::uint64_t>> _edges;
  std::uint64_t                                        _i;
  std::uint64_t                                        _local_edge_count;
};

struct uniform_edge_stream_generator_t {
  uniform_edge_stream_generator_t(ygm::comm &world, const parameters_t &params,
                                  const int trial)
      : _gen(make_seed(params.seed, world.size(), world.rank(), trial)),
        _dist(0, params.vertex_count - 1) {}

  std::pair<std::uint64_t, std::uint64_t> operator()() {
    return {_dist(_gen), _dist(_gen)};
  }

 private:
  std::mt19937                                 _gen;
  std::uniform_int_distribution<std::uint64_t> _dist;
};

struct rmat_edge_stream_generator_t {
  rmat_edge_stream_generator_t(ygm::comm &world, const parameters_t &params,
                               const int trial)
      : _seed(make_seed(params.seed, world.size(), world.rank(), trial)),
        _rmat(params.log_vertex_count, params.local_edge_count, _seed) {}

  std::pair<std::uint64_t, std::uint64_t> operator()() {
    return _rmat.generate_single_edge();
  }

 private:
  rmat_edge_generator _rmat;
  std::uint32_t       _seed;
};

int main(int argc, char **argv) {
  ygm::comm world(&argc, &argv);
  {
    parameters_t params = parse_args(argc, argv, world);

    if (params.rmat) {
      if (params.stream) {
        do_main<rmat_edge_stream_generator_t>(world, params);
      } else {
        do_main<rmat_edge_vec_generator_t>(world, params);
      }
    } else {
      if (params.stream) {
        do_main<uniform_edge_stream_generator_t>(world, params);
      } else {
        do_main<uniform_edge_vec_generator_t>(world, params);
      }
    }
  }
}
