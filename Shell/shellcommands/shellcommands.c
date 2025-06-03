//handle check for double quotes and single quotes

#include "shellcommands.h"


void checkQuotesError(const char* input,bool* hasError) {
    bool sinQuote=false, dQuotes = false;
    *hasError=false;
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] == '\'' && (i == 0 || input[i - 1] != backslash) && !dQuotes) {
            sinQuote = !sinQuote;
        }
        else if (input[i] == '"' && (i == 0 || input[i - 1] != backslash) && !sinQuote) {
            dQuotes = !dQuotes;
        }
        else if (sinQuote && input[i] == '\'' && (i == 0 || input[i - 1] != backslash)) {
            fprintf(stderr, "\nError: single quote inside single-quote!! \n");
            *hasError=true;
            return;
        }
        else if (dQuotes && input[i] == '"' && (i == 0 || input[i - 1] != backslash)) {
            fprintf(stderr, "\nError: double quote inside double-quote!! \n");
            *hasError=true;
            return;
        }
    }
    if (sinQuote) {
        fprintf(stderr, "\nError: single quote not match!!\n\n");
        *hasError=true;
        return;
    }
    if (dQuotes) {
        fprintf(stderr, "\nError: double quote  not match !!\n\n");
        *hasError=true;
        return;
    }
}


void enableRawMode(struct termios *original) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, original); 
    raw =*original;
    raw.c_lflag &= ~(ECHO | ICANON); 
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disableRawMode(struct termios *original) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, original);
}

void clearLine() {
    printf("\r\033[K"); 
}


void claimingZombieProcesses(int sig){
    if(sig==SIGCHLD){
        int more=1;
        int status; //NULL
        pid_t pidl;
        while(more){
        pidl = waitpid(-1, &status, WNOHANG);
            if(pidl<=0){
                more=0;
            }
        }
    }
}

void ignoreSignals() {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    // Ignore
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
    //setup the handler for zombie process
    sa.sa_handler = claimingZombieProcesses;
    sigaction(SIGCHLD, &sa, NULL);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("Error signal handler:");
        exit(1);
    }
}


char* toLowerCase(const char *str) {
    if (!str) return NULL;
    
    char *lowerStr = malloc(strlen(str) + 1);
    if (!lowerStr) return NULL;

    char *ptr = lowerStr;
    while (*str) {
        *ptr++ = tolower((unsigned char) *str++);
    }
    *ptr = '\0';
    return lowerStr;
}

void promptChange(char **prompt, Command *command) {
    // Check if more than one value is provided
    if (command->argc > 2) {
        printf("No spaces is allowed!  Use single string only!\n");
        return;
    }
    if(command->argc==1){
        //Set to default value
        free(*prompt); 
        *prompt = strdup("%");
    }
    else if (command->argc == 2) {
        if (strlen(command->argv[1]) > 20) { 
            printf("The prompt name can not exceed 20 characters! Try a shorter name!\n");
            return;
        }else{
        free(*prompt); 
        *prompt = strdup(command->argv[1]); // Set the new prompt using the first argument
        }
    }else{
        //nothing else
    }
}

void executeCdCommand(Command *command) {
    char defaultPath[MAX_INPUT];
    const char *cdPath;

    if (command->argc == 1) {
        // Set the path to the Home dir
        cdPath = getenv("HOME");
    } else if (command->argc == 2) {
        // Set the path to the provided argument
        cdPath = command->argv[1];
    } else {
        // Handle too many arguments
        fprintf(stderr, "cd: too many arguments\n\n");
        return;
    }

    // Attempt to change directory to the specified path
    if (chdir(cdPath) != 0) {
        perror("cd"); // Check for error
    }

    // Check and print current directory
    char cwd[MAX_INPUT];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("current directory: %s\n", cwd);
    } else {
        perror("cd@");
    }
}


void executePwdCommand() {
    char *pWd;
    char buf[MAX_INPUT];

    pWd=getcwd(buf,MAX_INPUT);
    if (pWd != NULL) {
        printf("\n %s \n\n", pWd);
    } else {
        perror("\npwd: \n"); //if unsuccess
    }
}

void processRedirection(Command *cmd){
    if (cmd==NULL) {
        fprintf(stderr, "command structure is NULL\n");
        return;
    }

    int fd;

    // Input Redirection
    if (cmd->stdin != NULL) {
        fd=open(cmd->stdin,O_RDONLY); 
        if (fd == -1) {
            perror("error open input file");
            exit(1);
        }
        if (dup2(fd,STDIN_FILENO) == -1) {
            perror("fail to redirect input");
            close(fd); //close before exit
            exit(1);
        }
        close(fd);
    }

    // Output Redirection
    if (cmd->stdout!=NULL) {
        fd = open(cmd->stdout, O_WRONLY | O_CREAT | O_APPEND, 0744);
        if (fd== -1) {
            perror("error open output file. ");
            exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) == -1) { 
            perror("Fail to redirect output");
            close(fd);
            exit(1);
        }
        close(fd);
    }

    // Error Redirection
    if (cmd->stderrout != NULL) {
        fd =open(cmd->stderrout, O_WRONLY | O_CREAT | O_TRUNC, 0744);
        if (fd == -1) {
            perror("Error open error file.");
            exit(1);
        }
        if (dup2(fd, STDERR_FILENO) == -1) {
            perror("Fail to redirect error output");
            close(fd);
            exit(1);
        }
        close(fd);
    }
}



void executePipedprocess(Command **pipedCommands, int pipeCount) {
    int i;
    int numPipes = pipeCount - 1;
    int pipes[numPipes][2];  // piping
    pid_t pid;
    pid_t pidn[pipeCount]; // store child PIDs
    int status; // status
    

    
    // Check if the last command is to be run in the background
    bool lastIsBackground = pipedCommands[pipeCount - 1]->background;

    // Create pipes
    for (i = 0; i < numPipes; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe error: ");
            exit(1);
        }
    }

    for (i = 0; i < pipeCount; i++) {
        pid = fork();
        if (pid < 0) {
            perror("fork: ");
            exit(1);
        }
        if (pid == 0) { // Child process
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) < 0) {
                    perror("dup2 stdin ");
                    exit(1);
                }
            }
            if (i < numPipes) {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2 stdout ");
                    exit(1);
                }
            }
            // Close all pipes
            for (int j = 0; j < numPipes; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            if (pipedCommands[i]->stdin != NULL || pipedCommands[i]->stdout != NULL || pipedCommands[i]->stderrout != NULL) {
                processRedirection(pipedCommands[i]);
            }

            // Execute the command
            if (execvp(pipedCommands[i]->cmdname, pipedCommands[i]->argv) < 0) {
                perror("Failed Pipeline");
                exit(0); // Exit with success to continue executing other processes
            }
        }
        pidn[i] = pid; // Store child PID
    }

    // Close all pipes in the parent
    for (i = 0; i < numPipes; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes if not in background
    if (!lastIsBackground) {
        for (i = 0; i < pipeCount; i++) {
            waitpid(pidn[i], &status, 0);
        }
    }
}

 void executeCommandprocess(Command * command){
    pid_t pid;
    int status; //waitpid
    pid=fork();
    
    
    if(pid==0){
        if ( command->stdin != NULL|| command->stdout != NULL|| command->stderrout != NULL) {
            processRedirection(command);
        }
        
        execvp(command->cmdname, command->argv);
        perror("Failed to execute command\n");
        exit(1);
    }else if (pid > 0){
        if(!command->background){
            waitpid(pid, &status, 0);
        }else {
            //printout the PID that running in the background
            //printf(" Command < %s > running in background with PID %d\n",command->cmdname,pid);
        }
    }else{
        perror("Failed to execute command\n");
    }
    
}

void executeCommand(char **prompt, Command **cmdLine,struct termios *original) {
    int i = 0;
    int pipeCount = 0;
    Command *pipedCommands[INIT_COMMANDS];  // Static array to handle to up to 100 piped commands
    while (cmdLine[i] != NULL) {
        //printf("passed command name %s\n", cmdLine[i]->cmdname);
        char *commandName = strdup(cmdLine[i]->cmdname);
        commandName = toLowerCase(commandName); // Convert command name to lowercase
        // Handle normal operations
        if (strcmp(commandName, "prompt") == 0) {
            promptChange(prompt, cmdLine[i]);
        } else if (strcmp(commandName, "cd") == 0) {
            executeCdCommand(cmdLine[i]);
        } else if (strcmp(commandName, "pwd") == 0) {
            executePwdCommand();
        } else if (strcmp(commandName, "exit") == 0) {
            disableRawMode(original);
            freeCommands(cmdLine);
            exit(0);
        } else if (cmdLine[i]->pipeto != 0) {
            // Handle piped commands
            if (pipeCount < 100) {
                pipedCommands[pipeCount] = cmdLine[i];
                pipeCount++;
            } else {
                fprintf(stderr, "Maximum pipe command limit reached\n");
                exit(1);
            }
        } else if (cmdLine[i]->pipeto == 0 && pipeCount > 0) {  // End of a pipe sequence
            if (pipeCount < 100) {
                pipedCommands[pipeCount] = cmdLine[i];
                pipeCount++;
            } else {
                fprintf(stderr, "Maximum pipe command limit reached\n");
                exit(1);
            }
            // Process piped elements
            //printf("pipeCount %i\n", pipeCount);
            //displayCommands(pipedCommands);
            executePipedprocess(pipedCommands, pipeCount);
            
            // Reset pipe handling
            memset(pipedCommands, 0, sizeof(Command*) * pipeCount);
            pipeCount = 0;
        } else {
            //printf("passed norm %i\n", i);
            // Handle background and sequence process
            executeCommandprocess(cmdLine[i]);
        }
        i++;
        free(commandName);  // Free the allocated memory for commandName
    }
}




bool isInputValid(const char *input) {
    while (*input != '\0' && isspace((unsigned char) *input)) {
        input++;
    }
    return *input != '\0';  // Returns false if input is empty or all spaces
}

void trim(char *string) 
{
    int end = strlen(string) - 1;
    while (end >= 0 && isspace((unsigned char)string[end])) {
        string[end] = '\0';
        end--;
	}
    int start = 0;
    while (string[start] != '\0' && isspace((unsigned char)string[start])) {
        start++;
    }
    if (start > 0) {
        int i = 0;
        while (string[start] != '\0') {
            string[i++] = string[start++];
        }
        string[i] = '\0';
    }
}

bool checkSequencesError(const char* input) {
	int n = strlen(input)-1;
	const char seps[] = {';' ,'|' ,'&'}; //check for initial 
    const char specialChars[] = {'&', ';', '|', '<', '>', '!', '\0'};//the '\'', '"', has different logic,hence handled somewhere else to check for validation.
    int length = strlen(input);
	for (int i = 0; i < 3; i++)
	{
		if (input[0] == seps[i])
		{
			fprintf(stderr,"cannot start with a separator re enter the command\n");
			return false;
		}
	}
	
	if (input[strlen(input)-1] == '|')
	{
		fprintf(stderr,"cannot end with a pipe re enter the command\n");
		return false;
	}
	
    for (int i = 0; i < length - 1; i++) {
        // Check if current character is a special character and is not preceded by a backslash
        if (strchr(specialChars, input[i]) && (i == 0 || input[i - 1] != backslash)) {
            // Move past spaces between special characters
            int j = i + 1;
            while (j < length && isspace(input[j])) {
                j++;
            }
            // Check if the next non-space character is also a special character and not escaped
            if (j < length && strchr(specialChars, input[j]) && (j == 0 || input[j - 1] != backslash)) {
                // Ensure the next character is not preceded by a backslash
                if (j > 0 && input[j - 1] == backslash) {
                    continue;  // Skip this itecreateCommandsration as it's escaped
                }
                fprintf(stderr, "\nError: '%c%c' found in input. Re enter the command.\n", input[i], input[j]);
                return false;
            }
        }
    }
	return true;
}

//hanldler for retrivee Command from the history
char * retrieveCommandFromHistory(char * userinput, history * hist) {
    if (userinput[0] != '!' || strlen(userinput) < 2) {
        if (strlen(userinput) == 1) {  // Handle only "!" is the  input
            if (hist->entries > 0) {
                return hist->commands[hist->entries - 1];  //Return the last command
            } else {
                printf("No commands in history.\n");
            }
        }
        return NULL;
    }else if ( isdigit(userinput[1]) && (isdigit(userinput[2]) || userinput[2] == '\0')) {   //check if it a number
        int li = atoi(userinput + 1) - 1; // Skipping the '!'
        if (li >= 0 && li < hist->entries) {
            return hist->commands[li];
        } else {
            printf("No command at the index <%d> \n", li + 1);
        }
    } else if (strcmp(userinput, "!history") == 0)
	{
		for(int i = 0; i < hist->entries; i++)
		{
			printf("%d: %s\n", i,hist->commands[i]);
		}
	}
	else {  // handle !xyz string
        for (int i = hist->entries - 1; i >= 0; i--) {
            if (strncmp(userinput + 1, hist->commands[i], strlen(userinput) - 1) == 0) {
                return hist->commands[i];
            }
        }
        printf("No matching command found for <%s>\n", userinput + 1);
    }
    return NULL;
}

void handleInput(char *userinputs, history *hist, char *prompt) {
    int ch; 
    int i = 0;

    memset(userinputs, 0, MAX_INPUT); 
    while (1) {
        clearLine();
        printf("%s %s", prompt, userinputs);
        ch = getchar();

        if (ch == 10) {
            break;
        } else if (ch == 127) { // Backspace
            if (i > 0) {
                userinputs[--i] = '\0';
                clearLine();
                printf("%s %s", prompt, userinputs);
                fflush(stdout);
            }
        } else if (ch == 27) { // Arrow keys handling
            ch = getchar(); 
            ch = getchar(); 

            if (ch == 'A') { // Up arrow
                char *temp = getPrev(hist);
                if (temp != NULL) {
                    strncpy(userinputs, temp, MAX_INPUT);
                    i = strlen(temp);
                }
            } else if (ch == 'B') { // Down arrow
                char *temp = getNext(hist);
                if (temp != NULL) {
                    strncpy(userinputs, temp, MAX_INPUT);
                    i = strlen(temp);
                }
            }
        } else if (i < MAX_INPUT - 1) { 
            userinputs[i++] = ch;
        }
        if (errno == EINTR) { // * Handle interrupted system call requ
            clearLine();
            printf("%s %s", prompt, userinputs);
            errno = 0;
            continue;
        }
    }
}
