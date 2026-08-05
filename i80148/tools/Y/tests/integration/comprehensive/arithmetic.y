int main(void) {
    int a;
    int b;
    a = 17;
    b = 5;

    if (a + b != 22) return 1;
    if (a - b != 12) return 2;
    if (a * b != 85) return 3;
    if (a / b != 3) return 4;
    if (a % b != 2) return 5;

    if ((a & b) != 1) return 6;
    if ((a | b) != 21) return 7;
    if ((a ^ b) != 20) return 8;
    if ((a << 2) != 68) return 9;
    if ((a >> 2) != 4) return 10;

    return 0;
}
