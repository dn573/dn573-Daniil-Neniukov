1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

After forking each child process for a pipeline, the shell calls `waitpid()` on every child’s PID in a loop, this ensures the shell does not prompt for another command until all children have finished, if we skip calling `waitpid()`, orphaned processes would keep running in the background, and the shell might print a prompt too early, that can lead to unpredictable resource usage and potential zombie processes

2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?

Once `dup2()` has duplicated a file descriptor for reading or writing, the original pipe endpoints are no longer needed by that child, keeping extra pipe ends open could cause the processes to block indefinitely if they expect a pipe closure to signal EOF, it also wastes system resources and can lead to unintended behavior in inter process communication

3. Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require execvp(). Why is cd implemented as a built-in rather than an external command? What challenges would arise if cd were implemented as an external process?

The cd command changes the working directory of the running shell process itself, which cannot be done from a child process spawned by `execvp()`, if cd were external, changing the directory would only affect that child, and the shell would remain in the old directory, this would break the intended behavior of navigating the shell

4. Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?

I could dynamically allocate memory for each parsed command and store them in a resizable data structure (like a linked list or an expandable array), rather than a fixed array, the trade offs involve increased complexity in managing memory and ensuring efficient expansion, I would also have to balance simplicity against potentially higher overhead for frequent reallocation
