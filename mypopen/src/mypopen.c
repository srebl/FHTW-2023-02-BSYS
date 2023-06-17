#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#include "mypopen.h"

struct node{
    struct node *next_ptr;
    FILE *f_ptr;
    pid_t pid;
};

static struct node *list_head = NULL;

static void add_node(struct node *n)
{
    n->next_ptr = list_head;
    list_head = n;
}

static void remove_node(struct node *n, struct node *prev)
{
    if(list_head == n)
    {
        list_head = n->next_ptr;
    }
    else
    {
        prev->next_ptr = n->next_ptr;
    }
}

FILE *mypopen(const char *command, const char *type)
{
    //no valid arguments
    if 
    (
        command == NULL
        || type == NULL
        || (*type != 'r' && *type != 'w')
    )
    {
        return NULL;
    }

    //open pipe(2)
    int pipefd[2];
    if(pipe(pipefd) < 0)
    {
        return NULL;
    }

    //create node for the list
    struct node *current;
    if((current = malloc(sizeof(struct node))) == NULL)
    {
        close(pipefd[0]); //read
        close(pipefd[1]); //write
        return NULL;
    }

    //create arguments for execvp(3)
    char *argv[4];
    argv[0] = "/sh";
	argv[1] = "-c";
	argv[2] = (char *)command;
	argv[3] = NULL;

    pid_t pid = 0;
    switch(pid = fork())
    {
        case -1: //error -> close pipe and free current
            close(pipefd[0]);
            close(pipefd[1]);
            free(current);
            return NULL;
            break;
        case 0: //child
        {
            if(*type == 'r')
            {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO); //duplicate file descriptor
                close(pipefd[1]);
            }
            else
            {
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO); //duplicate file descriptor
                close(pipefd[0]);
            }

            execvp("/bin/sh", argv);
            exit(127);
            break;
        }
    }

    FILE *f_ptr = NULL;

    if(*type == 'r')
    {
        f_ptr = fdopen(pipefd[0], type);
        close(pipefd[1]);
    }
    else
    {
        f_ptr = fdopen(pipefd[1], type);
        close(pipefd[0]);
    }

    //save information into node
    current->f_ptr = f_ptr;
    current->pid = pid;

    add_node(current);

    return f_ptr;
}

int mypclose(FILE *stream)
{
    struct node *current, *prev = NULL;

    //search for node with the same FILE
    current = list_head;
    while (current != NULL)
    {
        if(current->f_ptr == stream)
        {
            break;
        }

        prev = current;
        current = current->next_ptr;
    }
    
    //never opened or already closed
    if(current == NULL)
    {
        return -1;
    }

    //remove node and close FILE
    remove_node(current, prev);
    fclose(stream);
    
    int pstat = 0;
    pid_t pid = waitpid(current->pid, &pstat, 0);
    
    free(current);

    return WEXITSTATUS(pstat);
}