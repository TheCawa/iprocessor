int add(int a, int b) {
    return a + b;
}

int mul(int a, int b, int c) {
    return a * b * c;
}

int identity(int x) {
    return x;
}

int main(void) {
    if (add(2, 3) != 5) return 1;
    if (mul(2, 3, 4) != 24) return 2;
    if (identity(identity(7)) != 7) return 3;
    return 0;
}
