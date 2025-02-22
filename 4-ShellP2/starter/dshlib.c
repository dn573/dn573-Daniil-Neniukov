#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"
#include <errno.h>

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

int last_return_code = 0;

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    memset(cmd_buff, 0, sizeof(cmd_buff_t));

    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (cmd_buff->_cmd_buffer == NULL)
    {
        return ERR_MEMORY;
    }

    char *cmd_ptr = cmd_buff->_cmd_buffer;
    while (isspace((unsigned char)*cmd_ptr)) cmd_ptr++;
    if (*cmd_ptr == '\0') return WARN_NO_CMDS;

    bool in_quotes = false;
    char *token_start = cmd_ptr;
    for (int i = 0; cmd_ptr[i] != '\0'; i++)
    {
        if (cmd_ptr[i] == '"') 
        {
            in_quotes = !in_quotes;
            memmove(&cmd_ptr[i], &cmd_ptr[i + 1], strlen(&cmd_ptr[i]));
            i--;
        }
        else if (!in_quotes && isspace(cmd_ptr[i]))
        {
            cmd_ptr[i] = '\0';
            cmd_buff->argv[cmd_buff->argc++] = token_start;
            token_start = &cmd_ptr[i + 1];

            while (isspace((unsigned char)*token_start))
                token_start++;
            
            i = token_start - cmd_ptr - 1;
        }
    }

    if (*token_start != '\0')
    {
        cmd_buff->argv[cmd_buff->argc++] = token_start;
    }

    cmd_buff->argv[cmd_buff->argc] = NULL;
    return OK;
}

int exec_cmd(cmd_buff_t *cmd)
{
    if (cmd->argc == 0)
    {
        last_return_code = WARN_NO_CMDS;
        return WARN_NO_CMDS;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        execvp(cmd->argv[0], cmd->argv);
        
        switch (errno)
        {
            case ENOENT:
                fprintf(stderr, "Command not found in PATH\n");
                exit(2);
            case EACCES:
                fprintf(stderr, "Permission denied: %s\n", cmd->argv[0]);
                exit(126);
            default:
                fprintf(stderr, "Execution failed: %s\n", cmd->argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
        last_return_code = WEXITSTATUS(status);
        return last_return_code;
    }
    else
    {
        perror("fork");
        last_return_code = ERR_EXEC_CMD;
        return ERR_EXEC_CMD;
    }
}

int exec_local_cmd_loop()
{
    char input_buffer[SH_CMD_MAX]; 
    cmd_buff_t cmd;

    while (1)
    {
        printf("%s", SH_PROMPT);
        if (fgets(input_buffer, SH_CMD_MAX, stdin) == NULL)
        {
            printf("\n");
            break;
        }

        input_buffer[strcspn(input_buffer, "\n")] = '\0';

        if (strlen(input_buffer) == 0)
        {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        }

        if (build_cmd_buff(input_buffer, &cmd) != OK)
        {
            continue;
        }

        Built_In_Cmds bi_result = exec_built_in_cmd(&cmd);
        if (bi_result == BI_CMD_EXIT)
        {
            break;
        }
        if (bi_result == BI_EXECUTED)
        {
            continue;
        }

    exec_cmd(&cmd);}

    return OK;
}

Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    if (cmd->argc == 0)
    {
        return BI_NOT_BI;
    }

    if (strcmp(cmd->argv[0], "exit") == 0) 
    {
        return BI_CMD_EXIT;
    }

    if (strcmp(cmd->argv[0], "rc") == 0)
    {
        printf("%d\n", last_return_code);
        return BI_EXECUTED;
    }


    if (strcmp(cmd->argv[0], "cd") == 0)
    {
        if (cmd->argc == 1)
        {
            return BI_EXECUTED;
        }
        if (chdir(cmd->argv[1]) != 0)
        {
            perror("cd");
            return ERR_CMD_ARGS_BAD;
        }
        return BI_EXECUTED;
    }

    return BI_NOT_BI;
}

