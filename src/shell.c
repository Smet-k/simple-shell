#define _GNU_SOURCE
#include "shell.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// TBD - (echo hello && echo world) >> test.txt 

#define MAX_TOKENS 1000
#define MAX_COMMANDS 100

typedef struct {
    char* output_file;
    int append;
} output_t;

typedef struct {
    char** tokens;
    int status;

    output_t custom_output;
} cmd_t;

typedef struct {
    cmd_t cmds[MAX_COMMANDS];
    int count;
} pipeline_t;

typedef struct {
    pipeline_t pipelines[MAX_COMMANDS];
    int count;
} sequence_t;

static int shell_cd(cmd_t* cmd, char** current_dir);
static int get_sequence(char* str, sequence_t* sequence);
static int execute_sequence(sequence_t* sequence, char** current_dir);
static void init_cmd(cmd_t* cmd);

int parse_cmd(char* line, char** current_dir) {
    int count = 0;
    sequence_t sequence = {0};
    char** tokens = calloc(sizeof(char), MAX_TOKENS);
    if (get_sequence(line, &sequence) < 0)
        return -1;

    execute_sequence(&sequence, current_dir);
    return 0;
}

static int execute_sequence(sequence_t* sequence, char** current_dir) {
    for (int si = 0; si < sequence->count; si++) {
        pipeline_t* pl = &sequence->pipelines[si];
        int prev_read = -1;
        for (int pi = 0; pi < pl->count; pi++) {
            cmd_t* cmd = &pl->cmds[pi];

            if (strcmp(*cmd->tokens, "cd") == 0) {
                shell_cd(cmd, current_dir);
                return 0;
            } else if (strcmp(*cmd->tokens, "exit") == 0)
                exit(0);

            int fd[2];

            if (pi < pl->count - 1) {
                if (pipe(fd) < 0) {
                    perror("pipe");
                    exit(1);
                }
            }
            pid_t pid = fork();

            if (!pid) {
                
                if(prev_read != -1) {
                    dup2(prev_read, STDIN_FILENO);
                    close(prev_read);
                }

                if(pi < pl->count - 1) {
                    dup2(fd[1], STDOUT_FILENO);
                    close(fd[0]);
                    close(fd[1]);
                }

                if (cmd->custom_output.output_file) {
                    int flags = O_WRONLY | O_CREAT;
                    if (cmd->custom_output.append)
                        flags |= O_APPEND;
                    else
                        flags |= O_TRUNC;

                    int outfd = open(cmd->custom_output.output_file, flags, 0644);
                    if (dup2(outfd, STDOUT_FILENO) < 0) {
                        perror("dup2");
                        exit(1);
                    }

                    if (dup2(outfd, STDERR_FILENO) < 0) {
                        perror("dup2");
                        exit(1);
                    }

                    close(outfd);
                }

                signal(SIGINT, SIG_DFL);
                execvp(*cmd->tokens, cmd->tokens);
                fprintf(stderr, "simple_shell: %s: command not found\n", *cmd->tokens);
                exit(127);
            }
            
            if(prev_read != -1)
                close(prev_read);
            
            if(pi < pl->count - 1) {
                close(fd[1]);
                prev_read = fd[0];
            }

        }
        for (int i = 0; i < pl->count; i++)
            wait(NULL);
    }
    return 0;
}

static int get_sequence(char* str, sequence_t* sequence) {
    char* line = strdup(str);
    sequence->count = 0;

    pipeline_t* pl = &sequence->pipelines[0];
    pl->count = 0;

    cmd_t cmd;
    init_cmd(&cmd);
    int i = 0;

    char* token = strtok(line, " ");

    while (token) {
        if (strcmp(token, ">") == 0 || strcmp(token, ">>") == 0) {
            const int is_append = (strcmp(token, ">>") == 0);

            token = strtok(NULL, " ");
            if (!token) {
                printf("Syntax error: no file after redirection\n");
                return -1;
            }

            cmd.custom_output.output_file = strdup(token);
            cmd.custom_output.append = is_append;

            token = strtok(NULL, " ");
        } else if (strcmp(token, "&&") == 0) {
            cmd.tokens[i] = NULL;
            pl->cmds[pl->count++] = cmd;

            sequence->count++;
            pl = &sequence->pipelines[sequence->count++];
            pl->count = 0;

            init_cmd(&cmd);
            i = 0;
        } else if (strcmp(token, "|") == 0) {
            cmd.tokens[i] = NULL;
            pl->cmds[pl->count++] = cmd;

            init_cmd(&cmd);
            i = 0;
        } else {
            cmd.tokens[i++] = token;
        }

        token = strtok(NULL, " ");
    }
    cmd.tokens[i] = NULL;
    pl->cmds[pl->count++] = cmd;
    sequence->count++;
    return 0;
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

static void init_cmd(cmd_t* cmd) {
    *cmd = (cmd_t){0};
    cmd->tokens = calloc(MAX_TOKENS, sizeof(char*));
    cmd->custom_output.output_file = NULL;
    cmd->custom_output.append = 0;
}
