int main(void) {
    int a;
    int b;
    a = 10;
    b = 20;

    if (a == b) return 1;
    if (!(a != b)) return 2;
    if (!(a < b)) return 3;
    if (a > b) return 4;
    if (!(a <= b)) return 5;
    if (a >= b) return 6;

    if (a < 0) return 7;
    if (-5 < 0) {} else return 8;

    return 0;
}
