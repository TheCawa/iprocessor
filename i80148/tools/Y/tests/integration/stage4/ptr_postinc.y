int main(void) {
    int a[2];
    int *p;
    a[0] = 5;
    a[1] = 6;
    p = a;
    return *p++;
}
