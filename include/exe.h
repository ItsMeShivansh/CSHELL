// ############## LLM Generated Code Begins ##############
#ifndef EXE_H
#define EXE_H
#include <sys/types.h>


typedef enum {
  RUNNING,
  STOPPED
} JobState;

typedef struct {
  pid_t pid;
  pid_t pgid;
  char commandLine[1024];
  int jobID;
  JobState state;
} Job;

// Global variable to track the current foreground process group
extern volatile pid_t foreground_pgid;

const Job* getJobTable(int* cnt);

void addJob(pid_t pid, pid_t pgid, const char* commandLine, JobState state);
void addJobWithId(pid_t pid, pid_t pgid, const char* commandLine, JobState state, int jobId);
void removeJob(pid_t pid);

void reapBackgroundJobs();

void handleExecution(char** args, int arg_count);

#endif
// ############## LLM Generated Code Ends ##############