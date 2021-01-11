#!/bin/bash
# Submission script for Lemaitre3 
#SBATCH --job-name=weak-scalability
#SBATCH --time=01:00:00
#
##SBATCH --ntasks=64
#SBATCH --ntasks-per-node=64
#SBATCH --mem-per-cpu=2625
# ##SBATCH --partition=batch,debug 
#
# #SBATCH --mail-user=thomas.gillis@uclouvain.be
# #SBATCH --mail-type=ALL

HOME_TST=/home/ucl/tfl/tgillis/openmpi-dbg/openmpi-osc-testcase
SCRATCH=$GLOBALSCRATCH
RUN_DIR=ompi_dbg_${SLURM_JOB_NUM_NODES}_${SLURM_JOB_ID}
#RUN_DIR=murphy_weak/murphy_weak_${SLURM_JOB_NUM_NODES}_${SLURM_JOB_ID}

# create the tmp directory
mkdir -p $SCRATCH/$RUN_DIR

# copy what is needed
cp -r $HOME_TST/ompi_active $SCRATCH/$RUN_DIR
cp -r $HOME_TST/ompi_passive $SCRATCH/$RUN_DIR

# go there
cd $SCRATCH/$RUN_DIR 

# run that shit
echo "UCX:"
mpirun --mca osc ucx ./ompi_active
mpirun --mca osc ucx ./ompi_passive

echo "PT2PT:"
mpirun --mca osc pt2pt ./ompi_active
mpirun --mca osc pt2pt ./ompi_passive
#mpirun --mca osc ucx valgrind --error-limit=no --leak-check=full --suppressions=${EBROOTOPENMPI}/share/openmpi/openmpi-valgrind.supp ./murphy --abc --profile --iter-max=100 --iter-dump=50 --dump-detail=0 --rtol=1e-4 --ctol=1e-6 --vr-sigma=0.025 --dom=1,1,$SLURM_JOB_NUM_NODES > log_$SLURM_JOB_ID.log
