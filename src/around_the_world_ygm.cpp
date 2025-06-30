// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <unistd.h>
#include <algorithm>
#include <string>
#include <utility.hpp>
#include <ygm/comm.hpp>
#include <ygm/utility/timer.hpp>

#include <boost/json/src.hpp>

struct parameters_t {
  int  num_trips;
  int  num_trials;
  bool use_wait_until;
  bool pretty_print;

  parameters_t()
      : num_trips(1000),
        num_trials(5),
        use_wait_until(false),
        pretty_print(false) {}
};

void usage(ygm::comm &comm) {
  comm.cerr0() << "around_the_world_ygm usage:"
               << "\n\t-n <int>\t- Number of trips around the world"
               << "\n\t-t <int>\t- Number of trials"
               << "\n\t-w\t\t- Use ygm::comm::wait_until()"
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

  while ((c = getopt(argc, argv, "n:t:wph")) != -1) {
    switch (c) {
      case 'h':
        prn_help = true;
        break;
      case 'n':
        params.num_trips = atoi(optarg);
        break;
      case 't':
        params.num_trials = atoi(optarg);
        break;
      case 'w':
        params.use_wait_until = true;
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

void run_atw(ygm::comm &world, const parameters_t &params) {
  static const parameters_t &s_params   = params;
  auto                       total_hops = params.num_trips * world.size();

  boost::json::object output;

  output["NAME"]                     = "ATW_YGM";
  output["TIME"]                     = boost::json::array();
  output["HOPS_PER_SEC"]             = boost::json::array();
  output["GLOBAL_ASYNC_COUNT"]       = boost::json::array();
  output["GLOBAL_ISEND_COUNT"]       = boost::json::array();
  output["GLOBAL_ISEND_BYTES"]       = boost::json::array();
  output["MAX_WAITSOME_ISEND_IRECV"] = boost::json::array();
  output["MAX_WAITSOME_IALLREDUCE"]  = boost::json::array();
  output["COUNT_IALLREDUCE"]         = boost::json::array();
  output["WAIT_UNTIL"]               = params.use_wait_until;
  output["NUM_TRIPS"]                = params.num_trips;
  output["TOTAL_HOPS"]               = total_hops;

  parse_welcome(world, output);

  static int curr_trip;
  curr_trip = 0;

  struct around_the_world_functor {
   public:
    void operator()(ygm::ygm_ptr<ygm::comm> pworld) {
      if (curr_trip < s_params.num_trips) {
        pworld->async((pworld->rank() + 1) % pworld->size(),
                      around_the_world_functor());
        ++curr_trip;
      }
    }
  };

  world.barrier();

  for (int trial = 0; trial < params.num_trials; ++trial) {
    world.stats_reset();

    curr_trip = 0;

    world.barrier();

    ygm::utility::timer trip_timer{};

    if (world.rank0()) {
      world.async(1, around_the_world_functor());
    }

    if (params.use_wait_until) {
      world.local_wait_until([]() { return curr_trip >= s_params.num_trips; });
    }

    world.barrier();

    double elapsed = trip_timer.elapsed();

    output["TIME"].as_array().emplace_back(elapsed);
    output["HOPS_PER_SEC"].as_array().emplace_back(total_hops / elapsed);

    parse_stats(world, output);
  }

  if (params.pretty_print) {
    pretty_print(world.cout0(), output);
    world.cout0() << "\n";
  } else {
    world.cout0(output);
  }
}

int main(int argc, char **argv) {
  ygm::comm world(&argc, &argv);

  // Need static params to use in around_the_world_functor
  parameters_t params = parse_cmd_line(argc, argv, world);

  /* wait_until currently unimplemented
bool        wait_until{false};
const char *wait_until_tmp = std::getenv("YGM_BENCH_ATW_WAIT_UNTIL");
std::string wait_until_str(wait_until_tmp ? wait_until_tmp : "false");
std::transform(
wait_until_str.begin(), wait_until_str.end(), wait_until_str.begin(),
[](unsigned char c) -> unsigned char { return std::tolower(c); });
if (wait_until_str == "true") {
wait_until = true;
}
  */

  run_atw(world, params);

  return 0;
}
