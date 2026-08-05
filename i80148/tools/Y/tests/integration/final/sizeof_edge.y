int main(void) {
    int *p;
    int arr[10];

    if (sizeof(char) != 1) return 1;
    if (sizeof(short) != 2) return 2;
    if (sizeof(int) != 4) return 3;
    if (sizeof(long) != 4) return 4;
    if (sizeof(p) != 4) return 5;
    if (sizeof(arr) != 40) return 6;

    return 0;
}
