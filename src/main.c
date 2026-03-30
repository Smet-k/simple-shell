#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "shell.h"
#include <errno.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>


static char *current_dir;

void sigint_handler(int signum);

int main(){
    char *line = NULL;

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    chdir(getenv("HOME"));
    current_dir = getcwd(NULL, 0);

    read_history(".simple_shell_history");
    while(1){
        char prompt[512];
        snprintf(prompt, sizeof(prompt), "[%s] simple_shell> ", current_dir);

        
        fflush(stdout);
        line = readline(prompt);

        if (!line) {
            printf("\n");
            break;
        }

        if(line[0] == '\0')
            continue;
        
        add_history(line);

        if(parse_cmd(line, &current_dir) < 0){
            free(line);
            return -1;
        }
    }
    write_history(".simple_shell_history");

    free(line);
    return 0;
}

void sigint_handler(int signum)
{
    write(STDOUT_FILENO, "\n", 1);
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
}