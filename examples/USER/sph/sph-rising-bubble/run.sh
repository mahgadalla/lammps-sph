#! /bin/bash

dname=data
mkdir -p ${dname}
mpirun -np 2  ../../../../src/lmp_linux -in bubble.lmp -var dname ${dname}

