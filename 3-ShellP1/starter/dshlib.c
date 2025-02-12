#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dshlib.h"

char *trim_spaces(char *str)
{
    while (isspace((unsigned char)*str)) str++;  
    if (*str == 0) return str;                   

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;  
    end[1] = '\0';  
    return str;
}

int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    memset(clist, 0, sizeof(command_list_t));  
    char *cmd_ptr = strtok(cmd_line, PIPE_STRING);
    int cmd_count = 0;

    while (cmd_ptr != NULL)
    {
        if (cmd_count >= CMD_MAX)
        {
            return ERR_TOO_MANY_COMMANDS;
        }

        cmd_ptr = trim_spaces(cmd_ptr);

        
        char *exe = strtok(cmd_ptr, " ");
        if (exe == NULL)
        {
            return WARN_NO_CMDS;
        }
        if (strlen(exe) >= EXE_MAX)
        {
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        strcpy(clist->commands[cmd_count].exe, exe);

        
        char *arg = strtok(NULL, "");
        if (arg != NULL)
        {
            if (strlen(arg) >= ARG_MAX)
            {
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            strcpy(clist->commands[cmd_count].args, arg);
        }

        cmd_count++;
        cmd_ptr = strtok(NULL, PIPE_STRING);
    }

    clist->num = cmd_count;
    return OK;
}

