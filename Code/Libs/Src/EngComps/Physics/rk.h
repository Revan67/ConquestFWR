#ifndef RK_H
#define RK_H

//
// Adaptive-stepsize 4th-order Runge-Kutta integrator. 
// Non-component-ized.
//

void RKInit(void);
void RKShutdown(void);

void RKStep(struct IODE * eq, float h_try, float & h_used, float & h_next, float tol);

//

#endif
