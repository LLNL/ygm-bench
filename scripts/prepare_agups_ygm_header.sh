#! /usr/bin/env bash

export YGM_BENCH_AGUPS_OUTPUT_FILE="${YGM_BENCH_OUTPUT_DIR}/agups_ygm.out"
export YGM_BENCH_AGUPS_STATS_DIR="${YGM_BENCH_OUTPUT_DIR}/agups_ygm_stats"

date >> ${YGM_BENCH_AGUPS_OUTPUT_FILE}
echo "Nodes, Ranks per Node, Buffer Capacity, Listener Threads, Routing Protocol, Global Table Size, Global Updaters, Updater Lifetime, Trial Time, Trial GUPS" \
	>> ${YGM_BENCH_AGUPS_OUTPUT_FILE}

mkdir -p ${YGM_BENCH_AGUPS_STATS_DIR}
