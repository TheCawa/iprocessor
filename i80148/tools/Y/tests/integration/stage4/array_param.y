int sum(int *a) {
    return a[0] + a[1];
}

int main(void) {
    int a[2];
    a[0] = 3;
    a[1] = 4;
    return sum(a);
}
