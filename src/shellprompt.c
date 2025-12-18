// ############## LLM Generated Code Begins ##############
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<limits.h>
#include<pwd.h>
#include "builtIn.h"

static char homeDir[PATH_MAX];

void initShell() {
  if(getcwd(homeDir, sizeof(homeDir)) == NULL) perror("getcwd");
  initBuiltIn();
}

const char* getHomeDir() {
  return homeDir;
}

void displayPrompt() {
  char hostName[_SC_HOST_NAME_MAX];
  char currentWorkingDir[PATH_MAX];
  char displayName[PATH_MAX];
  struct passwd *pw = getpwuid(getuid());
  char *userName = pw ? pw->pw_name : "user";

  if(gethostname(hostName, sizeof(hostName)) != 0) strncpy(hostName, "unknown", sizeof(hostName));

  if(getcwd(currentWorkingDir, sizeof(currentWorkingDir)) == NULL) perror("getcwd");

  if(strncmp(currentWorkingDir, homeDir, strlen(homeDir)) == 0) {
    strcpy(displayName, "~");
    strcat(displayName, currentWorkingDir + strlen(homeDir));
  }
  else {
    strcpy(displayName, currentWorkingDir);
  }
  printf("<%s@%s:%s> ", userName, hostName, displayName);
  fflush(stdout);
}
// ############## LLM Generated Code Ends ##############