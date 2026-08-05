int main(void) {
    int a;
    int b;
    a = 5;
    b = a + 3;
    if (b > 5) {
        a = 10;
    } else {
        a = 0;
    }
    while (a > 0) {
        a = a - 1;
    }
    return a;
}
