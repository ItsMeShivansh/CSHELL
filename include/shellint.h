// ############## LLM Generated Code Begins ##############
#ifndef INTRINSICS_H
#define INTRINSICS_H
#include "exe.h"

Job* findJob(int job_id);

void waitForJob(Job* job);

void handleActivities();

void handlePing(char** args, int arg_count);

void handleFg(char** args, int arg_count);

void handleBg(char** args, int arg_count);

#endif
// ############## LLM Generated Code Ends ##############