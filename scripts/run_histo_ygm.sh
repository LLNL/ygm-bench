#! /usr/bin/env bash

YGM_BENCH_HISTO_OUTPUT_FILE="${YGM_BENCH_OUTPUT_DIR}/histo_ygm.out"
echo $YGM_BENCH_HISTO_OUTPUT_FILE

global_log_table_sizes=(24 25 26 27)
local_updates=10000000
num_trials=10

echo "Running histo_ygm experiments"

for ((i = 0; i < "${#YGM_BENCH_NODES[@]}"; ++i)); do
	nodes="${YGM_BENCH_NODES[$i]}"
	table_size="${global_log_table_sizes[$i]}"
	for buffer_capacity in "${YGM_BENCH_BUFFER_CAPACITIES[@]}"; do
		$LAUNCH_PREFIX -N $nodes --ntasks-per-node $YGM_BENCH_PROCS_PER_NODE -t 1:00:00 ${YGM_BINARY_DIR}/histo_ygm $buffer_capacity $table_size \
		$local_updates $num_trials >> $YGM_BENCH_HISTO_OUTPUT_FILE
	done
done
