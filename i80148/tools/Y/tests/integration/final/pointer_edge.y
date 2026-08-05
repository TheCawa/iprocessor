int main(void) {
    int x;
    int *p;
    int arr[4];

    x = 42;
    p = &x;
    if (*p != 42) return 1;

    p = 0;
    if (p != 0) return 2;

    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;
    if (*(arr + 2) != 30) return 3;
    if (arr[2] != 30) return 4;
    if (2[arr] != 30) return 5; /* weird but legal C */

    p = arr;
    p = p + 1;
    if (*p != 20) return 6;

    return 0;
}
