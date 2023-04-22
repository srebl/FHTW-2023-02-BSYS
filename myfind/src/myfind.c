//#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <time.h>

//defines
#define TESTS_COUNT 3
#define ACTIONS_COUNT 2
#define KNOWN_ARGS_COUNT 5

//typedefs
typedef struct token * token_ptr;
typedef struct stat * stat_ptr;

typedef bool (*TOKEN_FUNC)(token_ptr token_ptr, stat_ptr s_ptr, const char *pathname);
typedef bool (*FILL_TOKEN_FUNC)(token_ptr token_ptr,  stat_ptr s_ptr, int argi, char* args[], int *index);

//structs
enum token_type 
{ 
    Default,
    Action, 
    Test,
    Name,
    Type,
    User,
    Print,
    Ls
};

union token_args
{
    char *str; //-name, 
};

struct token_prerequisites{
    bool HasValidExpression;
};

struct token_log{
    bool isValidExpression;
};

struct token
{
    enum token_type type;
    struct token_prerequisites prerequisites;
    TOKEN_FUNC func;
    union token_args args; 
};

struct func_mapping{
    char *key;
    enum token_type type;
    TOKEN_FUNC func;
    FILL_TOKEN_FUNC fill_func;
};


static struct token_log
create_token_log(){
    struct token_log out = {
        .isValidExpression = true
    };
    return out;
}

static struct token_prerequisites
create_token_prerequisites(){
    struct token_prerequisites out = {
        .HasValidExpression = false
    };
    return out;
}

static bool 
dummy_token_func(struct token *t_ptr, struct stat *s_ptr, const char *pathname){
    printf("dummy_token_func\n");
    return true;
}

static bool
dummy_token_fill(struct token *t_ptr, struct stat *s_ptr, int argi, char* args[], int *index){
    printf("dummy_token_fill\n");
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

    token_ptr->prerequisites = create_token_prerequisites();

    (*index)++;
    return password != NULL;
}

static bool
token_print_func(struct token *token_ptr, struct stat *stat_ptr, const char *pathname){
    return pathname != NULL && printf("%s\n", pathname);
}

//https://stackoverflow.com/questions/10323060/printing-file-permissions-like-ls-l-using-stat2-in-c
static int 
filetypeletter(int mode)
{
    char c;

    if (S_ISREG(mode))
        c = '-';
    else if (S_ISDIR(mode))
        c = 'd';
    else if (S_ISBLK(mode))
        c = 'b';
    else if (S_ISCHR(mode))
        c = 'c';
#ifdef S_ISFIFO
    else if (S_ISFIFO(mode))
        c = 'p';
#endif  /* S_ISFIFO */
#ifdef S_ISLNK
    else if (S_ISLNK(mode))
        c = 'l';
#endif  /* S_ISLNK */
#ifdef S_ISSOCK
    else if (S_ISSOCK(mode))
        c = 's';
#endif  /* S_ISSOCK */
#ifdef S_ISDOOR
    /* Solaris 2.6, etc. */
    else if (S_ISDOOR(mode))
        c = 'D';
#endif  /* S_ISDOOR */
    else
    {
        /* Unknown type -- possibly a regular file? */
        c = '?';
    }
    return(c);
}

static bool
token_ls_func(struct token *token_ptr, struct stat *stat_ptr, const char *pathname){
    if(stat_ptr == NULL){
        return false;
    }
    
    printf("%9d %6d ", stat_ptr->st_ino, stat_ptr->st_blocks);
    printf( (S_ISDIR(stat_ptr->st_mode)) ? "d" : "-");
    printf( (stat_ptr->st_mode & S_IRUSR) ? "r" : "-");
    printf( (stat_ptr->st_mode & S_IWUSR) ? "w" : "-");
    printf( (stat_ptr->st_mode & S_IXUSR) ? "x" : "-");
    printf( (stat_ptr->st_mode & S_IRGRP) ? "r" : "-");
    printf( (stat_ptr->st_mode & S_IWGRP) ? "w" : "-");
    printf( (stat_ptr->st_mode & S_IXGRP) ? "x" : "-");
    printf( (stat_ptr->st_mode & S_IROTH) ? "r" : "-");
    printf( (stat_ptr->st_mode & S_IWOTH) ? "w" : "-");
    printf( (stat_ptr->st_mode & S_IXOTH) ? "x" : "-");
    printf("  %3d ", stat_ptr->st_nlink);

    struct passwd *pw = NULL;
    printf("%s  ", (pw = getpwuid(stat_ptr->st_uid)) != NULL ? pw->pw_name : "");
    printf("%s ", (pw = getpwuid(stat_ptr->st_gid)) != NULL ? pw->pw_name : "");

    printf("%9d ", stat_ptr->st_size);
    char buff[100];
    strftime(buff, 100, "%b %d %H:%M", localtime(&(stat_ptr->st_mtime)));
    printf(buff);
    printf(" %s", pathname);

    printf("\n");
}

static bool
fill_print_token(struct token *token_ptr, struct stat *stat_ptr, int argi, char* args[], int *index){
    token_ptr->prerequisites = create_token_prerequisites();
    return true;
}

static const struct func_mapping KNOWN_ARGS[KNOWN_ARGS_COUNT]  = 
{
    { "-name", Name, dummy_token_func, dummy_token_fill},
    { "-type", Type, dummy_token_func, dummy_token_fill},
    { "-user", User, dummy_token_func, fill_user_token},
    { "-print", Print, token_print_func, fill_print_token},
    { "-ls", Ls, token_ls_func, dummy_token_fill}
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
has_valid_prerequisites(char *pathname, struct token *t, struct stat *s, struct token_log *l){
    if
    (
        t->prerequisites.HasValidExpression
        && !l->isValidExpression
    )
    {
        return false;
    }

    return true;
}

static bool
execute_tokens(char *pathname, int token_count, struct token token_list[], struct stat *s){
    struct token_log log = create_token_log();

    for(int i = 0; i < token_count; i++){
        struct token cur_token = token_list[i];
        if(log.isValidExpression && has_valid_prerequisites(pathname, &cur_token, s, &log)){
            bool result = cur_token.func(&(cur_token), s, pathname);
            log.isValidExpression &= result;
        } else {
            log.isValidExpression = false;
        }
    }

    return log.isValidExpression;
}

static bool
walk_tree(char *pathname, int token_count, struct token token_list[], struct stat *s, int level){
    bool result = true;

    if(stat (pathname, s) == 0){
        result &= execute_tokens(pathname, token_count, token_list, s);

        if(s->st_mode & __S_IFDIR){
            //https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/
            struct dirent *de;  // Pointer for directory entry
  
            // opendir() returns a pointer of DIR type. 
            DIR *dr = opendir(pathname);
        
            if (dr == NULL)  // opendir returns NULL if couldn't open directory
            {
                printf("Could not open directory %s\n", pathname );
                result = false;
            }
        
            // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html
            // for readdir()
            while ((de = readdir(dr)) != NULL){
                if
                (
                    strcmp(de->d_name, ".") != 0 
                    && strcmp(de->d_name, "..") != 0
                )
                {
                    size_t index = 0;
                    size_t strlength = strlen(pathname) + strlen(de->d_name) + 2;
                    char path[strlength];
                    strcpy(path, pathname);
                    strcat(path, "/");
                    strcat(path, de->d_name);
                    result &= walk_tree(path, token_count, token_list, s, level+1);
                }
            }
        
            closedir(dr); 
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

char *
clean_filename(char *filename){
    size_t lenght = strlen(filename);
    if (filename[lenght-1] == '/'){
        filename[lenght-1] = '\0';
    }

    return filename;
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
    char *start_point = ".";

    if(!is_known_command(1, args)){
        //TODO: check if arg is file or dict.
        if(stat (args[1], &s) == 0){
            if (is_dir_or_file(&s))
            {
                start_point = clean_filename(args[1]);
                is_path = true;
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
    struct token token_list[argc];
    bool has_print_or_ls_token = false;
    struct token *cur_token = NULL;

    for (int i = 1+is_path, j = 0; i < argc; i++, token_list_length++){
        cur_token = &(token_list[token_list_length]);
        if(!parse_command(&i, argc, args, &s, cur_token)){
            printf("ERROR parsing arguments\n");
            print_help();
            return EXIT_FAILURE;
        }

        has_print_or_ls_token = has_print_or_ls_token || cur_token->type == Print || cur_token->type == Ls;
    }

    if(!has_print_or_ls_token){
        cur_token = &(token_list[token_list_length]);
        for (int i = 0; i < KNOWN_ARGS_COUNT; i++)
        {
            if(KNOWN_ARGS[i].type == Print){
                if(!create_token(&i, i, argc, args, &s, cur_token)){
                    printf("ERROR parsing arguments\n");
                    print_help();
                    return EXIT_FAILURE;
                } else {
                    cur_token->prerequisites.HasValidExpression = true;
                    break;
                }
            }
        }   
        token_list_length++;    
    }

    walk_tree(start_point, token_list_length, token_list, &s, 0);

    return EXIT_SUCCESS;
}