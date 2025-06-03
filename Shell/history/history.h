#ifndef HISTORY_H
#define HISTORY_H

#include <string.h>
#include <stdio.h>
#include "../command/command.h"
#include <sys/sem.h>
#include <errno.h>

typedef struct history {
    int entries;
    int current;
    char *commands[100]; // Store pointers to command strings
} history;

void initHistory(history *hist);
void printHistory(history hist);

int addToHistory(char *Command, history *hist);

char *getPrev(history *hist);

char *getNext(history *hist);


char * getHistory(Command * comm, history hist);



#endif