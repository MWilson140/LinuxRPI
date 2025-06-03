#ifndef SHELLCOMMANDS_H
#define SHELLCOMMANDS_H

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
#include "../command/command.h"
#include "../history/history.h"
#include <termios.h>

// Function declarations

void trim(char * string);
void clearLine();
void disableRawMode(struct termios *original);
void enableRawMode(struct termios *original);
bool checkSequencesError(const char* input);
void checkQuotesError(const char* input,bool *hasError);
char* checkSeparators(char* input);
void freeCommands(Command **cmdLine);
char* toToken(char* input, const char* separators);
void displayCommands(Command **cmdLine);
void expandWildcard(char *token, Command *command);
void handleRedirections(char *input, Command *command);
void processCommand(char *input, Command *command);
void executeCommand(char **prompt, Command **cmdLine,struct termios *original);
char * retrieveCommandFromHistory(char * userinput, history * hist);
void handleInput(char *userinputs, history *hist, char *prompt);
bool isInputValid(const char *input);
void ignoreSignals();
#endif // SHELL_H
