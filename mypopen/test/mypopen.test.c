#include <stdlib.h>
#include <stdio.h>

#include "mypopen.h"

static int test_pipe(const char *arg1, const char *arg1_type, const char *arg2, const char *arg2_type, int test_no)
{
    printf("########## Test %2d ##########\n", test_no);
    printf("arg1: '%s', type: '%s', arg2: '%s', type: '%s'\n\n", arg1, arg1_type, arg2, arg2_type);

    // we create processes for both `ls` and `wc`
    // `popen` returns a FILE* that represents
    // the process' stdout (`r`) or stdin (`w`)
    FILE *ls = mypopen(arg1, arg1_type);
    FILE *wc = mypopen(arg2, arg2_type);

    // we consume the output of `ls` and feed it to `wc`
    char buf[1024];
    while (fgets(buf, sizeof(buf), ls) != NULL)
    {
        fputs(buf, wc);
    }
    
    // once we're done, we close the streams
    // close(fileno(ls));
    mypclose(ls);
    mypclose(wc);

    printf("######## End test %2d ########\n\n", test_no);

    return 1;
}

int main()
{
    int test_no = 1;

    test_pipe("ls", "r", "wc", "w", test_no++);

    test_pipe("ls ../", "r", "wc", "w", test_no++);

    test_pipe("ls ../../", "r", "wc", "w", test_no++);

    return EXIT_SUCCESS;
}