int main(void) {
    int a;
    int b;
    a = 1;
    b = 0;
    if (a && b) {
        a = 5;
    } else {
        a = 7;
    }
    if (a || b) {
        b = 3;
    }
    return a + b;
}
