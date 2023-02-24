#! /usr/bin/env bash

YGM_BENCH_BUFFER_CAPACITIES=(4096 16384 65536)
YGM_ROUTING_PROTOCOLS=("NONE" "NR", "NLNR")

global_log_table_size=$(echo ${SLURM_JOB_NUM_NODES} | python3 -c "import math; print(33+round(math.log2(float(input()))))")
local_updaters=2000000
updater_lifetime=10
num_trials=5

for routing_protocol in "${YGM_ROUTING_PROTOCOLS[@]}"; do
	for buffer_capacity in "${YGM_BENCH_BUFFER_CAPACITIES[@]}"; do
		$LAUNCH_PREFIX -N ${SLURM_JOB_NUM_NODES} --ntasks-per-node $YGM_BENCH_PROCS_PER_NODE -t 30:00 \
		--export=YGM_COMM_ROUTING=${routing_protocol},YGM_COMM_BUFFER_SIZE_KB=${buffer_capacity} ${YGM_BINARY_DIR}/agups_ygm \
		$global_log_table_size $local_updaters $updater_lifetime $num_trials $STATS_PREFIX >> $YGM_BENCH_AGUPS_OUTPUT_FILE
	done
done 
