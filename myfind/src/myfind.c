#define _GNU_SOURCE

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
#include <fnmatch.h>

//defines
#define TESTS_COUNT 3
#define ACTIONS_COUNT 2
#define KNOWN_ARGS_COUNT 5

//typedefs
typedef struct token * token_ptr;
typedef struct stat * stat_ptr;

typedef bool (*TOKEN_FUNC)(token_ptr token_ptr, stat_ptr s_ptr, const char *pathname);
typedef bool (*FILL_TOKEN_FUNC)(token_ptr token_ptr,  stat_ptr s_ptr, int argc, char* args[], int *index);

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
    unsigned int id; //-user
    char c; //-type
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

static void 
print_help(){
    printf("\nUSAGE\n\n");
    printf("myfind [path] [expression]\n\n");
    printf("ACTIONS: -print, -ls.\nActions do not take any argumants.\n\n");
    printf("TESTS: -name <pattern>, -type <t>, -user <name|id>.\nTests are always followed by a single argument.\n\n");
}

static void
print_error(char *message){
    if(message != NULL){
        printf("%s\n");
    }
    print_help();
}

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
fill_user_token(struct token *token_ptr, struct stat *stat_ptr, int argc, char* args[], int *index){
    if
    (
        token_ptr == NULL
        || args == NULL
        || index == NULL
        || *index >= argc-1
        || args[(*index)+1] == NULL
    )
    {
        return false;
    }
    int next_arg = (*index)+1;

    //check if arg is uid_t;
    char end_char = 0;
    char *end_ptr;
    long parsed_long = strtol(args[(*index)+1], &end_ptr, 10);
    __uid_t uid = parsed_long;
    struct passwd *password = NULL;

    if //no valid int
    (
        parsed_long != uid
        || end_ptr == args[next_arg] 
        || *end_ptr != '\0'
    )
    {
        //check for name
        password = getpwnam(args[next_arg]);
    }
    else //we have a valid int
    {
        password = getpwuid(uid);
    }

    if(password == NULL){
        printf("myfind: ‘%s’ is not the name of a known user", args[next_arg]);
        return false;
    }

    token_ptr->prerequisites = create_token_prerequisites();
    token_ptr->prerequisites.HasValidExpression = true;

    token_ptr->args.id = password->pw_uid;

    (*index)++;
    return true;
}

static bool
fill_print_token(struct token *token_ptr, struct stat *stat_ptr, int argc, char* args[], int *index){
    if (token_ptr == NULL){
        return false;
    }

    token_ptr->prerequisites = create_token_prerequisites();
    token_ptr->prerequisites.HasValidExpression = true;
    return true;
}

static bool
fill_ls_token(struct token *token_ptr, struct stat *stat_ptr, int argc, char* args[], int *index){
    if (token_ptr == NULL){
        return false;
    }

    token_ptr->prerequisites = create_token_prerequisites();
    token_ptr->prerequisites.HasValidExpression = true;
    return true;
}

static bool
fill_name_token(struct token *token_ptr, struct stat *stat_ptr, int argc, char* args[], int *index){
    if
    (
        token_ptr == NULL 
        || index == NULL
        || args == NULL
        || *index >= argc-1
        || args[(*index)+1] == NULL
    )
    {
        return false;
    }
    int next_arg = (*index)+1;

    token_ptr->prerequisites = create_token_prerequisites();
    token_ptr->prerequisites.HasValidExpression = true;
    token_ptr->args.str = args[next_arg];
    (*index)++;
    return true;
}

static bool
fill_type_token(struct token *token_ptr, struct stat *stat_ptr, int argc, char* args[], int *index){
    if
    (
        token_ptr == NULL 
        || index == NULL
        || args == NULL
        || *index >= argc-1
        || args[(*index)+1] == NULL
    )
    {
        return false;
    }

    int next_arg = (*index)+1;

    char *type = args[next_arg];
    if(strlen(type) != 1){
        return false;
    } 

    switch (*(args[next_arg]))
    {
    case 'b':
    case 'c':
    case 'd':
    case 'p':
    case 'f':
    case 'l':
    case 's':
        token_ptr->args.c = *(args[next_arg]);
        break;
    default:
        return false;
    }

    token_ptr->prerequisites = create_token_prerequisites();
    token_ptr->prerequisites.HasValidExpression = true;

    (*index)++;
    return true;
}

static bool
token_user_func(struct token *token_ptr, struct stat *stat_ptr, const char *pathname){
    if(token_ptr == NULL || stat_ptr == NULL){
        return false;
    }

    return stat_ptr->st_uid == token_ptr->args.id;
}


static bool
token_print_func(struct token *token_ptr, struct stat *stat_ptr, const char *pathname){
    return pathname != NULL && printf("%s\n", pathname);
}

//https://stackoverflow.com/questions/10323060/printing-file-permissions-like-ls-l-using-stat2-in-c
static char 
filetypeletter_ls(mode_t mode)
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

static char 
filetypeletter_type(mode_t mode)
{
    char c;

    if (S_ISREG(mode))
        c = 'f';
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

//https://stackoverflow.com/questions/10323060/printing-file-permissions-like-ls-l-using-stat2-in-c
/* Convert a mode field into "ls -l" type perms field. */
static char *lsperms(int mode)
{
    static const char *rwx[] = {"---", "--x", "-w-", "-wx",
    "r--", "r-x", "rw-", "rwx"};
    static char bits[11];

    bits[0] = filetypeletter_ls(mode);
    strcpy(&bits[1], rwx[(mode >> 6)& 7]);
    strcpy(&bits[4], rwx[(mode >> 3)& 7]);
    strcpy(&bits[7], rwx[(mode & 7)]);
    if (mode & S_ISUID)
        bits[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID)
        bits[6] = (mode & S_IXGRP) ? 's' : 'l';
#if defined __USE_MISC || defined __USE_XOPEN
    if (mode & S_ISVTX)
        bits[9] = (mode & S_IXOTH) ? 't' : 'T';
#endif
    bits[10] = '\0';
    return(bits);
}

static bool
token_ls_func(struct token *token_ptr, struct stat *stat_ptr, const char *pathname){
    if(stat_ptr == NULL){
        return false;
    }
    
    printf("%9d %6d ", stat_ptr->st_ino, (stat_ptr->st_blocks)/2);
    char *protection_bits = lsperms(stat_ptr->st_mode);
    printf("%s ", protection_bits);
    printf("%3d ", stat_ptr->st_nlink);

    struct passwd *pw = NULL;
    printf("%s  ", (pw = getpwuid(stat_ptr->st_uid)) != NULL ? pw->pw_name : "");
    printf("%s ", (pw = getpwuid(stat_ptr->st_gid)) != NULL ? pw->pw_name : "");

    printf("%9d ", stat_ptr->st_size);
    char buff[15];
    strftime(buff, 15, "%b %d %H:%M", localtime(&(stat_ptr->st_mtime)));
    printf(buff);
    printf(" %s", pathname);

    printf("\n");
    return true;
}


static bool
token_name_func(struct token *token_ptr, struct stat *stat_ptr, const char *pathname){
    if(token_ptr == NULL || pathname == NULL){
        return false;
    }

    char *name = basename(pathname);
    int result = fnmatch(token_ptr->args.str, name, 0);
    return !result;
};

static bool
token_type_func(struct token *token_ptr, struct stat *stat_ptr, const char *pathname){
    if
    (
        token_ptr == NULL
        || stat_ptr == NULL
    )
    {
        return false;
    }

    char c = filetypeletter_type(stat_ptr->st_mode);

    return token_ptr->args.c == c;
}


static const struct func_mapping KNOWN_ARGS[KNOWN_ARGS_COUNT]  = 
{
    { "-name", Name, token_name_func, fill_name_token},
    { "-type", Type, token_type_func, fill_type_token},
    { "-user", User, token_user_func, fill_user_token},
    { "-print", Print, token_print_func, fill_print_token},
    { "-ls", Ls, token_ls_func, fill_ls_token}
};

static bool
create_token(int *index, int i, int argc, char* args[], struct stat *stat_ptr, struct token *token){
    if(i < 0 || i > KNOWN_ARGS_COUNT){
        return false;
    }

    struct token out = {.type = KNOWN_ARGS[i].type, .func = KNOWN_ARGS[i].func};
    *token = out;
    return KNOWN_ARGS[i].fill_func(token, stat_ptr, argc, args, index);
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
        if(has_valid_prerequisites(pathname, &cur_token, s, &log)){
            log.isValidExpression &= cur_token.func(&(cur_token), s, pathname);
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
                //printf("Could not open directory %s\n", pathname );
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
        //printf("cannot get stat for %s.\n", pathname);
        return false;
    }

    return result;
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

    bool is_path = false;
    char *start_point = ".";

    if(argc > 1 && !is_known_command(1, args)){
        //TODO: check if arg is file or dict.
        if(stat (args[1], &s) == 0){
            if (is_dir_or_file(&s))
            {
                start_point = clean_filename(args[1]);
                is_path = true;
            }
            else {
                printf("myfind: ‘%s’: No such file or directory\n", args[1]);
                return EXIT_FAILURE;
            }
        }
        else{
            printf("myfind: unknown predicate `%s'\n", args[1]);
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
            printf("myfind: unknown predicate `%s'\n", args[i]);
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
                    printf("internal error\n");
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