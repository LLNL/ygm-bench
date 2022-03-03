#! /usr/bin/env bash

YGM_BENCH_BUFFER_CAPACITIES=(16777216 33554432 67108864)
YGM_ROUTING_PROTOCOLS=("YGM_ROUTING_PROTOCOL_DIRECT" "YGM_ROUTING_PROTOCOL_NR")

global_log_table_sizes=(24 25 26 27)
global_log_table_size=$(echo ${SLURM_JOB_NUM_NODES} | python3 -c "import math; print(24+round(math.log2(float(input()))))")
local_updates=1000000
num_trials=10

for routing_protocol in "${YGM_ROUTING_PROTOCOLS[@]}"; do
	for buffer_capacity in "${YGM_BENCH_BUFFER_CAPACITIES[@]}"; do
		$LAUNCH_PREFIX -N ${SLURM_JOB_NUM_NODES} --ntasks-per-node $YGM_BENCH_PROCS_PER_NODE -t 1:00:00 \
		--export=YGM_ROUTING_PROTOCOL=${routing_protocol} ${YGM_BINARY_DIR}/histo_ygm \
		$buffer_capacity $global_log_table_size $local_updates $num_trials >> $YGM_BENCH_HISTO_OUTPUT_FILE
	done
done 
