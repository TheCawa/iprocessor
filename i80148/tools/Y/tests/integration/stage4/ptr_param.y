int get(int *p) {
    return *p;
}

int main(void) {
    int x;
    x = 7;
    return get(&x);
}
