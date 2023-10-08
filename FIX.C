#include "fix.h"

long fix_mul(long a, long b)
{
  return (a * b) >> FIX_PREC;
}

long fix_sqr(long a)
{
  return (a * a) >> FIX_PREC;
}

long fix_div(long a, long b)
{
   return (a << FIX_PREC) / b;
}
