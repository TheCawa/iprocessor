int main(void) {
    int i;
    int s;

    /* while break */
    i = 0;
    while (1) {
        i = i + 1;
        if (i == 5) break;
    }
    if (i != 5) return 1;

    /* for continue */
    s = 0;
    for (i = 0; i < 5; i = i + 1) {
        if (i == 2) continue;
        s = s + i;
    }
    if (s != 0+1+3+4) return 2;

    /* do-while */
    i = 0;
    do {
        i = i + 1;
    } while (i < 3);
    if (i != 3) return 3;

    /* goto */
    i = 0;
    goto skip;
    i = 99;
skip:
    if (i != 0) return 4;

    return 0;
}
