#ifndef _FIX_H_
#define _FIX_H_

#define FIX_PREC 8
#define TO_FIX(x) ((long)((x)*(1<<FIX_PREC)))
#define TO_DBL(x) (((double)x)/(double)(1<<FIX_PREC))
#define TO_LONG(x) ((x)/(1<<FIX_PREC))

long fix_mul(long a, long b);
long fix_sqr(long a);
long fix_div(long a, long b);

#endif
