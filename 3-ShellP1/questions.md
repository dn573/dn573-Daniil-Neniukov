1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**: it's a good choice because it natively reads a line from standard input, stopping at the newline characters or end of file, and it works perfectly with shell's requirement to work on a line by line basis. Also we can avoid buffer overflow specifying how many characters can be read.

2. You needed to use `malloc()` to allocte memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**:  we needed `malloc()` because we want flexibility in working with variable-length commands rather than relying on a fixed-size buffer that could be too small or really large for typical input. Allocating memory at runtime allows the shell to handle potentially long commands without buffer overruns, and as a bonus we get more precise control over memory usage if the shell’s requirements grow or change over time (in future parts)


3. In `dshlib.c`, the function `build_cmd_list(`)` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**: Trimming leading and trailing spaces is necessary becasue we want to ensure that each command and its arguments are interpreted correctly, without accidental whitespace that could cause extra empty tokens, mismatched command names, or off byy one errors in argument parsing. If we did not trim spaces, executing a command that the user typed with preceding or trailing spaces might be misidentified potentially leading to commands that cannot be found or arguments that are improperly parsed. 

4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**: three examples that we should implement are: output redirection using the `>` operator, input redirection using the `<` operator, and append redirection using the `>>` operator. They allow a user to route the standard output or standard input of a process to or from files, or append to existing files without overwriting them. Implementing these involves challenges like: correctly duplicating file descriptors in the shell’s child processes, deciding when and how to restore original descriptors later, and correctly handling file-open errors or permission issues that might happen during the redirection process.

- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**: The key difference between redirection and piping is that redirection involves sending a command’s input or output to or from a file, while piping links the standard output of one command directly to the standard input of another. Redirection "isolates" data to or from the filesystem and is very useful for saving output permanently or reading in scripted input, but piping is used to chain commannds together so one command’s output dynamically feeds into the next command’s input without intermediate files.

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**: It is important to keep `STDERR` separate from `STDOUT` because they serve different purposes: STDOUT is intended for normal output, while STDERR is intended for error messages, I know this much is obvious from their names, but let me elaborate. By separating them, a user can redirect or capture a command’s output independently of any errors it produces. This separation is useful for logging errors separately, debugging issues more effectively, and for preventing error messages from mixing into a file or stream meant to hold ONLY regular output.

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**: Our custom shell should handle errors from commands that fail by checking the exit status once the child process completes. If a command fails, then the shell can simply capture and report that failure status so a user knows something went wrong. We should also allow the user to merge STDERR with STDOUT through redirection if they want so, for instance by letting them write something like `2>&1` so that errors become part of the same stream as normal output. By default, though, we keep them separate and leave it to the user’s needs whether or not to combine them.
