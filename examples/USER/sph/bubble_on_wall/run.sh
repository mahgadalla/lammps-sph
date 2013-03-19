#! /bin/bash

set -e
set -u
Lx=2.0
nx=30
ndim=3
np=1
D_heat_d=0.6
D_heat_g=0.1
sph_eta_d=0.069395
cv_d=1.0
cv_g=0.04
dT=0.1
Hwv=32.0
alpha=300
dprob=0.01
sph_rho_d=0.01
time_k=1.0
gy=$1
# parameters for kana
lmp=../../../../src/lmp_linux
mpirun=mpirun
proc="-np ${np}"

dname=data-nx${nx}-ndim${ndim}-Lx${Lx}-D_heat_d${D_heat_d}-alpha${alpha}\
-Hwv${Hwv}-dprob${dprob}-time_k${time_k}-cv_d${cv_d}-sph_rho_d${sph_rho_d}-dT${dT}\
-cv_g${cv_g}-D_heat_g${D_heat_g}-sph_eta_d${sph_eta_d}-gy${gy}

rm -rf ${dname}
mkdir -p ${dname}
${mpirun} ${proc} ${lmp} -in insert.lmp \
    -var alpha ${alpha} -var D_heat_d ${D_heat_d} -var ndim ${ndim} -var nx ${nx} \
    -var Hwv ${Hwv} -var dprob ${dprob} -var time_k ${time_k} \
    -var Lx ${Lx} -var cv_d ${cv_d} -var sph_rho_d ${sph_rho_d} -var dT ${dT} \
    -var cv_g ${cv_g} -var D_heat_g ${D_heat_g} \
    -var dname ${dname} -var sph_eta_d ${sph_eta_d} -var gy ${gy}
