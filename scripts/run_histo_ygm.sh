#! /usr/bin/env bash

YGM_BENCH_BUFFER_CAPACITIES=(4194304 16777216 67108864)
YGM_ROUTING_PROTOCOLS=("YGM_ROUTING_PROTOCOL_DIRECT" "YGM_ROUTING_PROTOCOL_NR")
#"YGM_ROUTING_PROTOCOL_NLNR")
YGM_BENCH_NUM_LISTENERS=(1 2)

global_log_table_size=$(echo ${SLURM_JOB_NUM_NODES} | python3 -c "import math; print(26+round(math.log2(float(input()))))")
local_updates=50000000
num_trials=5

for routing_protocol in "${YGM_ROUTING_PROTOCOLS[@]}"; do
	for buffer_capacity in "${YGM_BENCH_BUFFER_CAPACITIES[@]}"; do
		for num_listeners in "${YGM_BENCH_NUM_LISTENERS[@]}"; do
			STATS_PREFIX="${YGM_BENCH_HISTO_STATS_DIR}/histo_stats_N${SLURM_JOB_NUM_NODES}_buffer_capacity${buffer_capacity}_${routing_protocol}"
			$LAUNCH_PREFIX -N ${SLURM_JOB_NUM_NODES} --ntasks-per-node $YGM_BENCH_PROCS_PER_NODE -t 30:00 \
			--export=YGM_ROUTING_PROTOCOL=${routing_protocol} ${YGM_BINARY_DIR}/histo_ygm \
			$buffer_capacity $num_listeners $global_log_table_size $local_updates $num_trials $STATS_PREFIX >> $YGM_BENCH_HISTO_OUTPUT_FILE
		done
	done
done 
