#! /bin/bash




../../../../src/lmp_linux -in sdpd-polymer-inti.lmp
../../../../tools/restart2data poly.restart poly.txt


 awk -v cutoff=3.0 -v Nbeads=9 -v Nsolvent=18 -v Npoly=full \
     -f addpolymer.awk poly.txt > poly2.txt
 nbound=$(tail -n 1 poly2.txt | awk '{print $1}')
 sed "s/_NUMBER_OF_BOUNDS_/$nbound/1" poly2.txt > poly.txt

time /scratch/qingguang/prefix-nana/bin/mpirun -np 4  ../../../../src/lmp_linux -in sdpd-polymer-run.lmp
#../../../../src/lmp_linux -in sdpd-polymer-run.lmp
