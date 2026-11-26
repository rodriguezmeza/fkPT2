/* The legacy numerical routines share a workspace. Give each run exclusive
 * use of it and release solver allocations even when a routine raises error().
 */
#include "fkpt2_internal.h"
#include <pthread.h>
#include <setjmp.h>

struct allocation {
    struct allocation *next;
    void *memory;
};

struct workspace {
    jmp_buf escape;
    struct allocation *allocations;
    char *message;
    size_t message_size;
};

static pthread_mutex_t solver_lock = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local struct workspace *active;

static int run_guarded(struct workspace *scope, int (*run)(void *), void *state)
{
    if (setjmp(scope->escape) != 0)
        return -1;
    return run(state);
}

static int run_in_scope(struct workspace *scope, int (*run)(void *), void *state)
{
    struct workspace *previous = active;
    active = scope;
    int status = run_guarded(scope, run, state);
    while (scope->allocations != NULL)
        fkpt2_work_free(scope->allocations->memory);
    active = previous;
    return status;
}

int fkpt2_with_workspace(int (*run)(void *), void *state, char *message, size_t size)
{
    if (active != NULL || pthread_mutex_lock(&solver_lock) != 0) {
        snprintf(message, size, "Could not acquire the fkPT2 solver workspace");
        return -1;
    }

    struct workspace scope = {.message = message, .message_size = size};
    int status = run_in_scope(&scope, run, state);

    /* All active pointers are reinitialized by the next run. Clear the
     * workspace references here as well, including after early failures. */
    PSLCDMtab = PSLCDMLogtab = PSLT = NULL;
    nPSTable = nPSLogT = nPSLT = 0;
    kPS = pPS = pPS2 = pPS_nw = pPS2_nw = NULL;
    fkT = fkT2 = DplusT = NULL;
    kFArrays = (global_kFArrays){0};
    kFArrays_nw = (global_kFArrays){0};
    pGL = NULL;

    pthread_mutex_unlock(&solver_lock);
    return status;
}

#ifdef OPENMPCODE
struct row_job {
    void (*work)(void *, int);
    void *state;
    int index;
};

static int run_row_job(void *data)
{
    struct row_job *job = data;
    job->work(job->state, job->index);
    return 0;
}
#endif

void fkpt2_parallel_for(int count, int threads, void (*work)(void *, int), void *state)
{
#ifdef OPENMPCODE
    threads = MIN(MIN(threads, count), omp_get_thread_limit());
    if (threads > 1) {
        int first_failure = count;
        char error_message[FKPT2_ERROR_SIZE];
#pragma omp parallel for num_threads(threads) schedule(guided)
        for (int i = 0; i < count; ++i) {
            /* Keep both allocations and longjmp on the worker that owns them.
             * Workers may only publish into disjoint, preallocated result slots. */
            char message[FKPT2_ERROR_SIZE];
            struct workspace scope = {.message = message, .message_size = sizeof(message)};
            struct row_job job = {work, state, i};
            if (run_in_scope(&scope, run_row_job, &job) != 0) {
#pragma omp critical(fkpt2_worker_failure)
                {
                    if (i < first_failure) {
                        first_failure = i;
                        snprintf(error_message, sizeof(error_message), "%s", message);
                    }
                }
            }
        }
        if (first_failure < count)
            error("%s", error_message);
        return;
    }
#else
    (void)threads;
#endif
    for (int i = 0; i < count; ++i)
        work(state, i);
}

void *fkpt2_work_own(void *memory)
{
    if (memory != NULL && active != NULL) {
        struct allocation *entry = malloc(sizeof(*entry));
        if (entry == NULL) {
            free(memory);
            error("Out of memory tracking a solver allocation");
        }
        *entry = (struct allocation){active->allocations, memory};
        active->allocations = entry;
    }
    return memory;
}

void *fkpt2_work_calloc(size_t count, size_t size)
{
    void *memory = calloc(count, size);
    if (memory == NULL)
        error("Out of memory allocating solver workspace");
    return fkpt2_work_own(memory);
}

void fkpt2_work_free(void *memory)
{
    if (memory != NULL && active != NULL) {
        struct allocation **entry = &active->allocations;
        while (*entry != NULL && (*entry)->memory != memory)
            entry = &(*entry)->next;

        if (*entry != NULL) {
            struct allocation *released = *entry;
            *entry = released->next;
            free(released);
        }
    }
    free(memory);
}

void fkpt2_work_raise(const char *message)
{
    if (active != NULL) {
        while (*message == '\n')
            ++message;
        snprintf(active->message, active->message_size, "%s", message);
        longjmp(active->escape, 1);
    }
}
