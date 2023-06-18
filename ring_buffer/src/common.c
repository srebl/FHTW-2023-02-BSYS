#include <stdio.h>	   // perror()
#include <unistd.h>	   // getopt()
#include <stdlib.h>	   // atoi()
#include <sys/mman.h>  // POSIX shared memory library
// #include <sys/shm.h>   // POSIX shared memory library
#include <semaphore.h> // POSIX semaphore library
#include <sys/stat.h>  /* For mode constants */
#include <fcntl.h>	   /* For O_* constants */
#include <errno.h>	   // For errno obviously
#include <string.h>

#include "common.h"

static const char* MEMORY = "/shared_memory";
static const char* SEMAPHORE_READ = "/semaphore_read";
static const char* SEMAPHORE_WRITE = "/semaphore_write";

static sem_t * open_semaphore(const char *filename, size_t init_val);
static int detatch_semaphore(const char *filename);
static int destroy_semaphore(const char *filename, sem_t *sem);
static int create_memory(const char* filename, size_t size);
static void * attatch_memory(int memory, size_t size);
static int detatch_memory(void *memory_address, size_t size);
static int destroy_memory(const char *filename);
static int wait_semaphore(sem_t *sem);
static int post_semaphore(sem_t *sem);
static void print_help_message(); 

size_t parse_args(int argc, char *argv[])
{
    if(argc < 2)
    {
        print_help_message();
        exit(EXIT_FAILURE);
    }

    char *options = "hm:";
    char option;

    size_t size = 0;

    while ((option = getopt(argc, argv, options)) != -1)
    {
        switch (option)
        {
        case 'm':
        {
            char *end = NULL;
            errno = 0;
            // char *meh = optarg;
            size = strtoul(optarg, &end, 10);

            if(errno || *end != '\0')
            {
                fprintf(stderr, "could not parse argument to an int\n");
                exit(EXIT_FAILURE);
            }

            if(size == 0)
            {
                fprintf(stderr, "size mit be greater than 0\n");
                exit(EXIT_FAILURE);
            }
        }
            break;
        case 'h':
            //print some help message
            exit(EXIT_SUCCESS);
        default:
            fprintf(stderr, "Usage: %s -m size\n", argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }
    
    return size;
}

int initialize_ringbuffer(struct ring_buffer *self, size_t size)
{
    self->size = size;

    self->sem_read = open_semaphore(SEMAPHORE_READ, 0);
    self->sem_write = open_semaphore(SEMAPHORE_WRITE, self->size);
    // self->sem_write = open_semaphore(SEMAPHORE_WRITE, 1);

    if(self->sem_read == NULL || self->sem_write == NULL)
    {
        return FAILIURE;
    }

    int memory = create_memory(MEMORY, self->size * sizeof(int));

    if(memory == FAILIURE)
    {
        destroy_semaphore(SEMAPHORE_READ, self->sem_read);
        destroy_semaphore(SEMAPHORE_WRITE, self->sem_write);
        return FAILIURE;
    }

    self->memory_location = attatch_memory(memory, self->size * sizeof(int));

    if(self->memory_location == NULL)
    {
        destroy_semaphore(SEMAPHORE_READ, self->sem_read);
        destroy_semaphore(SEMAPHORE_WRITE, self->sem_write);
        return FAILIURE;
    }

    return SUCCESS;
}

int destroy_ringbuffer_read(struct ring_buffer *self)
{
    int result = destroy_semaphore(SEMAPHORE_READ, self->sem_read);
    result |= detatch_memory(self->memory_location, self->size * sizeof(int));
    result |= destroy_memory(MEMORY);

    return result == SUCCESS ? SUCCESS : FAILIURE;
}

int destroy_ringbuffer_write(struct ring_buffer *self)
{
    int result = destroy_semaphore(SEMAPHORE_WRITE, self->sem_write);
    result |= detatch_memory(self->memory_location, self->size * sizeof(int));
    result |= destroy_memory(MEMORY);

    return result == SUCCESS ? SUCCESS : FAILIURE;
}

void write_char(struct ring_buffer *self, size_t index, int character)
{
    ((int*)self->memory_location)[index] = character;
}

int read_char(struct ring_buffer *self, size_t index)
{
    return ((int*)self->memory_location)[index];
}

int wait_write(struct ring_buffer *self)
{
    return wait_semaphore(self->sem_write);
}

int post_write(struct ring_buffer *self)
{
    return post_semaphore(self->sem_write);
}

int wait_read(struct ring_buffer *self)
{
    return wait_semaphore(self->sem_read);
}

int post_read(struct ring_buffer *self)
{
    return post_semaphore(self->sem_read);
}

static void print_help_message()
{
    printf("some help message\n");
}

static int wait_semaphore(sem_t *sem)
{
    while (1)
	{
		int const signal = sem_wait(sem);

		if (signal == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}

            fprintf(stderr, "failed to wait for semaphore\n");
			return FAILIURE;
		}

		return SUCCESS;
	}
}

static int post_semaphore(sem_t *sem)
{
    while (1)
	{
		int const signal = sem_post(sem);

		if (signal == -1)
		{
            fprintf(stderr, "failed to post for semaphore\n");
			return FAILIURE;
		}

		return SUCCESS;
	}
}


static sem_t * open_semaphore(const char *filename, size_t init_val)
{
    sem_t *sem = sem_open(filename, O_CREAT, S_IRWXU, init_val);

    if(sem == SEM_FAILED)
    {
        fprintf(stderr, "failed to open semaphore\n");
        return NULL;
    }

    return sem;
}

static int detatch_semaphore(const char *filename)
{
    if(sem_unlink(filename) == -1)
    {
        fprintf(stderr, "failed to detatch semapore\n");
        return FAILIURE;
    }

    return SUCCESS;
}

static int destroy_semaphore(const char *filename, sem_t *sem)
{
    if(sem_close(sem) == -1)
    {
        fprintf(stderr, "failed to close semaphore\n");
        return FAILIURE;
    }

    if(sem_unlink(filename) == -1)
    {
        fprintf(stderr, "failed to unlink semaphore\n");
        return FAILIURE;
    }

    return SUCCESS;
}

static int create_memory(const char* filename, size_t size)
{
    int memory = shm_open(filename, O_RDWR | O_CREAT, S_IRWXU);

    if(memory == -1)
    {
        fprintf(stderr, "could not create shared memory\n");
        return FAILIURE;
    }

    if(ftruncate(memory, size) == -1)
    {
        fprintf(stderr, "failed to set size for memory\n");
        return FAILIURE;
    }

    return memory;
}

static void * attatch_memory(int memory, size_t size)
{
    void *output = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, memory, 0);

    if(output == MAP_FAILED)
    {
        fprintf(stderr, "failed to attatch memory\n");
        shm_unlink(MEMORY);
        return NULL;
    }

    close(memory);
    return output;
}

static int detatch_memory(void *memory_address, size_t size)
{
    if(munmap(memory_address, size) == -1)
    {
        fprintf(stderr, "failed to detatch memory\n");
        return -1;
    }

    return SUCCESS;
}

static int destroy_memory(const char *filename)
{
    if(shm_unlink(filename) == -1)
    {
        if(errno == ENOENT)
        {
            return SUCCESS;
        }

        fprintf(stderr, "failed to destroy memory\n");
        return FAILIURE;
    }

    return SUCCESS;
}