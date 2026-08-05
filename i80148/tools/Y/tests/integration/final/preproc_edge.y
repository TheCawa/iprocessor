#define A 1
#define B 2

#ifdef UNDEFINED
int x = 5;
#endif

#ifndef UNDEFINED
int y = 10;
#endif

#ifdef A
#ifdef B
int z = 20;
#else
int z = 0;
#endif
#else
int z = 0;
#endif

int main(void) {
    if (y != 10) return 1;
    if (z != 20) return 2;
    return 0;
}
