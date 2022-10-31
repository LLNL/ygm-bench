#! /usr/bin/env bash

YGM_BENCH_BUFFER_CAPACITIES=(67108864 268435456 1073741824)
YGM_ROUTING_PROTOCOLS=("YGM_ROUTING_PROTOCOL_DIRECT" "YGM_ROUTING_PROTOCOL_NR" "YGM_ROUTING_PROTOCOL_NLNR")

embedding_dimension=8
global_log_vertex_count=$(echo ${SLURM_JOB_NUM_NODES} | python3 -c "import math; print(26+round(math.log2(float(input()))))")
local_edge_count=10000000
num_trials=10
random_seed=1
embed_bool=1
stream_bool=0
rmat_bool=0

for routing_protocol in "${YGM_ROUTING_PROTOCOLS[@]}"; do
	for buffer_capacity in "${YGM_BENCH_BUFFER_CAPACITIES[@]}"; do
		STATS_PREFIX="${YGM_BENCH_EMBED_STATS_DIR}/embed_stats_N${SLURM_JOB_NUM_NODES}_buffer_capacity${buffer_capacity}_${routing_protocol}"
		$LAUNCH_PREFIX -N ${SLURM_JOB_NUM_NODES} \
		--ntasks-per-node $YGM_BENCH_PROCS_PER_NODE -t 30:00 \
		--export=YGM_ROUTING_PROTOCOL=${routing_protocol} \
		${YGM_BINARY_DIR}/embed_ygm \
		$buffer_capacity $embedding_dimension $global_log_vertex_count \
		$local_edge_count $num_trials $random_seed $STATS_PREFIX $embed_bool \
		$stream_bool $rmat_bool >> $YGM_BENCH_EMBED_OUTPUT_FILE
	done
done 
