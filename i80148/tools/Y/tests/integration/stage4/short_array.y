int main(void) {
    short s[2];
    short *p;
    s[0] = 0x1234;
    s[1] = 0x5678;
    p = s;
    p = p + 1;
    return *p;
}
