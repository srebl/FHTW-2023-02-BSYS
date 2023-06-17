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

    do
    {
        character = getchar();
        char c = (char)character;

        printf("read char: %c at index %d\n",character, index);

        if(wait_write(&buffer) == FAILIURE)
        {
            break;
        }

        write_char(&buffer, index, character);

        if(post_read(&buffer) == FAILIURE)
        {
            break;
        }

        index = ++index % size;

    } while (character != EOF);
    
    if(destroy_ringbuffer_write(&buffer) == FAILIURE)
    {
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}