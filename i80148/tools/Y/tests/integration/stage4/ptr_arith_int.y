int main(void) {
    int a[3];
    int *p;
    a[0] = 11;
    a[1] = 22;
    a[2] = 33;
    p = a;
    p = p + 2;
    return *p;
}
