#! /bin/bash

set -e
set -u
configfile=$HOME/lammps-sdpd.sh
if [ -f "${configfile}" ]; then
    source "${configfile}"
else
    printf "cannot find config file: %s\n" ${configfile} > "/dev/stderr"
    exit -1
fi

nproc=1
ndim=2
sdpd_eta=8.0
sdpd_background=0.00
sdpd_c=1.44e2
sdpd_gamma=1.00
nx=16
n=4.5
ktype=wendland4
temp=0.05
grid=1

dname=c${sdpd_c}-temp${temp}-gamma${sdpd_gamma}-eta${sdpd_eta}-background${sdpd_background}-nx${nx}-n${n}-ktype${ktype}-grid${grid}

vars="-var sdpd_gamma ${sdpd_gamma} -var ndim ${ndim} -var dname ${dname} -var sdpd_c ${sdpd_c} \
      -var nx   ${nx} -var sdpd_eta ${sdpd_eta} -var sdpd_background ${sdpd_background} \
      -var  n   ${n} -var ktype ${ktype} -var temp ${temp} -var grid ${grid}"

mkdir -p ${dname}
${mpirun} -np ${nproc} ${lmp} ${vars} -in solvent.lmp
