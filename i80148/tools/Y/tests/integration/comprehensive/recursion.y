int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main(void) {
    if (factorial(5) != 120) return 1;
    if (fib(10) != 55) return 2;
    return 0;
}
