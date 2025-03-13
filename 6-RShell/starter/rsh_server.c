
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>

//INCLUDES for extra credit
#include <signal.h>
#include <pthread.h>
//-------------------------

#include "dshlib.h"
#include "rshlib.h"

volatile sig_atomic_t server_running = 1;
int server_socket_fd = -1;

static int server_socket_closed = 0;

static void remove_single_quotes(char *str) {
    char *dst = str;
    while (*str) {
        if (*str != '\'') {
            *dst++ = *str;
        }
        str++;
    }
    *dst = '\0';
}

void sigint_handler(int signum) {
    (void)signum;
    server_running = 0;
    if (server_socket_fd >= 0 && !server_socket_closed) {
        server_socket_closed = 1;
        shutdown(server_socket_fd, SHUT_RDWR);
        close(server_socket_fd);
    }
    exit(0);
}

void sigchld_handler(int signum) {
    (void)signum;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}

typedef struct {
    int cli_socket;
} client_thread_args_t;

void *handle_client(void *arg) {
    client_thread_args_t *args = (client_thread_args_t *)arg;
    int cli_sock = args->cli_socket;
    free(args);

    int rc = exec_client_requests(cli_sock);

    close(cli_sock);

    if (rc == OK_EXIT) {
        server_running = 0;
        if (server_socket_fd >= 0 && !server_socket_closed) {
            server_socket_closed = 1;
            shutdown(server_socket_fd, SHUT_RDWR);
            close(server_socket_fd);
        }
    }

    pthread_exit(NULL);
}



/*
 * start_server(ifaces, port, is_threaded)
 *      ifaces:  a string in ip address format, indicating the interface
 *              where the server will bind.  In almost all cases it will
 *              be the default "0.0.0.0" which binds to all interfaces.
 *              note the constant RDSH_DEF_SVR_INTFACE in rshlib.h
 * 
 *      port:   The port the server will use.  Note the constant 
 *              RDSH_DEF_PORT which is 1234 in rshlib.h.  If you are using
 *              tux you may need to change this to your own default, or even
 *              better use the command line override -s implemented in dsh_cli.c
 *              For example ./dsh -s 0.0.0.0:5678 where 5678 is the new port  
 * 
 *      is_threded:  Used for extra credit to indicate the server should implement
 *                   per thread connections for clients  
 * 
 *      This function basically runs the server by: 
 *          1. Booting up the server
 *          2. Processing client requests until the client requests the
 *             server to stop by running the `stop-server` command
 *          3. Stopping the server. 
 * 
 *      This function is fully implemented for you and should not require
 *      any changes for basic functionality.  
 * 
 *      IF YOU IMPLEMENT THE MULTI-THREADED SERVER FOR EXTRA CREDIT YOU NEED
 *      TO DO SOMETHING WITH THE is_threaded ARGUMENT HOWEVER.  
 */

int start_server(char *ifaces, int port, int is_threaded) {
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

    int svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0) {
        perror("boot_server failed");
        return svr_socket;
    }
    server_socket_fd = svr_socket;

    printf("Server listening on %s:%d\n", ifaces, port);
    fflush(stdout);

    int rc = process_cli_requests(svr_socket, is_threaded);

    if (!server_socket_closed) {
        server_socket_closed = 1;
        stop_server(svr_socket);
    }
    return rc;
}

/*
 * stop_server(svr_socket)
 *      svr_socket: The socket that was created in the boot_server()
 *                  function. 
 * 
 *      This function simply returns the value of close() when closing
 *      the socket.  
 */
int stop_server(int svr_socket) {
    return close(svr_socket);
}

/*
 * boot_server(ifaces, port)
 *      ifaces & port:  see start_server for description.  They are passed
 *                      as is to this function.   
 * 
 *      This function "boots" the rsh server.  It is responsible for all
 *      socket operations prior to accepting client connections.  Specifically: 
 * 
 *      1. Create the server socket using the socket() function. 
 *      2. Calling bind to "bind" the server to the interface and port
 *      3. Calling listen to get the server ready to listen for connections.
 * 
 *      after creating the socket and prior to calling bind you might want to 
 *      include the following code:
 * 
 *      int enable=1;
 *      setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
 * 
 *      when doing development you often run into issues where you hold onto
 *      the port and then need to wait for linux to detect this issue and free
 *      the port up.  The code above tells linux to force allowing this process
 *      to use the specified port making your life a lot easier.
 * 
 *  Returns:
 * 
 *      server_socket:  Sockets are just file descriptors, if this function is
 *                      successful, it returns the server socket descriptor, 
 *                      which is just an integer.
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code is returned if the socket(),
 *                               bind(), or listen() call fails. 
 * 
 */

int boot_server(char *ifaces, int port) {
    int svr_socket;
    struct sockaddr_in addr;
    int enable = 1;

    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket < 0) {
        perror("socket creation failed");
        return ERR_RDSH_COMMUNICATION;
    }

    setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ifaces, &addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    if (bind(svr_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    if (listen(svr_socket, 20) < 0) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    return svr_socket;
}

/*
 * process_cli_requests(svr_socket)
 *      svr_socket:  The server socket that was obtained from boot_server()
 *   
 *  This function handles managing client connections.  It does this using
 *  the following logic
 * 
 *      1.  Starts a while(1) loop:
 *  
 *          a. Calls accept() to wait for a client connection. Recall that 
 *             the accept() function returns another socket specifically
 *             bound to a client connection. 
 *          b. Calls exec_client_requests() to handle executing commands
 *             sent by the client. It will use the socket returned from
 *             accept().
 *          c. Loops back to the top (step 2) to accept connecting another
 *             client.  
 * 
 *          note that the exec_client_requests() return code should be
 *          negative if the client requested the server to stop by sending
 *          the `stop-server` command.  If this is the case step 2b breaks
 *          out of the while(1) loop. 
 * 
 *      2.  After we exit the loop, we need to cleanup.  Dont forget to 
 *          free the buffer you allocated in step #1.  Then call stop_server()
 *          to close the server socket. 
 * 
 *  Returns:
 * 
 *      OK_EXIT:  When the client sends the `stop-server` command this function
 *                should return OK_EXIT. 
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code terminates the loop and is
 *                returned from this function in the case of the accept() 
 *                function failing. 
 * 
 *      OTHERS:   See exec_client_requests() for return codes.  Note that positive
 *                values will keep the loop running to accept additional client
 *                connections, and negative values terminate the server. 
 * 
 */

int process_cli_requests(int svr_socket, int is_threaded) {
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    while (server_running) {
        int cli_socket = accept(svr_socket, (struct sockaddr *)&cli_addr, &cli_len);
        if (cli_socket < 0) {
            if (!server_running) break;  
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        if (is_threaded) {
            pthread_t client_thread;
            client_thread_args_t *args = malloc(sizeof(client_thread_args_t));
            if (!args) {
                perror("malloc");
                close(cli_socket);
                continue;
            }
            args->cli_socket = cli_socket;

            if (pthread_create(&client_thread, NULL, handle_client, args) != 0) {
                perror("pthread_create");
                free(args);
                close(cli_socket);
                continue;
            }
            pthread_detach(client_thread);

        } else {
            pid_t pid = fork();
            if (pid == 0) {
                close(svr_socket);
                int rc = exec_client_requests(cli_socket);
                close(cli_socket);
                exit((unsigned char)rc);

            } else if (pid > 0) {
                close(cli_socket);
                int status;
                waitpid(pid, &status, 0);

                if (WIFEXITED(status)) {
                    int child_code = (signed char)WEXITSTATUS(status);
                    if (child_code == OK_EXIT) {
                        printf("Client requested server to stop, stopping...\n");
                        server_running = 0;
                        break;
                    }
                }
            } else {
                perror("fork");
                close(cli_socket);
            }
        }
    }

    return OK; 
}


/*
 * exec_client_requests(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client
 *   
 *  This function handles accepting remote client commands. The function will
 *  loop and continue to accept and execute client commands.  There are 2 ways
 *  that this ongoing loop accepting client commands ends:
 * 
 *      1.  When the client executes the `exit` command, this function returns
 *          to process_cli_requests() so that we can accept another client
 *          connection. 
 *      2.  When the client executes the `stop-server` command this function
 *          returns to process_cli_requests() with a return code of OK_EXIT
 *          indicating that the server should stop. 
 * 
 *  Note that this function largely follows the implementation of the
 *  exec_local_cmd_loop() function that you implemented in the last 
 *  shell program deliverable. The main difference is that the command will
 *  arrive over the recv() socket call rather than reading a string from the
 *  keyboard. 
 * 
 *  This function also must send the EOF character after a command is
 *  successfully executed to let the client know that the output from the
 *  command it sent is finished.  Use the send_message_eof() to accomplish 
 *  this. 
 * 
 *  Of final note, this function must allocate a buffer for storage to 
 *  store the data received by the client. For example:
 *     io_buff = malloc(RDSH_COMM_BUFF_SZ);
 *  And since it is allocating storage, it must also properly clean it up
 *  prior to exiting.
 * 
 *  Returns:
 * 
 *      OK:       The client sent the `exit` command.  Get ready to connect
 *                another client. 
 *      OK_EXIT:  The client sent `stop-server` command to terminate the server
 * 
 *      ERR_RDSH_COMMUNICATION:  A catch all for any socket() related send
 *                or receive errors. 
 */

int exec_client_requests(int cli_socket) {
    char io_buff[RDSH_COMM_BUFF_SZ];
    int last_return_code = 0;

    while (1) {
        int recv_size = recv(cli_socket, io_buff, RDSH_COMM_BUFF_SZ - 1, 0);
        if (recv_size <= 0) {
            break;
        }
        io_buff[recv_size] = '\0';

        if (strcmp(io_buff, "exit") == 0) {
            break;
        }
        if (strcmp(io_buff, "stop-server") == 0) {
            send_message_string(cli_socket, "Stopping server\n");
            shutdown(cli_socket, SHUT_RDWR);
            close(cli_socket);
            return OK_EXIT; 
        }
        if (strncmp(io_buff, "cd ", 3) == 0) {
            char *dir = io_buff + 3;
            if (chdir(dir) != 0) {
                send(cli_socket, "cd: No such file or directory\n", 30, 0);
                last_return_code = 1;
            } else {
                last_return_code = 0;
            }
            send_message_eof(cli_socket);
            continue;
        }
        if (strcmp(io_buff, "rc") == 0) {
            char rc_buff[16];
            snprintf(rc_buff, sizeof(rc_buff), "%d\n", last_return_code);
            send(cli_socket, rc_buff, strlen(rc_buff), 0);
            send_message_eof(cli_socket);
            continue;
        }

        command_list_t cmd_list;
        int rc_build = build_cmd_list(io_buff, &cmd_list);

        if (rc_build == OK) {
            for (int c = 0; c < cmd_list.num; c++) {
                cmd_buff_t *cb = &cmd_list.commands[c];
                for (int a = 0; a < cb->argc; a++) {
                    remove_single_quotes(cb->argv[a]);
                }
            }

            last_return_code = rsh_execute_pipeline(cli_socket, &cmd_list);

        } else if (rc_build == WARN_NO_CMDS) {
            send_message_eof(cli_socket);
            continue;

        } else {
            send(cli_socket, "Command not found in PATH\n", 26, 0);
            send_message_eof(cli_socket);
        }
    }
    return OK;
}

/*
 * send_message_eof(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client

 *  Sends the EOF character to the client to indicate that the server is
 *  finished executing the command that it sent. 
 * 
 *  Returns:
 * 
 *      OK:  The EOF character was sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the EOF character. 
 */
int send_message_eof(int cli_socket) {
    ssize_t s = send(cli_socket, &RDSH_EOF_CHAR, 1, 0);
    if (s < 0 || s == 0) {
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}


/*
 * send_message_string(cli_socket, char *buff)
 *      cli_socket:  The server-side socket that is connected to the client
 *      buff:        A C string (aka null terminated) of a message we want
 *                   to send to the client. 
 *   
 *  Sends a message to the client.  Note this command executes both a send()
 *  to send the message and a send_message_eof() to send the EOF character to
 *  the client to indicate command execution terminated. 
 * 
 *  Returns:
 * 
 *      OK:  The message in buff followed by the EOF character was 
 *           sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the message followed by the EOF character. 
 */
int send_message_string(int cli_socket, char *buff) {
    ssize_t sent = send(cli_socket, buff, strlen(buff), 0);
    if (sent < 0 || (size_t)sent < strlen(buff)) {
        return ERR_RDSH_COMMUNICATION;
    }
    return send_message_eof(cli_socket);
}


/*
 * rsh_execute_pipeline(int cli_sock, command_list_t *clist)
 *      cli_sock:    The server-side socket that is connected to the client
 *      clist:       The command_list_t structure that we implemented in
 *                   the last shell. 
 *   
 *  This function executes the command pipeline.  It should basically be a
 *  replica of the execute_pipeline() function from the last deliverable. 
 *  The only thing different is that you will be using the cli_sock as the
 *  main file descriptor on the first executable in the pipeline for STDIN,
 *  and the cli_sock for the file descriptor for STDOUT, and STDERR for the
 *  last executable in the pipeline.  See picture below:  
 * 
 *      
 *┌───────────┐                                                    ┌───────────┐
 *│ cli_sock  │                                                    │ cli_sock  │
 *└─────┬─────┘                                                    └────▲──▲───┘
 *      │   ┌──────────────┐     ┌──────────────┐     ┌──────────────┐  │  │    
 *      │   │   Process 1  │     │   Process 2  │     │   Process N  │  │  │    
 *      │   │              │     │              │     │              │  │  │    
 *      └───▶stdin   stdout├─┬──▶│stdin   stdout├─┬──▶│stdin   stdout├──┘  │    
 *          │              │ │   │              │ │   │              │     │    
 *          │        stderr├─┘   │        stderr├─┘   │        stderr├─────┘    
 *          └──────────────┘     └──────────────┘     └──────────────┘   
 *                                                      WEXITSTATUS()
 *                                                      of this last
 *                                                      process to get
 *                                                      the return code
 *                                                      for this function       
 * 
 *  Returns:
 * 
 *      EXIT_CODE:  This function returns the exit code of the last command
 *                  executed in the pipeline.  If only one command is executed
 *                  that value is returned.  Remember, use the WEXITSTATUS()
 *                  macro that we discussed during our fork/exec lecture to
 *                  get this value. 
 */

int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
    int num_cmds = clist->num;
    if (num_cmds < 1) {
        return 0;
    }

    int pipes[num_cmds - 1][2];
    pid_t pids[num_cmds];
    int status, exit_code = 0;

    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_RDSH_COMMUNICATION;
        }
    }

    for (int i = 0; i < num_cmds; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            if (i == 0) {
                dup2(cli_sock, STDIN_FILENO);
            }
            if (i == num_cmds - 1) {
                dup2(cli_sock, STDOUT_FILENO);
                dup2(cli_sock, STDERR_FILENO);
            }
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

            for (int argi = 0; argi < clist->commands[i].argc; argi++) {
                if (strcmp(clist->commands[i].argv[argi], ">") == 0) {
                    if (argi + 1 < clist->commands[i].argc) {
                        int fd = open(clist->commands[i].argv[argi + 1],
                                      O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd < 0) {
                            perror("open");
                            exit(1);
                        }
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                        clist->commands[i].argv[argi] = NULL;
                    }
                    break;
                }
            }

            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            send_message_string(cli_sock, "Command not found in PATH\n");
            perror("execvp");
            exit(127);

        } else if (pids[i] < 0) {
            perror("fork");
            return ERR_RDSH_COMMUNICATION;
        }
    }

    for (int i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], &status, 0);
        if (i == num_cmds - 1) {
            exit_code = WEXITSTATUS(status);
        }
    }

    send_message_eof(cli_sock);
    return exit_code;
}


/**************   OPTIONAL STUFF  ***************/
/****
 **** NOTE THAT THE FUNCTIONS BELOW ALIGN TO HOW WE CRAFTED THE SOLUTION
 **** TO SEE IF A COMMAND WAS BUILT IN OR NOT.  YOU CAN USE A DIFFERENT
 **** STRATEGY IF YOU WANT.  IF YOU CHOOSE TO DO SO PLEASE REMOVE THESE
 **** FUNCTIONS AND THE PROTOTYPES FROM rshlib.h
 **** 
 */

/*
 * rsh_match_command(const char *input)
 *      cli_socket:  The string command for a built-in command, e.g., dragon,
 *                   cd, exit-server
 *   
 *  This optional function accepts a command string as input and returns
 *  one of the enumerated values from the BuiltInCmds enum as output. For
 *  example:
 * 
 *      Input             Output
 *      exit              BI_CMD_EXIT
 *      dragon            BI_CMD_DRAGON
 * 
 *  This function is entirely optional to implement if you want to handle
 *  processing built-in commands differently in your implementation. 
 * 
 *  Returns:
 * 
 *      BI_CMD_*:   If the command is built-in returns one of the enumeration
 *                  options, for example "cd" returns BI_CMD_CD
 * 
 *      BI_NOT_BI:  If the command is not "built-in" the BI_NOT_BI value is
 *                  returned. 
 */
Built_In_Cmds rsh_match_command(const char *input)
{
    if (strcmp(input, "exit") == 0)
        return BI_CMD_EXIT;
    if (strcmp(input, "dragon") == 0)
        return BI_CMD_DRAGON;
    if (strcmp(input, "cd") == 0)
        return BI_CMD_CD;
    if (strcmp(input, "stop-server") == 0)
        return BI_CMD_STOP_SVR;
    if (strcmp(input, "rc") == 0)
        return BI_CMD_RC;
    return BI_NOT_BI;
}

/*
 * rsh_built_in_cmd(cmd_buff_t *cmd)
 *      cmd:  The cmd_buff_t of the command, remember, this is the 
 *            parsed version fo the command
 *   
 *  This optional function accepts a parsed cmd and then checks to see if
 *  the cmd is built in or not.  It calls rsh_match_command to see if the 
 *  cmd is built in or not.  Note that rsh_match_command returns BI_NOT_BI
 *  if the command is not built in. If the command is built in this function
 *  uses a switch statement to handle execution if appropriate.   
 * 
 *  Again, using this function is entirely optional if you are using a different
 *  strategy to handle built-in commands.  
 * 
 *  Returns:
 * 
 *      BI_NOT_BI:   Indicates that the cmd provided as input is not built
 *                   in so it should be sent to your fork/exec logic
 *      BI_EXECUTED: Indicates that this function handled the direct execution
 *                   of the command and there is nothing else to do, consider
 *                   it executed.  For example the cmd of "cd" gets the value of
 *                   BI_CMD_CD from rsh_match_command().  It then makes the libc
 *                   call to chdir(cmd->argv[1]); and finally returns BI_EXECUTED
 *      BI_CMD_*     Indicates that a built-in command was matched and the caller
 *                   is responsible for executing it.  For example if this function
 *                   returns BI_CMD_STOP_SVR the caller of this function is
 *                   responsible for stopping the server.  If BI_CMD_EXIT is returned
 *                   the caller is responsible for closing the client connection.
 * 
 *   AGAIN - THIS IS TOTALLY OPTIONAL IF YOU HAVE OR WANT TO HANDLE BUILT-IN
 *   COMMANDS DIFFERENTLY. 
 */
Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd)
{
    Built_In_Cmds ctype = BI_NOT_BI;
    ctype = rsh_match_command(cmd->argv[0]);

    switch (ctype)
    {
    // case BI_CMD_DRAGON:
    //     print_dragon();
    //     return BI_EXECUTED;
    case BI_CMD_EXIT:
        return BI_CMD_EXIT;
    case BI_CMD_STOP_SVR:
        return BI_CMD_STOP_SVR;
    case BI_CMD_RC:
        return BI_CMD_RC;
    case BI_CMD_CD:
        chdir(cmd->argv[1]);
        return BI_EXECUTED;
    default:
        return BI_NOT_BI;
    }
}
