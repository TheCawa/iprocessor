int main(void) {
    char c;
    short s;
    int i;
    unsigned int u;

    c = 0xFF;
    s = 0x1234;
    i = 0x12345678;
    u = 0xFFFFFFFF;

    if (c != 255) return 1;
    if (s != 4660) return 2;
    if (i != 305419896) return 3;
    if (u != 0xFFFFFFFF) return 4;

    if (sizeof(char) != 1) return 5;
    if (sizeof(short) != 2) return 6;
    if (sizeof(int) != 4) return 7;

    return 0;
}
