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

#include "string.h"
#include "compute_meso_diff_atom.h"
#include "sph_kernel_quintic.h"
#include "atom.h"
#include "input.h"
#include "variable.h"
#include "update.h"
#include "modify.h"
#include "comm.h"
#include "force.h"
#include "memory.h"
#include "domain.h"
#include "error.h"
#include "neighbor.h"
#include "neigh_list.h"
#include "neigh_request.h"

using namespace LAMMPS_NS;

/* ---------------------------------------------------------------------- */

ComputeMesoDiffAtom::ComputeMesoDiffAtom(LAMMPS *lmp, int narg, char **arg) :
  Compute(lmp, narg, arg)
{
  if (narg != 5) error->all(FLERR,"Illegal compute meso_rho/atom command");
  if (atom->rho_flag != 1) error->all(FLERR,"compute meso_rho/atom command requires atom_style with density (e.g. meso)");

  cutoff = force->numeric(FLERR,arg[3]);
  cutsq  = cutoff*cutoff;

  int iarg = 4;
  if (strncmp(arg[iarg],"v_",2) !=  0) {
    error->warning(FLERR," Illegal command meso_diff/atom, 4th argument must be a variable (v_*) ");    
  }
  int n = strlen(arg[iarg]);
  char *suffix = new char[n];
  strcpy(suffix,&arg[iarg][2]);
  char *ids = new char[n];
  strcpy(ids,suffix);
  ivariable = input->variable->find(ids);
  if (input->variable->atomstyle(ivariable) == 0)
    error->all(FLERR,"meso_diff/atomstyle variable is not atom-style variable");

  peratom_flag = 1;
  size_peratom_cols = 3;
  comm_forward = 1;

  nmax = 0;
  diffVector = NULL;
  varVector = NULL;
}

/* ---------------------------------------------------------------------- */

ComputeMesoDiffAtom::~ComputeMesoDiffAtom()
{
  memory->destroy(diffVector);
  memory->destroy(varVector);
}

/* ---------------------------------------------------------------------- */

void ComputeMesoDiffAtom::init()
{

  int count = 0;
  for (int i = 0; i < modify->ncompute; i++)
    if (strcmp(modify->compute[i]->style,"diffVector/atom") == 0) count++;
  if (count > 1 && comm->me == 0)
    error->warning(FLERR,"More than one compute diffVector/atom");

  // need a full neighbor list
  int irequest = neighbor->request(this);
  neighbor->requests[irequest]->pair = 0;
  neighbor->requests[irequest]->compute = 1;
  neighbor->requests[irequest]->half = 0;
  neighbor->requests[irequest]->full = 1;
  neighbor->requests[irequest]->occasional = 0;

}

void ComputeMesoDiffAtom::init_list(int id, NeighList *ptr)
{
  list = ptr;
}

/* ---------------------------------------------------------------------- */

void ComputeMesoDiffAtom::compute_peratom()
{
  double **x = atom->x;
  double *rho = atom->rho;
  int *type = atom->type;
  double *mass = atom->mass;

  int inum = list->inum;
  int* ilist = list->ilist;
  int* numneigh = list->numneigh;
  int** firstneigh = list->firstneigh;

  invoked_peratom = update->ntimestep;

  // grow diffVector array if necessary

  if (atom->nlocal > nmax) {
    memory->destroy(diffVector);
    memory->destroy(varVector);
    nmax = atom->nmax;
    memory->create(diffVector,nmax,3,"stress/atom:stress");
    memory->create(varVector,nmax,"stress/atom:stress");
    array_atom = diffVector;
  }

  input->variable->compute_atom(ivariable,igroup,varVector,1,0);
  comm->forward_comm_compute(this);

  for (int ii = 0; ii < inum; ii++) {
    int i = ilist[ii];
    int itype = type[i];
    diffVector[i][0] = 0.0;
    diffVector[i][1] = 0.0;
    diffVector[i][2] = 0.0;
  } // ii loop

  // add density at each atom via kernel function overlap
  for (int ii = 0; ii < inum; ii++) {
    int i = ilist[ii];
    double xtmp = x[i][0];
    double ytmp = x[i][1];
    double ztmp = x[i][2];
    int itype = type[i];
    int* jlist = firstneigh[i];
    int jnum = numneigh[i];

    for (int jj = 0; jj < jnum; jj++) {
      int j = jlist[jj];
      j &= NEIGHMASK;

      int jtype = type[j];
      double delx = xtmp - x[j][0];
      double dely = ytmp - x[j][1];
      double delz = ztmp - x[j][2];
      double rsq = delx * delx + dely * dely + delz * delz;

      if (rsq < cutsq) {
	double h = cutoff;
	double ih = 1.0 / h;
	double wfd;
	if (domain->dimension == 3) {
	  double r = sqrt(rsq) * ih;
	  wfd = sph_dw_quintic3d(r) * ih * ih * ih * ih;
	} else {
	  double r = sqrt(rsq) * ih;
	  wfd = sph_dw_quintic2d(r) * ih * ih * ih;
	}
	int jtype = type[j];
	double eij[domain->dimension];
	eij[0]= delx/sqrt(rsq);
	eij[1]= dely/sqrt(rsq);
	if (domain->dimension == 3) {
	  eij[2]= delz/sqrt(rsq);
	}
	diffVector[i][0] += mass[jtype] * wfd * varVector[j] / rho[j] * eij[0];
	diffVector[i][1] += mass[jtype] * wfd * varVector[j] / rho[j] * eij[1];
	if (domain->dimension == 3) {
	  diffVector[i][2] += mass[jtype] * wfd * varVector[j] / rho[j] * eij[2];
	}
      }
    } // jj loop
  } // ii loop
}

/* ----------------------------------------------------------------------
   memory usage of local atom-based array
------------------------------------------------------------------------- */

double ComputeMesoDiffAtom::memory_usage()
{
  double bytes = nmax * 4 * sizeof(double);
  return bytes;
}

int    ComputeMesoDiffAtom::pack_comm(int n, int *list, double *buf,
                                  int pbc_flag, int *pbc)
{
  int i,j,m;

  m = 0;
  for (i = 0; i < n; i++) {
    j = list[i];
    buf[m++] = varVector[j];
  }
  return 1;
}

/* ---------------------------------------------------------------------- */

void ComputeMesoDiffAtom::unpack_comm(int n, int first, double *buf)
{
  int i,m,last;

  m = 0;
  last = first + n;
  for (i = first; i < last; i++) varVector[i] = buf[m++];
}
