/*a newline alone ends the code- similar to EOF/^D*/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define LSH_RL_BUFSIZE 1024 //a number that corresponds to a physical block size on a peripheral device
#define LSH_TOK_BUFSIZE 64
#define LSH_CMD_DELIM "|"
#define LSH_TOK_DELIM " \t\r\n\a"
#define AUPPRES -10 //don't run the function
#define PUPPRES 1 //run the function
#define DELETE 11
#define KEEP 10

void lsh_loop(void);
char *lsh_read_line(void);
char **lsh_split_line(char *);
int lsh_cd(char **);
int lsh_help(char **);
int lsh_exit(char **);
int memory_print(char **);
int spec_mem(char **);
int lsh_num_builtins(void);
int lsh_num_problems(void);
int lsh_launch(char **, int);
int lsh_execute(char **, int);
void printdir(void);
int checkpipe(char *);
char **cmd_arr(char *, int);
char **copy_cmdarr(char **);

//MEM FUNCTIONS
struct cmd_mem *addmem(struct cmd_mem *, char *);
struct cmd_mem *findmem(struct cmd_mem *, int);
struct cmd_mem *uppressed(struct cmd_mem *, int);
void printmem(struct cmd_mem *);

//sig handler functions
void my_sig_handler(int);
void big_sig_handler(struct sigaction);
void sig_handler(int);
void charwrite(char *, int);
struct sigaction sa_init(struct sigaction);
void sig_handler(int);

struct cmd_mem {
    char *command;
    struct cmd_mem *nxt_cmd;
};

struct cmd_mem *mem;

char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "memory",
    "smem"
};

char *issues[] = {
    "Only whitespace separating arguments",
    "No quoting or backslash escaping",
    "No redirection",
    "Few standard builtins.",
    "No up-arrow command memory"
};

int (*builtin_func[]) (char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit,
    &memory_print,
    &spec_mem
};

char *cwd = "/"; //root
int struct_count = 0;
int exnum = 0;
int whilel = 0;

int main(int argc, char **argv) {

    //load config files

    lsh_loop();

    //run shutdown/cleanup

    return EXIT_SUCCESS;
}

//What a shell does: read, seperate/parse, execute
void lsh_loop(void) {
    char *line, *string, *struct_line_copy;
    char **args, **ca, **cca;
    int status, numpipes;

    struct sigaction sa;
    sa.sa_handler = my_sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    //change this into a function?

    do {
        printdir();
        fflush(stdout);
        big_sig_handler(sa);
        charwrite("> ", KEEP);
        line = lsh_read_line();
        string = strdup(line);
        struct_line_copy = strdup(line);
        mem = addmem(mem, struct_line_copy);
        numpipes = checkpipe(line);
        ca = cmd_arr(string, numpipes);
        if (!ca) continue;
        status = lsh_execute(ca, numpipes);

        free(line);
        free(ca);
        //whilel = 0;
        //free(args);
    } while (status);
}

char *lsh_read_line(void) {
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "lsh: line allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if ((c = getchar()) == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else
            buffer[position] = c;
        position++;

        //if buffer is exceeded- reallocate
        if (position >= bufsize) {
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize * sizeof(char));
            if (!buffer) {
                fprintf(stderr, "lsh: line re-allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char **cmd_arr(char *line, int numpipes) {
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **cmdarr = malloc(bufsize * sizeof(char *));
    char *cmd;

    if (!cmdarr) {
        fprintf(stderr, "lsh: cmd_arr allocation error\n");
        exit(EXIT_FAILURE);
    }

    cmd = strtok(line, LSH_CMD_DELIM);

    while (cmd != NULL) {
        cmdarr[position] = cmd;
        position++;

        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            cmdarr = realloc(cmdarr, bufsize * sizeof(char *));
            if (!cmdarr) {
                fprintf(stderr, "lsh: cmd_arr re-allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    
        cmd = strtok(NULL, LSH_CMD_DELIM);
    }

    cmdarr[position] = NULL;

    return cmdarr;
}

char **lsh_split_line(char *line) {
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    
    char *token;

    if (!tokens) {
        fprintf(stderr, "lsh: token allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens) {
                fprintf(stderr, "lsh: token re-allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }

    tokens[position] = NULL;
    return tokens;
}

int checkpipe(char *line) {
    int i, k = 0;
    for (i = 0; line[i] != '\0'; i++)
        if (line[i] == '|')
            k++;
    return k;
}

//fix this

int lsh_launch(char **cmdarr, int numpipe) {
    pid_t pid, wpid;
    int status;
    char **args;

    if (numpipe == 0) {
        args = lsh_split_line(cmdarr[0]);

        pid = fork();
        
        if (pid == 0) {
            
            if (execvp(args[0], args) == -1)
                perror("lsh");

            exit(EXIT_FAILURE);

        } else if (pid < 0)
            //forking error
            perror("lsh");
        else {
            //parent process
            do {
                wpid = waitpid(pid, &status, WUNTRACED); //suspends the parent process until the child is done
                
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }

        return 1;
  
    } else {
        int numcmds = numpipe + 1;
        int fd[2 * numpipe];

        for (int i = 0; i < numpipe; i++)
            if (pipe(fd + i*2) == -1) {
                fprintf(stderr, "lsh: pipe read/write ends error\n");
                exit(1);
            }
        
        for (int i = 0; i < numcmds; i++) {
            pid = fork();

            if (pid == 0) {
                if (i != 0)
                    dup2(fd[(i-1)*2], STDIN_FILENO);
            
                if (i != numcmds - 1)
                    dup2(fd[i*2+1], STDOUT_FILENO);
                
                for (int j = 0; j < 2 * numpipe; j++) {
                    close(fd[j]);
                }  

                args = lsh_split_line(cmdarr[i]);
                if (execvp(args[0], args) == -1) {
                    perror("execvp");
                    exit(1);
                }

            }

        }

        //parent closes all pipe fds
        for (int i = 0; i < 2 * numpipe; i++) {
            close(fd[i]);
        }

        //Wait for children
        for (int i = 0; i < numcmds; i++) {
            wait(NULL);
        }
    }

    return 1;
}

//# of builtin functions
int lsh_num_builtins(void) {
    return sizeof(builtin_str) / sizeof(char *);
} 

//# of problems listed in issues
int lsh_num_problems(void) {
    return sizeof(issues) / sizeof(char*);
}

//executes builtins or pipes over to lsh_launch
int lsh_execute(char **cmdarr, int numpipes) {
    char **orig_cmdarr = copy_cmdarr(cmdarr);
    
    char **args;
    args = lsh_split_line(cmdarr[0]);
    if (args[0] == NULL)
        return 0;
    
    for (int i = 0; i < lsh_num_builtins(); i++)
        if (strcmp(args[0], builtin_str[i]) == 0)
            return (*builtin_func[i]) (args);

    //if it doesnt match a builtin function it'll check the os
    return lsh_launch(orig_cmdarr, numpipes);
}

void printdir(void) {
    int bufsize = LSH_RL_BUFSIZE;
    char *cwd = NULL;
    while (1) {
        cwd = malloc(bufsize);
        if (!cwd) {
            perror("lsh");
            exit(EXIT_FAILURE);
        }
        if (getcwd(cwd, bufsize) != NULL)
            break;

        free(cwd);
        bufsize += LSH_RL_BUFSIZE;
    }
    
    charwrite(cwd, DELETE);
}

char **copy_cmdarr(char **cmdarr) {
    int count = 0;

    while (cmdarr[count] != NULL) {
        count++;
    }

    //Allocate memory for the array of pointers (+1 for NULL terminator)
    char **copy = malloc((count + 1) * sizeof(char *));
    if (!copy) return NULL;

    //Duplicate each string
    for (int i = 0; i < count; i++) {
        copy[i] = strdup(cmdarr[i]);
        if (!copy[i]) {
            //handle allocation failure: free previously allocated strings
            for (int j = 0; j < i; j++)
                free(copy[j]);
            free(copy);
            return NULL;
        }
    }

    //NULL-terminate the array
    copy[count] = NULL;

    return copy;
}

/*Builtin function implementations*/

int memory_print(char **args) {

    printf("printing memory\n\n");
    printmem(mem);
    printf("\nfound all memory\n");

    return 1;
}

int spec_mem(char **args) {
    int n;
    if (args[1] == NULL)
        fprintf(stderr, "lsh: expected argument to \"specmem\"\n");
    else if ((sscanf(args[1], "%d", &n)) != 1)
        fprintf(stderr, "lsh: error converting \"%s\" to int", args[1]);
    else
        for (int i = 0; i < n; i++)
            uppressed(mem, PUPPRES);
        printf("%s\n", uppressed(mem, PUPPRES)->command);
    return 1;
}

int lsh_cd(char **args) {
    if (args[1] == NULL)
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    else if (chdir(args[1]) != 0) //chdir returns 0 if successful
            perror("lsh");

    return 1;
}

int lsh_help(char **args) {
    printf("Type program names and hit enter.\nThe follow programs are built in:\n");
    for (int i = 0; i < lsh_num_builtins(); i++)
        printf("\t%s\n", builtin_str[i]);
        printf("\nHowever there are some issues, such as:\n");
    for (int i = 0; i < lsh_num_problems(); i++)
        printf("\t%s\n", issues[i]);
    return 1;
}

int lsh_exit(char **args) {
    //return 0; //not exit(0) or exit(EXIT_SUCCESS)?
    exit(0);
}

/*End of builtin functions*/

/*Start of memory functions:*/

struct cmd_mem *addmem(struct cmd_mem *p, char *w) {
    if (p == NULL) {
        p = malloc(sizeof(struct cmd_mem));
        p->command = strdup(w);
        p->nxt_cmd = NULL;
        struct_count++;
        uppressed(NULL, AUPPRES);
    } else p->nxt_cmd = addmem(p->nxt_cmd, w);

    return p;
} //SHOULD add an "element" to p

struct cmd_mem *findmem(struct cmd_mem *p, int i) {
    static int count = 0;
    if (count == i) {
        count = 0;
        return p;
    }
    else {
        count++;
        return findmem(p->nxt_cmd, i);
    }
} //SHOULD return a pointer i "elements" in p

struct cmd_mem *uppressed(struct cmd_mem *p, int status) {
    static int not = 0;
    if (status == AUPPRES) {
        not = 0;
        return p;
    }
    else {
        not++;
        return findmem(p, struct_count-not);
    }
} //may have a fencepost error thing
//does not work when up arrow is pressed- but is used in other mem functions

void printmem(struct cmd_mem *p) {
    if (p->nxt_cmd == NULL) {
        return;
    }
    else {
        printf("%s\n", p->command);
        printmem(p->nxt_cmd);
    }
}

/*End of memory functions*/

/*CTRL-C Work*/

void big_sig_handler(struct sigaction sa) {
    for (int i = 0; i < NSIG; i++)
        sigaction(i, &sa, NULL);
}


void charwrite(char *string, int status) {
    if (status == DELETE)
        write(1, "\r\e[2K", 6);

    unsigned long len = strlen(string);
    unsigned long bytes = write(1, string, len);

    if (bytes == -1) 
        write(2, "Error writing to stdout\n", 25);

}

struct sigaction sa_init(struct sigaction sa) {
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    return sa;
}

void my_sig_handler(int sig) {
    switch (sig) {
        case SIGINT:
            charwrite("", DELETE);
            printdir();
            charwrite("> SIGINT\n", KEEP);
            printdir();
            charwrite("> ", KEEP);
            fflush(stdout);
            whilel = 1;
            break;
        case SIGTSTP:
            charwrite("", DELETE);
            printdir();
            charwrite("> ", KEEP); 
            fflush(stdout);
            whilel = 1;
            break;
        case SIGQUIT:
            charwrite("", DELETE);
            printdir();
            charwrite("> ", KEEP); 
            fflush(stdout);
            whilel = 1;
            break;
    }
}

void sig_handler(int sig) {
    char *str;
    sprintf(str, "sig_handler( %d ) called for ", sig);
    charwrite(str, DELETE);
    switch (sig) {
        /*case SIGINT:
            charwrite("SIGINT\n", KEEP);
            break;
        case SIGTSTP:
            charwrite("SIGTSTP\n", KEEP);
            break;
        case SIGQUIT:
            charwrite("SIGQUIT\n", KEEP);
            break;*/
        default:
            charwrite("UNKNOWN\n", KEEP);
            break;
    }
}
