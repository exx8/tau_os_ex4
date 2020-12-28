#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
typedef struct _QueueNode {
    char *path[PATH_MAX];
    struct _QueueNode *next;
} QueueNode;

static QueueNode *firstInLine = NULL;

static pthread_mutex_t queue_mute=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_cv=PTHREAD_COND_INITIALIZER;

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
void *thread_func(void *thread_param) {
    printf("In thread #%ld\n", pthread_self());
    printf("I received \"%s\" from my caller\n", (char *)thread_param);

    pthread_exit((void *)EXIT_SUCCESS);
    //return (void *)EXIT_SUCCESS;// <- same as this
}

int main(int argc, const char *argv[]) {
    const char *root = argv[1];
    const char *term = argv[2];
    const int thread_num = atoi(argv[3]);

    for(int i=0;i<thread_num;i++)
    {
        pthread_t thread_id;
        int rc = pthread_create(&thread_id, NULL, thread_func, NULL);
    }
    printf("Hello, World!\n");
    return 0;
}
