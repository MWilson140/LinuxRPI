#include "history.h"

void printHistory(history hist)
{
	int i = 0; 
	printf("printing history\n");
	while (hist.commands[i] != NULL)
	{
		printf("%d%s\n", i, hist.commands[i]);
		i++;
	}
	
}

void initHistory(history *hist) {
    hist->entries = 0; 
    hist->current = 0; 
    for (int i = 0; i < 100; i++) {
        hist->commands[i] = NULL;
    }
    //history_sem = getsem(1234, 1); 
}

int addToHistory(char *command, history *hist) {
   // p(history_sem); // Lock the semaphore within the function
    if (hist->entries < 100) { 
        hist->commands[hist->entries] = strdup(command); 
        hist->entries++;
        hist->current = hist->entries;
        //printf("added to history %s %d\n", command, hist->entries);
        //v(history_sem); // Unlock after operation
        return 0;
    }
    //v(history_sem); // Unlock if history limit is reached
    return -1;
}

char *getPrev(history *hist) { 
    //p(history_sem); // Lock for safe access
    if (hist->entries != 0) {
        hist->current--;
        if (hist->current < 0) {
            hist->current++;
        }
       // v(history_sem); // Unlock after retrieving entry
        return hist->commands[hist->current];
    }
   // v(history_sem);
    return NULL;
}

char *getNext(history *hist) { 
   // p(history_sem); // Lock for safe access
    if (hist->entries != 0) {
        hist->current++;
        if (hist->current > hist->entries - 1) {
            hist->current--;
        }
        //v(history_sem); // Unlock after retrieving entry
        return hist->commands[hist->current];
    }
   // v(history_sem);
    return NULL;
}

char * getHistory(Command *comm, history hist) 
{
    int n;
    memmove(comm->argv[0], comm->argv[0] + 1, strlen(comm->argv[0]));	
    fflush(stdout);
    if (strcmp(comm->argv[0], "history") == 0) 
	{
		for (int i = 0; i < hist.entries; i++)
		{
			printf("%d ", i);
			printf("%s\n", hist.commands[i]);
		}
		return("\0");
    }
    else if ((n = atoi(comm->argv[0])) != 0 || strcmp(comm->argv[0], "0") == 0) //it will be a number
	{
		if (n >= hist.entries -1 || n < 0)
		{
			perror("invalid n value");
			return("\0");			
		}
		if (hist.commands[n][1] - '0' == n)
		{
			perror("command number is the same as the called number");
			return "\0";
		}
		
		else 
		{
			if ((hist.commands[n][1] - '0') == n )
			{
				perror("command number is the same as the called number");
				return "\0";
			}
			return(hist.commands[n]);
		}
    } else 
	{
		for (int i = 0; i < hist.entries; i++)
		{
			//change this to use strprk and see if it is at the start of the string
			if ((strncmp(comm->argv[0], hist.commands[i], strlen(comm->argv[0]))) == 0)
			{
				return(hist.commands[i]);
			}
		}
		return("\0");
    }
    fflush(stdout);
    return("\0"); 
}
