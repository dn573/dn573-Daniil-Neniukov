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
    char *cmd_ptr = cmd_line;
    int cmd_count = 0;

    while (cmd_ptr != NULL)
    {
        if (cmd_count >= CMD_MAX)
        {
            return ERR_TOO_MANY_COMMANDS;
        }

        char *pipe_pos = strchr(cmd_ptr, PIPE_CHAR);
        if (pipe_pos != NULL)
        {
            *pipe_pos = '\0';  
        }

        cmd_ptr = trim_spaces(cmd_ptr);
        if (strlen(cmd_ptr) == 0)
        {
            cmd_ptr = (pipe_pos != NULL) ? pipe_pos + 1 : NULL;
            continue;
        }

        char *exe = strtok(cmd_ptr, " ");
        if (exe == NULL || strlen(exe) >= EXE_MAX)
        {
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        strcpy(clist->commands[cmd_count].exe, exe);

        char *arg = strtok(NULL, "");
        if (arg != NULL)
        {
            arg = trim_spaces(arg);
            if (strlen(arg) >= ARG_MAX)
            {
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            strcpy(clist->commands[cmd_count].args, arg);
        }

        cmd_count++;
        cmd_ptr = (pipe_pos != NULL) ? pipe_pos + 1 : NULL;
    }

    if (cmd_count == 0)
    {
        return WARN_NO_CMDS;
    }

    clist->num = cmd_count;
    return OK;
}

