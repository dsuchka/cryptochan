#include "common.h"
#include "random.h"
#include "cyclic_queue.h"

#include <pthread.h>
#include <sched.h>

#define __PRODUCERS_COUNT       10
#define __CONSUMERS_COUNT       ((__PRODUCERS_COUNT * 3) / 4)
#define __TEST_VALUES_COUNT     ((CYCLIC_QUEUE_MIN_CAPACITY) * 100 * (__PRODUCERS_COUNT))

typedef struct __complex_uint64 {
    uint64_t real;
    uint64_t img;
} complex_uint64_t;

typedef struct __runner_data_t {
    pthread_t           self;
    char                *name;
    cyclic_queue_t      *queue;
    complex_uint64_t    *values;
    _Atomic int         *vindex;
    _Atomic int         *consumed_count;
    _Atomic int         *stage;
    complex_uint64_t    result;
    int                 processed_count;
} runner_data_t;


void *run_producer(void *arg);
void *run_consumer(void *arg);

int main(int argc, char **argv)
{
    _Atomic int stage = 0;
    _Atomic int vindex = 0;
    _Atomic int consumed_count = 0;
    cyclic_queue_t queue;
    complex_uint64_t *values = malloc(__TEST_VALUES_COUNT * sizeof(complex_uint64_t));
    fill_random((uint8_t*)values, __TEST_VALUES_COUNT * sizeof(complex_uint64_t));

    runner_data_t producers_data[__PRODUCERS_COUNT];
    runner_data_t consumers_data[__CONSUMERS_COUNT];

    memset(producers_data, 0, sizeof(producers_data));
    memset(consumers_data, 0, sizeof(consumers_data));

    cyclic_queue_init(&queue, sizeof(complex_uint64_t), 0);
    queue.max_capacity = 0x2000;

    for (int i = 0; i < __PRODUCERS_COUNT; ++i) {
        producers_data[i].queue = &queue;
        producers_data[i].values = values;
        producers_data[i].vindex = &vindex;
        producers_data[i].consumed_count = &consumed_count;
        producers_data[i].stage = &stage;
        if (pthread_create(&producers_data[i].self, NULL, run_producer, &producers_data[i]) != 0)
            { perror("pthread_create"); return EXIT_FAILURE; }
        if (asprintf(&producers_data[i].name, "producer-%d", i) == -1)
            { perror("asprintf"); return EXIT_FAILURE; }
    }

    for (int i = 0; i < __CONSUMERS_COUNT; ++i) {
        consumers_data[i].queue = &queue;
        consumers_data[i].values = values;
        consumers_data[i].vindex = &vindex;
        consumers_data[i].consumed_count = &consumed_count;
        consumers_data[i].stage = &stage;
        if (pthread_create(&consumers_data[i].self, NULL, run_consumer, &consumers_data[i]) != 0)
            { perror("pthread_create"); return EXIT_FAILURE; }
        if (asprintf(&consumers_data[i].name, "consumer-%d", i) == -1)
            { perror("asprintf"); return EXIT_FAILURE; }
    }

    // go
    stage = 1;

    for (int i = 0; i < __PRODUCERS_COUNT; ++i) {
        if (pthread_join(producers_data[i].self, NULL) != 0)
            { perror("pthread_join"); return EXIT_FAILURE; }
        printf("[main] thread [%s] finished\n", producers_data[i].name);
    }

    for (int i = 0; i < __CONSUMERS_COUNT; ++i) {
        if (pthread_join(consumers_data[i].self, NULL) != 0)
            { perror("pthread_join"); return EXIT_FAILURE; }
        printf("[main] thread [%s] finished\n", consumers_data[i].name);
    }

    // calc total sum and collect results (sum them), then compare
    complex_uint64_t expected_sum = {0, 0};
    complex_uint64_t consumed_sum = {0, 0};
    for (int i = 0; i < __TEST_VALUES_COUNT; ++i) {
        expected_sum.real += values[i].real;
        expected_sum.img += values[i].img;
    }
    for (int i = 0; i < __PRODUCERS_COUNT; ++i) {
        printf("[main] [%s] processed count: %d\n",
            producers_data[i].name, producers_data[i].processed_count);
    }
    for (int i = 0; i < __CONSUMERS_COUNT; ++i) {
        consumed_sum.real += consumers_data[i].result.real;
        consumed_sum.img += consumers_data[i].result.img;
        printf("[main] [%s] processed count: %d\n",
            consumers_data[i].name, consumers_data[i].processed_count);
    }

    printf("[main] expected sum: (%ld + %ldj)\n", expected_sum.real, expected_sum.img);
    printf("[main] consumed sum: (%ld + %ldj)\n", consumed_sum.real, consumed_sum.img);

    if ((expected_sum.real != consumed_sum.real) || (expected_sum.img != consumed_sum.img)) {
        abort();
    }

    return EXIT_SUCCESS;
}


void *run_producer(void *arg)
{
    runner_data_t *info = (runner_data_t*)arg;
    struct timespec sleep_time = {0, 0};
    int vidx = -1;
    bool cached = false;

    srand(*((unsigned int*)(info->values)));

    while ((vidx < __TEST_VALUES_COUNT) || cached) {
        // wait start signal
        if (*info->stage < 1) { sched_yield(); continue; }

        // sleep randomly
        if (rand() % 2) {
            sleep_time.tv_nsec = 300 + (rand() % 1234);
            if (nanosleep(&sleep_time, NULL) != 0) {
                fprintf(stderr, "WARN: [%s] nanosleep: %s\n", info->name, strerror(errno));
            }
        }

        // push next value

        // if value is not cached: get next index to read from values
        if (!cached) {
            do {
                vidx = atomic_load_explicit(info->vindex, memory_order_relaxed);
            } while (!atomic_compare_exchange_weak_explicit(
                    info->vindex, &vidx, vidx + 1,
                    memory_order_acquire, memory_order_relaxed
            ));
            if (vidx >= __TEST_VALUES_COUNT) { break; }
            //printf("INFO: [%s] push #%d (%ld + %ldj)\n",
            //    info->name, vidx, info->values[vidx].real, info->values[vidx].img);
            cached = true;
        }

        // try to push an element at cached index
        if (cyclic_queue_push(info->queue, &info->values[vidx])) {
            cached = false; // success, reset cached value (index)
            info->processed_count++;
        } else {
            //fprintf(stderr, "WARN: [%s] failed to push\n", info->name);
            sleep_time.tv_nsec = 50000;
            if (nanosleep(&sleep_time, NULL) != 0) {
                fprintf(stderr, "WARN: [%s] nanosleep: %s\n", info->name, strerror(errno));
            }
        }
    }

    return arg;
}

void *run_consumer(void *arg)
{
    runner_data_t *info = (runner_data_t*)arg;
    struct timespec sleep_time = {0, 0};
    complex_uint64_t next_val;

    srand(*((unsigned int*)(info->values + 10)));

    while (*info->consumed_count < __TEST_VALUES_COUNT) {
        // wait start signal
        if (*info->stage < 1) { sched_yield(); continue; }

        // sleep randomly
        if (rand() % 2) {
            sleep_time.tv_nsec = 300 + (rand() % 1234);
            if (nanosleep(&sleep_time, NULL) != 0) {
                fprintf(stderr, "WARN: [%s] nanosleep: %s\n", info->name, strerror(errno));
            }
        }

        // take next value
        if (cyclic_queue_take(info->queue, &next_val)) {
            info->result.real += next_val.real;
            info->result.img += next_val.img;
            //printf("INFO: [%s] took (%ld + %ldj)\n",
            //    info->name, next_val.real, next_val.img);
            atomic_fetch_add_explicit(info->consumed_count, 1, memory_order_relaxed);
            //printf("INFO: [%s] count = %d / %d\n",
            //    info->name, *(info->consumed_count), __TEST_VALUES_COUNT);
            info->processed_count++;
        }
    }

    return arg;
}

