#! /usr/bin/env bash

YGM_BENCH_BUFFER_CAPACITIES=(4194304 16777216 67108864)
YGM_ROUTING_PROTOCOLS=("YGM_ROUTING_PROTOCOL_DIRECT" "YGM_ROUTING_PROTOCOL_NR")
#"YGM_ROUTING_PROTOCOL_NLNR")
YGM_BENCH_NUM_LISTENERS=(1 2)

global_log_table_size=$(echo ${SLURM_JOB_NUM_NODES} | python3 -c "import math; print(33+round(math.log2(float(input()))))")
local_updaters=2000000
updater_lifetime=10
num_trials=5

for routing_protocol in "${YGM_ROUTING_PROTOCOLS[@]}"; do
	for buffer_capacity in "${YGM_BENCH_BUFFER_CAPACITIES[@]}"; do
		for num_listeners in "${YGM_BENCH_NUM_LISTENERS[@]}"; do
			STATS_PREFIX="${YGM_BENCH_AGUPS_STATS_DIR}/agups_stats_N${SLURM_JOB_NUM_NODES}_buffer_capacity${buffer_capacity}_${routing_protocol}"
			$LAUNCH_PREFIX -N ${SLURM_JOB_NUM_NODES} --ntasks-per-node $YGM_BENCH_PROCS_PER_NODE -t 30:00 \
			--export=YGM_ROUTING_PROTOCOL=${routing_protocol} ${YGM_BINARY_DIR}/agups_ygm \
			$buffer_capacity $num_listeners $global_log_table_size $local_updaters $updater_lifetime $num_trials $STATS_PREFIX >> $YGM_BENCH_AGUPS_OUTPUT_FILE
		done
	done
done 
