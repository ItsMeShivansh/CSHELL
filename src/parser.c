// ############## LLM Generated Code Begins ##############
#include"parser.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>

int tokenize(const char* str, Token *tokens, int maxTokens) {
  int i = 0;
  int j = 0;
  while(str[i] && j < maxTokens) {
    if(isspace(str[i])) {
      i++;
      continue;
    }
    else if(str[i] == '|') {
      tokens[j].type = PIPE;
      tokens[j].value[0] = '|';
      tokens[j].value[1] = '\0';
      j++;
      i++;
    }
    else if(str[i] == '&') {
      if(str[i+1] == '&') {
        tokens[j].type = ANDAND;
        tokens[j].value[0] = '&';
        tokens[j].value[1] = '&';
        tokens[j].value[2] = '\0';
        j++;
        i += 2;
      } else {
        tokens[j].type = AND;
        tokens[j].value[0] = '&';
        tokens[j].value[1] = '\0';
        j++;
        i++;
      }
    }
    else if(str[i] == '<') {
      tokens[j].type = INPUT;
      tokens[j].value[0] = '<';
      tokens[j].value[1] = '\0';
      j++;
      i++;
    }
    else if(str[i] == '>') {
      if(str[i+1] == '>') {
        tokens[j].type = OUTPUT_APPEND;
        tokens[j].value[0] = '>';
        tokens[j].value[1] = '>';
        tokens[j].value[2] = '\0';
        j++;
        i += 2;
      } else {
        tokens[j].type = OUTPUT;
        tokens[j].value[0] = '>';
        tokens[j].value[1] = '\0';
        j++;
        i++;
      }
    }
    else if(str[i] == '\n' || str[i] == ';') {
      tokens[j].type = END;
      if (str[i] == '\n')
        tokens[j].value[0] = '\n';
      else
        tokens[j].value[0] = ';';
      tokens[j].value[1] = '\0';
      j++;
      i++;
    }
    else {
      int k = 0;
      while (str[i] && !isspace(str[i]) && strchr("|&><;", str[i]) == NULL && k < 1023)
        tokens[j].value[k++] = str[i++];
      tokens[j].value[k] = '\0';
      tokens[j].type = NAME;
      j++;
    }
  }
  tokens[j].type = END;
  tokens[j].value[0] = '\0';
  return j;
}

int parseShellCmd(Token *tokens, int *pos, int maxTokens) {
  if(!parseCmdGroup(tokens, pos, maxTokens)) return 0;
  while(tokens[*pos].type == AND || tokens[*pos].type == ANDAND) {
    if(tokens[*pos].type == AND) {
      (*pos)++;
      if(!parseCmdGroup(tokens, pos, maxTokens)) {
        break;
      }
    }
    else {
      (*pos)++;
      if(!parseCmdGroup(tokens, pos, maxTokens)) return 0;
    }
  }
  if(tokens[*pos].type == AND) *pos += 1;
  return 1;
}

int parseCmdGroup(Token *tokens, int *pos, int maxTokens) {
  if(!parseAtomic(tokens, pos, maxTokens)) return 0;
  while(tokens[*pos].type == PIPE) {
    (*pos)++;
    if(!parseAtomic(tokens, pos, maxTokens)) return 0;
  }
  if(tokens[*pos].type == PIPE) *pos += 1;
  return 1;
}

int parseAtomic(Token *tokens, int *pos, int maxTokens) {
  if(tokens[*pos].type == END) return 0;
  if(!parseName(tokens, pos, maxTokens)) return 0;
  while (1) {
    int prevpos = *pos;
    parseName(tokens, pos, maxTokens);
    parseInput(tokens, pos, maxTokens);
    parseOutput(tokens, pos, maxTokens);
    if (*pos == prevpos) break;
  }
  return 1;
}

int parseInput(Token *tokens, int *pos, int maxTokens) {
  if(tokens[*pos].type == INPUT) {
    (*pos)++;
    if(!parseName(tokens, pos, maxTokens)) return 0;
  }
  return 1;
}

int parseOutput(Token *tokens, int *pos, int maxTokens) {
  if(tokens[*pos].type == OUTPUT || tokens[*pos].type == OUTPUT_APPEND) {
    (*pos)++;
    if(!parseName(tokens, pos, maxTokens)) return 0;
  }
  return 1;
}

int parseName(Token *tokens, int *pos, int maxTokens) {
  if(tokens[*pos].type == NAME) {
    (*pos)++;
    return 1;
  }
  return 0;
}

int validateInput(char *input) {
  Token tokens[100];
  tokenize(input, tokens, 100);
  int pos = 0;
  int valid = parseShellCmd(tokens, &pos, 100);
  return valid && tokens[pos].type == END;
}
// ############## LLM Generated Code Ends ##############

