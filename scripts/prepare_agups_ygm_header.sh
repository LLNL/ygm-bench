#! /usr/bin/env bash

export YGM_BENCH_AGUPS_OUTPUT_FILE="${YGM_BENCH_OUTPUT_DIR}/agups_ygm.out"

date >> ${YGM_BENCH_AGUPS_OUTPUT_FILE}
echo "Nodes, Ranks per Node, Buffer Capacity, Routing Protocol, Global Table Size, Global Updaters, Updater Lifetime, Trial Time, Trial GUPS" \
	>> ${YGM_BENCH_AGUPS_OUTPUT_FILE}
