#! /bin/bash

configfile=$HOME/lammps-sph.sh
if [ -f "${configfile}" ]; then
    source "${configfile}"
else
    printf "cannot find config file: %s\n" ${configfile} > "/dev/stderr"
    exit -1
fi

rm -rf dum* im* vx.av log* poly*
${lmp} -in sdpd-polymer3D-inti.lmp
${restart2data} poly3d.restart poly3d.txt


 awk -v cutoff=3.0 -v Nbeads=5 -v Nsolvent=5 -v Npoly=full \
     -f addpolymer.awk poly3d.txt > poly3.txt
 nbound=$(tail -n 1 poly3.txt | awk '{print $1}')
 sed "s/_NUMBER_OF_BOUNDS_/$nbound/1" poly3.txt > poly3d.txt


time ${mpirun} -np 4  ${lmp} -in sdpd-polymer3D-run.lmp
#mpirun  -np 2 ../../../../src/lmp_linux -in sdpd-polymer3D-run.lmp
#../../../../src/lmp_linux -in sdpd-polymer-run.lmp
