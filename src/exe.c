// ############## LLM Generated Code Begins ##############
#include "exe.h"
#include "builtIn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>


#define MAX_JOBS 50

// volatile pid_t foreground_pgid = 0;


static Job jobTable[MAX_JOBS];
static int nextJobId = 1;

const Job* getJobTable(int* count) {
  *count = MAX_JOBS;
  return jobTable;
}

void addJob(pid_t pid, pid_t pgid, const char* cmdLine, JobState state) {
  for (int i = 0; i < MAX_JOBS; i++) {
    if (jobTable[i].pid == 0) {
      jobTable[i].pid = pid;
      jobTable[i].pgid = pgid;
      strncpy(jobTable[i].commandLine, cmdLine, sizeof(jobTable[i].commandLine) - 1);
      jobTable[i].jobID = nextJobId++;
      jobTable[i].state = state;
      return;
    }
  }
  fprintf(stderr, "Error: Maximum background jobs reached.\n");
}

void addJobWithId(pid_t pid, pid_t pgid, const char* cmdLine, JobState state, int jobId) {
  for (int i = 0; i < MAX_JOBS; i++) {
    if (jobTable[i].pid == 0) {
      jobTable[i].pid = pid;
      jobTable[i].pgid = pgid;
      strncpy(jobTable[i].commandLine, cmdLine, sizeof(jobTable[i].commandLine) - 1);
      jobTable[i].jobID = jobId;  // Use specified job ID
      jobTable[i].state = state;
      return;
    }
  }
  fprintf(stderr, "Error: Maximum background jobs reached.\n");
}

void removeJob(pid_t pid) {
  for(int i = 0; i < MAX_JOBS; i++) {
    if(jobTable[i].pid == pid) {
      jobTable[i].pid = 0; // Mark slot as empty
      jobTable[i].pgid = 0;
      jobTable[i].commandLine[0] = '\0';
      jobTable[i].jobID = 0;
      jobTable[i].state = RUNNING;
      return;
    }
  }
}

void reapBackgroundJobs() {
  int status;
  pid_t pid;
  
  for (int i = 0; i < MAX_JOBS; i++) {
    if (jobTable[i].pid != 0) {
      pid = waitpid(jobTable[i].pid, &status, WNOHANG | WUNTRACED);
      if (pid > 0) {
        if (WIFEXITED(status)) {
          printf("%s with pid %d exited normally\n", jobTable[i].commandLine, jobTable[i].pid);
          removeJob(pid);
        } else if (WIFSIGNALED(status)) {
          printf("%s with pid %d exited abnormally\n", jobTable[i].commandLine, jobTable[i].pid);
          removeJob(pid);
        } else if (WIFSTOPPED(status)) {
          jobTable[i].state = STOPPED;
        }
      }
    }
  }
}

void executeSingleCommand(char** args) {
  // Check for built-in commands: reveal, log, hop, true, exit
  if (args[0] == NULL) exit(0);

  int argsCnt = 0;
  while (args[argsCnt] != NULL) argsCnt++;
  if (strcmp(args[0], "reveal") == 0) {
    handleReveal(args, argsCnt);
    exit(0);
  }
  else if (strcmp(args[0], "hop") == 0) {
    handleHop(args, argsCnt);
    exit(0);
  }
  else if (strcmp(args[0], "true") == 0) {
    exit(0); // true always succeeds
  }
  else if (strcmp(args[0], "exit") == 0) {
    exit(0); // exit the process
  }
  else {
    execvp(args[0], args);
    printf("Command not found!\n");
    exit(EXIT_FAILURE);
  }
}

void executeCmd(char** args, int cnt, bool backgroundJob) {
  if (cnt == 0) return;

  char** commands[cnt+1]; // Array for smaller command segments deliminated by pipes
  int cmdArgCnt[cnt+1];
  for(int i=0; i<=cnt; i++) cmdArgCnt[i] = 0;
  int cmdIndex = 0;

  commands[0] = malloc((cnt+1) * sizeof(char*));
  int argIndex = 0;
  for (int i = 0; i < cnt; i++) {       // This loop stores the arguments for each command
    if (strcmp(args[i], "|") == 0) {
      commands[cmdIndex][argIndex] = NULL;
      cmdArgCnt[cmdIndex] = argIndex;
      cmdIndex++;
      commands[cmdIndex] = malloc((cnt+1) * sizeof(char*));
      argIndex = 0;
    } else {
      commands[cmdIndex][argIndex++] = args[i];
    }
  }
  commands[cmdIndex][argIndex] = NULL;
  cmdArgCnt[cmdIndex] = argIndex;
  cmdIndex++;

  // File redirection variables
  char* inputFile = NULL;
  char* outputFile = NULL;
  bool appendMode = false;

  // Check the first command for input redirection (only the last < takes effect)
  char** firstCmdClean = malloc((cnt+1) * sizeof(char*));
  int firstCleanCnt = 0;
  for (int i = 0; i < cmdArgCnt[0]; i++) {
    if (strcmp(commands[0][i], "<") == 0) {
      if (++i < cmdArgCnt[0]) {
        // Check if input file exists before proceeding
        if (access(commands[0][i], F_OK) != 0) {
          printf("No such file or directory\n");
          // Clean up and return
          for (int j = 0; j < cmdIndex; j++) {
            free(commands[j]);
          }
          free(firstCmdClean);
          return;
        }
        inputFile = commands[0][i]; // Only last input redirection takes effect
      }
    } else {
      firstCmdClean[firstCleanCnt++] = commands[0][i];
    }
  }
  firstCmdClean[firstCleanCnt] = NULL;
  free(commands[0]);
  commands[0] = firstCmdClean;
  cmdArgCnt[0] = firstCleanCnt;

  // Check the last command for output redirection (only the last > or >> takes effect)
  if (cmdIndex > 0) {
    int lastCmdIndex = cmdIndex - 1;
    char** lastCmdClean = malloc((cnt+1) * sizeof(char*));
    int lastCleanCnt = 0;
    for (int i = 0; i < cmdArgCnt[lastCmdIndex]; i++) {
      if (strcmp(commands[lastCmdIndex][i], ">") == 0) {
        if (++i < cmdArgCnt[lastCmdIndex]) {
          // Test if we can create/write to the file
          int test_fd = open(commands[lastCmdIndex][i], O_WRONLY | O_CREAT | O_TRUNC, 0644);
          if (test_fd < 0) {
            printf("Unable to create file for writing\n");
            // Clean up and return
            for (int j = 0; j < cmdIndex; j++) {
              free(commands[j]);
            }
            free(lastCmdClean);
            return;
          }
          close(test_fd);
          outputFile = commands[lastCmdIndex][i]; // Only last output redirection takes effect
          appendMode = false;
        }
      } else if (strcmp(commands[lastCmdIndex][i], ">>") == 0) {
        if (++i < cmdArgCnt[lastCmdIndex]) {
          // Test if we can create/write to the file
          int test_fd = open(commands[lastCmdIndex][i], O_WRONLY | O_CREAT | O_APPEND, 0644);
          if (test_fd < 0) {
            printf("Unable to create file for writing\n");
            // Clean up and return
            for (int j = 0; j < cmdIndex; j++) {
              free(commands[j]);
            }
            free(lastCmdClean);
            return;
          }
          close(test_fd);
          outputFile = commands[lastCmdIndex][i]; // Only last output redirection takes effect
          appendMode = true;
        }
      } else {
        lastCmdClean[lastCleanCnt++] = commands[lastCmdIndex][i];
      }
    }
    lastCmdClean[lastCleanCnt] = NULL;
    free(commands[lastCmdIndex]);
    commands[lastCmdIndex] = lastCmdClean;
    cmdArgCnt[lastCmdIndex] = lastCleanCnt;
  }

  // Main pipeline execution logic
  int numPipes = cmdIndex - 1;
  int pipes[numPipes][2];
  pid_t pids[cmdIndex];
  pid_t pgid = 0;

  //  Creating pipes (return if error in creating a pipeline)
  for (int i = 0; i < numPipes; i++) {
    if (pipe(pipes[i]) < 0) {
      perror("pipe failed");
      return;
    }
  }

  // Forking processes for each command in the pipeline
  for (int i = 0; i < cmdIndex; i++) {
    pids[i] = fork();
    if (pids[i] < 0) {
      perror("fork failed");
      return;
    }

    if (pids[i] == 0) {
      // --- Child Process ---

      // Reset signal handlers to default behavior
      signal(SIGINT, SIG_DFL);
      signal(SIGTSTP, SIG_DFL);
      signal(SIGTTIN, SIG_DFL);
      signal(SIGTTOU, SIG_DFL);

      // Set process group for all children to pgid of first child
      if (i == 0) {
        setpgid(0, 0); // setpgid(child, child)
      } else {
        setpgid(0, pids[0]);
      }

      // For foreground jobs, give terminal control to the process group
      // if (!backgroundJob && i == 0) {
      //   tcsetpgrp(STDIN_FILENO, getpid());
      // }

      // Handle input: from inputFile or previous pipe
      if (i == 0) { // If first command
        if (inputFile) {
          int fd = open(inputFile, O_RDONLY);
          if (fd < 0) { 
            printf("No such file or directory\n"); 
            exit(1); 
          }
          dup2(fd, STDIN_FILENO);
          close(fd);
        }
      } else {
        dup2(pipes[i - 1][0], STDIN_FILENO);
      }

      // Handle output: to outputFile or next pipe
      if (i == cmdIndex - 1) { // If last command
        if (outputFile) {
          int flags = O_WRONLY | O_CREAT | (appendMode ? O_APPEND : O_TRUNC);
          int fd = open(outputFile, flags, 0644);
          if (fd < 0) { 
            printf("Unable to create file for writing\n"); 
            exit(1); 
          }
          dup2(fd, STDOUT_FILENO);
          close(fd);
        }
      } else { 
        dup2(pipes[i][1], STDOUT_FILENO);
      }

      // Close all pipe ends in the child
      for (int j = 0; j < numPipes; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }
      
      // If background job, detach from terminal input
      if (backgroundJob) {
        int devNull = open("/dev/null", O_RDONLY);
        dup2(devNull, STDIN_FILENO);
        close(devNull);
      }
      executeSingleCommand(commands[i]);
    } else {
      // --- Parent Process ---
      // Set process group in parent as well to avoid race conditions
      if (i == 0) {
        pgid = pids[0];
        setpgid(pids[0], pids[0]); // Parent sets child's process group
      } else {
        setpgid(pids[i], pgid); // Parent sets child's process group to match first child
      }
    }
  }

  // --- Parent Process ---
  // Close all pipe ends in the parent
  for (int i = 0; i < numPipes; i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }

  if (backgroundJob) {
    // Store the job with pgid and full command line
    char fullCmd[1024] = "";
    for (int i = 0; i < cnt; i++) {
      strcat(fullCmd, args[i]);
      if (i < cnt - 1) strcat(fullCmd, " ");
    }
    addJob(pids[0], pgid, fullCmd, RUNNING);
    
    // Find the job that was just added to get its job ID
    int jobCnt = 0;
    const Job* jobTable = getJobTable(&jobCnt);
    int jobID = -1;
    for (int i = 0; i < jobCnt; i++) {
      if (jobTable[i].pid == pids[0]) {
        jobID = jobTable[i].jobID;
        break;
      }
    }
    printf("[%d] %d\n", jobID, pids[0]); // Print background job info with correct job ID
    
    // Do not wait for children, just free memory
    for (int i = 0; i < cmdIndex; i++) {
      free(commands[i]);
    }
    return;
  }
  
  // Set foreground process group for signal handling
  foreground_pgid = pgid;
  
  // Give terminal control to the foreground process group
  tcsetpgrp(STDIN_FILENO, pgid);
  
  // -- Replace the entire waiting/cleanup block in the parent process with this --

  // 1. WAIT FOR THE ENTIRE PIPELINE TO COMPLETE OR STOP
  // int status = 0;
  int stopped = 0;

  // This loop waits for every child process that was forked.
  for (int i = 0; i < cmdIndex; i++) {
    int child_status;
    pid_t waited_pid = waitpid(pids[i], &child_status, WUNTRACED);

    // If any process in the job is stopped, we mark the entire job as stopped.
    if (waited_pid > 0 && WIFSTOPPED(child_status)) {
      stopped = 1;
    }
  }

  // 2. NOW THAT THE JOB IS FINISHED, RESTORE THE SHELL'S TERMINAL CONTROL
  if (!backgroundJob) {
    // signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(STDIN_FILENO, getpgrp());
    // signal(SIGTTOU, SIG_IGN);
    foreground_pgid = 0;
  }

  // 3. Free the memory used by the command segments
  for (int i = 0; i < cmdIndex; i++) {
    free(commands[i]);
  }

  // 4. NOW, HANDLE THE "STOPPED" STATE IF NECESSARY
  // This part only runs after the shell is safely in the foreground.
  if (stopped) {
    char fullCmd[1024] = "";
    for (int i = 0; i < cnt; i++) {
      strcat(fullCmd, args[i]);
      if (i < cnt - 1) strcat(fullCmd, " ");
    }
    addJob(pids[0], pgid, fullCmd, STOPPED);

    int jobCnt = 0;
    const Job* jobTable = getJobTable(&jobCnt);
    int jobID = -1;
    for (int i = 0; i < jobCnt; i++) {
      if (jobTable[i].pid == pids[0]) {
        jobID = jobTable[i].jobID;
        break;
      }
    }
    printf("[%d] Stopped %s\n", jobID, fullCmd);
  }
}

void handleExecution(char** args, int cnt) {
  if (cnt == 0) return;
  bool backgroundJob = false;

  int writeIdx = 0;
  for (int i = 0; i < cnt; i++) {
    if (strcmp(args[i], "&") == 0) {
      args[writeIdx] = NULL;
      if (writeIdx > 0) {
        backgroundJob = true;
        executeCmd(args, writeIdx, backgroundJob);
      }
      writeIdx = 0;
      backgroundJob = false;
    } else {
      args[writeIdx++] = args[i];
    }
  }
  if (writeIdx > 0) {
    args[writeIdx] = NULL;
    executeCmd(args, writeIdx, backgroundJob);
  }
}
// ############## LLM Generated Code Ends ##############