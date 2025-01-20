#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define BUFFER_SZ 50

//prototypes
void usage(char *);
void print_buff(char *, int);
int  setup_buff(char *, char *, int);

//prototypes for functions to handle required functionality
int  count_words(char *, int, int);
//add additional prototypes here
//
// EXTRA CREDIT
int  replace_substring(char *buff, int buff_len, int *p_str_len,
                       const char *find_str, const char *replace_str);

int setup_buff(char *buff, char *user_str, int len){
    //TODO: #4:  Implement the setup buff as per the directions


    int stored_chars = 0;
    int i = 0;
    int last_was_space = 0; // boolean but using int for simplicity

    memset(buff, '.', len);  // fill buffer with dotss

    while (*(user_str + i) != '\0') {
        int is_space = (*(user_str + i) == ' ' || *(user_str + i) == '\t' ||
                        *(user_str + i) == '\n' || *(user_str + i) == '\r' ||
                        *(user_str + i) == '\v' || *(user_str + i) == '\f');

        if (is_space) {
            if (!last_was_space) {
                if (stored_chars >= len) return -1; // too long
                *(buff + stored_chars) = ' ';
                stored_chars++;
                last_was_space = 1;
            }
        } else {
            if (stored_chars >= len) return -1; // too long
            *(buff + stored_chars) = *(user_str + i);
            stored_chars++;
            last_was_space = 0;
        }
        i++;
    }
    return stored_chars;
}

void print_buff(char *buff, int len){
    printf("Buffer:  ");
    for (int i=0; i<len; i++){
        putchar(*(buff+i));
    }
    putchar('\n');
}

void usage(char *exename){
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);

}

int count_words(char *buff, int len, int str_len){
    // if str_len is out of range return an error
    if (str_len < 0 || str_len > len) {
        return -1;
    }
    
    if (str_len == 0) return 0;

    int word_count = 0;
    int in_space = 1;
    for (int i = 0; i < str_len; i++) {
        char c = *(buff + i);
        if (c == ' ') {
            in_space = 1;
        } else {
            if (in_space) {
                word_count++;
            }
            in_space = 0;
        }
    }
    return word_count;
}

//ADD OTHER HELPER FUNCTIONS HERE FOR OTHER REQUIRED PROGRAM OPTIONS

int reverse_string(char *buff, int str_len){
    int start = 0;
    int end   = str_len - 1;
    while (start < end) {
        char tmp      = *(buff + start);
        *(buff + start)   = *(buff + end);
        *(buff + end)     = tmp;
        start++;
        end--;
    }
    return 0;
}

int main(int argc, char *argv[]){

    char *buff;             //placehoder for the internal buffer
    char *input_string;     //holds the string provided by the user on cmd line
    char opt;               //used to capture user option from cmd line
    int  rc;                //used for return codes
    int  user_str_len;      //length of user supplied string

    //TODO:  #1. WHY IS THIS SAFE, aka what if arv[1] does not exist?
    // ANSWER #1: we first check (argc < 2) and if it's true we call usage() and exit
    // that ensures argv[1] does exist before we use *argv[1]
    if ((argc < 2) || (*argv[1] != '-')){
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1]+1);   //get the option flag

    //handle the help flag and then exit normally
    if (opt == 'h'){
        usage(argv[0]);
        exit(0);
    }

    //WE NOW WILL HANDLE THE REQUIRED OPERATIONS

    //TODO:  #2 Document the purpose of the if statement below
    //ANSWER #2: this if-statement checks if there is a third argument (argv[2])
    //if nott the user didnt provide the string we need to process
    //so we show usage and exit
    if (argc < 3){
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2]; //capture the user input string

    //TODO:  #3 Allocate space for the buffer using malloc and
    //          handle error if malloc fails by exiting with a 
    //          return code of 99
    // CODE GOES HERE FOR #3

    buff = (char *)malloc(BUFFER_SZ);
    if (!buff) {
        fprintf(stderr, "error: malloc failed.\n");
        //the directions say return code 2, not 99, so i decided 2 is the way
        //please do not punish me for inconsistencies in the directions/comments :c
        exit(2);
    }
    memset(buff, '.', BUFFER_SZ);


    user_str_len = setup_buff(buff, input_string, BUFFER_SZ);     //see todos
    if (user_str_len < 0){
    fprintf(stderr, "error: Provided input string is too long\n");
    exit(3);
    }

    switch (opt){
        case 'c':
            rc = count_words(buff, BUFFER_SZ, user_str_len);
            if (rc < 0){
                printf("Error counting words, rc = %d\n", rc);
                exit(3);
            }
            printf("Word Count: %d\n", rc);
            break;

        //TODO:  #5 Implement the other cases for 'r' and 'w' by extending
        //       the case statement options

        case 'r':
            if (user_str_len > 0) {
                reverse_string(buff, user_str_len);
                printf("Reversed String: ");
                for (int i = 0; i < user_str_len; i++) {
                    putchar(*(buff + i));
                }
                putchar('\n');
            }
            break;

        case 'w': {
            printf("Word Print\n----------\n");

            int i = 0;
            int word_count = 0;
            int char_count = 0;       // track number of chars in the current word
            int start_of_word = 1;    // boolean like to detect when we begin a new word

            for (; i < user_str_len; i++) {
                if (*(buff + i) == ' ') {
                    if (!start_of_word) {
                        // we just ended a word so print the length in parentheses
                        printf(" (%d)\n", char_count);
                        char_count = 0;
                        start_of_word = 1;
                    }
                } else {
                    // a non-space character
                    if (start_of_word) {
                        word_count++;
                        printf("%d. ", word_count);
                        start_of_word = 0;
                        char_count = 0;
                    }
                    putchar(*(buff + i));
                    char_count++;
                }
            }
            // if we ended still in a word (did not just print the length) handle that)
            if (!start_of_word) {
                printf(" (%d)\n", char_count);
            }
            break;
        }

        // EXTRA CREDIT
        case 'x':
        {
            // we expect 5 total arguments eg
            // ./stringfun -x "some string" find_str replace_str
            // if less than 5 we cannot proceed
            if (argc < 5) {
                usage(argv[0]);
                exit(1);
            }
            const char *find_str = argv[3];
            const char *replace_str = argv[4];

            rc = replace_substring(buff, BUFFER_SZ, &user_str_len, find_str, replace_str);
            if (rc < 0) {
                printf("Error in substring replacement, rc=%d\n", rc);
                free(buff);
                exit(3);
            }

            printf("Modified String: ");
            for (int i = 0; i < user_str_len; i++) {
                putchar(*(buff + i));
            }
            putchar('\n');
            break;
        }

        default:
            usage(argv[0]);
            exit(1);
    }

    //TODO:  #6 Dont forget to free your buffer before exiting
    print_buff(buff,BUFFER_SZ);
    free(buff);

    exit(0);
}

//TODO:  #7  Notice all of the helper functions provided in the 
//          starter take both the buffer as well as the length.  Why
//          do you think providing both the pointer and the length
//          is a good practice, after all we know from main() that 
//          the buff variable will have exactly 50 bytes?
//  
//          PLACE YOUR ANSWER HERE
// ANSWER #7: it's safer design functions can validate that they do not
//exceed the specified length this also makes functions
//more general and reusable if the buffer size changes

// EXTRA CREDIT
int replace_substring(char *buff, int buff_len, int *p_str_len,
                      const char *find_str, const char *replace_str)
{
    int str_len = *p_str_len;
    if (*find_str == '\0') {
        return 0;
    }

    //find first occurrence of find_str in buff up to str_len
    char *match_ptr = NULL;
    for (int i = 0; i < str_len; i++) {
        char *b = buff + i;
        const char *f = find_str;
        int offset = 0;
        int mismatch = 0;
        while (1) {
            char bc = *(b + offset);
            char fc = *(f + offset);
            if (fc == '\0') {
                break;
            }
            if ((i + offset) >= str_len || bc != fc) {
                mismatch = 1;
                break;
            }
            offset++;
        }
        if (!mismatch) {
            match_ptr = b; // found at i
            break;
        }
    }
    if (!match_ptr) {
        return 0;
    }

    //calculate lengths of find_str and replace_str
    int find_len = 0;
    for (const char *fp = find_str; *fp; fp++) {
        find_len++;
    }
    int replace_len = 0;
    for (const char *rp = replace_str; *rp; rp++) {
        replace_len++;
    }

    //if new string exceeds buffer_sz handle error or partial truncation
    int new_len = str_len - find_len + replace_len;
    if (new_len > buff_len) {
        return -1; //overflow
    }

    int match_index = (int)(match_ptr - buff);
    int tail_start  = match_index + find_len;
    int tail_len    = str_len - tail_start;

    //shift buffer to make room or fill the gap
    if (replace_len > find_len) {
        // shift right
        int shift = replace_len - find_len;
        for (int i = tail_len - 1; i >= 0; i--) {
            *(buff + (tail_start + i + shift)) = *(buff + (tail_start + i));
        }
    } else if (replace_len < find_len) {
        // shift left
        int shift = find_len - replace_len;
        for (int i = 0; i < tail_len; i++) {
            *(buff + (tail_start + i - shift)) = *(buff + (tail_start + i));
        }
    }

    //copy replace_str into match location
    for (int i = 0; i < replace_len; i++) {
        *(buff + (match_index + i)) = *(replace_str + i);
    }

    *p_str_len = new_len;

    for (int i = new_len; i < buff_len; i++) {
        *(buff + i) = '.';
    }
    return 0;
}

