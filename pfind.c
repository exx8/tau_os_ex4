#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <errno.h>

typedef struct _QueueNode {
    char path[PATH_MAX];
    struct _QueueNode *next;
} QueueNode;

static QueueNode *firstInLine = NULL;

static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_cv = PTHREAD_COND_INITIALIZER;
static atomic_int activeThreads = 0;
static atomic_bool errInThread = 0;
static atomic_int howManyFiles = 0;

void exit_with_error_main(char *errorMsg) {
    fprintf(stderr, "%s", errorMsg);
    exit(1);
}

void exit_with_error_thread(char *errorMsg) {
    fprintf(stderr, "%s", errorMsg);
    activeThreads--;
    pthread_cond_broadcast(&queue_cv);
    pthread_exit(NULL);
}

void check_args(int argc) {
    if (argc != 4) {
        exit_with_error_main("invalid num of args\n");
    }
}

void genericErrMain() {
    exit_with_error_main("an error has occurred\n");
}

void genericErrThread() {
    exit_with_error_thread("an error has occurred\n");
}

void checkErrMain(int status) {
    if (status != 0) {
        genericErrMain();
    }
}

void checkErrThread(int status) {
    if (status != 0) {
        genericErrThread();
    }
}

QueueNode *newQueueNode() {
    void *pVoid = calloc(1, sizeof(QueueNode));
    if (!pVoid)
        genericErrMain();
    return pVoid;
}

void wakeUpAll() {
    checkErrMain(pthread_cond_broadcast(&queue_cv));
}

void insert(QueueNode *q, QueueNode **pNode) {

    if (*pNode != NULL) {
        QueueNode *currentInLine = *pNode;

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

int isAfile(const char *path) {


    struct stat fileStat;
    checkErrMain(lstat(path, &fileStat));
    if (S_ISREG(fileStat.st_mode)) {
        return 1;
    }
    return 0;

}


int shouldTrack(const char *path, void (*errChecker)(int)) {

    if (endsWith(path, "/.") || endsWith(path, "/..")) {
        return 0;
    }
    struct stat fileStat;
    errChecker(lstat(path, &fileStat));
    const __mode_t mode = fileStat.st_mode;
    if (S_ISLNK(mode))
        return 0;
    int isdir = S_ISDIR(mode);
    if (!isdir)
        return 0;
    if (!(mode & S_IRUSR)) {
        printf("Directory %s: Permission denied.\n", path);
        return 0;
    }
    return 1;
}

void debug2(const QueueNode *newNode) {
    return;
    printf("%s \n", newNode->path);
}


/**
 *
 * @param term
 * @param full_path the path must contain name!!!
 * @param name
 */
void checkIfFileAndProcess(const char *term, const char *full_path, const char *name) {
    if (isAfile(full_path)) {
        if (strstr(name, term) != NULL) {
            howManyFiles++;
            printf("%s\n", full_path);
        }
    }
}

QueueNode *dir(char *path, char *term, void(*errChecker)(int)) {
    struct dirent *dp;
    DIR *dirp;
    dirp = opendir(path);
    if (dirp == NULL) {
        errChecker(-1);
    }
    QueueNode *localQueue = NULL;
    errno = 0;
    while ((dp = readdir(dirp)) != NULL) {


        QueueNode *newNode = newQueueNode();
        strcpy(newNode->path, path);
        strcat(newNode->path, "/");
        strcat(newNode->path, dp->d_name);
        debug2(newNode);
        checkIfFileAndProcess(term, newNode->path, dp->d_name);
        if (!shouldTrack(newNode->path, errChecker)) {
            continue;
        }
        insert(newNode, &localQueue);
        errno = 0;
    }
    errChecker(errno);
    closedir(dirp);
    return localQueue;
}

void wait4FirstInLine() {
    activeThreads--;
    wakeUpAll();
    checkErrThread(pthread_mutex_lock(&queue_mutex));

    while (firstInLine == NULL) {
        checkErrThread(pthread_cond_wait(&queue_cv, &queue_mutex));
    }
}

void debug1() {
    return;
    printf("%d wait4Zero \n", activeThreads);
}

void debug3() {
    return;
    printf("%d before loop \n", activeThreads);
}

void wait4ZeroActive() {
    debug3();

    while (activeThreads > 0) {
        checkErrMain(pthread_mutex_lock(&queue_mutex));
        debug1();
        checkErrMain(pthread_cond_wait(&queue_cv, &queue_mutex));
        checkErrMain(pthread_mutex_unlock(&queue_mutex));
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
            checkErrThread(pthread_mutex_unlock(&queue_mutex));
        }
        QueueNode *q = dir(popEle->path, thread_param, checkErrThread);
        checkErrThread(pthread_mutex_lock(&queue_mutex));
        insert(q, &firstInLine);
        checkErrThread(pthread_cond_broadcast(&queue_cv));
        checkErrThread(pthread_mutex_unlock(&queue_mutex));

    }

}


int main(int argc, char *argv[]) {
    check_args(argc);
    const char *root = argv[1];
    char *term = argv[2];
    const int thread_num = atoi(argv[3]);
    QueueNode *rootNode = newQueueNode();
    strcpy((rootNode->path), root);
    if (shouldTrack(root, checkErrMain)) {
        insert(rootNode, &firstInLine);
    }
    else
    {
        genericErrMain();
    }
    for (int i = 0; i < thread_num; i++) {
        pthread_t thread_id;

         checkErrMain(pthread_create(&thread_id, NULL, thread_func, term));
        activeThreads++;
    }
    wakeUpAll();
    wait4ZeroActive();
    printf("Done searching, found %d files\n", howManyFiles);
    return errInThread;
}
