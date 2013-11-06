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

#include "stdio.h"
#include "string.h"
#include "fix_meso_constrained.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"
#include "atom.h"
#include "comm.h"
#include "force.h"
#include "neighbor.h"
#include "neigh_list.h"
#include "neigh_request.h"
#include "update.h"
#include "integrate.h"
#include "respa.h"
#include "memory.h"
#include "error.h"
#include "pair.h"

using namespace LAMMPS_NS;
using namespace FixConst;

/* ---------------------------------------------------------------------- */

FixMesoConstrained::FixMesoConstrained(LAMMPS *lmp, int narg, char **arg) :
  Fix(lmp, narg, arg) {

  if ((atom->e_flag != 1) || (atom->rho_flag != 1))
    error->all(FLERR,
        "fix meso/constrained command requires atom_style with both energy and density");

  if (narg != 3)
    error->all(FLERR,"Illegal number of arguments for fix meso/constrained command");

  time_integrate = 1;
}

/* ---------------------------------------------------------------------- */

int FixMesoConstrained::setmask() {
  int mask = 0;
  mask |= INITIAL_INTEGRATE;
  mask |= FINAL_INTEGRATE;
  return mask;
}

/* ---------------------------------------------------------------------- */

void FixMesoConstrained::init() {
  dtv = update->dt;
  dtf = 0.5 * update->dt * force->ftm2v;
}

/* ----------------------------------------------------------------------
 allow for both per-type and per-atom mass
 ------------------------------------------------------------------------- */

void FixMesoConstrained::initial_integrate(int vflag) {
  // update v and x and rho and e of atoms in group

  double **x = atom->x;
  double **v = atom->v;
  double **f = atom->f;
  double **vest = atom->vest;
  double *rho = atom->rho;
  double *drho = atom->drho;
  double *e = atom->e;
  double *de = atom->de;
  double *mass = atom->mass;
  double *rmass = atom->rmass;
  int rmass_flag = atom->rmass_flag;

  int *type = atom->type;
  int *mask = atom->mask;
  int nlocal = atom->nlocal;
  int i;
  double dtfm;

  if (igroup == atom->firstgroup)
    nlocal = atom->nfirst;

  for (i = 0; i < nlocal; i++) {
    if (mask[i] & groupbit) {
      if (rmass_flag) {
        dtfm = dtf / rmass[i];
      } else {
        dtfm = dtf / mass[type[i]];
      }

      e[i] += dtf * de[i]; // half-step update of particle internal energy
      rho[i] += dtf * drho[i]; // ... and density

      // extrapolate velocity for use with velocity-dependent potentials, e.g. SPH
      vest[i][0] = v[i][0] + 2.0 * dtfm * f[i][0];
      vest[i][1] = v[i][1] + 2.0 * dtfm * f[i][1];
      vest[i][2] = v[i][2] + 2.0 * dtfm * f[i][2];

      v[i][0] += dtfm * f[i][0];
      v[i][1] += dtfm * f[i][1];
      v[i][2] += dtfm * f[i][2];

      /// particles must stay on the surface of the sphere
      x[i][0] += dtv * v[i][0];
      x[i][1] += dtv * v[i][1];
      x[i][2] += dtv * v[i][2];
      back_to_surface(&x[i][0], &x[i][1], &x[i][2]);
    }
  }
}

double LAMMPS_NS::back_to_surface(double* x, double* y, double* z) {
  double x0 = 0.6;
  double y0 = 0.6;
  double z0 = 0.6;
  double R = 0.44;
  double dx = *x - x0;
  double dy = *y - y0;
  double dz = *z - z0;
  double r = sqrt(dx*dx + dy*dy + dz*dz);
  *x = x0 + dx/r*R;
  *y = y0 + dy/r*R;
  *z = z0 + dz/r*R;
}

/* ---------------------------------------------------------------------- */

void FixMesoConstrained::final_integrate() {

  // update v, rho, and e of atoms in group

  double **v = atom->v;
  double **f = atom->f;
  double *e = atom->e;
  double *de = atom->de;
  double *rho = atom->rho;
  double *drho = atom->drho;
  int *type = atom->type;
  int *mask = atom->mask;
  double *mass = atom->mass;
  int nlocal = atom->nlocal;
  if (igroup == atom->firstgroup)
    nlocal = atom->nfirst;
  double dtfm;
  double *rmass = atom->rmass;
  int rmass_flag = atom->rmass_flag;

  for (int i = 0; i < nlocal; i++) {
    if (mask[i] & groupbit) {

      if (rmass_flag) {
        dtfm = dtf / rmass[i];
      } else {
        dtfm = dtf / mass[type[i]];
      }
      v[i][0] += dtfm * f[i][0];
      v[i][1] += dtfm * f[i][1];
      v[i][2] += dtfm * f[i][2];

      e[i] += dtf * de[i];
      rho[i] += dtf * drho[i];
    }
  }
}

/* ---------------------------------------------------------------------- */

void FixMesoConstrained::reset_dt() {
  dtv = update->dt;
  dtf = 0.5 * update->dt * force->ftm2v;
}