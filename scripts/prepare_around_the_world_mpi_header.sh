#! /usr/bin/env bash

export YGM_BENCH_ATW_MPI_OUTPUT_FILE="${YGM_BENCH_OUTPUT_DIR}/around_the_world_mpi.out"

date >> ${YGM_BENCH_ATW_MPI_OUTPUT_FILE}
echo "Nodes, Ranks per Node, Circumnavigations, Trial Time, Hops per Second" \
	>> ${YGM_BENCH_ATW_MPI_OUTPUT_FILE}
