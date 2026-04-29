#include "swap.h"

void swap_int(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

void swap_float(float *a, float *b)
{
    float temp = *a;
    *a = *b;
    *b = temp;
}