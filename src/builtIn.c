// ############## LLM Generated Code Begins ##############
#include "builtIn.h"
#include "exe.h"
#include "shellprompt.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>

#define MAX_HISTORY 15
#define MAX_CMDSIZE 1024

static char prevWD[PATH_MAX];
static bool prevWDSet = false;

static char* cmdHistory[MAX_HISTORY];
static int historyCnt = 0;
static int historyStart = 0;

static char historyFilePath[PATH_MAX];

void setupHistoryPath() {
  const char* homeDIR = getHomeDir();
  snprintf(historyFilePath, sizeof(historyFilePath), "%s/.shell_history", homeDIR);
}

static int cmp(const void *a, const void *b) {
  const char *str1 = *(const char **)a;
  const char *str2 = *(const char **)b;
  return strcmp(str1, str2); // ASCII-based lexicographic comparison
}

void initBuiltIn() {
  if(!getcwd(prevWD, sizeof(prevWD))) perror("getcwd");
  prevWDSet = false; // Initially not set
}
 
void handleHop(char** inputs, int inputcnt) {

  if(inputcnt == 1) {
    chdir(getHomeDir());
    return;
  }
  for(int i=1 ; i<inputcnt ; i++) {
    char* input = inputs[i];
    char currDir[PATH_MAX];
    bool success = false;

    if(!getcwd(currDir, sizeof(currDir))) {
      perror("hop: getcwd error");
      return;
    }

    if(strcmp(input, "~") == 0) {
      if(chdir(getHomeDir()) == 0) success = true;
    }
    else if(strcmp(input, "-") == 0) {
      if(prevWDSet) {
        if(chdir(prevWD) == 0) success = true;
      }
      else success = true;
    }
    else if(strcmp(input, ".") == 0) {
      success = true;
    }
    else if(strcmp(input, "..") == 0) {
      if (chdir("..") == 0) success = true;
    }
    else {
      if(chdir(input) == 0) success = true;
    }

    if (success) {
      strncpy(prevWD, currDir, sizeof(prevWD));
      prevWDSet = true;
    } else {
      printf("No such directory!\n");
      return;
    }
  }
}

void handleReveal(char** args, int cnt) {
  bool showHidden = false;
  bool newLine = false;
  char* path = NULL;
  int pathCount = 0;

  for (int i = 1; i < cnt; i++) {
    if (args[i][0] == '-' && strlen(args[i]) > 1) {
      // It's a flag argument, check for 'a' and 'l'
      for (int j = 1; args[i][j] != '\0'; j++) {
        if (args[i][j] == 'a') showHidden = true;
        if (args[i][j] == 'l') newLine = true;
      }
    } else {
      // It's a path argument (including standalone "-")
      path = args[i];
      pathCount++;
    }
  }

  // Check for too many arguments (more than one path)
  if (pathCount > 1) {
    printf("reveal: Invalid Syntax!\n");
    return;
  }

  char targetDir[PATH_MAX];
  if (!path || strcmp(path, ".") == 0) {
    getcwd(targetDir, sizeof(targetDir));
  }
  else if (strcmp(path, "~") == 0) {
    strncpy(targetDir, getHomeDir(), sizeof(targetDir));
  }
  else if (strcmp(path, "..") == 0) {
    getcwd(targetDir, sizeof(targetDir));
    strcat(targetDir, "/..");
  }
  else if (strcmp(path, "-") == 0) {
    if (prevWDSet) {
      strncpy(targetDir, prevWD, sizeof(targetDir));
    } else {
      printf("No such directory!\n");
      return;
    }
  }
  else {
    strncpy(targetDir, path, sizeof(targetDir));
  }

  DIR *d = opendir(targetDir);
  if (d == NULL) {
    printf("No such directory!\n");
    return;
  }

  struct dirent *dir;
  char **entries = malloc(1024 * sizeof(char*)); // Buffer for directory entries
  int count = 0;
  while ((dir = readdir(d)) != NULL) {
    entries[count++] = strdup(dir->d_name);
  }
  closedir(d);

  // Sort the entries lexicographically (ASCII values)
  qsort(entries, count, sizeof(char*), cmp);

  // Print each entry in new line if -l is specified, else space-separated
  for (int i = 0; i < count; i++) {
    if (!showHidden && entries[i][0] == '.') {
      continue;
    }
    newLine ? printf("%s\n", entries[i]) : printf("%s ", entries[i]);
  }
  if(!newLine) printf("\n");

  // Free allocated memory
  for (int i = 0; i < count; i++) {
    free(entries[i]);
  }
  free(entries);
}

void loadHistory() {
  setupHistoryPath();
  FILE* file = fopen(historyFilePath, "r");
  if (!file) return;

  char line[MAX_CMDSIZE];
  while (fgets(line, sizeof(line), file) != NULL) {
    line[strcspn(line, "\n")] = 0;
    addToHistory(line);
  }
  fclose(file);
}

void saveHistory() {
  FILE* file = fopen(historyFilePath, "w");
  if (!file) return;

  for (int i = 0; i < historyCnt; i++) {
    int index = (historyStart + i) % MAX_HISTORY;
    fprintf(file, "%s\n", cmdHistory[index]);
  }
  fclose(file);
}

void addToHistory(const char* commandLine) {
  // Don't store 'log' commands
  if (strncmp(commandLine, "log", 3) == 0) return;

  // Don't store if identical to the previous command
  if (historyCnt > 0) {
    int last_index = (historyStart + historyCnt - 1) % MAX_HISTORY;
    if (strcmp(cmdHistory[last_index], commandLine) == 0) return;
  }

  // Add the command
  if (historyCnt < MAX_HISTORY) {
    int next_index = (historyStart + historyCnt) % MAX_HISTORY;
    cmdHistory[next_index] = strdup(commandLine);
    historyCnt++;
  } else {
    // Overwrite the oldest command (circular buffer)
    free(cmdHistory[historyStart]);
    cmdHistory[historyStart] = strdup(commandLine);
    historyStart = (historyStart + 1) % MAX_HISTORY;
  }
}

char* handleLog(char** args, int cnt) {
  if (cnt == 1) {
    // No arguments: Print history
    for (int i = 0; i < historyCnt; i++) {
      int index = (historyStart + i) % MAX_HISTORY;
      printf("%s\n", cmdHistory[index]);
    }
  } else if (strcmp(args[1], "purge") == 0) {
    // Purge: Clear history
    for (int i = 0; i < historyCnt; i++) {
      free(cmdHistory[(historyStart + i) % MAX_HISTORY]);
    }
    historyCnt = 0;
    historyStart = 0;
    remove(historyFilePath); // Delete the file
  } else if (strcmp(args[1], "execute") == 0) {
    if (cnt < 3) {
      fprintf(stderr, "log: execute requires an index\n");
      return NULL;
    }
    int index = atoi(args[2]);
    if (index < 1 || index > historyCnt) {
      fprintf(stderr, "log: invalid history index\n");
      return NULL;
    }
    // Newest to oldest: index 1 is the last command
    int actual_index = (historyStart + historyCnt - index) % MAX_HISTORY;
    return strdup(cmdHistory[actual_index]);
  }
  return NULL;
}
// ############## LLM Generated Code Ends ##############
