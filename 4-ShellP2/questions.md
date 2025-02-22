1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**: We use `fork/execvp` instaed of just calling `execvp` because fork system call creates a separate child process that executes the new program independently of the parent shell, if we tried to call execvp directly in the parent process, the shell itself would be replaced by the new program, terminating the shell, by using fork, the parent shell stays active and waits for the child process to complete, allowing the user to continue issuing commands after the executed program finishes, thanks to this we also get things like process control, background execution, and handling multiple concurrent processes

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**:  If `fork()` fails it would mean that the system has run out of process resources like process table slots or memry, when `fork()` fails, it returns -1, indicating an error, in my implementation, I check the return value of `fork()` and handle this scenario by printing an error message using perror("fork"), then returning an error code to indicate that process creation failed, this prevents the shell from attempting to execute commands in an invalid state and let's the user understand what's wrong

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**: `execvp()` searches for the specified command by looking through the directories listed in the PATH environment variable, the PATH variable contains a colon separated list of directories where executable programs are kept, such as `/bin` `/usr/bin` `/usr/local/bin`, when `execvp()` is called, it iterates through these directories and tries to locate the requested command, if the command is found, it replaces the current process image with the new program, otherwise `execvp()` returns an error indicating that the command was not found

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**: The `wait()` function is used in the parent process to pause execution until the child process finishes, to ensure that the shell does not immediately return to the prompt while a command is still running, and if `wait()` is not called, the child process would become a zombie process after termination, taking up system resources until the parent kills it, plus, without `wait()`, the shell could spawn multiple child processes without managing them properly, which means possible unpredictable behavior such as orphaned processes or race conditions

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**: `WEXITSTATUS()` extracts the exit code of a terminated child process from the status returned by `wait()`, this exit code indicates whether the command executed successfully or encountered an error, in a shell, capturing and understanding this exit status gets up features like error handling, conditional execution, and the ability to get the last command’s status using `$?` in other more advanced shells, so without `WEXITSTATUS()`, the shell would have no way of determining whether a command ran successfully or failed

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**: My implementation of build_cmd_buff() ensures that quoted arguments are treated as a single entity rather than being split by spaces, it detects double quotes (" ") and keeps spaces within them while still separating unquoted words, this is necessary because many shell commands require multi word arguments, such as `echo` "Hello World" or `grep` "error message" logfile.txt, so without handling quotes correctly, commands would be incorrectly tokenized, leading to unintended behavior or syntax errors

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**: The main change was moving from a pipeline based command list to a single command structure, making the parsing logic simpler but still working well, additionally, I implemented handling for quoted arguments and ensured that unnecessary spaces were removed while preserving spacing inside quotes, one unexpected challenge was refactoring how arguments were stored—previously, commands were split using simple strtok() calls, but this had to be adapted to account for quoted input

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**: Signals in Linux are used for asynchronous process control, like allowing processes to respond to events such as termination requests, interrupts, or alarms, but unlike other forms of IPC (pipes, message queues, or shared memory etc.), signals do not transfer data but instead act as notifications that a particular event has occurred, so for example, a process receiving SIGTERM is requested to terminate gracefully, whereas SIGKILL forcibly ends it, signals are lightweight and system-wide, making them suitable for process management and handling exceptional conditions

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**: SIGKILL (signal 9) immediately terminates a process without allowing it to clean up, it cannot be caught or ignored, making it useful for forcibly stopping unresponsive or malicious processes 
    SIGTERM (signal 15) is the default termination signal and allows a process to exit gracefully, processes receiving SIGTERM can clean up resources before shutting down
    SIGINT (signal 2) is sent when the user presses Ctrl+C in a terminal, allowing the process to handle or ignore the interrupt instead of terminating immediately most shells ignore SIGINT to prevent accidental exits

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**: When a procses receives SIGSTOP (signal 19), it is paused (suspended) by the system and stops executing until it is resumed with SIGCONT, Unlike SIGINT, SIGSTOP cannot be caught, ignored, or blocked because it is meant to provide a surefire way to stop processes, this is useful for debugging (like suspending a process with Ctrl+Z) or system-level process control (like stopping tasks before resuming them later)
