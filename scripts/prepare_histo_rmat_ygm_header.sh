#! /usr/bin/env bash

export YGM_BENCH_HISTO_RMAT_OUTPUT_FILE="${YGM_BENCH_OUTPUT_DIR}/histo_rmat_ygm.out"
export YGM_BENCH_HISTO_RMAT_STATS_DIR="${YGM_BENCH_OUTPUT_DIR}/histo_rmat_ygm_stats"

date >> ${YGM_BENCH_HISTO_RMAT_OUTPUT_FILE}
echo "Nodes, Ranks per Node, Buffer Capacity, Routing Protocol, Global Histogram Size, Global Insertions, Trial Time, Trial Insertions/Sec (Billions)" \
	>> ${YGM_BENCH_HISTO_RMAT_OUTPUT_FILE}

mkdir -p ${YGM_BENCH_HISTO_RMAT_STATS_DIR}
