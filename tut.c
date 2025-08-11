/*Only whitespace separating arguments, no quoting or backslash escaping.
No piping or redirection.
Few standard builtins.
No globbing.
*/

//Cleanup code after piping is added

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define LSH_RL_BUFSIZE 1024 //a number that corresponds to a physical block size on a peripheral device
#define LSH_TOK_BUFSIZE 64
#define LSH_CMD_DELIM "|"
#define LSH_TOK_DELIM " \t\r\n\a"

void lsh_loop(void);
char *lsh_read_line(void);
char **lsh_split_line(char *);
int lsh_cd(char **);
int lsh_help(char **);
int lsh_exit(char **);
int lsh_num_builtins(void);
int lsh_num_problems(void);
int lsh_launch(char **, int);
int lsh_execute(char **, int);
void printdir(void);
int checkpipe(char *);
char **cmd_arr(char *, int);

char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

char *issues[] = {
    "Only whitespace separating arguments, no quoting or backslash escaping.",
    "No piping or redirection.",
    "Few standard builtins."
};

int (*builtin_func[]) (char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit
}; //arr of pointers to functions that return (int) and take in a (char **)

char *cwd = "/";

int main(int argc, char **argv) {
    //load config files

    lsh_loop();

    //run shutdown/cleanup

    return EXIT_SUCCESS;
}


//What a shell does: read, seperate/parse, execute
void lsh_loop(void) {
    char *line;
    char **args, **ca;
    int status, numpipes;

    do {
        printdir();
        printf("> "); //a prompt
        line = lsh_read_line();
        numpipes = checkpipe(line);
        ca = cmd_arr(line, numpipes);

        args = lsh_split_line(ca[0]);
        status = lsh_execute(args, numpipes);

        free(line);
        free(args);
    } while (status);
}
//line is a string, args is a string arr -> status is a value of 1 if successful?

//mem allocation: start with a block and then if it is exceeded, then you reallocate
char *lsh_read_line(void) {
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0; //not char *bufp? -> makes it easier to reallocate, and index buffer
    char *buffer = malloc(/*sizeof(char) **/ bufsize); //where the line is stored
    /*malloc returns a pointer to beginning of the block of n space, that pointer is going to char *buffer
    it returns NULL (#define NULL 0) if there is an issue*/
    //sizeof(char) is 1, so multiplication by it is not needed
    int c;

    if (!buffer) /*if buffer is NULL, then !0 is 1 (or !False is True) and this will run*/ {
        fprintf(stderr, "lsh: line allocation error\n");
        exit(EXIT_FAILURE); //ends the ENTIRE prgm instead of just the function- like return does
    }

    while (1) {
        //c = getchar();

        if ((c = getchar()) /*c*/ == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else
            buffer[position] = c;
        position++;

        //if buffer is exceeded- reallocate
        if (position >= bufsize) {
            bufsize += LSH_RL_BUFSIZE; //doubling it
            buffer = realloc(buffer, bufsize * sizeof(char));
            /*realloc can grow, shrink, move, and preserve a block of memory, it returns the new memory address (if needed)
            to the malloc() pointer: buffer*/
            if (!buffer) /*error check again*/ {
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

    int i;
    for (i = 0; cmdarr[i] != NULL; i++);

    if (i != (numpipes +1)) {
        printf("CMD_ARR ISSUE:\nnumpipes = %d\ncmdarr length/i = %d\ncmdarr = ", numpipes, i);
        for (int i = 0; cmdarr[i] != NULL; i++)
            printf("%s", cmdarr[i]);
        printf("\n");
        exit(EXIT_FAILURE);
    } /*else {
        printf("CMD_ARR NON-ISSUE:\nnumpipes = %d\ncmdarr length/i = %d\ncmdarr = ", numpipes, i);
        for (int i = 0; cmdarr[i] != NULL; i++)
            printf("%s (next element)", cmdarr[i]);
        printf("\n");
        exit(EXIT_SUCCESS);
    }*/

    return cmdarr;
}

/*parse through the line, however there is no quoting or backslash escaping from the cl
only whitespace will seperate the arguments*/
char **lsh_split_line(char *line) {
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    /*tokens is a string (char *) array, and it is taking mem space for the char *'s*/
    char *token;

    if (!tokens) { //error check
        fprintf(stderr, "lsh: token allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    /*strtok either returns a pointer to the next token in the string, or a NULL pointer
    it saves the current position with a static pointer, so subsequent calls using the same
    string will use NULL as the first argument rather than reusing the string*/
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) { //same reallocation as lsh_read_line()
            bufsize += LSH_TOK_BUFSIZE; //doubling it
            tokens = realloc(tokens, bufsize * sizeof(char *));
            /*realloc can grow, shrink, move, and preserve a block of memory, it returns the new memory address (if needed)
            to the malloc() pointer: buffer*/
            if (!tokens) /*error check again*/ {
                fprintf(stderr, "lsh: token re-allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }

    tokens[position] = NULL; //not '\0'? -> array of pointers instead of array of characters, last pointer in the ptr array is NULL
    return tokens;
}
//now there is an array of tokens ready to launch/execute

int checkpipe(char *line) {
    //0 if no pipe, 1 if one pipe, 2+ if multiple pipes
    int i, k = 0;
    for (i = 0; line[i] != '\0'; i++)
        if (line[i] == '|')
            k++;
    return k;
}

int lsh_launch(char **args, int numpipe) {
    pid_t pid, wpid;
    int status;

    if (!numpipe) { //numpipe == 0
        pid = fork(); //lsh_launch is copied, and the PID of the child process is returned to pid, (in the child, the pid is 0)
        /*(pid == 0) you are the child
        (pid > 0) you are the parent, and you know the childs PID
        (pid < 0) fork() failed and no child was created*/

        if (pid == 0) {
            if (execvp(args[0], args) == -1)
                //exec vp runs args[0] by letting the OS search the PATH, passes the arg list to the new progrm, it returns -1 if theres an error
                //execvp is also typically not supposed to return any value 
                perror("lsh"); /*perror prints the systems error message
                ex.) lsh: [error message]*/
            exit(EXIT_FAILURE); //if the child process is still in this process, then it failed
            //if it succeeded then 136+ will never be read
        } else if (pid < 0)
            //forking error
            perror("lsh");
        else {
            //parent process
            do {
                wpid = waitpid(pid, &status, WUNTRACED); //suspends the parent process until the child (found with the pid - first arg) is done
                //status has info on HOW the child changed state
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            /*the macros used are provided with waitpid(), so they are used to wait until either
            the processes are exited or killed*/
            /*WIFEXITED(status) is true if the child has exited normally
            WIFSIGNALED(status) is true if the child was terminated by a signal
            therefore: the loop continues so long as the child has NOT exited and NOT signaled to terminate*/
        }

        //return 1;
        /*signal to the claling function that we should prompt for input again*/
    } else if (numpipe == 1) {
        //code for one pipe
        printf("%d pipe (one pipe)\n", numpipe);
    } else if (numpipe >= 2) {
        //code for pipe pipes
        printf("%d pipes (many pipes)\n", numpipe);
    }

    return 1;
}

int lsh_num_builtins(void) {
    return sizeof(builtin_str) / sizeof(char *);
} //returns the number of builtin fucntions- meaning we can change the amount if wanted

int lsh_num_problems(void) {
    return sizeof(issues) / sizeof(char*);
}

int lsh_execute(char **args, int numpipes) {
    if (args[0] == NULL)
        return 0;
    
    for (int i = 0; i < lsh_num_builtins(); i++)
        if (strcmp(args[0], builtin_str[i]) == 0)
            return (*builtin_func[i]) (args); /*return, ends function
            (*builtin_func[i]) (args) runs ith function in builtin_func[] with (args) 
            as the arguments passed to the function*/

    return lsh_launch(args, numpipes); //if it doesnt match a builtin function it'll check the os
}

void printdir(void) {
    int bufsize = LSH_RL_BUFSIZE;
    char *cwd = NULL;
    for (;;) {
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
    printf("%s", cwd);
}

/*Builtin function implementations*/

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
