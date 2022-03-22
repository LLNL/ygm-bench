#! /usr/bin/env bash

YGM_ROUTING_PROTOCOLS=("YGM_ROUTING_PROTOCOL_DIRECT" "YGM_ROUTING_PROTOCOL_NR" "YGM_ROUTING_PROTOCOL_NLNR")
YGM_WAIT_UNTIL=("true" "false")

circumnavigations=1000
num_trials=10

for routing_protocol in "${YGM_ROUTING_PROTOCOLS[@]}"; do
		for wait_until in "${YGM_WAIT_UNTIL[@]}"; do
		$LAUNCH_PREFIX -N ${SLURM_JOB_NUM_NODES} --ntasks-per-node $YGM_BENCH_PROCS_PER_NODE -t 30:00 \
		--export=YGM_ROUTING_PROTOCOL=${routing_protocol},YGM_BENCH_ATW_WAIT_UNTIL=${wait_until} ${YGM_BINARY_DIR}/around_the_world_ygm \
		$circumnavigations $num_trials >> $YGM_BENCH_ATW_YGM_OUTPUT_FILE
	done
done 
