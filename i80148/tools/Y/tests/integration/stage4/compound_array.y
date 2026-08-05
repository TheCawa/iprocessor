int main(void) {
    int a[2];
    a[0] = 10;
    a[0] += 5;
    a[1] = a[0] * 2;
    return a[0] + a[1];
}
