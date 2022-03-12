#! /usr/bin/env bash

export YGM_BENCH_ATW_YGM_OUTPUT_FILE="${YGM_BENCH_OUTPUT_DIR}/around_the_world_ygm.out"

date >> ${YGM_BENCH_ATW_YGM_OUTPUT_FILE}
echo "Nodes, Ranks per Node, Routing Protocol, Circumnavigations, Trial Time, Hops per Second, Total Header Bytes, Total Message Bytes, Barrier Allreduce Count" \
	>> ${YGM_BENCH_ATW_YGM_OUTPUT_FILE}
