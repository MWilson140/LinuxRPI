#include "command.h"

Command* createCommands() {
    Command* cmd = malloc(sizeof(Command)); // Allocate memory for Command
    if (cmd == NULL) {
        exit(1);
    }
    // initialize
    cmd->cmdname = NULL;
    cmd->argv = NULL;
    cmd->stdin = NULL;
    cmd->stdout = NULL;
    cmd->stderrout = NULL;
    cmd->argc = 0;
    cmd->pipeto = 0;
    cmd->background = false;
    cmd->argv = malloc(INIT_ARGUMENTS * sizeof(char *));
    if (cmd->argv == NULL) {
        perror("Failed to allocate memory for argv");
        free(cmd); // Free the allocated Command structure if argv allocation fails
        exit(1);
    }

    return cmd; // Return the pointer to the initialized Command structure
}
//remove backslash in argv
void removeBackslashes(char* input) {
    int length = strlen(input);
    int j = 0; 
    for (int i = 0; i < length; i++) {
        if (input[i] != backslash) { 
            input[j++] = input[i];
        }
    }
    input[j] = '\0';
}


//Check for  '&' '|' and ';' 
char* checkSeparators(char* input) {
    for (int i = 0; input[i] != '\0'; i++) {
        if ((input[i] == ';' || input[i] == '&' || input[i] == '|') 
        && (i == 0 || input[i - 1] != backslash)) { //handle backslash
            return &input[i]; // Return the location of the separator
        }
    }
    return NULL;
}


void freeCommands(Command **cmdLine) {
    if (cmdLine == NULL) {
        return;
    }

    int i = 0;
    int j = 0;
    while (cmdLine[i] != NULL) {
        j=0;
        if (cmdLine[i]->cmdname != NULL) {
            //causing double free as pointer to the agrv[0] fixed using strdup instead
            free(cmdLine[i]->cmdname);
            cmdLine[i]->cmdname = NULL;
        }
        if (cmdLine[i]->argv != NULL) {
            while (cmdLine[i]->argv[j] != NULL) {
                free(cmdLine[i]->argv[j]);
                cmdLine[i]->argv[j] = NULL;
                j++;
            }
            free(cmdLine[i]->argv);
            cmdLine[i]->argv = NULL;
        }
        if (cmdLine[i]->stdin != NULL) {
            free(cmdLine[i]->stdin);
        }
        if (cmdLine[i]->stdout != NULL) {
            free(cmdLine[i]->stdout);
        }
        if (cmdLine[i]->stderrout != NULL) {
            free(cmdLine[i]->stderrout);
        }
        free(cmdLine[i]);
        i++;
    }
    free(cmdLine);
    cmdLine=NULL;
}



//strstr tokenrise the input string 
char* toToken(char* input, const char* separators) {
    // Skipping and check process while finding values
    static char* currentPosition = NULL;
    char* first = NULL;
    bool tokenStarted = false;
    bool singleQuote = false; // Ignore spaces inside "" and ''
    bool doubleQuote = false;

    if (input != NULL) {
        currentPosition = input;
    } else if (currentPosition == NULL) {
        return NULL;
    }

    for (char* p = currentPosition; *p != '\0'; p++) {
        // Handle backslash skipping
        if (*p == backslash) {
            if (*(p + 1) != '\0') {
                p++;  // Skip the next character
                continue;
            }
        }

        // Stop if the current character is a newline
        if (*p == '\n') {
            *p = '\0';  // Replace newline with null terminator to mark end of token
            currentPosition = NULL; // Indicate end of tokenization
            if (tokenStarted) {
                return first;
            } else {
                return NULL;
            }
        }

        // Toggle quote status
        if (*p == '\'' && !doubleQuote) {
            if (!tokenStarted) { 
                first = p; 
                tokenStarted = true;
            }
            singleQuote = !singleQuote;
            continue;
        }
        if (*p == '"' && !singleQuote) {
            if (!tokenStarted) {
                first = p;
                tokenStarted = true;
            }
            doubleQuote = !doubleQuote;
            continue;
        }

        // Process token
        if (strchr(separators, *p) == NULL || singleQuote || doubleQuote) {
            if (!tokenStarted) {
                first = p;  // Mark the beginning of the token
                tokenStarted = true;
            }
        } else if (tokenStarted && !singleQuote && !doubleQuote) {
            *p = '\0';
            currentPosition = p + 1;
            return first;
        }
    }

    // Handle the case where the last token ends without a separator
    if (tokenStarted) {
        currentPosition = NULL;
        return first;
    }

    currentPosition = NULL; // Set currentPosition to NULL to indicate end of parsing
    return NULL;
}


void displayCommands(Command **cmdLine) {
    printf("\nCommands Display:\n");
    int cmdc = 0;
    while (cmdLine[cmdc] != NULL) {
        printf("Command name: %s\n", cmdLine[cmdc]->cmdname);
        printf("Argument count: %d\n", cmdLine[cmdc]->argc);
        printf("Arguments:\n");
        for (int i = 0; i < cmdLine[cmdc]->argc; i++) {
            printf("  argv[%d]: %s\n", i, cmdLine[cmdc]->argv[i]);
        }
        //printf("Background: %d\n", cmdLine[cmdc]->background);
        //printf("Pipe to: %d\n", cmdLine[cmdc]->pipeto);
        //printf("Input redirection: %s\n", cmdLine[cmdc]->stdin ? cmdLine[cmdc]->stdin : "None");
        //printf("Output redirection: %s\n", cmdLine[cmdc]->stdout ? cmdLine[cmdc]->stdout : "None");
        //printf("Error redirection: %s\n", cmdLine[cmdc]->stderrout ? cmdLine[cmdc]->stderrout : "None");
        printf("\n");
        cmdc++;
    }
}




void expandWildcard(char *token, Command *command) {
    removeBackslashes(token);
    if(strpbrk(token, "*?") == NULL){
        // If no wildcard match, treat it as a nom arg
        if (command->argc >= INIT_ARGUMENTS && command->argc % EXPAND_SIZE == 0) {
            char **temp = realloc(command->argv, (command->argc + EXPAND_SIZE) * sizeof(char *));
            if (temp == NULL) {
                perror("Error reallocating memory for arguments");
                freeCommands(&command);  // Free the command structure on error
                exit(-1);
            }
            command->argv = temp;
        }
        
        command->argv[command->argc] = strdup(token);
        command->argc++;
        return;
    }
    
    
    glob_t gResult;
    memset(&gResult, 0, sizeof(gResult));//set to 0 

    int Ret = glob(token, GLOB_TILDE, NULL, &gResult);//handle * and ?
    if (Ret == 0) {
        for (size_t i = 0; i < gResult.gl_pathc; ++i) {
            if (command->argc >= INIT_ARGUMENTS && command->argc % EXPAND_SIZE == 0) {
                char **temp = realloc(command->argv, (command->argc + EXPAND_SIZE) * sizeof(char *));
                if (temp == NULL) {
                    perror("Error reallocating memory for arguments");
                    globfree(&gResult);
                    freeCommands(&command);  // Free the command structure on error
                    exit(-1);
                }
                command->argv = temp;
            }
            command->argv[command->argc] = strdup(gResult.gl_pathv[i]);
            command->argc++;
        }
    } else{
        perror("Error wildcard:");
    }
    globfree(&gResult);
}


//handle < > and 2> 
void handleRedirections(char *input, Command *command) {
    char *input_pos = strstr(input, "<");
    char *output_pos = strstr(input, ">");
    char *errorout_pos = strstr(input, "2>");

    //handle input redirection
    if (input_pos != NULL) {
        *input_pos = '\0';
        input_pos += strlen("<");
        while (*input_pos == whiteSpaceCharacters[0]) input_pos++; 
        command->stdin = strdup(input_pos);
        //printf("Input redirection %s\n", command->stdin);
    }else{
        command->stdin=NULL;
    }
    
    //output
    if (output_pos != NULL) {
        *output_pos = '\0'; 
        output_pos += strlen(">");
        while (*output_pos == whiteSpaceCharacters[0]) output_pos++; //Skip spaces after '>'

        command->stdout = strdup(output_pos);
        //printf("Output redirection %s\n ", command->stdout);
    }else{
        command->stdout=NULL;
    }
    
    //errorou
    if (errorout_pos != NULL) {
        *errorout_pos = '\0';
        errorout_pos += strlen("2>");
        while (*errorout_pos == whiteSpaceCharacters[0]) errorout_pos++;
        command->stderrout = strdup(errorout_pos);
        //printf("Error output: %s\n", command->stderrout);
    }else{
        command->stderrout=NULL;
    }
}



void processCommand(char *input, Command *command) {
    char *token = NULL;
    
    command->argc = 0; //ensure is 0

    //printf("input %s\n\n", input);
    //handle < > and 2> direction
    handleRedirections(input, command);
    
    // now i have finished the redirection section 
    //for the strstr it will not handle qoute string to skip the prelimited space allocation and backslash issue
    while ((token = toToken(input, whiteSpaceCharacters)) != NULL) {
        //wildcard handle
        expandWildcard(token, command);
        input = NULL; // Set input to NULL to continue tokenizing the same input string
    }
    
    command->cmdname = strdup(command->argv[0]);

    //printf("Command name: %s\n", command->cmdname);
    //printf("Argument count: %d\n", command->argc);
    //printf("Background: %d\n", command->background);
    //printf("Pipe: %d\n", command->pipeto);
}



Command ** processCommandLine(char *input, int news, bool background, int pipeTo) {
    static Command **cmdLine;
    char *separator = checkSeparators(input);
    char *token = NULL;
    static int cmdc = 0; // Command count for each sets
    static int cmdCaps = INIT_COMMANDS;
    bool b = false; //background
    int p = 0;      // Pipe to next cmd
    
    // Reset command count for a new sequence
    if (news == 1) {
        cmdc = 0;
        cmdLine=NULL;
        cmdLine = malloc(INIT_COMMANDS * sizeof(Command *));
        if (cmdLine == NULL) {
            perror("failed: ");
            exit(1);
        }
        
    }
    //printf("Input received: %s\n", input);

    // Find the separator ; & and |
    if (separator == NULL) { //not found then process command
        if (input != NULL && *input != '\0') {
            if (cmdc == cmdCaps) {
                cmdCaps += EXPAND_SIZE;
                Command **tmp = realloc(cmdLine, cmdCaps * sizeof(Command *));
                if (tmp == NULL) {
                    perror("Error");
                    //(cmdLine);
                    exit(1);
                }
                cmdLine = tmp;
            }
            cmdLine[cmdc]=createCommands();
            cmdLine[cmdc]->background=b;
            cmdLine[cmdc]->pipeto=p;
            processCommand(input,cmdLine[cmdc]);
            cmdc++;
        }
    } else {
        // check and if found set the background or pipe flags to its active status
        if (*separator == '&') {
            b = true;
            //printf("Found '&' separator.\n");
        } else if (*separator == '|') {
            p = cmdc + 1;
            //printf("Found '|' separator.\n");
        }

        *separator = '\0';
        token = toToken(input, "");
        if (cmdc == cmdCaps) {
            cmdCaps += EXPAND_SIZE;
            Command **tmp = realloc(cmdLine, cmdCaps * sizeof(Command *));
            if (tmp == NULL) {
                perror("Error");
                freeCommands(cmdLine);
                exit(1);
            }
            cmdLine = tmp;
        }

        cmdLine[cmdc]=createCommands();
        cmdLine[cmdc]->background = b;
        cmdLine[cmdc]->pipeto = p;
        //printf("Command %d: '%s'\n", cmdc, token);
        //printf("background: %s\n", b ? "true" : "false");
        //printf("p: %d\n", p);
        processCommand(token, cmdLine[cmdc]);
        cmdc++;
        //Process the second parts *loop
        processCommandLine(separator + 1, 0, b, p);
    }
    return cmdLine;
}
