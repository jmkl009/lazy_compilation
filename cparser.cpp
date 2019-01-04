//
// Created by WangJingjin on 2018/10/25.
//
#include "cparser.hpp"

#define isMeaningfulCharacter(ch) ((ch) >= 33 && (ch) <= 126)

static const char * const DEFINE = "define";
static const char * const INCLUDE = "include";

ssize_t getline_split_by(char **line_ptr, int *capacity_ptr, FILE * in, char token);

int str_contains_at_beginning(const char * s1, const char * s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) {
            break;
        }
        s1++;
        s2++;
    }
    return !*s2; //0 if not contain, non-null if contains
}

int back_slash_exists(const char *lineptr) {
    while (*lineptr) {
        if (*lineptr == '\\') {
            return 1;
        }
        lineptr++;
    }
    return 0;
}

int resolveDefine(FILE * inFile, FILE * outFile) {

    //There are more lines for it
    char * new_line = NULL;
    int capacity = 0;
    unsigned nbytes = getline_split_by(&new_line, &capacity, inFile, '\r');
    while (nbytes > 0) {
        fwrite(new_line, nbytes, 1, outFile);
        if (!back_slash_exists(new_line)) {
            break;
        }
        nbytes = getline_split_by(&new_line, &capacity, inFile, '\r');
    }

    free(new_line);
    return nbytes;
}

int resolveInclude(char * const searchStart, FILE *outFile) {
    //ASSUMPTION: the #include never uses absolute paths
    char *search_ptr = searchStart;
    while (!isMeaningfulCharacter(*search_ptr)) {
        search_ptr++;
    }

    if (*search_ptr == '<') { //No need for parent path
        fwrite(searchStart, strlen(searchStart), 1, outFile);
    } else if (*search_ptr == '"') {
        char *right_bound = strstr(search_ptr + 1, "\"");

        *right_bound = '\0';
        char *full_library_path = realpath(search_ptr + 1, NULL);

        fwrite(" \"", 2, 1, outFile);
        fwrite(full_library_path, strlen(full_library_path), 1, outFile);
        *right_bound = '"';

        free(full_library_path);

        fwrite(right_bound, strlen(right_bound), 1, outFile);
    }

    return 0;
}

char * findLeftParenthesis(char *lineptr) {
    while (*lineptr) {
        if (*lineptr == '(') {
            return lineptr;
        }
        lineptr++;
    }
    return NULL;
}

int checkCurlyBrackets(char *lineptr) {
    int curly_brackets_count = 0;
    while (*lineptr) {
        char curr_ch = *lineptr;
        if (curr_ch == '{') {
            curly_brackets_count++;
        } else if(curr_ch == '}') {
            curly_brackets_count--;
        }
        lineptr++;
    }
    return curly_brackets_count;
}

unsigned findFirstMeaningfulCharacterIdx(const char * line) {
    unsigned idx_counter = 0;
    while (line[idx_counter] && !isMeaningfulCharacter(line[idx_counter])) {
        idx_counter++;
    }
    return idx_counter;
}
//
//int checkAndWriteFunc(char * start_line, const char * funcName, unsigned firstMeaningfulCharacterIdx) {
//    char * token = strtok(start_line + firstMeaningfulCharacterIdx, " (");
//    bool functionNameFound = false;
//    while (token) {
//        if (!strcmp(funcName, token)) {
//            functionNameFound = true;
//        }
//        token = strtok(NULL, " (");
//    }
//    if (!token) { //Function not found
//        return 0;
//    }
//}

int curly_bracket_before_semicolon(char * lineptr) {
    while (*lineptr) {
        char curr_ch = *lineptr;
        if (curr_ch == '{') {
            return 1;
        } else if (curr_ch == ';') {
            return -1;
        }
        lineptr++;
    }
    return 0;
}

#define SEMICOLON_FOUND -1
#define UNDEFINED -2
#define IS_VARIABLE -3
#define LEFT_PARENTHESIS_FOUND -4
#define CURLY_BRACKET_FOUND -5
#define NO_CONTENT -6
#define EXTERN "extern "

int determine_type(char *lineptr, char **const new_line_ptr) {
    //Checking that there is no bracelets and the semi colon is after the closing bracelet
    //TODO: Add global variable support
    if (lineptr[findFirstMeaningfulCharacterIdx(lineptr)] == '\0') {
        return NO_CONTENT;
    }

//    bool curly_brackets_found = false;
//    bool is_variable = true;
    while (*lineptr) {
        const char curr_ch = *lineptr;
        if (curr_ch == '{') {
//            curly_brackets_found = true;
//            lineptr++;
//            break;
            return CURLY_BRACKET_FOUND;
        } else if (curr_ch == ';') {
            *new_line_ptr = lineptr + 1;
            return SEMICOLON_FOUND;
        } else if(curr_ch == '(') {
            *new_line_ptr = lineptr + 1;
            return LEFT_PARENTHESIS_FOUND;
        }
//        else if (is_variable && *lineptr == '(') {
//            is_variable = false;
//        }
        lineptr++;
    }

//    if (!curly_brackets_found) {
//        return UNDEFINED;
//    }

//    int curly_brackets_count = 1;
//    while (*lineptr) {
//        if (*lineptr == '{') {
//            curly_brackets_count ++;
//        } else if (*lineptr == '}') {
//            curly_brackets_count --;
//        }
//        lineptr ++;
//    };

    return UNDEFINED; //Unable to determine whether it is a forward declaration or not
}

char * concatenate(char *big, char *small) {
    if (big == NULL) {
        big = strdup(small);
    } else {
        big = (char *)realloc(big, strlen(big) + strlen(small) + 1);
        strcat(big, small);
    }
    return big;
}

char *strstr_strict(char *big, char *small) {
    char *ptr = strstr(big, small);
    if (ptr) {
        size_t small_size = strlen(small);
        char ch_after = *(ptr + small_size);
        if (isalpha(ch_after) || ch_after == '-' || ch_after == '_') {
            ptr = NULL;
        }
    }
    return ptr;
}

#define WRITTEN 1
int checkAndWriteFunc(char * start_line, char * funcName, FILE * in, FILE * out, char * prependage) {
//    char * funcName_start = strstr(start_line, funcName);
//    if (!funcName_start) {
//        return 0;
//    }

    char * new_line_ptr = NULL;
    int type = determine_type(start_line, &new_line_ptr);

    if (type == NO_CONTENT || type == CURLY_BRACKET_FOUND) {
        return 0;
    }

    if (type == SEMICOLON_FOUND) { //Insert extern
        if (!strstr(start_line, "static")) {
            fwrite("extern ", 7, 1, out);
            fwrite(start_line, new_line_ptr - start_line, 1, out);
            fwrite("\r", 1, 1, out); //Insert return to the end of an instruction
        }
        return checkAndWriteFunc(new_line_ptr + findFirstMeaningfulCharacterIdx(new_line_ptr), funcName, in, out, NULL);
    }

    if (type == UNDEFINED) { //The end of line is reached
        // No semicolon so we search on
        char * next_line = NULL;
        int capacity = 0;
        getline_split_by(&next_line, &capacity, in, '\r');
        int result = checkAndWriteFunc(next_line, funcName, in, out, concatenate(prependage, next_line));
        free(next_line);
        return result;
    }

    if (type == LEFT_PARENTHESIS_FOUND
        && ((prependage && strstr_strict(prependage, funcName)) || strstr_strict(start_line, funcName))) {
        //May be forward declaration, so we try to seek a ';' before '{' to determine what is the case
        char * line = start_line;
        int capacity = 0;
        ssize_t nbytes = strlen(line);
        do {
            int result = curly_bracket_before_semicolon(line);
            if (result == 0) { // Neither semicolon nor curly bracket found
                prependage = concatenate(prependage, line);
            } else if (result == -1) {
                if (prependage) {
                    free(prependage);
                }
                return 0; //Semicolon found before any curly bracket
            } else {
                break; //curly bracket found before semicolon
            }
            nbytes = getline_split_by(&line, &capacity, in, '\r');
        } while (1);

        int curly_brackets_count = 0;
        if (prependage) {
            fwrite(prependage, strlen(prependage), 1, out);
            free(prependage);
        }

        do {
            if (!str_contains_at_beginning(line + findFirstMeaningfulCharacterIdx(line), "//")) {
                fwrite(line, nbytes, 1, out);
                curly_brackets_count += checkCurlyBrackets(line);
                if (curly_brackets_count == 0) {
                    break;
                }
            }
            nbytes = getline_split_by(&line, &capacity, in, '\r');
        } while (nbytes > 0);
//        free(line);
        return 1;
    }

    return 0;
}

#define INITIAL_CAPACITY 1024
ssize_t getline_split_by(char **line_ptr, int *capacity_ptr, FILE * in, char token) {
    int capacity = *capacity_ptr ? *capacity_ptr : INITIAL_CAPACITY;
    char * line = *line_ptr ? *line_ptr : (char *)malloc(INITIAL_CAPACITY);
    if (feof(in)) {
        return -1;
    }
    char ch = fgetc(in);
    int bytesRead = 0;
    while (ch != EOF) {
        line[bytesRead] = ch;
        bytesRead ++;
        if (bytesRead == capacity) {
            capacity <<= 1;
            line = (char *)realloc(line, capacity);
        }
        if (ch == token || ch == '\n') {
            break;
        }
        ch = fgetc(in);
    }

    line[bytesRead] = '\0';
    *line_ptr = line;
    *capacity_ptr = capacity;

    return bytesRead;
}

#define CLEAN_UP \
free(line);\
fclose(in);\
fclose(out);
int isolateFunction(char* inFile, char * funcName, char* writeFile) {
    FILE * in = fopen(inFile, "r");
    if (in == NULL) {
        fprintf(stderr, "input file parser error (%s): ", inFile);
        perror("");
        return -1;
    }
    FILE * out = fopen(writeFile, "w");
    if (out == NULL) {
        fprintf(stderr, "output file parser error (%s): ", writeFile);
        perror("");
        fclose(in);
        return -1;
    }

    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    //Change directory into the same as inFile to make things a big easy
    char *full_path = realpath(inFile, NULL);
    //Tokenize full_path to get parent_path
    size_t end_idx = strlen(full_path) - 1;
    while (full_path[end_idx] != '/') {
        end_idx--;
    }
    full_path[end_idx] = '\0';
    char *parent_path = full_path;
    chdir(parent_path);
    //Also change inFile so that inFile still refers to the right place.
    inFile = full_path + end_idx + 1;

    int func_found = 0;
    int curly_brackets_count = 0;

    char * line = NULL;
    int capacity = 0;

    ssize_t nbytes = getline_split_by(&line, &capacity, in, '\r');
    while (nbytes > 0) { //Simply write every line before the target function
//        line[nbytes - 1] = '\0';
        if (curly_brackets_count == 0) {
            const unsigned firstMeaningfulCharacterIdx = findFirstMeaningfulCharacterIdx(line);
            char * meaningfulLineStart = line + firstMeaningfulCharacterIdx;
            const ssize_t write_bytes = nbytes - firstMeaningfulCharacterIdx;
            if (*meaningfulLineStart == '#') { //A compiler directive
                if (str_contains_at_beginning(meaningfulLineStart + 1, DEFINE)) { //The directive is a define
                    fwrite(meaningfulLineStart, write_bytes, 1, out);
                    if (back_slash_exists(meaningfulLineStart)) {
                        resolveDefine(in, out);
                    }
                }
                else if (str_contains_at_beginning(meaningfulLineStart + 1, INCLUDE)) {
                    //sizeof "#include" = 8
                    fwrite(meaningfulLineStart, 8, 1, out);
                    resolveInclude(meaningfulLineStart + 8, out);
                }
                nbytes = getline_split_by(&line, &capacity, in, '\r');
                continue;
            }

            if (str_contains_at_beginning(meaningfulLineStart, "//")) { // A comment
                nbytes = getline_split_by(&line, &capacity, in, '\r');
                continue;
            } else if (str_contains_at_beginning(meaningfulLineStart, "/*")) { // Also a comment
                //Skip all the comments
                do {
                    nbytes = getline_split_by(&line, &capacity, in, '\r');
                }  while (line[findFirstMeaningfulCharacterIdx(line)] == '*');
                continue;
            } else if (str_contains_at_beginning(meaningfulLineStart, "typedef")
                       || str_contains_at_beginning(meaningfulLineStart, "enum")
                        || (str_contains_at_beginning(meaningfulLineStart, "struct") && strstr(meaningfulLineStart, "(") == NULL)) {
                //Write the whole thing until a semicolon is met.
                bool curlyBracketFound = false;
                do {
                    fwrite(meaningfulLineStart, strlen(meaningfulLineStart), 1, out);
                    nbytes = getline_split_by(&line, &capacity, in, '\r');
                    if(!curlyBracketFound && strstr(meaningfulLineStart, "}")) {
                        curlyBracketFound = true;
                    }
                } while (!(curlyBracketFound && strstr(meaningfulLineStart, ";"))); //A curly bracket before semicolon denotes the end.
                fwrite(meaningfulLineStart, strlen(meaningfulLineStart), 1, out);
                nbytes = getline_split_by(&line, &capacity, in, '\r');
                continue;
            }

//        fwrite(line, nbytes, 1, out);
//        curly_brackets_count += checkCurlyBrackets(line);

            if (checkAndWriteFunc(line, funcName, in, out, NULL)) { //It is a possible function declaration
//                if (checkAndWriteFunc(line, funcName, in, out, NULL) == )
                func_found = 1;
                break;
            }
        }

        curly_brackets_count += checkCurlyBrackets(line);
        nbytes = getline_split_by(&line, &capacity, in, '\r');
    }

    free(line);
    fclose(in);
    fclose(out);
//    CLEAN_UP
    chdir(cwd);
    return func_found;
}
