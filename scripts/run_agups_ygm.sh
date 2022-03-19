#! /usr/bin/env bash

YGM_BENCH_BUFFER_CAPACITIES=(1048576 4194304 16777216 67108864 268435456)
YGM_ROUTING_PROTOCOLS=("YGM_ROUTING_PROTOCOL_DIRECT" "YGM_ROUTING_PROTOCOL_NR" "YGM_ROUTING_PROTOCOL_NLNR")

global_log_table_size=$(echo ${SLURM_JOB_NUM_NODES} | python3 -c "import math; print(33+round(math.log2(float(input()))))")
local_updaters=1000000
updater_lifetime=10
num_trials=10

for routing_protocol in "${YGM_ROUTING_PROTOCOLS[@]}"; do
	for buffer_capacity in "${YGM_BENCH_BUFFER_CAPACITIES[@]}"; do
		STATS_PREFIX="${YGM_BENCH_AGUPS_STATS_DIR}/agups_stats_N${SLURM_JOB_NUM_NODES}_buffer_capacity${buffer_capacity}_${routing_protocol}"
		$LAUNCH_PREFIX -N ${SLURM_JOB_NUM_NODES} --ntasks-per-node $YGM_BENCH_PROCS_PER_NODE -t 30:00 \
		--export=YGM_ROUTING_PROTOCOL=${routing_protocol} ${YGM_BINARY_DIR}/agups_ygm \
		$buffer_capacity $global_log_table_size $local_updaters $updater_lifetime $num_trials $STATS_PREFIX >> $YGM_BENCH_AGUPS_OUTPUT_FILE
	done
done 
