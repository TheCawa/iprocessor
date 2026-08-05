int main(void) {
    int arr[4];
    int *p;
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;
    p = arr;
    p = p + 2;
    return *p;
}
