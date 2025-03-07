#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include "dshlib.h"

/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the 
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 * 
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 * 
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 * 
 *   Also, use the constants in the dshlib.h in this code.  
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 * 
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *   
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
 */

static int last_return_code = 0;

static char *trim_spaces(char *str) {
    while (isspace((unsigned char)*str)) {
        str++;
    }
    if (*str == 0) return str; 
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    end[1] = '\0';
    return str;
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    memset(cmd_buff, 0, sizeof(cmd_buff_t));
    
    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (!cmd_buff->_cmd_buffer) {
        return ERR_MEMORY;
    }

    char *cmd_ptr = cmd_buff->_cmd_buffer;
    while (isspace((unsigned char)*cmd_ptr)) cmd_ptr++;
    if (*cmd_ptr == '\0') {
        return WARN_NO_CMDS;
    }

    bool in_quotes = false;
    char *token_start = cmd_ptr;

    for (int i = 0; cmd_ptr[i] != '\0'; i++) {
        if (cmd_ptr[i] == '"') {
            in_quotes = !in_quotes;
            memmove(&cmd_ptr[i], &cmd_ptr[i + 1], strlen(&cmd_ptr[i]));
            i--;
        }
        else if (!in_quotes && isspace((unsigned char)cmd_ptr[i])) {
            cmd_ptr[i] = '\0';
            cmd_buff->argv[cmd_buff->argc++] = token_start;
            token_start = &cmd_ptr[i + 1];
            while (isspace((unsigned char)*token_start)) {
                token_start++;
            }
            i = token_start - cmd_ptr - 1;
        }
    }
    if (*token_start != '\0') {
        cmd_buff->argv[cmd_buff->argc++] = token_start;
    }
    cmd_buff->argv[cmd_buff->argc] = NULL;
    return OK;
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    memset(clist, 0, sizeof(*clist));
    
    char *segment = strtok(cmd_line, "|");
    int cmd_count = 0;
    
    while (segment) {
        if (cmd_count >= CMD_MAX) {
            return ERR_TOO_MANY_COMMANDS;
        }
        segment = trim_spaces(segment);
        if (strlen(segment) == 0) {
            return WARN_NO_CMDS;
        }
        int rc = build_cmd_buff(segment, &clist->commands[cmd_count]);
        if (rc != OK && rc != WARN_NO_CMDS) {
            return rc;
        }
        
        cmd_count++;
        segment = strtok(NULL, "|");
    }
    
    clist->num = cmd_count;
    return OK;
}


Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (cmd->argc == 0) {
        return BI_NOT_BI;
    }

    if (strcmp(cmd->argv[0], "exit") == 0) {
        exit(0); 
    }

    if (strcmp(cmd->argv[0], "rc") == 0) {
        printf("%d\n", last_return_code);
        fflush(stdout);
        return BI_EXECUTED; 
    }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        if (cmd->argc < 2) {
            fprintf(stderr, "cd: missing argument\n");
            last_return_code = 1; 
            return BI_EXECUTED;
        }
        if (chdir(cmd->argv[1]) != 0) {
            fprintf(stderr, "cd: No such file or directory\n");
            last_return_code = 1;
            return BI_EXECUTED;
        }
        
        last_return_code = 0;
        return BI_EXECUTED;
    }

    return BI_NOT_BI;
}

static void handle_redirection(cmd_buff_t *cmd) {
    for (int i = 0; i < cmd->argc; i++) {
        if (!cmd->argv[i]) continue;

        if (strcmp(cmd->argv[i], "<") == 0 && i+1 < cmd->argc) {
            int fd = open(cmd->argv[i+1], O_RDONLY);
            if (fd == -1) {
                perror("open input file");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            cmd->argv[i]   = NULL;
            cmd->argv[i+1] = NULL;
        }
        else if (strcmp(cmd->argv[i], ">") == 0 && i+1 < cmd->argc) {
            int fd = open(cmd->argv[i+1], O_WRONLY|O_CREAT|O_TRUNC, 0644);
            if (fd == -1) {
                perror("open output file");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            cmd->argv[i]   = NULL;
            cmd->argv[i+1] = NULL;
        }
        else if (strcmp(cmd->argv[i], ">>") == 0 && i+1 < cmd->argc) {
            int fd = open(cmd->argv[i+1], O_WRONLY|O_CREAT|O_APPEND, 0644);
            if (fd == -1) {
                perror("open append file");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            cmd->argv[i]   = NULL;
            cmd->argv[i+1] = NULL;
        }
    }
}

int execute_pipeline(command_list_t *clist) {
    int num_cmds = clist->num;
    if (num_cmds <= 0) return 0;

    if (num_cmds == 1) {
        cmd_buff_t *single = &clist->commands[0];
        Built_In_Cmds ret = exec_built_in_cmd(single);

        if (ret == BI_NOT_BI) {
            pid_t pid = fork();
            if (pid == 0) {
                handle_redirection(single);
                execvp(single->argv[0], single->argv);
                fprintf(stderr, "Command not found in PATH: %s\n", single->argv[0]);
                exit(2); 
            } else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
                last_return_code = WEXITSTATUS(status);
                return last_return_code;
            } else {
                perror("fork");
                last_return_code = 1;
                return 1;
            }
        }
        else if (ret == BI_CMD_EXIT) {
            exit(0);
        }
        else if (ret == BI_CMD_CD) {
        }
        else if (ret == BI_EXECUTED) {
            return last_return_code;
        }
        return last_return_code;
    }

    int pipes[CMD_MAX - 1][2];
    pid_t pids[CMD_MAX];

    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }

    for (int i = 0; i < num_cmds; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            if (i < num_cmds - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            for (int j = 0; j < num_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            handle_redirection(&clist->commands[i]);
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            fprintf(stderr, "Command not found in PATH: %s\n", clist->commands[i].argv[0]);
            exit(2);
        }
    }

    for (int i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < num_cmds; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        last_return_code = WEXITSTATUS(status);
    }
    return last_return_code;
}

int exec_local_cmd_loop() {
    char input_buffer[SH_CMD_MAX];
    command_list_t clist;

    while (1) {
        printf("%s", SH_PROMPT);
        if (fgets(input_buffer, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        input_buffer[strcspn(input_buffer, "\n")] = '\0';

        if (build_cmd_list(input_buffer, &clist) != OK) {
            continue;
        }

        execute_pipeline(&clist);
    }

    return OK;
}
