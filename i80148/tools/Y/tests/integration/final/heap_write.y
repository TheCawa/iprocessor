#include <ystdlib.h>

extern int _heap_current;
extern int _heap_start;
extern int _free_list;

int main(void) {
    int *p;
    p = _heap_current;
    *p = 0x12345678;
    printf("addr=%p val=0x%x cur=%p start=%p list=%p\n", p, *p, _heap_current, _heap_start, _free_list);
    return 0;
}
