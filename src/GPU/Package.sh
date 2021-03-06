# Update package files in LAMMPS
# cp package file to src if doesn't exist or is different
# do not copy gayberne files if non-GPU version does not exist

for file in *.cpp *.h; do
  if (test $file = pair_gayberne_gpu.cpp -a ! -e ../pair_gayberne.cpp) then
    continue
  fi
  if (test $file = pair_gayberne_gpu.h -a ! -e ../pair_gayberne.cpp) then
    continue
  fi
  if (test $file = pair_lj_cut_coul_long_gpu.cpp -a ! -e ../pair_lj_cut_coul_long.cpp) then
    continue
  fi
  if (test $file = pair_lj_cut_coul_long_gpu.h -a ! -e ../pair_lj_cut_coul_long.cpp) then
    continue
  fi
  if (test $file = pair_coul_long_gpu.cpp -a ! -e ../pair_coul_long.cpp) then
    continue
  fi
  if (test $file = pair_coul_long_gpu.h -a ! -e ../pair_coul_long.cpp) then
    continue
  fi
  if (test $file = pair_cg_cmm_gpu.cpp -a ! -e ../pair_cg_cmm.cpp) then
    continue
  fi
  if (test $file = pair_cg_cmm_gpu.h -a ! -e ../pair_cg_cmm.cpp) then
    continue
  fi
  if (test $file = pair_cg_cmm_coul_long_gpu.cpp -a ! -e ../pair_cg_cmm_coul_long.cpp) then
    continue
  fi
  if (test $file = pair_cg_cmm_coul_long_gpu.h -a ! -e ../pair_cg_cmm_coul_long.cpp) then
    continue
  fi
  if (test $file = pair_cg_cmm_coul_msm.cpp -a ! -e ../pair_cg_cmm.cpp) then
    continue
  fi
  if (test $file = pair_cg_cmm_coul_msm.h -a ! -e ../pair_cg_cmm.cpp) then
    continue
  fi
  if (test $file = pair_cg_cmm_coul_msm_gpu.cpp -a ! -e ../pair_cg_cmm.cpp) then
    continue
  fi
  if (test $file = pair_cg_cmm_coul_msm_gpu.h -a ! -e ../pair_cg_cmm.cpp) then
    continue
  fi

  if (test ! -e ../$file) then
    echo "  creating src/$file"
    cp $file ..
  elif (test "`diff --brief $file ../$file`" != "") then
    echo "  updating src/$file"
    cp $file ..
  fi
done
