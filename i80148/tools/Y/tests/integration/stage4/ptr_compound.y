int main(void) {
    int a[3];
    int *p;
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    p = a;
    p += 2;
    return *p;
}
