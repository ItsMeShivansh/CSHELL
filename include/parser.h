// ############## LLM Generated Code Begins ##############
#ifndef PARSER_H
#define PARSER_H

typedef enum {
  NAME, PIPE, AND, ANDAND, INPUT, OUTPUT,
  OUTPUT_APPEND, END, INVALID
} TokenType;

typedef struct Token {
  TokenType type;
  char value[1024];
} Token;

int validateInput(char *str);
int parseShellCmd(Token *tokens, int *pos, int maxTokens);
int parseCmdGroup(Token *tokens, int *pos, int maxTokens);
int parseAtomic(Token *tokens, int *pos, int maxTokens);
int parseInput(Token *tokens, int *pos, int maxTokens);
int parseOutput(Token *tokens, int *pos, int maxTokens);
int parseName(Token *tokens, int *pos, int maxTokens);

#endif
// ############## LLM Generated Code Ends ##############
