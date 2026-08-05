#include <ystdlib.h>

extern int test_write(void);
extern int _heap_current;

int main(void) {
    int *p;
    printf("before: cur=%p\n", _heap_current);
    test_write();
    printf("after: cur=%p val=%d\n", _heap_current, *(int*)_heap_current);
    return 0;
}
