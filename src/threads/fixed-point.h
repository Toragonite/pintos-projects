#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

/* Fixed-point arithmetic implementation for MLFQS.
   Uses 17.14 fixed-point representation. */

typedef int fixed_t;

/* Fixed-point constant for 17.14 representation */
#define F (1 << 14)

/* Convert integer n to fixed-point */
#define INT_TO_FP(n) ((fixed_t)(n) * F)

/* Convert fixed-point x to integer (rounding toward zero) */
#define FP_TO_INT_ZERO(x) ((x) / F)

/* Convert fixed-point x to integer (rounding to nearest) */
#define FP_TO_INT_NEAR(x) ((x) >= 0 ? ((x) + F/2) / F : ((x) - F/2) / F)

/* Add two fixed-point values */
#define FP_ADD(x, y) ((x) + (y))

/* Subtract fixed-point y from x */
#define FP_SUB(x, y) ((x) - (y))

/* Add integer n to fixed-point x */
#define FP_ADD_INT(x, n) ((x) + INT_TO_FP(n))

/* Subtract integer n from fixed-point x */
#define FP_SUB_INT(x, n) ((x) - INT_TO_FP(n))

/* Multiply two fixed-point values */
#define FP_MUL(x, y) ((fixed_t)(((int64_t)(x)) * (y) / F))

/* Multiply fixed-point x by integer n */
#define FP_MUL_INT(x, n) ((x) * (n))

/* Divide fixed-point x by y */
#define FP_DIV(x, y) ((fixed_t)(((int64_t)(x)) * F / (y)))

/* Divide fixed-point x by integer n */
#define FP_DIV_INT(x, n) ((x) / (n))

#endif /* threads/fixed-point.h */
