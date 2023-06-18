#include <stdlib.h>
#include <stdio.h>

#include "receiver.h"
#include "common.h"

int main(int argc, char *argv[])
{
    size_t size = parse_args(argc, argv);

    struct ring_buffer buffer = {0};

    if(initialize_ringbuffer(&buffer, size) == FAILIURE)
    {
        exit(EXIT_FAILURE);
    }

    int character = EOF;
    size_t index = 0;

    FILE *out = freopen(NULL, "wb", stdout);    

    do
    {
        if(wait_read(&buffer) == FAILIURE)
        {
            break;
        }

        character = read_char(&buffer, index);

        if(post_write(&buffer) == FAILIURE)
        {
            break;
        }

        if(character != EOF)
        {
            unsigned char c = character;
            fwrite(&c, sizeof(unsigned char), 1, out);
        }

        index = (++index) % size;

    } while (character != EOF);
    
    if(destroy_ringbuffer_read(&buffer) == FAILIURE)
    {
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}