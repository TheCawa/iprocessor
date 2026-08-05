int main(void) {
    int a[3];
    int *p;
    a[2] = 77;
    p = &a[2];
    return *p;
}
