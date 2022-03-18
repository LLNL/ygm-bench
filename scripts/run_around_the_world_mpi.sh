#! /usr/bin/env bash

circumnavigations=1000
num_trials=10

$LAUNCH_PREFIX -N ${SLURM_JOB_NUM_NODES} --ntasks-per-node $YGM_BENCH_PROCS_PER_NODE -t 30:00 \
${YGM_BINARY_DIR}/around_the_world_mpi \
$circumnavigations $num_trials >> $YGM_BENCH_ATW_MPI_OUTPUT_FILE
