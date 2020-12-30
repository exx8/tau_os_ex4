#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdatomic.h>

typedef struct _QueueNode {
    char path[PATH_MAX];
    struct _QueueNode *next;
} QueueNode;

static QueueNode *firstInLine = NULL;

static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_cv = PTHREAD_COND_INITIALIZER;
static atomic_int activeThreads = 0;

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
        QueueNode *currentInLine = pNode;

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
    if (top == NULL)
        return NULL;
    firstInLine = firstInLine->next;
    return top;
}

/**
 * @copyright https://stackoverflow.com/questions/744766/how-to-compare-ends-of-strings-in-c/744822#744822
 */
int endsWith(const char *str, const char *suffix) {
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix > lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}


int shouldTrack(const char *path) {

    if (endsWith(path, "/.") || endsWith(path, "/..")) {
        return 0;
    }
    struct stat fileStat;
    lstat(path, &fileStat);
    if (S_ISLNK(fileStat.st_mode))
        return 0;
    return S_ISDIR(fileStat.st_mode);

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
        printf("%s \n", newNode->path);
        if (!shouldTrack(newNode->path)) {
            continue;
        }
        insert(newNode, &localQueue);

    }
    closedir(dirp);
    return localQueue;
}

void wait4FirstInLine() {
    activeThreads--;
    wakeUpAll();
    pthread_mutex_lock(&queue_mutex);

    while (firstInLine == NULL) {
        pthread_cond_wait(&queue_cv, &queue_mutex);
    }
}

void wait4ZeroActive() {
    printf("%d before loop \n", activeThreads);

    while (activeThreads > 0) {
        pthread_mutex_lock(&queue_mutex);
        printf("%d wait4Zero \n", activeThreads);
        pthread_cond_wait(&queue_cv, &queue_mutex);
        pthread_mutex_unlock(&queue_mutex);
    }
}

void *thread_func(void *thread_param) {

    //pthread_detach(pthread_self());

    while (1) {
        QueueNode *popEle = NULL;
        while (popEle == NULL) {
            wait4FirstInLine();
            activeThreads++;
            popEle = pop();
            pthread_mutex_unlock(&queue_mutex);
        }
        QueueNode *q = dir(popEle->path);
        pthread_mutex_lock(&queue_mutex);
        insert(q, &firstInLine);
        pthread_cond_broadcast(&queue_cv);
        pthread_mutex_unlock(&queue_mutex);

    }

    pthread_exit((void *) EXIT_SUCCESS);
}

void exit_with_error(char *errorMsg) {
    fprintf(stderr, errorMsg);
    exit(1);
}

void check_args(int argc) {
    if (argc != 4) {
        exit_with_error("invalid num of args");
    }
}

int main(int argc, const char *argv[]) {
    check_args(argc);
    const char *root = argv[1];
    const char *term = argv[2];
    const int thread_num = atoi(argv[3]);
    QueueNode *rootNode = newQueueNode();
    strcpy((rootNode->path), root);
    insert(rootNode, &firstInLine);
    for (int i = 0; i < thread_num; i++) {
        pthread_t thread_id;

        int rc = pthread_create(&thread_id, NULL, thread_func, NULL);
        activeThreads++;
    }
    wakeUpAll();
    wait4ZeroActive();
    return 0;
}
