#! /usr/bin/env python3

import argparse
import subprocess
import os
import time
import itertools
import sys

class command_parameter_generator:
    def __init__(self, name):
        self.name = name
        self.required_args = []
        self.iterated_args = {}
        self.iterated_flags = []

    def add_required_arg(self, flag, value):
        self.required_args.extend([flag, value])

    def add_required_flag(self, flag):
        self.required_args.append(flag)

    def add_arg(self, flag, values):
        if flag in self.iterated_args:
            self.iterated_args[flag].extend(values)
        else:
            self.iterated_args[flag] = values

    def add_flag(self, flag):
        self.iterated_flags.append(flag)

    def generate_command_list(self):
        # Generate powerset of iterated flags
        flags_powerset = itertools.chain.from_iterable(itertools.combinations(self.iterated_flags, l) \
                for l in range(0, len(self.iterated_flags)+1))

        # Generate all combinations of args in an array with associated flags
        args_combinations = list(itertools.product(*itertools.chain.from_iterable([[[key], value] \
                for key, value in self.iterated_args.items()])))

        to_return = []
        for flags in flags_powerset:
            for args in args_combinations:
                command_arr = (self.name,) + tuple(self.required_args) + flags + args
                to_return.append(command_arr)

        return to_return

def parse_commands():
    parser = argparse.ArgumentParser()

    # Arguments for job launcher
    parser.add_argument("-N", "--nodes", help="Number of nodes in Slurm")
    parser.add_argument("--ntasks-per-node", nargs="*", help="Number of tasks per node in Slurm")
    parser.add_argument("-A", "--account", help="Bank to use with Slurm")
    parser.add_argument("--ygm-comm-routing", nargs="*", help="YGM_COMM_ROUTING values to use (default is NONE)")
    parser.add_argument("--ygm-comm-buffer-size-kb", nargs="*", help="YGM_COMM_BUFFER_SIZE_KB values to use (default is 16MB)")
    parser.add_argument("--use-lsf", action="store_true", help="Use LSF scheduler instead of Slurm")

    # Arguments used for all experiments
    parser.add_argument("-p", "--pretty-print", action="store_true", help="Pretty-print all JSON output")
    parser.add_argument("-t", "--num-trials", help="Number of trials to run of each experiment")
    parser.add_argument("-o", "--output", help="File for output (writes to stdout if unspecified)")

    # Experiment arguments
    parser.add_argument("--no-atw-ygm", action="store_true", help="Skip around-the-world ygm experiment")
    parser.add_argument("--no-atw-mpi", action="store_true", help="Skip around-the-world MPI experiment")
    parser.add_argument("--no-histo-rmat", action="store_true", help="Skip histogram test with RMAT inputs")
    parser.add_argument("--no-histo-rmat-reducing-adapter", action="store_true", help="Skip histogram test with \
            RMAT inputs using reducing adapter")
    parser.add_argument("--no-histo-uniform", action="store_true", help="Skip histogram test with uniformly generated inputs")
    parser.add_argument("--no-agups", action="store_true", help="Skip agups experiment")
    parser.add_argument("--no-cc-rmat", action="store_true", help="Skip connected components RMAT experiment")
    parser.add_argument("--no-cc-linked-list", action="store_true", help="Skip connected components linked-list experiment")
    parser.add_argument("--no-embed-ygm", action="store_true", help="Skip krowkee experiment embedding graph vertices")

    parser.add_argument("-n", "--num-trips", nargs="*", help="Number of trips around the world in around-the-world experiments")
    parser.add_argument("--no-wait-until", action="store_true", help="Do not test ygm::comm::wait_until() in around-the-world")
    parser.add_argument("-s", "--table-scale", nargs="*", help="log_2 of table size for use in histo and agups experiments")
    parser.add_argument("-i", "--histo-inserts-per-rank", nargs="*", help="Number of insertions spawned by each rank in histo experiments")
    parser.add_argument("-u", "--agups-updaters-per-rank", nargs="*", help="Number of updaters spawned by each rank in agups experiments")
    parser.add_argument("-l", "--agups-updater-lifetime", nargs="*", help="Number of jumps made by each updater in agups experiments")
    parser.add_argument("-g", "--cc-graph-scale", nargs="*", help="Logarithmic graph scale for connected components \
            experiments (overridden by --cc-rmat-graph-scale and --cc-linked-list-graph-scale)")
    parser.add_argument("--cc-rmat-graph-scale", nargs="*", help="Logarithmic graph scale for connected components \
            RMAT experiments")
    parser.add_argument("--cc-linked-list-graph-scale", nargs="*", help="Logarithmic graph scale for connected components \
            linked list experiments")
    parser.add_argument("--cc-edgefactor", nargs="*", help="Edgefactor for connected components RMAT experiments")
    parser.add_argument("-d", "--embedding-dimension", nargs="*", help="Number of embedding dimensions for krowkee \
            experiments")
    parser.add_argument("-v", "--krowkee-log-vertex-count", nargs="*", help="log_2 of number of vertices for krowkee \
            experiments")
    parser.add_argument("--krowkee-edges-per-rank", nargs="*", help="Edges generated per rank for krowkee experiments")
    parser.add_argument("--krowkee-seed", nargs="*", help="Seed for krowkee experiments")

    args = parser.parse_args()

    exp_commands = {}

    # ATW_YGM arguments
    if (not args.no_atw_ygm):
        exp_commands["atw_ygm"] = command_parameter_generator('../build/src/around_the_world_ygm')
        if args.num_trips:
            exp_commands["atw_ygm"].add_arg('-n', args.num_trips)
        if not args.no_wait_until:
            exp_commands["atw_ygm"].add_flag('-w')

    # ATW_MPI arguments
    if (not args.no_atw_mpi):
        exp_commands["atw_mpi"] = command_parameter_generator('../build/src/around_the_world_mpi')
        if args.num_trips:
            exp_commands["atw_mpi"].add_arg('-n', args.num_trips)

    # HISTO_UNIFORM arguments
    if (not args.no_histo_uniform):
        exp_commands["histo_uniform"] = command_parameter_generator('../build/src/histo_ygm')
        if args.table_scale:
            exp_commands["histo_uniform"].add_arg('-s', args.table_scale)
        if args.histo_inserts_per_rank:
            exp_commands["histo_uniform"].add_arg('-i', args.histo_inserts_per_rank)

    # HISTO_RMAT arguments
    if (not args.no_histo_rmat):
        exp_commands["histo_rmat"] = command_parameter_generator('../build/src/histo_ygm')
        exp_commands["histo_rmat"].add_required_flag("-r")
        if args.table_scale:
            exp_commands["histo_rmat"].add_arg("-s", args.table_scale)
        if args.histo_inserts_per_rank:
            exp_commands["histo_rmat"].add_arg("-i", args.histo_inserts_per_rank)

    # HISTO_RMAT with reducing_adapter arguments
    if (not args.no_histo_rmat_reducing_adapter):
        exp_commands["histo_rmat_ra"] = command_parameter_generator('../build/src/histo_ygm')
        exp_commands["histo_rmat_ra"].add_required_flag("-r")
        exp_commands["histo_rmat_ra"].add_required_flag("-a")
        if args.table_scale:
            exp_commands["histo_rmat_ra"].add_arg("-s", args.table_scale)
        if args.histo_inserts_per_rank:
            exp_commands["histo_rmat_ra"].add_arg("-i", args.histo_inserts_per_rank)

    # AGUPS arguments
    if (not args.no_agups):
        exp_commands["agups"] = command_parameter_generator("../build/src/agups_ygm")
        if args.table_scale:
            exp_commands["agups"].add_arg("-s", args.table_scale)
        if args.agups_updaters_per_rank:
            exp_commands["agups"].add_arg("-u", args.agups_updaters_per_rank)
        if args.agups_updater_lifetime:
            exp_commands["agups"].add_arg("-l", args.agups_updater_lifetime)

    # CC_RMAT arguments
    if (not args.no_cc_rmat):
        exp_commands["cc_rmat"] = command_parameter_generator("../build/src/cc_ygm")
        if args.cc_rmat_graph_scale:
            exp_commands["cc_rmat"].add_arg("-g", args.cc_rmat_graph_scale)
        elif args.cc_graph_scale:
            exp_commands["cc_rmat"].add_arg("-g", args.cc_graph_scale)
        if args.cc_edgefactor:
            exp_commands["cc_rmat"].add_arg("-e", args.cc_edgefactor)

    # CC_LINKED_LIST arguments
    if (not args.no_cc_linked_list):
        exp_commands["cc_linked_list"] = command_parameter_generator("../build/src/cc_ygm")
        exp_commands["cc_linked_list"].add_required_flag("-l")
        if args.cc_linked_list_graph_scale:
            exp_commands["cc_linked_list"].add_arg("-g", args.cc_linked_list_graph_scale)
        elif args.cc_graph_scale:
            exp_commands["cc_linked_list"].add_arg("-g", args.cc_graph_scale)

    # EMBED_YGM
    if (not args.no_embed_ygm):
        exp_commands["embed_ygm"] = command_parameter_generator("../build/src/embed_ygm")
        exp_commands["embed_ygm"].add_required_flag("-b")
        exp_commands["embed_ygm"].add_flag("-m")
        exp_commands["embed_ygm"].add_flag("-r")
        if args.embedding_dimension:
            exp_commands["embed_ygm"].add_arg("-d", args.embedding_dimension)
        if args.krowkee_log_vertex_count:
            exp_commands["embed_ygm"].add_arg("-v", args.krowkee_log_vertex_count)
        if args.krowkee_seed:
            exp_commands["embed_ygm"].add_arg("-s", args.krowkee_seed)

    # Shared arguments
    if args.num_trials:
        for exp_name, command in exp_commands.items():
            exp_commands[exp_name].add_required_arg("-t", args.num_trials)
    if args.pretty_print:
        for exp_name, command in exp_commands.items():
            exp_commands[exp_name].add_required_flag("-p")
    if args.output:
        output = args.output
    else:
        output = None

    if args.ygm_comm_routing:
        routing_protocols = args.ygm_comm_routing
    else:
        routing_protocols = ["NONE"]

    if args.ygm_comm_buffer_size_kb:
        buffer_sizes = args.ygm_comm_buffer_size_kb
    else:
        buffer_sizes = ["16384"]

    if args.use_lsf:
        launcher = command_parameter_generator('lrun')
    else:
        launcher = command_parameter_generator('srun')
        launcher.add_required_flag('--exclusive')

    if args.ntasks_per_node:
        if args.use_lsf:
            launcher.add_arg('-T', args.ntasks_per_node)
        else:
            launcher.add_arg('--ntasks-per-node', args.ntasks_per_node)
    if args.account:
        if args.use_lsf:
            launcher.add_arg('-G', args.account)
        else:
            launcher.add_required_arg('-A', args.account)
    if args.nodes:
        launcher.add_required_arg('-N', str(args.nodes))

    return launcher, exp_commands, routing_protocols, buffer_sizes, output


def main():
    launcher, commands, routing_protocols, buffer_sizes, output = parse_commands();

    for command_gen in commands.values():
        for l in launcher.generate_command_list():
            for command in command_gen.generate_command_list():
                for routing in routing_protocols:
                    for buffer_size in buffer_sizes:
                        time.sleep(1)
                        process = subprocess.run(l + command, \
                                env=dict(os.environ, YGM_COMM_ROUTING=routing, YGM_COMM_BUFFER_SIZE_KB=buffer_size), \
                                stdout=sys.stdout, text=True)


if __name__ == "__main__":
    main()
