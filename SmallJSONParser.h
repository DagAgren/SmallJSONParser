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

// Pull API.

typedef bool JSONInputProviderCallbackFunction(JSONParser *parser,void *context);

typedef struct JSONProvider {
	JSONInputProviderCallbackFunction *callback;
	void *context;
	char *buffer;
	size_t buffersize;
} JSONProvider;

static inline void InitialiseJSONProvider(JSONProvider *self,
JSONInputProviderCallbackFunction *callback,void *context,char *buffer,size_t buffersize)
{
	self->callback=callback;
	self->context=context;
	self->buffer=buffer;
	self->buffersize=buffersize;
}

JSONToken NextJSONTokenWithProvider(JSONParser *self,JSONProvider *provider);

static inline bool IsJSONTokenTruncated(JSONToken token) { return token.typeandflags&TruncatedJSONTokenFlag; }

// Token parsing functions.

int UnescapeStringToken(JSONToken token,char *unescapedbuffer,size_t buffersize);
bool UnescapeStringTokenInPlace(JSONToken *token);
bool ParseNumberTokenAsInteger(JSONToken token,int *result);
bool ParseNumberTokenAsFloat(JSONToken token,float *result);

// Structure parsing functions.

bool ExpectJSONTokenOfTypeWithProvider(JSONParser *self,JSONProvider *provider,int expectedtype,JSONToken *token);
bool SkipJSONValueWithProvider(JSONParser *self,JSONProvider *provider);
bool ExpectAndSkipJSONValueOfTypeWithProvider(JSONParser *self,JSONProvider *provider,int expectedtype);
bool SkipUntilEndOfJSONObjectWithProvider(JSONParser *self,JSONProvider *provider);
bool SkipUntilEndOfJSONArrayWithProvider(JSONParser *self,JSONProvider *provider);
bool ExpectAndScanJSONObjectForKeyWithProvider(JSONParser *self,JSONProvider *provider,const char *key);

static inline bool ExpectJSONTokenOfType(JSONParser *self,int expectedtype,JSONToken *token) { return ExpectJSONTokenOfTypeWithProvider(self,NULL,expectedtype,token); }
static inline bool SkipJSONValue(JSONParser *self) { return SkipJSONValueWithProvider(self,NULL); }
static inline bool ExpectAndSkipJSONValueOfType(JSONParser *self,int expectedtype) { return ExpectAndSkipJSONValueOfTypeWithProvider(self,NULL,expectedtype); }
static inline bool SkipUntilEndOfJSONObject(JSONParser *self) { return SkipUntilEndOfJSONObjectWithProvider(self,NULL); }
static inline bool SkipUntilEndOfJSONArray(JSONParser *self) { return SkipUntilEndOfJSONArrayWithProvider(self,NULL); }
static inline bool ExpectAndScanJSONObjectForKey(JSONParser *self,const char *key) { return ExpectAndScanJSONObjectForKeyWithProvider(self,NULL,key); }

#endif
