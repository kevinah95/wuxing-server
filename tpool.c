#include <pthread.h>
#include <tpool.h>
#include <stdlib.h>

struct tpool_work {
    thread_func_t      func;
    void              *arg;
    struct tpool_work *next;
};
typedef struct tpool_work tpool_work_t;

struct tpool {
    tpool_work_t    *work_first;
    tpool_work_t    *work_last;
    pthread_mutex_t  work_mutex;
    pthread_cond_t   work_cond;
    pthread_cond_t   working_cond;
    size_t           working_cnt;
    size_t           thread_cnt;
    bool             stop;
};

static tpool_work_t *tpool_work_create(thread_func_t func, void *arg)
{
    tpool_work_t *work;

    if (func == NULL)
        return NULL;

    work       = malloc(sizeof(*work));
    work->func = func;
    work->arg  = arg;
    work->next = NULL;
    return work;
}

static void tpool_work_destroy(tpool_work_t *work)
{
    if (work == NULL)
        return;
    free(work);
}

static tpool_work_t *tpool_work_get(tpool_t *tp)
{
    tpool_work_t *work;

    if (tp == NULL)
        return NULL;

    work = tp->work_first;
    if (work == NULL)
        return NULL;

    if (work->next == NULL) {
        tp->work_first = NULL;
        tp->work_last  = NULL;
    } else {
        tp->work_first = work->next;
    }

    return work;
}

static void *tpool_worker(void *arg)
{
    tpool_t      *tp = arg;
    tpool_work_t *work;

    while (1) {
        pthread_mutex_lock(&(tp->work_mutex));

        while (tp->work_first == NULL && !tp->stop)
            pthread_cond_wait(&(tp->work_cond), &(tp->work_mutex));

        if (tp->stop)
            break;

        work = tpool_work_get(tp);
        tp->working_cnt++;
        pthread_mutex_unlock(&(tp->work_mutex));

        if (work != NULL) {
            work->func(work->arg);
            tpool_work_destroy(work);
        }

        pthread_mutex_lock(&(tp->work_mutex));
        tp->working_cnt--;
        if (!tp->stop && tp->working_cnt == 0 && tp->work_first == NULL)
            pthread_cond_signal(&(tp->working_cond));
        pthread_mutex_unlock(&(tp->work_mutex));
    }

    tp->thread_cnt--;
    pthread_cond_signal(&(tp->working_cond));
    pthread_mutex_unlock(&(tp->work_mutex));
    return NULL;
}

tpool_t *tpool_create(size_t num)
{
    tpool_t   *tp;
    pthread_t  thread;
    size_t     i;

    if (num == 0)
        num = 2;

    tp             = calloc(1, sizeof(*tp));
    tp->thread_cnt = num;

    pthread_mutex_init(&(tp->work_mutex), NULL);
    pthread_cond_init(&(tp->work_cond), NULL);
    pthread_cond_init(&(tp->working_cond), NULL);

    tp->work_first = NULL;
    tp->work_last  = NULL;

    for (i=0; i<num; i++) {
        pthread_create(&thread, NULL, tpool_worker, tp);
        pthread_detach(thread);
    }

    return tp;
}

void tpool_destroy(tpool_t *tp)
{
    tpool_work_t *work;
    tpool_work_t *work2;

    if (tp == NULL)
        return;

    pthread_mutex_lock(&(tp->work_mutex));
    work = tp->work_first;
    while (work != NULL) {
        work2 = work->next;
        tpool_work_destroy(work);
        work = work2;
    }
    tp->stop = true;
    pthread_cond_broadcast(&(tp->work_cond));
    pthread_mutex_unlock(&(tp->work_mutex));

    tpool_wait(tp);

    pthread_mutex_destroy(&(tp->work_mutex));
    pthread_cond_destroy(&(tp->work_cond));
    pthread_cond_destroy(&(tp->working_cond));

    free(tp);
}

bool tpool_add_work(tpool_t *tp, thread_func_t func, void *arg)
{
    tpool_work_t *work;

    if (tp == NULL)
        return false;

    work = tpool_work_create(func, arg);
    if (work == NULL)
        return false;

    pthread_mutex_lock(&(tp->work_mutex));
    if (tp->work_first == NULL) {
        tp->work_first = work;
        tp->work_last  = tp->work_first;
    } else {
        tp->work_last->next = work;
        tp->work_last       = work;
    }

    pthread_cond_broadcast(&(tp->work_cond));
    pthread_mutex_unlock(&(tp->work_mutex));

    return true;
}

void tpool_wait(tpool_t *tp)
{
    if (tp == NULL)
        return;

    pthread_mutex_lock(&(tp->work_mutex));
    while (1) {
        if ((!tp->stop && tp->working_cnt != 0) || (tp->stop && tp->thread_cnt != 0)) {
            pthread_cond_wait(&(tp->working_cond), &(tp->work_mutex));
        } else {
            break;
        }
    }
    pthread_mutex_unlock(&(tp->work_mutex));
}