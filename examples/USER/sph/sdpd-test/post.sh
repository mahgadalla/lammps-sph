#! /bin/bash

awk 'fl{print $3, $4, $5, $6} /ITEM: ATOMS/{fl=1}' *.dat > punto.dat
