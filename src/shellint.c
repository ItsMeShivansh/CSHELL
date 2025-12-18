// ############## LLM Generated Code Begins ##############
#include "exe.h"
#include "shellint.h"
#include "builtIn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>


static int compareJobs(const void *a, const void *b) {
  const Job *jobA = (const Job *)a;
  const Job *jobB = (const Job *)b;
  return strcmp(jobA->commandLine, jobB->commandLine);
}

void handleActivities() {
  int jobCnt = 0;
  const Job* jobTable = getJobTable(&jobCnt);

  // Copy active jobs into a temporary array for sorting
  Job activeJobs[jobCnt];
  int activeCnt = 0;
  for (int i = 0; i < jobCnt; i++) {
    if (jobTable[i].pid != 0) {
      activeJobs[activeCnt++] = jobTable[i];
    }
  }

  if (activeCnt == 0) {
    return; // Nothing to print
  }

  // Sort the active jobs by command name
  qsort(activeJobs, activeCnt, sizeof(Job), compareJobs);

  for (int i = 0; i < activeCnt; i++) {
    const char* stateStr = (activeJobs[i].state == RUNNING) ? "Running" : "Stopped";
    printf("[%d] : %s = %s\n", activeJobs[i].pid, activeJobs[i].commandLine, stateStr);
  }
}

// Find job by jobID (returns pointer to job in jobTable, or NULL)
Job* findJob(int job_id) {
  int jobCnt = 0;
  const Job* jobTable = getJobTable(&jobCnt);
  for (int i = 0; i < jobCnt; i++) {
    if (jobTable[i].pid != 0 && jobTable[i].jobID == job_id) {
      // Cast away const for modification
      return (Job*)&jobTable[i];
    }
  }
  return NULL;
}

// Wait for a job to finish or stop
void waitForJob(Job* job) {
  int status;
  if (!job) return;
  while (1) {
    pid_t pid = waitpid(job->pid, &status, WUNTRACED);
    if (pid < 0) break;
    if (WIFSTOPPED(status)) {
      job->state = STOPPED;
      break;
    } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
      job->pid = 0;
      break;
    }
  }
}

void handlePing(char** args, int arg_count) {
  if (arg_count < 3) {
    printf("Usage: ping <pid> <signal_number>\n");
    return;
  }
  pid_t pid = atoi(args[1]);
  int sig = atoi(args[2]) % 32;
  if (sig <= 0) {
    printf("Invalid syntax!\n");
    return;
  }
  if (kill(pid, sig) == -1) {
    printf("No such process found");
  }
  else printf("Sent signal %d to process with pid %d\n", sig, pid);
}

// Helper: find most recent job (highest jobID)
static Job* findMostRecentJob() {
  int jobCnt = 0;
  const Job* jobTable = getJobTable(&jobCnt);
  Job* best = NULL;
  for (int i = 0; i < jobCnt; i++) {
    if (jobTable[i].pid != 0) {
      if (!best || jobTable[i].jobID > best->jobID) {
        best = (Job*)&jobTable[i];
      }
    }
  }
  return best;
}

void handleFg(char** args, int arg_count) {
  Job* job = NULL;
  if (arg_count == 1) {
    job = findMostRecentJob();
  } else {
    int job_id = atoi(args[1]);
    job = findJob(job_id);
  }
  if (!job || job->pid == 0) {
    printf("No such job\n");
    return;
  }
  printf("%s\n", job->commandLine);
  
  // Store job info before removing it
  pid_t job_pid = job->pid;
  pid_t job_pgid = job->pgid;
  int original_job_id = job->jobID;
  char job_cmd[1024];
  strcpy(job_cmd, job->commandLine);
  
  // If stopped, send SIGCONT
  if (job->state == STOPPED) {
    kill(-job->pgid, SIGCONT);
  }
  
  // Remove job from background list
  removeJob(job->pid);
  
  // Set terminal control to job
  foreground_pgid = job_pgid;
  tcsetpgrp(STDIN_FILENO, job_pgid);
  
  // Wait for the process group with WUNTRACED option
  int status;
  pid_t waited_pid = waitpid(-job_pgid, &status, WUNTRACED);
  
  // Restore terminal control to shell
  tcsetpgrp(STDIN_FILENO, getpgrp());
  foreground_pgid = 0;
  
  // Check if job was stopped and add it back with the same job ID
  if (waited_pid > 0 && WIFSTOPPED(status)) {
    addJobWithId(job_pid, job_pgid, job_cmd, STOPPED, original_job_id);
    printf("[%d] Stopped %s\n", original_job_id, job_cmd);
  }
}

void handleBg(char** args, int arg_count) {
  Job* job = NULL;
  if (arg_count == 1) {
    job = findMostRecentJob();
  } else {
    int job_id = atoi(args[1]);
    job = findJob(job_id);
  }
  if (!job || job->pid == 0) {
    printf("No such job\n");
    return;
  }
  if (job->state == RUNNING) {
    printf("Job already running\n");
    return;
  }
  // Only stopped jobs can be resumed
  kill(-job->pgid, SIGCONT);
  job->state = RUNNING;
  printf("[%d] %s &\n", job->jobID, job->commandLine);
}
// ############## LLM Generated Code Ends ##############

