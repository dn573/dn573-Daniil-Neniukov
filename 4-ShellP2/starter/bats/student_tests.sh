#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

@test "Example: check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF

    # Assertions
    [ "$status" -eq 0 ]
}

@test "basic external command execution" {
    run ./dsh <<EOF
ls
exit
EOF
    [ "$status" -eq 0 ]
}

@test "execute a command with arguments" {
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

@test "attempt to execute a file without execution permission" {
    run ./dsh <<EOF
touch non_executable
chmod -x non_executable
./non_executable
rc
exit
EOF
    [[ "$output" == *"Permission denied"* ]]
    [[ "$output" == *"126"* ]]
}

@test "run a valid command and check for case sensitivity" {
    run ./dsh <<EOF
ECHO "This should fail if case-sensitive"
exit
EOF
    [[ "$output" != *"This should fail if case-sensitive"* ]]
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

@test "attempt to execute an absolute path to a non-existent command" {
    run ./dsh <<EOF
/bin/doesnotexist
exit
EOF
    [[ "$output" == *"Command not found in PATH"* ]]
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

@test "background processes can be started" {
    run ./dsh <<EOF
sleep 1 &
exit
EOF
    [ "$status" -eq 0 ]
}

@test "background process does not block exit" {
    run ./dsh <<EOF
sleep 5 &
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

@test "shell handles recursive execution" {
    run ./dsh <<EOF
./dsh
exit
EOF
    [ "$status" -eq 0 ]
}

@test "shell does not hang with long input" {
    long_input=$(printf 'a%.0s' {1..10000})
    run ./dsh <<EOF
echo $long_input
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

