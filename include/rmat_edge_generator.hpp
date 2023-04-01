// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#pragma once
#include <ygm/comm.hpp>

#include <random>

#include <assert.h>
#include <stdint.h>

///
/// Hash functions
///

inline uint32_t hash32(uint32_t a) {
  a = (a + 0x7ed55d16) + (a << 12);
  a = (a ^ 0xc761c23c) ^ (a >> 19);
  a = (a + 0x165667b1) + (a << 5);
  a = (a + 0xd3a2646c) ^ (a << 9);
  a = (a + 0xfd7046c5) + (a << 3);
  a = (a ^ 0xb55a4f09) ^ (a >> 16);
  return a;
}

inline uint16_t hash16(uint16_t a) {
  a = (a + 0x5d16) + (a << 6);
  a = (a ^ 0xc23c) ^ (a >> 9);
  a = (a + 0x67b1) + (a << 5);
  a = (a + 0x646c) ^ (a << 7);
  a = (a + 0x46c5) + (a << 3);
  a = (a ^ 0x4f09) ^ (a >> 8);
  return a;
}

inline uint64_t shifted_n_hash32(uint64_t input, int n) {
  uint64_t to_hash = input >> n;
  uint64_t mask    = 0xFFFFFFFF;
  to_hash &= mask;
  to_hash = hash32(to_hash);

  to_hash <<= n;
  mask <<= n;
  // clear bits
  input &= ~mask;
  input |= to_hash;
  return input;
}

inline uint64_t shifted_n_hash16(uint64_t input, int n) {
  uint64_t to_hash = input >> n;
  uint64_t mask    = 0xFFFF;
  to_hash &= mask;
  to_hash = hash16(to_hash);

  to_hash <<= n;
  mask <<= n;
  // clear bits
  input &= ~mask;
  input |= to_hash;
  return input;
}

inline uint64_t hash_nbits(uint64_t input, int n) {
  // std::cout << "hash_nbits(" << input << ", " << n << ") = ";
  if (n == 32) {
    input = hash32(input);
  } else if (n > 32) {
    assert(n > 32);
    n -= 32;
    for (int i = 0; i <= n; ++i) {
      input = shifted_n_hash32(input, i);
    }
    for (int i = n; i >= 0; --i) {
      input = shifted_n_hash32(input, i);
    }
  } else if (n < 32) {
    assert(n < 32);
    assert(n > 16 && "Hashing less than 16bits is not supported");
    n -= 16;
    for (int i = 0; i <= n; ++i) {
      input = shifted_n_hash16(input, i);
    }
    for (int i = n; i >= 0; --i) {
      input = shifted_n_hash16(input, i);
    }
  }
  // std::cout << input << std::endl;
  return input;
}

/// RMAT edge generator, based on Boost Graph's RMAT generator
///
/// Options include scrambling vertices based on a hash funciton, and
/// symmetrizing the list.   Generated edges are not sorted.  May contain
/// duplicate and self edges.
class rmat_edge_generator {
 public:
  typedef uint64_t                      vertex_descriptor;
  typedef std::pair<uint64_t, uint64_t> value_type;
  typedef value_type                    edge_type;

  /// seed used to be 5489
  rmat_edge_generator(uint64_t vertex_scale, uint64_t edge_count,
                      uint64_t seed = 1234, bool scramble = true,
                      bool undirected = false, double a = 0.57, double b = 0.19,
                      double c = 0.19, double d = 0.05)
      : m_seed(seed),
        m_gen(seed),
        m_distribution(0.0, 1.0),
        m_vertex_scale(vertex_scale),
        m_edge_count(edge_count),
        m_scramble(scramble),
        m_undirected(undirected),
        m_rmat_a(a),
        m_rmat_b(b),
        m_rmat_c(c),
        m_rmat_d(d) {}

  uint64_t max_vertex_id() {
    return (uint64_t(1) << uint64_t(m_vertex_scale)) - 1;
  }

  size_t size() { return m_edge_count; }

  template <typename Function>
  void for_all(Function fn) {
    m_gen.seed(m_seed);
    for (uint64_t i = 0; i < m_edge_count; ++i) {
      const auto [src, dest] = generate_edge();
      fn(src, dest);

      if (m_undirected) {
        fn(dest, src);
      }
    }
  }

  edge_type generate_single_edge() { return generate_edge(); }

 protected:
  /// Generates a new RMAT edge.  This function was adapted from the Boost Graph
  /// Library.
  edge_type generate_edge() {
    double   rmat_a = m_rmat_a;
    double   rmat_b = m_rmat_b;
    double   rmat_c = m_rmat_c;
    double   rmat_d = m_rmat_d;
    uint64_t u = 0, v = 0;
    uint64_t step = (uint64_t(1) << m_vertex_scale) / 2;
    for (unsigned int j = 0; j < m_vertex_scale; ++j) {
      double p = m_distribution(m_gen);

      if (p < rmat_a)
        ;
      else if (p >= rmat_a && p < rmat_a + rmat_b)
        v += step;
      else if (p >= rmat_a + rmat_b && p < rmat_a + rmat_b + rmat_c)
        u += step;
      else {  // p > a + b + c && p < a + b + c + d
        u += step;
        v += step;
      }

      step /= 2;

      // 0.2 and 0.9 are hardcoded in the reference SSCA implementation.
      // The maximum change in any given value should be less than 10%
      rmat_a *= 0.9 + 0.2 * m_distribution(m_gen);
      rmat_b *= 0.9 + 0.2 * m_distribution(m_gen);
      rmat_c *= 0.9 + 0.2 * m_distribution(m_gen);
      rmat_d *= 0.9 + 0.2 * m_distribution(m_gen);

      double S = rmat_a + rmat_b + rmat_c + rmat_d;

      rmat_a /= S;
      rmat_b /= S;
      rmat_c /= S;
      // d /= S;
      // Ensure all values add up to 1, regardless of floating point errors
      rmat_d = 1. - rmat_a - rmat_b - rmat_c;
    }
    if (m_scramble) {
      u = hash_nbits(u, m_vertex_scale);
      v = hash_nbits(v, m_vertex_scale);
    }

    return std::make_pair(u, v);
  }

  const uint64_t                         m_seed;
  std::mt19937                           m_gen;
  std::uniform_real_distribution<double> m_distribution;
  const uint64_t                         m_vertex_scale;
  const uint64_t                         m_edge_count;
  const bool                             m_scramble;
  const bool                             m_undirected;
  const double                           m_rmat_a;
  const double                           m_rmat_b;
  const double                           m_rmat_c;
  const double                           m_rmat_d;
};

// RMAT generator that splits edge generation responsibilities across ranks
class distributed_rmat_edge_generator {
 public:
  distributed_rmat_edge_generator(ygm::comm &world, uint64_t vertex_scale,
                                  uint64_t global_edge_count,
                                  uint64_t seed = 1234, bool scramble = true,
                                  bool undirected = false, double a = 0.57,
                                  double b = 0.19, double c = 0.19,
                                  double d = 0.05)
      : m_local_generator(
            vertex_scale,
            global_edge_count / world.size() +
                (world.rank() < (global_edge_count % world.size())),
            seed * world.size() + world.rank(), scramble, undirected, a, b, c,
            d) {}

  template <typename Function>
  void for_all(Function fn) {
    m_local_generator.for_all(fn);
  }

 private:
  rmat_edge_generator m_local_generator;
};
