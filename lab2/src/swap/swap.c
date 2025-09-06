#include "swap.h"

void Swap(char *left, char *right)
{
    char box = *right;
    *right = *left;
    *left = box;
}