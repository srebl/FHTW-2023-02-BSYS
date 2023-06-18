#pragma once

#include <semaphore.h>

#define SUCCESS (1)
#define FAILIURE (-1)

struct ring_buffer
{
    size_t size;
    sem_t *sem_read;
    sem_t *sem_write;
    int memory_address;
    void *memory_location;
};

size_t parse_args(int argc, char *argv[]);

int initialize_ringbuffer(struct ring_buffer *self, size_t size);

void write_char(struct ring_buffer *self, size_t index, int character);

int read_char(struct ring_buffer *self, size_t index);

int destroy_ringbuffer_read(struct ring_buffer *self);

int destroy_ringbuffer_write(struct ring_buffer *self);

int wait_write(struct ring_buffer *self);

int post_write(struct ring_buffer *self);

int wait_read(struct ring_buffer *self);

int post_read(struct ring_buffer *self);
