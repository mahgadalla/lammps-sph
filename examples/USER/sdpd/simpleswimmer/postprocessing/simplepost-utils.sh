#!/bin/bash

configfile=$HOME/lammps-sdpd.sh
if [ -f "${configfile}" ]; then
    source "${configfile}"
else
    printf "cannot find config file: %s\n" ${configfile} > "/dev/stderr"
    exit -1
fi

# get a simulation parameter from the directory name
function getp_aux() {
    local d=$1
    local p=$2
    
    function cmd() {
	sed -e 's/^.*lmp_linux//g' -e 's/-in .*//g' ${d}/run.command
    }

    (cat ${d}/vars.lmp; echo "print \${${p}}") | ${lmp} $(cmd) | tail -n 1	
}

function getntype() {
    local d=$1
    local atype=$(getp_aux ${d} $2)
    awk -v type=${atype} 'f&&($2==type){s++}; /ITEM: ATOMS/{f=1} END {print s}' ${d}/confs/dump.000000000.dat
}

function getp() {
    local d=$1
    local p=$2
    if [ ${p} = "polymer_concentration" ]; then
	# get concentration
	awk -v n1=$(getntype ${d} solventatomtype) -v n2=$(getntype ${d} polymeratomtype) 'BEGIN {print n2/(n1+n2)}'

    else
	getp_aux ${d} ${p}
    fi
}


getp ../supermuc-data/c3e2-nbeads10-nsolvent5-K_wave500-T_wave20-v_wave2-dsize150mass3/ polymer_concentration
getp ../supermuc-data/c3e2-nbeads10-nsolvent20-K_wave500-T_wave20-v_wave2-dsize150mass3/ polymer_concentration
getp ../supermuc-data/c3e2-nbeads0-nsolvent20-K_wave500-T_wave20-v_wave2-dsize150mass3/ polymer_concentration

getp ../supermuc-data/c3e2-nbeads0-nsolvent20-K_wave500-T_wave20-v_wave2-dsize150mass3/ v_wave
