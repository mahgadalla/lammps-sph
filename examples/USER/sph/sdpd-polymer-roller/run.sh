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


nproc=8
ndim=2d
Nbeads=8
Nsolvent=8
nx=128
#Force=164
Force=112
etas=3e-2
etap=3e-2
H0=80
R0=4
Delta=0.8
dname=feex-nb${Nbeads}-ns${Nsolvent}-nx${nx}-H${H0}-R0${R0}-D${Delta}-f${Force}-etap${etap}
#dname=harmonic-nb${Nbeads}-ns${Nsolvent}-nx${nx}-H${H0}-R0${R0}-f${Force}-etap${etap}
vars="-var nx ${nx} -var ndim ${ndim} -var dname ${dname} \ 
      -var force ${Force} -var etas ${etas} -var etap ${etap} \
      -var H0 ${H0} -var R0 ${R0} -var Delta ${Delta}"

${lmp} ${vars} -in sdpd-polymer-init.lmp
${restart2data} poly3d.restart poly3d.txt


 awk -v cutoff=3.0 -v Nbeads=${Nbeads} -v Nsolvent=${Nsolvent} -v Npoly=full \
     -f addpolymer.awk poly3d.txt > poly3.txt
 nbound=$(tail -n 1 poly3.txt | awk '{print $1}')
 sed "s/_NUMBER_OF_BOUNDS_/$nbound/1" poly3.txt > poly3d.txt

# output directory name

mkdir -p ${dname}
${mpirun} -np ${nproc} ${lmp} ${vars} -in sdpd-polymer-run.lmp
