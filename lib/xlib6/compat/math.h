/*
 * math.h -- minimal <math.h> for POSIX X clients (backed by compat/xpsxlibc.c:
 * Taylor sin/cos, Newton sqrt, rational atan; no libm on the subsystem).
 * MIT/X license (see copyright.h).
 */
#ifndef _PSX_MATH_H_
#define _PSX_MATH_H_

#ifndef M_PI
#define M_PI		3.14159265358979323846
#define M_PI_2		1.57079632679489661923
#endif

extern double sin(double);
extern double cos(double);
extern double atan(double);
extern double atan2(double, double);
extern double hypot(double, double);
extern double sqrt(double);
extern double fabs(double);
extern double floor(double);
extern double ceil(double);
extern double fmod(double, double);

#endif /* _PSX_MATH_H_ */
