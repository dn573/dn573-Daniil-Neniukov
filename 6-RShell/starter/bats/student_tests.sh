#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file
#


@test "Example: check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF

    # Assertions
    [ "$status" -eq 0 ]
}


#!/usr/bin/env bats

@test "server starts and accepts client connection" {
    ./dsh -s -p 5679 & 
    server_pid=$!
    sleep 1
    OUTPUT=$(echo "echo server test" | ./dsh -c -p 5679)
    echo "stop-server" | ./dsh -c -p 5679
    sleep 1
    [[ "$OUTPUT" == *"server test"* ]]
}

@test "server echoes client input" {
    ./dsh -s -p 5680 &
    server_pid=$!
    sleep 1
    OUTPUT=$(echo "echo hello" | ./dsh -c -p 5680)
    echo "stop-server" | ./dsh -c -p 5680
    sleep 1
    [[ "$OUTPUT" == *"hello"* ]]
}

@test "client sends null-terminated commands" {
    ./dsh -s -p 5681 &
    server_pid=$!
    sleep 1
    OUTPUT=$(printf "echo test\0" | ./dsh -c -p 5681)
    echo "stop-server" | ./dsh -c -p 5681
    sleep 1
    [[ "$OUTPUT" == *"test"* ]]
}

@test "server properly handles multiple clients sequentially" {
    ./dsh -s -p 5682 &
    server_pid=$!
    sleep 1

    OUTPUT1=$(echo "echo first" | ./dsh -c -p 5682)
    [[ "$OUTPUT1" == *"first"* ]]

    OUTPUT2=$(echo "echo second" | ./dsh -c -p 5682)
    [[ "$OUTPUT2" == *"second"* ]]

    echo "stop-server" | ./dsh -c -p 5682
    sleep 1
}

@test "server handles multiple clients concurrently (multithreaded mode)" {
    ./dsh -s -p 5683 -x &
    server_pid=$!
    sleep 1

    (echo "echo client1" | ./dsh -c -p 5683) &
    (echo "echo client2" | ./dsh -c -p 5683) &
    sleep 2

    echo "stop-server" | ./dsh -c -p 5683
    sleep 1
}

@test "server correctly handles built-in 'exit' command" {
    ./dsh -s -p 5684 &
    server_pid=$!
    sleep 1
    OUTPUT=$(echo "exit" | ./dsh -c -p 5684)
    echo "stop-server" | ./dsh -c -p 5684
    sleep 1
    [ "$OUTPUT" != "" ]
}

@test "server correctly handles built-in 'stop-server' command" {
    ./dsh -s -p 5685 &
    sleep 1
    echo "stop-server" | ./dsh -c -p 5685
    sleep 1
    ! kill -0 $server_pid 2>/dev/null
}

@test "server correctly changes directory with 'cd' command" {
    ./dsh -s -p 5686 &
    SERVER_PID=$!
    sleep 1

    OUTPUT=$(echo -e "cd bats\npwd" | ./dsh -c -p 5686)

    echo "$OUTPUT"

    LAST_DIR=$(echo "$OUTPUT" | grep -Eo '/[^/]+$' | tail -n 1 | tr -d '/')

    [[ "$LAST_DIR" == "bats" ]]

    echo "stop-server" | ./dsh -c -p 5686
    sleep 1
}



@test "server correctly reports error for invalid directory in 'cd'" {
    ./dsh -s -p 5687 &
    server_pid=$!
    sleep 1
    OUTPUT=$(echo "cd /nonexistent" | ./dsh -c -p 5687)
    echo "stop-server" | ./dsh -c -p 5687
    sleep 1
    [[ "$OUTPUT" == *"No such file or directory"* ]]
}

@test "server correctly runs remote fork/exec commands" {
    ./dsh -s -p 5688 &
    server_pid=$!
    sleep 1
    OUTPUT=$(echo "ls" | ./dsh -c -p 5688)
    echo "stop-server" | ./dsh -c -p 5688
    sleep 1
    [[ "$OUTPUT" == *"dsh"* ]]
}
#puke


@test "server correctly handles multiple commands in one session" {
    ./dsh -s -p 5689 &
    server_pid=$!
    sleep 1

    OUTPUT=$(echo -e "echo first\necho second\necho third" | ./dsh -c -p 5689)

    echo "stop-server" | ./dsh -c -p 5689
    sleep 1

    [[ "$OUTPUT" == *"first"* ]] && [[ "$OUTPUT" == *"second"* ]] && [[ "$OUTPUT" == *"third"* ]]
}

@test "server correctly handles invalid commands" {
    ./dsh -s -p 5691 &
    server_pid=$!
    sleep 1

    OUTPUT=$(echo "notarealcommand" | ./dsh -c -p 5691)

    # Print debug output before stopping the server
    echo "DEBUG: Received output -> '$OUTPUT'"

    [[ "$OUTPUT" == *"Command not found in PATH"* ]]

    # Now stop the server
    echo "stop-server" | ./dsh -c -p 5691
    sleep 1
}

@test "server correctly maintains last return code" {
    ./dsh -s -p 5692 &
    server_pid=$!
    sleep 1

    OUTPUT=$(echo -e "false\nrc" | ./dsh -c -p 5692)

    echo "stop-server" | ./dsh -c -p 5692
    sleep 1

    [[ "$OUTPUT" == *"1"* ]]
}

@test "server resets return code after successful command" {
    ./dsh -s -p 5693 &
    server_pid=$!
    sleep 1

    OUTPUT=$(echo -e "false\nrc\ntrue\nrc" | ./dsh -c -p 5693)

    echo "stop-server" | ./dsh -c -p 5693
    sleep 1

    [[ "$OUTPUT" == *"1"* ]] && [[ "$OUTPUT" == *"0"* ]]
}

@test "server correctly handles file redirection" {
    ./dsh -s -p 5694 &
    server_pid=$!
    sleep 1

    echo "echo 'redirect test' > testfile" | ./dsh -c -p 5694
    OUTPUT=$(cat testfile)

    echo "stop-server" | ./dsh -c -p 5694
    sleep 1
    rm testfile  

    [[ "$OUTPUT" == *"redirect test"* ]]
}

@test "server handles piping between multiple commands" {
    ./dsh -s -p 5695 &
    server_pid=$!
    sleep 1

    OUTPUT=$(echo "echo 'piped' | grep piped" | ./dsh -c -p 5695)

    echo "stop-server" | ./dsh -c -p 5695
    sleep 1

    [[ "$OUTPUT" == *"piped"* ]]
}

@test "server correctly executes absolute path commands" {
    ./dsh -s -p 5696 &
    server_pid=$!
    sleep 1

    OUTPUT=$(echo "/bin/echo absolute" | ./dsh -c -p 5696)

    echo "stop-server" | ./dsh -c -p 5696
    sleep 1

    [[ "$OUTPUT" == *"absolute"* ]]
}

@test "server correctly handles SIGINT" {
    ./dsh -s -p 5698 &
    server_pid=$!
    sleep 1

    kill -SIGINT $server_pid
    sleep 1

    ! kill -0 $server_pid 2>/dev/null
}

@test "server piped commands with redirection" {
  ./dsh -s -p 6001 &
  srv_pid=$!
  sleep 1

  (echo "echo hello | sed 's/hello/goodbye/' > output.txt" | ./dsh -c -p 6001) >/dev/null

  OUTPUT=$(cat output.txt)
  rm -f output.txt

  echo "stop-server" | ./dsh -c -p 6001
  sleep 1

  [[ "$OUTPUT" == "goodbye" ]]
}

@test "server concurrent clients where one stops the server" {
  ./dsh -s -p 6002 -x &
  srv_pid=$!
  sleep 1

  (echo "sleep 10" | ./dsh -c -p 6002) &

  echo "stop-server" | ./dsh -c -p 6002

  sleep 1
  ! kill -0 "$srv_pid" 2>/dev/null
}

@test "server built-in interplay" {
  ./dsh -s -p 6004 &
  srv_pid=$!
  sleep 1

  OUTPUT=$(echo -e "cd /tmp\npwd\nnot_a_command\nrc" | ./dsh -c -p 6004)
  echo "stop-server" | ./dsh -c -p 6004
  sleep 1

  [[ "$OUTPUT" == *"/tmp"* ]]
  [[ "$OUTPUT" == *"2"* || "$OUTPUT" == *"127"* ]]
}


################################################################################
#TESTS FROM THE PREVIOUS ASSIGNMENT TO MAKE SURE DSH OPERATES CORRECTLY LOCALLY#
################################################################################

@test "basic: echo command" {
    run ./dsh <<EOF
echo Hello, world!
exit
EOF
    [[ "$output" == *"Hello, world!"* ]]
}

@test "handle multiple spaces between arguments" {
    run ./dsh <<EOF
echo    multiple     spaces
exit
EOF
    [[ "$(echo "$output" | tr -s ' ')" == *"multiple spaces"* ]]
}

@test "run a command with mixed quoted and unquoted arguments" {
    run ./dsh <<EOF
echo "hello" world
exit
EOF
    [[ "$output" == *"hello world"* ]]
}

@test "run a command with special characters" {
    run ./dsh <<EOF
echo "!@#$%^&*()"
exit
EOF
    [[ "$output" == *"!@#$%^&*()"* ]]
}

@test "attempt to cd into a non-existent directory" {
    run ./dsh <<EOF
cd /does_not_exist
exit
EOF
    [[ "$output" == *"cd: No such file or directory"* ]]
}

@test "command not found prints expected error" {
    run ./dsh <<EOF
not_a_real_command
exit
EOF
    [[ "$output" == *"Command not found in PATH"* ]]
}

@test "check return code after an invalid command" {
    run ./dsh <<EOF
not_a_real_command
rc
exit
EOF
    [[ "$output" == *"2"* ]]
}

@test "check return code after a successful command" {
    run ./dsh <<EOF
ls
rc
exit
EOF
    [[ "$output" == *"0"* ]]
}

@test "redirect standard output to a file and verify contents" {
    run ./dsh <<EOF
echo "test output" > testfile
cat testfile
exit
EOF
    [[ "$output" == *"test output"* ]]
}

@test "exit command" {
    run ./dsh <<EOF
exit
EOF
    [ "$status" -eq 0 ]
}

@test "attempt to execute an absolute path command" {
    run ./dsh <<EOF
/bin/echo absolute path command
exit
EOF
    [[ "$output" == *"absolute path command"* ]]
}

@test "execute a command that takes input and check response" {
    run ./dsh <<EOF
echo "hello" | cat
exit
EOF
    [[ "$output" == *"hello"* ]]
}

@test "input handling does not break with empty input" {
    run ./dsh <<EOF

exit
EOF
    [ "$status" -eq 0 ]
}

@test "extra spaces at the start of a command are ignored" {
    run ./dsh <<EOF
    echo "trimmed"
exit
EOF
    [[ "$output" == *"trimmed"* ]]
}

@test "extra spaces at the end of a command are ignored" {
    run ./dsh <<EOF
echo "trimmed"    
exit
EOF
    [[ "$output" == *"trimmed"* ]]
}

@test "shell can handle very long command input" {
    long_command=$(printf 'echo %.0sA{1..2000}')
    run ./dsh <<EOF
$long_command
exit
EOF
    [ "$status" -eq 0 ]
}

@test "shell ignores empty command" {
    run ./dsh <<EOF
     
exit
EOF
    [ "$status" -eq 0 ]
}

@test "return code of previous failed command is maintained" {
    run ./dsh <<EOF
false
rc
exit
EOF
    [[ "$output" == *"1"* ]]
}

@test "shell does not crash with deeply nested commands" {
    run ./dsh <<EOF
echo $(echo $(echo $(echo "nested")) )
exit
EOF
    [[ "$output" == *"nested"* ]]
}

@test "shell can handle multiple commands sequentially" {
    run ./dsh <<EOF
pwd
ls
echo "multiple"
exit
EOF
    [[ "$output" == *"multiple"* ]]
}

@test "rc returns correct value after multiple commands" {
    run ./dsh <<EOF
true
rc
false
rc
exit
EOF
    [[ "$output" == *"0"* ]]
    [[ "$output" == *"1"* ]]
}

