#define _GNU_SOURCE
#include "shell.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_TOKENS 1000
#define MAX_COMMANDS 100

typedef struct {
    char** tokens;
    int status;
    pid_t cmd_process; 
} cmd_t;

static int shell_cd(cmd_t* cmd, char** current_dir);
static int get_tokens(char* str, cmd_t* cmd, int* cmd_count);
static int execute_cmds(cmd_t* cmds, int count, char** current_dir);

int parse_cmd(char* line, char** current_dir) {
    int count = 0;
    cmd_t cmd[MAX_COMMANDS];
    char** tokens = calloc(sizeof(char), MAX_TOKENS);
    if(!get_tokens(line, cmd, &count))
        return -1;

    execute_cmds(cmd, count, current_dir);
        
    return 0;
}

static int execute_cmds(cmd_t* cmds, int count, char** current_dir){
    for(int i = 0;i < count;i++){
        if (strcmp(*cmds[i].tokens, "cd") == 0) {
            shell_cd(&cmds[i], current_dir);
            return 0;
        } else if (strcmp(*cmds[i].tokens, "exit") == 0) {
            exit(0);
        }

        pid_t pid = fork();
        cmds[i].cmd_process = pid;
        if(pid < 0) {
            perror("fork");
            return -1;
        }
        if (!pid) {
            signal(SIGINT, SIG_DFL);
            execvp(*cmds[i].tokens, cmds[i].tokens);
            fprintf(stderr, "simple_shell: %s: command not found\n", *cmds[i].tokens);
            return -1;
        } else {
            waitpid(cmds[i].cmd_process, &cmds[i].status, 0);
            if(cmds[i].status != 0) return -1;
        }
    }
    return 0;
}

static int get_tokens(char* str, cmd_t* cmd, int* cmd_count) {
    char* line = strdup(str);
    *cmd_count = 0;
    int i = 0;

    char* token = strtok(line, " ");
    cmd[*cmd_count].tokens = calloc(sizeof(char), MAX_TOKENS);
    while (token) {
        // Think of a better check, maybe move it to a function
        if(strcmp(token, "&&") == 0){
            cmd[(*cmd_count)++].tokens[i] = NULL;
            cmd[*cmd_count].tokens = calloc(sizeof(char), MAX_TOKENS);
            token = strtok(NULL, " ");
            i = 0;
            continue;
        } else if(strcmp(token, ">") == 0)
            printf("Output redirection\n");
        else if(strcmp(token, ">>") == 0)
            printf("Output redirection append\n");
        else if(strcmp(token, "||") == 0)
            printf("Pipe\n");

        cmd[*cmd_count].tokens[i++] = token;
        token = strtok(NULL, " ");
    }
    cmd[(*cmd_count)++].tokens[i] = NULL;
    return i;
}

static int shell_cd(cmd_t* cmd, char** current_dir) {
    if (cmd->tokens[1] == NULL)
        cmd->tokens[1] = getenv("HOME");

    if (chdir(cmd->tokens[1]) != 0) {
        perror("cd");
        return -1;
    } else {
        free(*current_dir);
        *current_dir = getcwd(NULL, 0);
    }
    return 0;
}



