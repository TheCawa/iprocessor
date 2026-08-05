int g[4];

int main(void) {
    int l[4];
    int i;

    for (i = 0; i < 4; i = i + 1) {
        l[i] = i * i;
        g[i] = i + 10;
    }

    if (l[0] != 0) return 1;
    if (l[1] != 1) return 2;
    if (l[2] != 4) return 3;
    if (l[3] != 9) return 4;

    if (g[0] != 10) return 5;
    if (g[3] != 13) return 6;

    /* pointer arithmetic on arrays */
    if (*(l + 2) != 4) return 7;
    if (*(g + 1) != 11) return 8;

    return 0;
}
