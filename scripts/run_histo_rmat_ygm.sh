#! /usr/bin/env bash

YGM_BENCH_BUFFER_CAPACITIES=(4096 16384 65536)
YGM_ROUTING_PROTOCOLS=("NONE" "NR", "NLNR")

global_log_table_size=$(echo ${SLURM_JOB_NUM_NODES} | python3 -c "import math; print(26+round(math.log2(float(input()))))")
local_updates=10000000
num_trials=5

for routing_protocol in "${YGM_ROUTING_PROTOCOLS[@]}"; do
	for buffer_capacity in "${YGM_BENCH_BUFFER_CAPACITIES[@]}"; do
		$LAUNCH_PREFIX -N ${SLURM_JOB_NUM_NODES} --ntasks-per-node $YGM_BENCH_PROCS_PER_NODE -t 30:00 \
		--export=YGM_COMM_ROUTING=${routing_protocol},YGM_COMM_BUFFER_SIZE_KB=${buffer_capacity} ${YGM_BINARY_DIR}/histo_rmat_ygm \
		$buffer_capacity $num_listeners $global_log_table_size $local_updates $num_trials $STATS_PREFIX >> $YGM_BENCH_HISTO_RMAT_OUTPUT_FILE
	done
done 
