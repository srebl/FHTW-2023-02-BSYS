#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <sys/types.h>
#include <pwd.h>

//defines
#define TESTS_COUNT 3
#define ACTIONS_COUNT 2
#define KNOWN_ARGS_COUNT 5

//typedefs
typedef bool (*TOKEN_FUNC)(struct token *token_ptr, struct stat *stat_ptr, const char *pathname);
typedef bool (*FILL_TOKEN_FUNC)(struct token *token_ptr, struct stat *stat_ptr, int argi, char* args[], int *index);

//structs
enum token_type 
{ 
    Default,
    Action, 
    Test,
    User
};

union token_args
{
    char *str //-name, 
};

struct token
{
    enum token_type type;
    TOKEN_FUNC func;
    union token_args args; 
};

struct func_mapping{
    char *key;
    enum token_type type;
    TOKEN_FUNC func;
    FILL_TOKEN_FUNC fill_func;
};

static bool 
dummy_token_func(struct token *token_ptr, struct stat *stat_ptr, const char *pathname){
    printf("dummy_token_func\n");
    return true;
}

static bool
dummy_token_fill(struct token *token_ptr, struct stat *stat_ptr, int argi, char* args[], int *index){
    printf("dummy_token_fill\n");
    return true;
}

static bool
fill_action(struct token *token_ptr, struct stat *stat_ptr, int argi, char* args[], int *index){
    return true;
}

static bool
fill_user_token(struct token *token_ptr, struct stat *stat_ptr, int argi, char* args[], int *index){
    //check if arg is uid_t;
    char end_char = 0;
    char *end_ptr;
    long parsed_long = strtol(args[argi], &end_ptr, 10);
    __uid_t uid = parsed_long;
    struct passwd *password = NULL;

    if //no valid int
    (
        parsed_long != uid
        || end_ptr == args[argi] 
        || *end_ptr != '\0'
    )
    {
        //check for name
        password = getpwnam(args[argi]);
    }
    else //we have a valid int
    {
        password = getpwuid(uid);
    }

    (*index)++;
    return password != NULL;
}

static const struct func_mapping KNOWN_ARGS[KNOWN_ARGS_COUNT]  = 
{
    { "-name", Test, dummy_token_func, dummy_token_fill},
    { "-type", Test, dummy_token_func, dummy_token_fill},
    { "-user", User, dummy_token_func, fill_user_token},
    { "-print", Action, dummy_token_func, dummy_token_fill},
    { "-ls", Action, dummy_token_func, dummy_token_fill}
};

static bool
create_token(int *index, int i, int argc, char* args[], struct stat *stat_ptr, struct token *token){
    if(i < 0 || i > KNOWN_ARGS_COUNT){
        return false;
    }

    struct token out = {.type = KNOWN_ARGS[i].type, .func = KNOWN_ARGS[i].func};
    *token = out;
    return KNOWN_ARGS[i].fill_func(token, stat_ptr, i, args, index);
}

static bool
parse_command(int *index, int argc, char* args[], struct stat *stat_ptr, struct token *token){

    for (int i = 0; i < KNOWN_ARGS_COUNT; i++){
        if (strcmp(KNOWN_ARGS[i].key, args[*index]) == 0){
            return create_token(index, i, argc, args, stat_ptr, token);
        }
    }

    return false;
}

static bool
is_known_command(int index, char* args[]){

    for (int i = 0; i < KNOWN_ARGS_COUNT; i++){
        if (strcmp(KNOWN_ARGS[i].key, args[index]) == 0){
            return true;
        }
    }

    return false;
}

static bool
is_dir_or_file(struct stat *s){
    return (s->st_mode & __S_IFDIR) || (s->st_mode & __S_IFREG);
}

static bool
execute_tokens(char *pathname, int token_count, struct token token_list[], struct stat *s){
    bool result = true;

    for(int i = 0; i < token_count; i++){
        result &= token_list[i].func(&token_list[i], s, pathname);
    }

    return result;
}

static bool
walk_tree(char *pathname, int token_count, struct token token_list[], struct stat *s){
    bool result = true;

    if(stat (pathname, &s) == 0){
        if(s->st_mode & __S_IFDIR){
            //https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/
            struct dirent *de;  // Pointer for directory entry
  
            // opendir() returns a pointer of DIR type. 
            DIR *dr = opendir(pathname);
        
            if (dr == NULL)  // opendir returns NULL if couldn't open directory
            {
                printf("Could not open directory %s\n", pathname );
                return false;
            }
        
            // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
            // for readdir()
            while ((de = readdir(dr)) != NULL){
                if(stat (pathname, &s) == 0){
                    if((s->st_mode & __S_IFDIR)){
                        result &= walk_tree(de->d_name, token_count, token_list, s);
                    } else if(s->st_mode & __S_IFREG){
                        result &= execute_tokens(de->d_name, token_count, token_list, s);
                    }
                }
            }
        
            closedir(dr); 
        } else if(s->st_mode & __S_IFREG){
            result &= execute_tokens(pathname, token_count, token_list, s);
        }
    }
    else{
        printf("cannot get stat for %s.\n", pathname);
        return false;
    }

    return result;
}

static void 
print_help(){
    printf("\nUSAGE\n\n");
    printf("myfind [path] [expression]\n\n");
    printf("ACTIONS: -print, -ls.\nActions do not take any argumants.\n\n");
    printf("TESTS: -name <pattern>, -type <t>, -user <name|id>.\nTests are always followed by a single argument.\n\n");
}

int 
main (int argc, char* args[])
{
    struct stat s;

    if(argc == 1){
        printf("find all in current dic and print\n");
        return EXIT_SUCCESS;
    }

    bool is_path = false;
    char *start_point = NULL;

    if(!is_known_command(1, args)){
        //TODO: check if arg is file or dict.
        if(stat (args[1], &s) == 0){
            if (is_dir_or_file(&s))
            {
                is_path = true;
                start_point = args[1];
            }
            else {
                printf("start point must be a directory or a file.\n");
                print_help();
                return EXIT_FAILURE;
            }
        }
        else{
            printf("cannot get stat.\n");
            print_help();
            return EXIT_FAILURE;
        }
    }

    int token_list_length = 0;
    struct token token_list[argc-2-is_path];

    for (int i = 1+is_path, j = 0; i < argc; i++, token_list_length++){
        struct token cur_token = token_list[token_list_length];
        if(!parse_command(&i, argc, args, &s, &cur_token)){
            printf("ERROR parsing arguments\n");
            print_help();
            return EXIT_FAILURE;
        }
    }

    walk_tree(start_point, token_list_length, token_list, &s);

    return EXIT_SUCCESS;
}