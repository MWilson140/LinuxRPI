#ifndef COMMAND_H
#define COMMAND_H

#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#define MAX_INPUT 1156  //more than 1000 so the argument can have more than 1000 argv also for command more than 100 command
#define EXPAND_SIZE 10
#define INIT_COMMANDS  100
#define INIT_ARGUMENTS 100  //the requirement mension atleast 1000 argv and with my approach it reallocate everytime reaching the init and increament by 10 to save memory.
static const char backslash = (char) 0x5C; //convert back slash \ to hex so avoid c programming language limitations
static const char whiteSpaceCharacters[3] = { (char) 0x20, (char) 0x09, (char) 0x00 };


typedef struct commandStructure {
    char *cmdname;
    char *stdin;
    char *stdout;
    char *stderrout;
    char **argv;
    int argc;
    int pipeto;  // Count number of pipeline in the cmd input
    bool background;
} Command;

// Function declarations
Command* createCommands();
char* checkSeparators(char* input);
void removeBackslashes(char* input);
void freeCommands(Command **cmdLine);
char* toToken(char* input, const char* separators);
void displayCommands(Command **cmdLine);
void expandWildcard(char *token, Command *command);
void handleRedirections(char *input, Command *command);
void processCommand(char *input, Command *command);
Command ** processCommandLine(char *input, int news, bool background, int pipeTo);

#endif // COMMAND_H
