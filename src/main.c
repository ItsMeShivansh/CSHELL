// ############## LLM Generated Code Begins ##############
#include "shellprompt.h"
#include "shellint.h"
#include "exe.h"
#include "parser.h"
#include "builtIn.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <sys/wait.h>
// Track the current foreground process group
volatile pid_t foreground_pgid = 0;

void sigint_handler(int signo) {
  if (foreground_pgid > 0) {
    kill(-foreground_pgid, SIGINT);
  }
}

void sigtstp_handler(int signo) {
  if (foreground_pgid > 0) {
    kill(-foreground_pgid, SIGTSTP);
  }
}

void kill_all_jobs_and_exit() {
  int jobCnt = 0;
  const Job* jobTable = getJobTable(&jobCnt);
  for (int i = 0; i < jobCnt; i++) {
    if (jobTable[i].pid != 0) {
      kill(-jobTable[i].pgid, SIGKILL);
    }
  }
  printf("logout\n");
  exit(0);
}

void processCommand(char* command_line);

int main() {
  initShell();
  loadHistory();

  // Initialize shell for job control
  // Put shell in its own process group
  setpgid(0, 0);
  
  // Take control of the terminal
  tcsetpgrp(STDIN_FILENO, getpgrp());
  
  // Ignore certain signals for the shell itself
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);

  // Set up signal handlers for foreground processes
  struct sigaction sa_int, sa_tstp;
  sa_int.sa_handler = sigint_handler;
  sigemptyset(&sa_int.sa_mask);
  sa_int.sa_flags = SA_RESTART;
  sigaction(SIGINT, &sa_int, NULL);

  sa_tstp.sa_handler = sigtstp_handler;
  sigemptyset(&sa_tstp.sa_mask);
  sa_tstp.sa_flags = SA_RESTART;
  sigaction(SIGTSTP, &sa_tstp, NULL);

  char *inputstr = NULL;
  size_t length = 0;

  while(1) {
    reapBackgroundJobs();

    displayPrompt();
    if (getline(&inputstr, &length, stdin) == -1) {
      // EOF (Ctrl-D)
      kill_all_jobs_and_exit();
    }
    inputstr[strcspn(inputstr, "\n")] = 0;

    addToHistory(inputstr);

    processCommand(inputstr);
  }

  saveHistory();
  free(inputstr);
  return 0;
}

void processCommand(char* commandLine) {

  char *lineCopy = strdup(commandLine);
  
  // Split commands by semicolons, but respect quotes
  char **commands = malloc(100 * sizeof(char*));
  int commandCount = 0;
  
  char *currentCmd = malloc(strlen(commandLine) + 1);
  int cmdLen = 0;
  int inQuotes = 0;
  
  for (int i = 0; commandLine[i] != '\0'; i++) {
    char c = commandLine[i];
    if (c == '"') {
      inQuotes = !inQuotes;
      currentCmd[cmdLen++] = c;
    } else if (c == ';' && !inQuotes) {
      // End current command
      currentCmd[cmdLen] = '\0';
      commands[commandCount++] = strdup(currentCmd);
      cmdLen = 0;
    } else {
      currentCmd[cmdLen++] = c;
    }
  }
  
  // Add the last command
  if (cmdLen > 0) {
    currentCmd[cmdLen] = '\0';
    commands[commandCount++] = strdup(currentCmd);
  }
  
  free(currentCmd);

  // Process each command
  for (int cmdIdx = 0; cmdIdx < commandCount; cmdIdx++) {
    char *command = commands[cmdIdx];
    
    // Trim leading/trailing whitespace
    while (*command == ' ' || *command == '\t') command++;
    int len = strlen(command);
    while (len > 0 && (command[len-1] == ' ' || command[len-1] == '\t')) {
      command[--len] = '\0';
    }
    
    if (strlen(command) == 0) continue;
    
    if (!validateInput(command)) {
      printf("Invalid Syntax!\n");
      break;
    }

    // Tokenize the command with quote handling
    char **tokens = malloc(100 * sizeof(char*));
    int tokenCnt = 0;
    char currArg[1024];
    int currLen = 0;
    inQuotes = 0;
    for (int i = 0; command[i] != '\0'; i++) {
      char c = command[i];
      if (c == '"') {
        inQuotes = !inQuotes;
      } else if ((c == ' ' || c == '\t') && !inQuotes) {
        if (currLen > 0) {
          currArg[currLen] = '\0';
          tokens[tokenCnt++] = strdup(currArg);
          currLen = 0;
        }
      } else {
        currArg[currLen++] = c;
      }
    }
    if (currLen > 0) {
      currArg[currLen] = '\0';
      tokens[tokenCnt++] = strdup(currArg);
    }
    tokens[tokenCnt] = NULL;

    // Command Dispatcher
    if (tokenCnt > 0) {
      if (strcmp(tokens[0], "hop") == 0) {
        handleHop(tokens, tokenCnt);
      }
      else if (strcmp(tokens[0], "log") == 0) {
        char* commandToExecute = handleLog(tokens, tokenCnt);
        if (commandToExecute != NULL) {
          processCommand(commandToExecute);
          free(commandToExecute);
        }
      }
      else if (strcmp(tokens[0], "activities") == 0) {
        handleActivities();
      }
      else if (strcmp(tokens[0], "ping") == 0) {
        handlePing(tokens, tokenCnt);
      }
      else if (strcmp(tokens[0], "fg") == 0) {
        handleFg(tokens, tokenCnt);
      }
      else if (strcmp(tokens[0], "bg") == 0) {
        handleBg(tokens, tokenCnt);
      }
      else if (strcmp(tokens[0], "true") == 0) {
        // true command does nothing and always succeeds
      }
      else {
        handleExecution(tokens, tokenCnt);
      }
    }
    
    // Free tokens for this command
    for (int i = 0; i < tokenCnt; i++) {
      free(tokens[i]);
    }
    free(tokens);
  }
  
  // Free command array
  for (int i = 0; i < commandCount; i++) {
    free(commands[i]);
  }
  free(commands);
  free(lineCopy);
}
// ############## LLM Generated Code Ends ##############