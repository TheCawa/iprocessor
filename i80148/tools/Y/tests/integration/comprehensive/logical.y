int main(void) {
    int a;
    int b;
    a = 1;
    b = 0;

    if (!(a && 1)) return 1;
    if (a && b) return 2;
    if (!(a || b)) return 3;
    if (!a) return 4;

    /* short-circuit */
    if (0 && (1 / 0)) return 5;  /* right side must not be evaluated */
    if (1 || (1 / 0)) {} else return 6;

    return 0;
}
