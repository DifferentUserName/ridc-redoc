#include "ridc.h"
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <gsl/gsl_linalg.h>


void newt(double t, double *uprev, double *uguess,
	  double *g, PARAMETER param);
/**< Helper function to compute the next newton step
   for solving a system of equations

   @return (by reference) g, the function that we are attempting to find the zero of.
   @param param: structure containing number of equations, number of
   time steps, initial and final time, time step
   @param t: current time step
   @param uguess: function value at the current time step
   @param uprev: solution from previous time step
   @param g: ODE specific value
*/



void jac(double t, double *u, double *J, PARAMETER param);
/**< Helper function to the jacobian matrix (using finite differences)
   for advancing the solution from time t(n) to t(n+1) using an
   implicit Euler step on a system of equations

   @return (by reference) J, the Jacobian for the newton step
   @param param: structure containing number of equations, number of
   time steps, initial and final time, time step
   @param t: current time step
   @param u: function value at the current time step
   @param J: Jacobian, returned by reference
*/



template <>
void rhs(double t, double *u, PARAMETER param, double *f) {
  double A = 1.0;
  double B = 3.0;
  double alpha = 0.02;
  double dx = 1.0/(param.neq+1);
  int Nx = param.neq/2;

  f[0] = A+u[0]*u[0]*u[Nx] -(B+1.0)*u[0] +alpha/dx/dx*(u[1]-2*u[0]+1.0);
  f[Nx-1] = A+u[Nx-1]*u[Nx-1]*u[2*Nx-1] -(B+1.0)*u[Nx-1]
    + alpha/dx/dx*(1.0-2*u[Nx-1]+u[Nx-2]);

  f[Nx] = B*u[0]- u[0]*u[0]*u[Nx] + alpha/dx/dx*(u[Nx+1]-2*u[Nx]+3.0);
  f[2*Nx-1] = B*u[Nx-1]- u[Nx-1]*u[Nx-1]*u[2*Nx-1]
    + alpha/dx/dx*(3.0-2*u[2*Nx-1]+u[2*Nx-2]);

  for (int i=1; i<Nx-1; i++) {
    f[i] = A + u[i]*u[i]*u[Nx+i] -(B+1.0)*u[i] +alpha/dx/dx*(u[i+1]-2*u[i]+u[i-1]);
    f[Nx+i] = B*u[i]- u[i]*u[i]*u[Nx+i] + alpha/dx/dx*(u[Nx+i+1]-2*u[Nx+i]+u[Nx+i-1]);
  }
}



///////////////////////////////////////////////////////
// Function Name: newt
// Usage: function for newton solve
//
///////////////////////////////////////////////////////
// Assumptions:
//
///////////////////////////////////////////////////////
// Inputs:
//         t, time
//         dt, timestep
//         uprev, solution at previous time level
//         uguess, solution for new time level
//
///////////////////////////////////////////////////////
// Outputs: (by reference)
//          g: function value at time t
//
///////////////////////////////////////////////////////
// Functions Called:
//
///////////////////////////////////////////////////////
// Called By:
//
///////////////////////////////////////////////////////
void newt(double t, double *uprev, double *uguess,
    double *g, PARAMETER param) {
  rhs<PARAMETER>(t,uguess,param,g);
  for (int i =0; i<param.neq; i++) {
    g[i] = uguess[i]-uprev[i]-param.dt*g[i];
  }
}


///////////////////////////////////////////////////////
// Function Name: jac
// Usage: compute approximation to jacobian
//
///////////////////////////////////////////////////////
// Assumptions:
//
///////////////////////////////////////////////////////
// Inputs:
//         t, independent variable
//         dt, timestep
//         u, dependent variable
//         param, problem parameters (d.g. neq, dt)
//
///////////////////////////////////////////////////////
// Outputs: (by reference)
//          J: approximate Jacobian at time t
//
///////////////////////////////////////////////////////
// Functions Called:
//
///////////////////////////////////////////////////////
// Called By: ???
//
///////////////////////////////////////////////////////
void jac(double t, double *u, double *J, PARAMETER param) {


  double d = 1e-5; // numerical jacobian approximation
  double *u1;
  double *f1;
  double *f;

  // need to allocate memory
  u1 = new double[param.neq];
  f1 = new double[param.neq];
  f = new double[param.neq];

  // note, instead of using newt, use rhs for efficiency
  rhs<PARAMETER>(t,u,param,f);

  for (int i = 0; i<param.neq; i++) {
    for (int j = 0; j<param.neq; j++) {
      u1[j] = u[j];
    }
    u1[i] = u1[i] + d;

    rhs<PARAMETER>(t,u1,param,f1);

    for (int j = 0; j<param.neq; j++) {
      J[j*param.neq+i] = -param.dt*(f1[j]-f[j])/d;
    }
    J[i*param.neq+i] = 1.0+J[i*param.neq+i];
  }

  // need to delete memory
  delete [] u1;
  delete [] f1;
  delete [] f;
}


template <>
void step(double t, double* uold, PARAMETER param, double* unew) {
  // t is current time

  double tnew = t + param.dt;

  double NEWTON_TOL = 1.0e-10;
  int NEWTON_MAXSTEP = 10;

  
  double* stepsize = new double[param.neq];
  double* uguess = new double[param.neq];

  double *J = new double[param.neq*param.neq];

  // GSL
  gsl_matrix_view m;
  gsl_vector_view b;
  gsl_vector *x = gsl_vector_alloc(param.neq);
  gsl_permutation *p = gsl_permutation_alloc(param.neq);


  // set initial condition
  for (int i=0; i<param.neq; i++) {
    uguess[i] = uold[i];
  }

  double maxstepsize;

  int counter=0;

  while (1) {

    // create rhs
    newt(tnew,uold,uguess,stepsize,param);

    // compute numerical Jacobian
    jac(tnew,uguess,J,param);
    
    m = gsl_matrix_view_array(J,param.neq,param.neq);
    b = gsl_vector_view_array(stepsize,param.neq);

    int s; // presumably, this is some flag info

    gsl_linalg_LU_decomp (&m.matrix,p,&s);
    gsl_linalg_LU_solve (&m.matrix,p,&b.vector, x);


    // check for convergence
    maxstepsize = 0.0;
    for (int i = 0; i<param.neq; i++) {
      uguess[i] = uguess[i] - x->data[i];
      if (std::abs(stepsize[i])>maxstepsize) {
	maxstepsize = std::abs(x->data[i]);
      }
    }

    // if update sufficiently small enough
    if ( maxstepsize < NEWTON_TOL) {
      break;
    }

    counter++;
    //error if too many steps
    if (counter > NEWTON_MAXSTEP) {
      fprintf(stderr,"max newton iterations reached\n");
      exit(42);
    }
  } // end newton iteration

  for (int i=0; i<param.neq; i++) {
    unew[i] = uguess[i];
  }

  delete [] uguess;
  delete [] stepsize;
  delete [] J;

  gsl_permutation_free (p);
  gsl_vector_free (x);

}
