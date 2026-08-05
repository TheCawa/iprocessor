int main(void) {
    int a;
    int b;

    a = 17;
    b = 5;
    if (a / b != 3) return 1;
    if (a % b != 2) return 2;

    a = -7;
    if (a / 3 != -2) return 3; /* signed division truncates toward zero */
    if (a % 3 != -1) return 4;

    a = 0x0F;
    b = 0xF0;
    if ((a & b) != 0) return 5;
    if ((a | b) != 0xFF) return 6;
    if ((a ^ b) != 0xFF) return 7;
    if ((~a & 0xFF) != 0xF0) return 8;
    if ((a << 2) != 0x3C) return 9;
    if ((b >> 4) != 0x0F) return 10;

    return 0;
}
