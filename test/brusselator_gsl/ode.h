/**
 * 2014--6--18
 * ode.h
 */
#ifndef _ODE_H_
#define _ODE_H_

/**
 * Requires at least
 * dt - delta t
 * neq - number of equations
 */
struct PARAMETER {
    int neq;
    int nt;
    double ti;
    double tf;
    double dt;
};

void init_params(const int& nt, PARAMETER &param);
void initial_condition(PARAMETER param, double* u);
void rhs(double t, double *u, PARAMETER param, double* f);
void newt(double t, double *uprev, double *uguess,
	  double *g, PARAMETER param);
void jac(double t, double *u, double *J, PARAMETER param);
void step(double t, double* u, PARAMETER param, double* unew);

#endif // _ODE_H_