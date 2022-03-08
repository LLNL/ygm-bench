// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <ygm/comm.hpp>
#include <ygm/utility.hpp>

static int num_trips;
static int curr_trip = 0;

struct around_the_world_functor {
 public:
  template <typename Comm>
  void operator()(Comm *world) {
    if (curr_trip < num_trips) {
      world->async((world->rank() + 1) % world->size(),
                   around_the_world_functor());
      ++curr_trip;
    }
  }
};

int main(int argc, char **argv) {
  ygm::comm world(&argc, &argv);

  num_trips = atoi(argv[1]);
  int num_trials{atoi(argv[2])};

  if (world.rank0()) {
    std::cout << world.layout().node_size() << ", "
              << world.layout().local_size() << ", " << world.routing_protocol()
              << ", " << num_trips;
  }

  world.barrier();

  for (int trial = 0; trial < num_trials; ++trial) {
    curr_trip = 0;
    world.barrier();

    ygm::timer trip_timer{};

    if (world.rank0()) {
      world.async(1, around_the_world_functor());
    }

    world.barrier();

    double elapsed = trip_timer.elapsed();

    // world.cout0("Went around the world ", num_trips, " times in ", elapsed,
    //" seconds\nAverage trip time: ", elapsed / num_trips);

    if (world.rank0()) {
      auto total_hops = num_trips * world.size();
      std::cout << ", " << elapsed << ", " << total_hops / elapsed;
    }
  }

  return 0;
}
