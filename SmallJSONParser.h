#ifndef __SMALL_JSON_PARSER_H__
#define __SMALL_JSON_PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define StringObjectJSONToken 1
#define NumberObjectJSONToken 2
#define StartObjectJSONToken 3
#define EndObjectJSONToken 4
#define StartArrayJSONToken 5
#define EndArrayJSONToken 6
#define TrueJSONToken 7
#define FalseJSONToken 8
#define NullJSONToken 9
#define OutOfDataJSONToken 10
#define ParseErrorJSONToken 11

#define JSONTokenFlagMask 0xc000
#define PartialJSONTokenFlag 0x8000
#define TruncatedJSONTokenFlag 0x4000

typedef struct JSONParser
{
	const char *currentbyte,*end;
	int state,partialtokentype;
} JSONParser;

typedef struct JSONToken
{
	unsigned int typeandflags;
	const char *start,*end;
} JSONToken;

// Minimal API.

void InitialiseJSONParser(JSONParser *self);
void ProvideJSONInput(JSONParser *self,const char *bytes,size_t length);
JSONToken NextJSONToken(JSONParser *self);

static inline int JSONTokenType(JSONToken token) { return token.typeandflags&~JSONTokenFlagMask; }
static inline bool IsJSONTokenPartial(JSONToken token) { return token.typeandflags&PartialJSONTokenFlag; }

// Extended API.

typedef bool JSONInputProviderCallbackFunction(JSONParser *parser,void *context);

JSONToken NextJSONTokenWithInputProvider(JSONParser *self,char *tokenbuffer,size_t buffersize,
JSONInputProviderCallbackFunction *callback,void *context);

static inline bool IsJSONTokenTruncated(JSONToken token) { return token.typeandflags&TruncatedJSONTokenFlag; }

#endif
