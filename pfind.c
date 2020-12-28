#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef struct _QueueNode {
    char *path[PATH_MAX];
    struct _QueueNode *next;
} QueueNode;

static QueueNode *firstInLine = NULL;

static pthread_mutex_t queue_mutex;
static pthread_cond_t queue_cv;

QueueNode *newQueueNode() {
    return calloc(1, sizeof(QueueNode));
}

void insert(QueueNode *q) {

    if (firstInLine != NULL) {
        QueueNode *currentInLine = firstInLine;

        while (currentInLine->next != NULL) {
            currentInLine = currentInLine->next;
        }
        currentInLine->next = q;
    } else
        firstInLine = q;

}

int main(int argc, const char *argv[]) {
    const char *root = argv[1];
    const char *term = argv[2];
    const int thread_num = atoi(argv[3]);

    printf("Hello, World!\n");
    return 0;
}
