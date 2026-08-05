int main(void) {
    int x;
    int *p;
    int **pp;

    x = 42;
    p = &x;
    pp = &p;

    if (*p != 42) return 1;
    if (**pp != 42) return 2;

    *p = 7;
    if (x != 7) return 3;

    p = p + 1;
    p = p - 1;
    if (*p != 7) return 4;

    if (p != &x) return 5;
    if (p == 0) return 6;

    return 0;
}
