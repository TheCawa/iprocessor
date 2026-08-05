int main(void) {
    char c;
    unsigned char uc;
    short s;
    unsigned short us;
    int i;
    unsigned int u;

    c = -1;
    if (c != 255) return 1; /* char is unsigned by default */

    uc = 255;
    if (uc != 255) return 2;

    s = -1;
    if (s != -1) return 3;

    us = 0xFFFF;
    if (us != 0xFFFF) return 4;

    i = -1;
    if (i != -1) return 5;

    u = 0xFFFFFFFF;
    if (u != 0xFFFFFFFF) return 6;

    return 0;
}
