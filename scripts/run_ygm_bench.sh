#! /usr/bin/env bash

# Define experiment variables. Can be overridden safely by individual experiments
YGM_BENCH_NODES=(4 8 16 32 64 128 256)
export YGM_BENCH_MACHINE=$(hostname | sed 's/[0-9]*//g')
export YGM_BENCH_GCC_VERSION=$(gcc --version | grep -E -o "[0-9]+\.[0-9]+\.[0-9]+" | head -n 1)
export YGM_BENCH_PROCS_PER_NODE=$(nproc)

export YGM_BENCH_YGM_REPO="https://github.com/steiltre/ygm.git"
export YGM_BENCH_YGM_TAG="feature/routing"

# Build ygm-bench
cd ..
rm -rf build
mkdir -p build
cd build
cmake ..
make

# Gather information about ygm-bench and ygm current commits
ygm_bench_hash=$(git rev-parse HEAD)
cd _deps/ygm-src
ygm_hash=$(git rev-parse HEAD)
cd ../../..

# Create location for output of results
export YGM_BENCH_OUTPUT_DIR="/p/lustre1/steil1/experiments/ygm_bench_results/${YGM_BENCH_MACHINE}/ygm-bench_${ygm_bench_hash}/ygm_${ygm_hash}/gcc_${YGM_BENCH_GCC_VERSION}"
mkdir -p ${YGM_BENCH_OUTPUT_DIR}

export YGM_BINARY_DIR="${YGM_BENCH_OUTPUT_DIR}/src"

# Copy build directory to avoid issues if build directory is deleted or code is recompiled before tests run
cp -r build/src ${YGM_BINARY_DIR}
cp -r scripts ${YGM_BENCH_OUTPUT_DIR}

# All tests run from output directory
export LAUNCH_PREFIX="srun -D ${YGM_BENCH_OUTPUT_DIR}"
cd ${YGM_BENCH_OUTPUT_DIR}/scripts

. prepare_headers.sh

# Run individual test scripts
for nodes in "${YGM_BENCH_NODES[@]}"; do
	salloc -N ${nodes} -t 8:00:00 -A hpcgeda ./run_experiment_loop.sh &
done

wait
