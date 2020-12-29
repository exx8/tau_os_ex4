#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

typedef struct _QueueNode {
    char path[PATH_MAX];
    struct _QueueNode *next;
} QueueNode;

static QueueNode *firstInLine = NULL;

static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_cv = PTHREAD_COND_INITIALIZER;

QueueNode *newQueueNode() {
    return calloc(1, sizeof(QueueNode));
}
void wakeUpAll() {
    pthread_mutex_lock(&queue_mutex);
    pthread_cond_broadcast(&queue_cv);
    pthread_mutex_unlock(&queue_mutex);
}

void insert(QueueNode *q, QueueNode **pNode) {

    if (*pNode != NULL) {
        QueueNode* currentInLine = pNode;

        while (currentInLine->next != NULL) {
            currentInLine = currentInLine->next;
        }
        currentInLine->next = q;
    } else
        *pNode = q;

}

/**
 * @attention critical access only
 */
QueueNode *pop() {
    QueueNode *top = firstInLine;
    firstInLine = firstInLine->next;
    return top;
}

QueueNode *dir(char *path) {
    struct dirent *dp;
    DIR *dirp;
    dirp = opendir(path);
    QueueNode *localQueue = NULL;
    while ((dp = readdir(dirp)) != NULL) {
        QueueNode *newNode = newQueueNode();
        strcpy(newNode->path, path);
        strcat(newNode->path, "/");
        strcat(newNode->path, dp->d_name);
        insert(newNode, &localQueue);
        printf("%s \n",newNode->path);

    }
    closedir(dirp);
    return localQueue;
}

void wait4Signal() {
    pthread_mutex_lock(&queue_mutex);

    while (firstInLine == NULL) {
        pthread_cond_wait(&queue_cv, &queue_mutex);
    }
}

void *thread_func(void *thread_param) {
    pthread_mutex_lock(&queue_mutex);

    QueueNode *popEle;
    popEle = pop();
    pthread_mutex_unlock(&queue_mutex);


    QueueNode *q = dir(popEle->path);
    pthread_mutex_lock(&queue_mutex);
    insert(q, &firstInLine);
    pthread_mutex_unlock(&queue_mutex);
    wakeUpAll();
    pthread_detach(pthread_self());
    pthread_exit((void *) EXIT_SUCCESS);
}



int main(int argc, const char *argv[]) {
    const char *root = argv[1];
    const char *term = argv[2];
    const int thread_num = atoi(argv[3]);
    QueueNode *rootNode = newQueueNode();
    strcpy((rootNode->path), root);
    insert(rootNode, &firstInLine);
    for (int i = 0; i < thread_num; i++) {
        pthread_t thread_id;

        int rc = pthread_create(&thread_id, NULL, thread_func, NULL);
    }
    wakeUpAll();

    sleep(1);//@todo remove, with no sleep process halts before threads end
    return 0;
}
