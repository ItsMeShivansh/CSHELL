// ############## LLM Generated Code Begins ##############
#ifndef BUILTIN_H
#define BUILTIN_H
#include <stdbool.h>

void handleHop(char **inputs, int inputcnt);

void handleReveal(char** args, int cnt);

void initBuiltIn();

void loadHistory();

void saveHistory();

void addToHistory(const char* command);

char* handleLog(char** args, int cnt);

#endif
// ############## LLM Generated Code Ends ##############