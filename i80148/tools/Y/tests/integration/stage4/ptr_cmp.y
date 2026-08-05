int main(void) {
    int a[3];
    int *p;
    int *q;
    p = a;
    q = a + 2;
    if (p < q) {
        return 1;
    }
    return 0;
}
