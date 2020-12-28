#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef struct _QueueNode
{
char* path[PATH_MAX];
struct _QueueNode* next;
}QueueNode;
int main(int argc, const char* argv[]) {
    const char* root=argv[1];
    const char* term=argv[2];
    const int thread_num=atoi(argv[3]);

    printf("Hello, World!\n");
    return 0;
}
