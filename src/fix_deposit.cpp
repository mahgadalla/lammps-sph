/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under 
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#include "math.h"
#include "stdlib.h"
#include "string.h"
#include "fix_deposit.h"
#include "atom.h"
#include "atom_vec.h"
#include "force.h"
#include "update.h"
#include "comm.h"
#include "domain.h"
#include "lattice.h"
#include "region.h"
#include "random_park.h"
#include "memory.h"
#include "error.h"

using namespace LAMMPS_NS;

/* ---------------------------------------------------------------------- */

FixDeposit::FixDeposit(LAMMPS *lmp, int narg, char **arg) :
  Fix(lmp, narg, arg)
{
  if (narg < 7) error->all("Illegal fix deposit command");

  // required args

  ninsert = atoi(arg[3]);
  ntype = atoi(arg[4]);
  nfreq = atoi(arg[5]);
  seed = atoi(arg[6]);

  // set defaults

  iregion = -1;
  globalflag = localflag = 0;
  lo = hi = deltasq = 0.0;
  nearsq = 0.0;
  maxattempt = 10;
  rateflag = 0;
  vxlo = vxhi = vylo = vyhi = vzlo = vzhi = 0.0;
  scaleflag = 1;

  // read options from end of input line

  options(narg-7,&arg[7]);

  // error check on region

  if (iregion == -1) error->all("Must specify a region in fix deposit");
  if (domain->regions[iregion]->interior == 0)
    error->all("Must use region with side = in with fix deposit");

  // store extent of region

  xlo = domain->regions[iregion]->extent_xlo;
  xhi = domain->regions[iregion]->extent_xhi;
  ylo = domain->regions[iregion]->extent_ylo;
  yhi = domain->regions[iregion]->extent_yhi;
  zlo = domain->regions[iregion]->extent_zlo;
  zhi = domain->regions[iregion]->extent_zhi;

  // setup scaling

  if (scaleflag && domain->lattice == NULL)
    error->all("Use of fix deposit with undefined lattice");

  double xscale,yscale,zscale;
  if (scaleflag) {
    xscale = domain->lattice->xlattice;
    yscale = domain->lattice->ylattice;
    zscale = domain->lattice->zlattice;
  }
  else xscale = yscale = zscale = 1.0;

  // apply scaling to all input parameters with dist/vel units

  if (force->dimension == 2) {
    lo *= yscale;
    hi *= yscale;
    rate *= yscale;
  } else {
    lo *= zscale;
    hi *= zscale;
    rate *= zscale;
  }
  deltasq *= xscale*xscale;
  nearsq *= xscale*xscale;
  vxlo *= xscale;
  vxhi *= xscale;
  vylo *= yscale;
  vyhi *= yscale;
  vzlo *= zscale;
  vzhi *= zscale;

  // store extent of region

  xlo = domain->regions[iregion]->extent_xlo;
  xhi = domain->regions[iregion]->extent_xhi;
  ylo = domain->regions[iregion]->extent_ylo;
  yhi = domain->regions[iregion]->extent_yhi;
  zlo = domain->regions[iregion]->extent_zlo;
  zhi = domain->regions[iregion]->extent_zhi;

  // random number generator, same for all procs

  random = new RanPark(lmp,seed);

  // set up reneighboring

  force_reneighbor = 1;
  next_reneighbor = update->ntimestep + 1;
  nfirst = next_reneighbor;
  ninserted = 0;
}

/* ---------------------------------------------------------------------- */

FixDeposit::~FixDeposit()
{
  delete random;
}

/* ---------------------------------------------------------------------- */

int FixDeposit::setmask()
{
  int mask = 0;
  mask |= PRE_EXCHANGE;
  return mask;
}

/* ----------------------------------------------------------------------
   perform particle insertion
------------------------------------------------------------------------- */

void FixDeposit::pre_exchange()
{
  int flag,flagall;
  double coord[3],lamda[3],delx,dely,delz,rsq;
  double *newcoord;

  // just return if should not be called on this timestep

  if (next_reneighbor != update->ntimestep) return;

  // compute current offset = bottom of insertion volume

  double offset = 0.0;
  if (rateflag) offset = (update->ntimestep - nfirst) * update->dt * rate;

  double *sublo,*subhi;
  if (domain->triclinic == 0) {
    sublo = domain->sublo;
    subhi = domain->subhi;
  } else {
    sublo = domain->sublo_lamda;
    subhi = domain->subhi_lamda;
  }

  // attempt an insertion until successful
  
  int success = 0;
  int attempt = 0;
  while (attempt < maxattempt) {
    attempt++;

    // choose random position for new atom within region

    coord[0] = xlo + random->uniform() * (xhi-xlo);
    coord[1] = ylo + random->uniform() * (yhi-ylo);
    coord[2] = zlo + random->uniform() * (zhi-zlo);
    while (domain->regions[iregion]->match(coord[0],coord[1],coord[2]) == 0) {
      coord[0] = xlo + random->uniform() * (xhi-xlo);
      coord[1] = ylo + random->uniform() * (yhi-ylo);
      coord[2] = zlo + random->uniform() * (zhi-zlo);
    }

    // adjust vertical coord by offset

    if (force->dimension == 2) coord[1] += offset;
    else coord[2] += offset;

    // if global, reset vertical coord to be lo-hi above highest atom
    // if local, reset vertical coord to be lo-hi above highest "nearby" atom
    // local computation computes lateral distance between 2 particles w/ PBC

    if (globalflag || localflag) {
      int dim;
      double max,maxall,delx,dely,delz,rsq;

      if (force->dimension == 2) {
	dim = 1;
	max = domain->boxylo;
      } else {
	dim = 2;
	max = domain->boxzlo;
      }

      double **x = atom->x;
      int nlocal = atom->nlocal;
      for (int i = 0; i < nlocal; i++) {
	if (localflag) {
	  delx = coord[0] - x[i][0];
	  dely = coord[1] - x[i][1];
	  delz = 0.0;
	  domain->minimum_image(delx,dely,delz);
	  if (force->dimension == 2) rsq = delx*delx;
	  else rsq = delx*delx + dely*dely;
	  if (rsq > deltasq) continue;
	}
	if (x[i][dim] > max) max = x[i][dim];
      }

      MPI_Allreduce(&max,&maxall,1,MPI_DOUBLE,MPI_MAX,world);
      if (force->dimension == 2)
	coord[1] = maxall + lo + random->uniform()*(hi-lo);
      else
	coord[2] = maxall + lo + random->uniform()*(hi-lo);
    }      

    // now have final coord
    // if distance to any atom is less than near, try again

    double **x = atom->x;
    int nlocal = atom->nlocal;

    flag = 0;
    for (int i = 0; i < nlocal; i++) {
      delx = coord[0] - x[i][0];
      dely = coord[1] - x[i][1];
      delz = coord[2] - x[i][2];
      domain->minimum_image(delx,dely,delz);
      rsq = delx*delx + dely*dely + delz*delz;
      if (rsq < nearsq) flag = 1;
    }
    MPI_Allreduce(&flag,&flagall,1,MPI_INT,MPI_MAX,world);
    if (flagall) continue;

    // insertion will proceed
    // choose random velocity for new atom

    double vxtmp = vxlo + random->uniform() * (vxhi-vxlo);
    double vytmp = vylo + random->uniform() * (vyhi-vylo);
    double vztmp = vzlo + random->uniform() * (vzhi-vzlo);

    // check if new atom is in my sub-box or above it if I'm highest proc
    // if so, add to my list via create_atom()
    // initialize info about the atoms
    // set group mask to "all" plus fix group
    
    if (domain->triclinic) {
      domain->x2lamda(coord,lamda);
      newcoord = lamda;
    } else newcoord = coord;

    flag = 0;
    if (newcoord[0] >= sublo[0] && newcoord[0] < subhi[0] &&
	newcoord[1] >= sublo[1] && newcoord[1] < subhi[1] &&
	newcoord[2] >= sublo[2] && newcoord[2] < subhi[2]) flag = 1;
    else if (force->dimension == 3 && newcoord[2] >= domain->boxzhi &&
	     comm->myloc[2] == comm->procgrid[2]-1 &&
	     newcoord[0] >= sublo[0] && newcoord[0] < subhi[0] &&
	     newcoord[1] >= sublo[1] && newcoord[1] < subhi[1]) flag = 1;
    else if (force->dimension == 2 && newcoord[1] >= domain->boxyhi &&
	     comm->myloc[1] == comm->procgrid[1]-1 &&
	     newcoord[0] >= sublo[0] && newcoord[0] < subhi[0]) flag = 1;

    if (flag) {
      atom->avec->create_atom(ntype,coord,0);
      int m = atom->nlocal - 1;
      atom->type[m] = ntype;
      atom->mask[m] = 1 | groupbit;
      atom->v[m][0] = vxtmp;
      atom->v[m][1] = vytmp;
      atom->v[m][2] = vztmp;
    }
    MPI_Allreduce(&flag,&success,1,MPI_INT,MPI_MAX,world);
    break;
  }

  // warn if not successful b/c too many attempts or no proc owned particle

  if (comm->me == 0)
    if (success == 0)
      error->warning("Particle deposition was unsuccessful");

  // set tag # of new particle beyond all previous atoms
  // reset global natoms
  // if global map exists, reset it

  if (success && atom->tag_enable) {
    atom->tag_extend();
    atom->natoms += 1;
    if (atom->map_style) {
      atom->map_init();
      atom->map_set();
    }
  }

  // next timestep to insert

  if (ninserted < ninsert) next_reneighbor += nfreq;
  else next_reneighbor = 0;
}

/* ----------------------------------------------------------------------
   parse optional parameters at end of input line 
------------------------------------------------------------------------- */

void FixDeposit::options(int narg, char **arg)
{
  if (narg < 0) error->all("Illegal fix indent command");

  int iarg = 0;
  while (iarg < narg) {
    if (strcmp(arg[iarg],"region") == 0) {
      if (iarg+2 > narg) error->all("Illegal fix deposit command");
      for (iregion = 0; iregion < domain->nregion; iregion++)
	if (strcmp(arg[iarg+1],domain->regions[iregion]->id) == 0) break;
      if (iregion == domain->nregion) 
	error->all("Fix deposit region ID does not exist");
      iarg += 2;
    } else if (strcmp(arg[iarg],"global") == 0) {
      if (iarg+3 > narg) error->all("Illegal fix deposit command");
      globalflag = 1;
      localflag = 0;
      lo = atof(arg[iarg+1]);
      hi = atof(arg[iarg+2]);
      iarg += 3;
    } else if (strcmp(arg[iarg],"local") == 0) {
      if (iarg+4 > narg) error->all("Illegal fix deposit command");
      localflag = 1;
      globalflag = 0;
      lo = atof(arg[iarg+1]);
      hi = atof(arg[iarg+2]);
      deltasq = atof(arg[iarg+3])*atof(arg[iarg+3]);
      iarg += 4;
    } else if (strcmp(arg[iarg],"near") == 0) {
      if (iarg+2 > narg) error->all("Illegal fix deposit command");
      nearsq = atof(arg[iarg+1])*atof(arg[iarg+1]);
      iarg += 2;
    } else if (strcmp(arg[iarg],"attempt") == 0) {
      if (iarg+2 > narg) error->all("Illegal fix deposit command");
      maxattempt = atoi(arg[iarg+1]);
      iarg += 2;
    } else if (strcmp(arg[iarg],"rate") == 0) {
      if (iarg+2 > narg) error->all("Illegal fix deposit command");
      rateflag = 1;
      rate = atof(arg[iarg+1]);
      iarg += 2;
    } else if (strcmp(arg[iarg],"vx") == 0) {
      if (iarg+3 > narg) error->all("Illegal fix deposit command");
      vxlo = atof(arg[iarg+1]);
      vxhi = atof(arg[iarg+2]);
      iarg += 3;
    } else if (strcmp(arg[iarg],"vy") == 0) {
      if (iarg+3 > narg) error->all("Illegal fix deposit command");
      vylo = atof(arg[iarg+1]);
      vyhi = atof(arg[iarg+2]);
      iarg += 3;
    } else if (strcmp(arg[iarg],"vz") == 0) {
      if (iarg+3 > narg) error->all("Illegal fix deposit command");
      vzlo = atof(arg[iarg+1]);
      vzhi = atof(arg[iarg+2]);
      iarg += 3;
    } else if (strcmp(arg[iarg],"units") == 0) {
      if (iarg+2 > narg) error->all("Illegal fix deposit command");
      if (strcmp(arg[iarg+1],"box") == 0) scaleflag = 0;
      else if (strcmp(arg[iarg+1],"lattice") == 0) scaleflag = 1;
      else error->all("Illegal fix deposit command");
      iarg += 2;
    } else error->all("Illegal fix deposit command");
  }
}
