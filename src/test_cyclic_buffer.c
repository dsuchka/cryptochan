#include "common.h"
#include "random.h"
#include "cyclic_buffer.h"

#include "max_scalar.h"

#include <pthread.h>
#include <sched.h>

typedef struct __runner_data_t {
    pthread_t       self;
    cyclic_buffer_t *cbuf;
    uint8_t         *buf;
    uint32_t        len;
    const char      *name;
    _Atomic bool    *read_done;
} runner_data_t;

int run_simple_test();
int run_thread_test();

void *run_writer(void *arg);
void *run_recoder(void *arg);
void *run_reader(void *arg);

int main(int argc, char **argv)
{
    if (argc > 3) {
        fprintf(stderr, "Usage: %s [thread|simple]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int (*run_func)() = run_simple_test;

    if (argc == 2) {
        if (!strcasecmp(argv[1], "thread")) {
            run_func = run_thread_test;
        } else if (!strcasecmp(argv[1], "simple")) {
            run_func = run_simple_test;
        } else {
            fprintf(stderr, "Unknown mode: %s\n", argv[1]);
            return EXIT_FAILURE;
        }
    }

    return run_func();
}

int run_thread_test()
{
    uint8_t sbuf[0x40000];
    uint8_t dbuf[0x40000];
    uint8_t xbuf[0x1234];
    cyclic_buffer_t buffer;
    runner_data_t runners[3];
    _Atomic bool read_done;

    fill_random(sbuf, sizeof(sbuf));
    fill_random(dbuf, sizeof(dbuf));
    fill_random(xbuf, sizeof(xbuf));

    printf("max scalar size = %ld\n", sizeof(max_scalar_t));

    cyclic_buffer_init(&buffer, 1);
    memset(runners, 0, sizeof(runners));

#define __INIT_RUNNER(IDX, BUF, NAME) \
    { \
        runners[IDX].cbuf = &buffer; \
        runners[IDX].buf = BUF; \
        runners[IDX].len = sizeof(BUF); \
        runners[IDX].name = NAME; \
        runners[IDX].read_done = &read_done; \
    }

    __INIT_RUNNER(0, sbuf, "writer")
    __INIT_RUNNER(1, dbuf, "reader")
    __INIT_RUNNER(2, xbuf, "recoder")

    if (pthread_create(&runners[0].self, NULL, run_writer, &runners[0]) != 0)
        { perror("pthread_create"); return EXIT_FAILURE; }
    if (pthread_create(&runners[1].self, NULL, run_reader, &runners[1]) != 0)
        { perror("pthread_create"); return EXIT_FAILURE; }
    if (pthread_create(&runners[2].self, NULL, run_recoder, &runners[2]) != 0)
        { perror("pthread_create"); return EXIT_FAILURE; }

    for (int i = 0; i < sizeof(runners) / sizeof(runner_data_t); ++i) {
        if (pthread_join(runners[i].self, NULL) != 0)
            { perror("pthread_join"); return EXIT_FAILURE; }
        printf("[main] thread [%s] finished\n", runners[i].name);
    }

    // xor back
    for (int d_idx = 0, x_idx = 0; d_idx < sizeof(dbuf);) {
        dbuf[d_idx++] ^= xbuf[x_idx++];
        if (x_idx == sizeof(xbuf)) { x_idx = 0; }
    }

    if (memcmp(sbuf, dbuf, sizeof(sbuf))) {
        int i = 0;
        for (; i < sizeof(sbuf); ++i) {
            if (sbuf[i] != dbuf[i]) break;
            printf("%02x", sbuf[i]);
        }
        printf("\n");
        printf("dbuf[%d] = 0x%02x vs sbuf[%d] = 0x%02x", i, dbuf[i], i, sbuf[i]);
        printf("\n");
        exit(0);
    }

    return EXIT_SUCCESS;
}


void *run_writer(void *arg)
{
    runner_data_t *info = (runner_data_t*)arg;
    struct timespec sleep_time;

    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 0;

    srand(*((unsigned int*)(info->buf)));

    for (int b_idx = 0; b_idx < info->len;) {
        // sleep randomly
        if (rand() % 3) {
            sleep_time.tv_nsec = 500 + (rand() % 12345);
            if (nanosleep(&sleep_time, NULL) != 0) {
                fprintf(stderr, "WARN: [%s] nanosleep: %s\n", info->name, strerror(errno));
            }
        }

        // write next portion of data
        int size = (rand() % 2) ? (info->len / 10) : (1 + (rand() & 0x3F));
        size = MIN(size, info->len - b_idx);
        int n = cyclic_buffer_write(info->cbuf, info->buf + b_idx, size);
        printf("INFO: [%s] b_idx = 0x%04x   n = %d/%d\n", info->name, b_idx, n, size);

        b_idx += n;
        if (n == 0) sched_yield();
    }

    return arg;
}


void *run_recoder(void *arg)
{
    runner_data_t *info = (runner_data_t*)arg;
    struct timespec sleep_time;

    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 0;

    srand(*((unsigned int*)(info->buf)));

    for (int b_idx = 0; b_idx < info->len;) {
        // sleep randomly
        if (rand() % 2) {
            sleep_time.tv_nsec = 300 + (rand() % 3456);
            if (nanosleep(&sleep_time, NULL) != 0) {
                fprintf(stderr, "WARN: [%s] nanosleep: %s\n", info->name, strerror(errno));
            }
        }

        // recode next portion of data
        int size = (rand() % 2) ? (info->len / 3) : (5 + (rand() & 0x3F));
        size = MIN(size, info->len - b_idx);
        int n = cyclic_buffer_recode_xor(info->cbuf, info->buf + b_idx, size);
        printf("INFO: [%s] b_idx = 0x%04x   n = %d/%d\n", info->name, b_idx, n, size);

        b_idx += n;
        if (b_idx == info->len) { b_idx = 0; }
        if (n == 0) {
            sched_yield();
            if (*info->read_done) break;
        }
    }

    return arg;
}


void *run_reader(void *arg)
{
    runner_data_t *info = (runner_data_t*)arg;
    struct timespec sleep_time;

    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 0;

    srand(*((unsigned int*)(info->buf)));

    for (int b_idx = 0; b_idx < info->len;) {
        // sleep randomly
        if (rand() % 2) {
            sleep_time.tv_nsec = 200 + (rand() % 2345);
            if (nanosleep(&sleep_time, NULL) != 0) {
                fprintf(stderr, "WARN: [%s] nanosleep: %s\n", info->name, strerror(errno));
            }
        }

        // read next portion of data
        int size = (rand() % 2) ? (info->len / 10) : (3 + (rand() & 0x7F));
        size = MIN(size, info->len - b_idx);
        int n = cyclic_buffer_read(info->cbuf, info->buf + b_idx, size);
        printf("INFO: [%s] b_idx = 0x%04x   n = %d/%d\n", info->name, b_idx, n, size);

        b_idx += n;
        if (n == 0) sched_yield();
    }

    *info->read_done = true;

    return arg;
}


int run_simple_test()
{
    cyclic_buffer_t buffer;
    cyclic_buffer_init(&buffer, 1);

    uint8_t sbuf[0x40000];
    uint8_t dbuf[0x40000];
    uint8_t xbuf[0x1234];

    for (int k = 0; k < 5; ++k) {
        fill_random(sbuf, sizeof(sbuf));
        fill_random(dbuf, sizeof(dbuf));
        fill_random(xbuf, sizeof(xbuf));
        srand(*((unsigned int*)sbuf));
        for (int s_idx = 0, d_idx = 0, x_idx = 0; d_idx < sizeof(dbuf);) {
            if (rand() % 5) {
                int to_write = (rand() % 2) ? sizeof(sbuf) : rand() & 0x3F;
                to_write  = MIN(to_write, sizeof(sbuf) - s_idx);
                int n = cyclic_buffer_write(&buffer, &sbuf[s_idx], to_write);
                printf("wrt: k = %-3d s_idx = 0x%04x d_idx = 0x%04x n = %d/%d\n",
                    k, s_idx, d_idx, n, to_write);
                s_idx += n;
            }

            if (rand() % 3) {
                int to_recode = (rand() % 2) ? sizeof(xbuf) : rand() & 0x3F;
                to_recode = MIN(to_recode, sizeof(xbuf) - x_idx);
                int n = cyclic_buffer_recode_xor(&buffer, &xbuf[x_idx], to_recode);
                printf("recoded: n = %d (x_idx = %d)\n", n, x_idx);
                x_idx += n;
                if (x_idx == sizeof(xbuf)) { x_idx = 0; }
            }

            if (rand() % 4) {
                int to_read = (rand() % 2) ? sizeof(dbuf) : rand() & 0x7F;
                to_read  = MIN(to_read, sizeof(dbuf) - d_idx);
                int n = cyclic_buffer_read(&buffer, &dbuf[d_idx], to_read);
                printf("rdn: k = %-3d s_idx = 0x%04x d_idx = 0x%04x n = %d/%d\n",
                    k, s_idx, d_idx, n, to_read);
                d_idx += n;
            }
        }

        printf("\n");

        // xor back
        for (int d_idx = 0, x_idx = 0; d_idx < sizeof(dbuf);) {
            dbuf[d_idx++] ^= xbuf[x_idx++];
            if (x_idx == sizeof(xbuf)) { x_idx = 0; }
        }

        if (memcmp(sbuf, dbuf, sizeof(sbuf))) {
            int i = 0;
            for (; i < sizeof(sbuf); ++i) {
                if (sbuf[i] != dbuf[i]) break;
                printf("%02x", sbuf[i]);
            }
            printf("\n");
            printf("dbuf[%d] = 0x%02x vs sbuf[%d] = 0x%02x", i, dbuf[i], i, sbuf[i]);
            printf("\n");
            exit(0);
        }
    }

    return EXIT_SUCCESS;
}

