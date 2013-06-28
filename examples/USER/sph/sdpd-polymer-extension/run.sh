#! /bin/bash

set -e
set -u
configfile=$HOME/lammps-sph.sh
if [ -f "${configfile}" ]; then
    source "${configfile}"
else
    printf "cannot find config file: %s\n" ${configfile} > "/dev/stderr"
    exit -1
fi

rm -rf dum* im* poly* log.lammps

nproc=1
ndim=2d
Nbeads=16
Nsolvent=
Force=0.01
nx=48
eta=0.03
dname=fene-nb${Nbeads}-ns${Nsolvent}-nx${nx}-H0.4-bg1.0-f${Force}-eta${eta}

vars="-var nx ${nx} -var Nbeads ${Nbeads} -var Nsolvent ${Nsolvent} -var ndim ${ndim} -var dname ${dname} -var force ${Force}"

${lmp} ${vars} -in sdpd-polymer-init.lmp
${restart2data} poly3d.restart poly3d.txt


 awk -v cutoff=3.0 -v Nbeads=${Nbeads} -v Nsolvent=${Nsolvent} -v Npoly=full \
     -f addpolymer.awk poly3d.txt > poly3.txt
 nbound=$(tail -n 1 poly3.txt | awk '{print $1}')
 sed "s/_NUMBER_OF_BOUNDS_/$nbound/1" poly3.txt > poly3d.txt

# output directory name

mkdir -p ${dname}
${mpirun} -np ${nproc} ${lmp} ${vars} -in sdpd-polymer-run.lmp
