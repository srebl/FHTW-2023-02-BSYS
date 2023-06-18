#include <stdlib.h>
#include <stdio.h>

#include "sender.h"
#include "common.h"

int main(int argc, char *argv[])
{
    size_t size = parse_args(argc, argv);

    struct ring_buffer buffer;

    if(initialize_ringbuffer(&buffer, size) == FAILIURE)
    {
        exit(EXIT_FAILURE);
    }

    int character = EOF;
    size_t index = 0;

    FILE *in = freopen(NULL, "rb", stdin);    
    size_t read_size = 0;
    do
    {
        read_size = fread(&character, sizeof(unsigned char), 1, in);

        if(read_size == 0)
        {
            character = EOF;
        }

        if(wait_write(&buffer) == FAILIURE)
        {
            break;
        }

        write_char(&buffer, index, character);

        if(post_read(&buffer) == FAILIURE)
        {
            break;
        }

        index = (++index) % size;

    } while (read_size != 0);
    
    if(destroy_ringbuffer_write(&buffer) == FAILIURE)
    {
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}