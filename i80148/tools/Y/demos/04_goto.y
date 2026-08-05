int main(void) {
    int x;
    x = 0;
loop:
    x = x + 1;
    if (x < 5) {
        goto loop;
    }
    return x;
}
