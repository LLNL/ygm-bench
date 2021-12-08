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

  world.barrier();

  ygm::timer trip_timer{};

  if (world.rank0()) {
    world.async(1, around_the_world_functor());
  }

  world.barrier();

  double elapsed = trip_timer.elapsed();

  world.cout0("Went around the world ", num_trips, " times in ", elapsed,
              " seconds\nAverage trip time: ", elapsed / num_trips);

  return 0;
}
