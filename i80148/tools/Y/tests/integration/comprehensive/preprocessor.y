#define VALUE 42

#ifndef GUARD
#define GUARD
int g;
#endif

#ifdef UNDEFINED
int wrong;
#endif

int main(void) {
    g = VALUE;
    if (g != 42) return 1;

    return 0;
}
