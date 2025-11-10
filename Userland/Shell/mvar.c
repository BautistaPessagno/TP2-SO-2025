#include <stdint.h>
#include <stdio.h>
#include <sys.h>

#include "tests/test_util.h"

#define DEFAULT_WRITERS 5
#define DEFAULT_READERS 2
#define MAX_WRITERS 26

#define MVAR_EMPTY_SEM_ID 3700
#define MVAR_FULL_SEM_ID 3701
#define MVAR_PRINT_SEM_ID 3702

#define MVAR_PROCESS_PRIORITY 2
#define MIN_DELAY_SPINS 15000u
#define RANDOM_DELAY_SPINS 60000u

static volatile char mvar_slot = 0;

static const char *const reader_colors[] = {
    "\e[31m", "\e[32m", "\e[33m", "\e[34m", "\e[35m", "\e[36m", "\e[37m",
    "\e[90m", "\e[91m", "\e[92m", "\e[93m", "\e[94m", "\e[95m", "\e[96m",
    "\e[97m"
};

#define MAX_READERS ((int)(sizeof(reader_colors) / sizeof(reader_colors[0])))

static inline uint16_t sem_handle(uint16_t id) { return id; }

static void random_delay(void) {
    uint32_t spins = MIN_DELAY_SPINS + GetUniform(RANDOM_DELAY_SPINS);
    bussy_wait(spins);
}

static int writer_process(int argc, char **argv);
static int reader_process(int argc, char **argv);
static int reset_mvar_sync(void);
static int parse_positive(const char *arg, int *out);

int cmd_mvar(int argc, char **argv) {
    int writers = DEFAULT_WRITERS;
    int readers = DEFAULT_READERS;

    if (argc == 2) {
        if (parse_positive(argv[0], &writers) != 0 ||
            parse_positive(argv[1], &readers) != 0) {
            fprintf(FD_STDERR, "mvar: parametros invalidos\n");
            return 1;
        }
    } else if (argc != 0) {
        fprintf(FD_STDERR, "mvar: uso mvar [escritores] [lectores]\n");
        return 1;
    }

    if (writers <= 0 || writers > MAX_WRITERS) {
        fprintf(FD_STDERR, "mvar: cantidad de escritores debe estar entre 1 y %d\n", MAX_WRITERS);
        return 1;
    }
    if (readers <= 0 || readers > MAX_READERS) {
        fprintf(FD_STDERR, "mvar: cantidad de lectores debe estar entre 1 y %d\n", MAX_READERS);
        return 1;
    }

    if (reset_mvar_sync() != 0) {
        fprintf(FD_STDERR, "mvar: no se pudieron inicializar los semaforos\n");
        return 1;
    }

    int32_t writer_pids[MAX_WRITERS] = {0};
    int32_t reader_pids[MAX_READERS] = {0};
    int launched_writers = 0;
    int launched_readers = 0;

    for (int i = 0; i < writers; i++) {
        char letter[2] = {(char)('A' + i), 0};
        char *writer_args[] = {letter, NULL};
        int32_t pid = createProcessWithFds(writer_process, writer_args, "mvar-writer",
                                           MVAR_PROCESS_PRIORITY, NULL);
        if (pid < 0) {
            fprintf(FD_STDERR, "mvar: error creando escritor %d\n", i);
            goto cleanup;
        }
        writer_pids[launched_writers++] = pid;
    }

    for (int i = 0; i < readers; i++) {
        char *reader_args[] = {(char *)reader_colors[i], NULL};
        int32_t pid = createProcessWithFds(reader_process, reader_args, "mvar-reader",
                                           MVAR_PROCESS_PRIORITY, NULL);
        if (pid < 0) {
            fprintf(FD_STDERR, "mvar: error creando lector %d\n", i);
            goto cleanup;
        }
        reader_pids[launched_readers++] = pid;
    }

    printf("[mvar] Se lanzaron %d escritores y %d lectores. Use ps/nice/kill para controlarlos.\n",
           writers, readers);
    return 0;

cleanup:
    for (int i = 0; i < launched_writers; i++) {
        if (writer_pids[i] > 0) {
            killProcess((uint16_t)writer_pids[i]);
        }
    }
    for (int i = 0; i < launched_readers; i++) {
        if (reader_pids[i] > 0) {
            killProcess((uint16_t)reader_pids[i]);
        }
    }
    fprintf(FD_STDERR, "mvar: abortando lanzamiento por errores\n");
    return 1;
}

static int reset_mvar_sync(void) {
    semDestroy(sem_handle(MVAR_EMPTY_SEM_ID));
    semDestroy(sem_handle(MVAR_FULL_SEM_ID));
    semDestroy(sem_handle(MVAR_PRINT_SEM_ID));

    mvar_slot = 0;

    if (semOpen(sem_handle(MVAR_EMPTY_SEM_ID), 1) < 0) {
        return -1;
    }
    if (semOpen(sem_handle(MVAR_FULL_SEM_ID), 0) < 0) {
        return -1;
    }
    if (semOpen(sem_handle(MVAR_PRINT_SEM_ID), 1) < 0) {
        return -1;
    }
    return 0;
}

static int writer_process(int argc, char **argv) {
    (void)argc;
    if (argv == NULL || argv[0] == NULL) {
        return -1;
    }

    char value = argv[0][0];

    while (1) {
        random_delay();
        semWait(sem_handle(MVAR_EMPTY_SEM_ID));
        mvar_slot = value;
        semPost(sem_handle(MVAR_FULL_SEM_ID));
    }
    return 0;
}

static int reader_process(int argc, char **argv) {
    (void)argc;
    static const char *const default_color = "\e[0m";
    const char *color = (argv == NULL || argv[0] == NULL) ? default_color : argv[0];

    while (1) {
        random_delay();
        semWait(sem_handle(MVAR_FULL_SEM_ID));
        char value = mvar_slot;
        mvar_slot = 0;
        semPost(sem_handle(MVAR_EMPTY_SEM_ID));

        semWait(sem_handle(MVAR_PRINT_SEM_ID));
        printf("%s%c\e[0m", color, value);
        semPost(sem_handle(MVAR_PRINT_SEM_ID));

        yield();
    }
    return 0;
}

static int parse_positive(const char *arg, int *out) {
    if (arg == NULL || out == NULL) {
        return -1;
    }
    int value = (int)satoi((char *)arg);
    if (value <= 0) {
        return -1;
    }
    *out = value;
    return 0;
}
