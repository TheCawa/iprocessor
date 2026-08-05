int a[4];

int main(void) {
    int i;
    int sum;
    for (i = 0; i < 4; i = i + 1) {
        a[i] = i * i;
    }
    sum = 0;
    for (i = 0; i < 4; i = i + 1) {
        sum = sum + a[i];
    }
    return sum;
}
