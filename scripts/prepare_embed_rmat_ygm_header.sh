#! /usr/bin/env bash

export YGM_BENCH_EMBED_RMAT_OUTPUT_FILE="${YGM_BENCH_OUTPUT_DIR}/embed_rmat_ygm.out"
export YGM_BENCH_EMBED_RMAT_STATS_DIR="${YGM_BENCH_OUTPUT_DIR}/embed_rmat_ygm_stats"

date >> ${YGM_BENCH_EMBED_RMAT_OUTPUT_FILE}
echo "Nodes, Ranks per Node, Buffer Capacity, Routing Protocol, Embedding Dimension, Vertex Count, Edge Count, Compaction Threshold, Promotion Threshold, Random Seed, Trial Time, Trial Insertions/Sec (Billions)" \
	>> ${YGM_BENCH_EMBED_RMAT_OUTPUT_FILE}

mkdir -p ${YGM_BENCH_EMBED_RMAT_STATS_DIR}
