#ifndef __SMALL_JSON_PARSER_H__
#define __SMALL_JSON_PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define StringJSONToken 1
#define NumberJSONToken 2
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
	const uint8_t *currentbyte,*end;
	int state,partialtokentype;
} JSONParser;

typedef struct JSONToken
{
	unsigned int typeandflags;
	const uint8_t *start,*end;
} JSONToken;

// Minimal API.

void InitialiseJSONParser(JSONParser *self);
void ProvideJSONInput(JSONParser *self,const void *bytes,size_t length);
JSONToken NextJSONToken(JSONParser *self);

static inline int JSONTokenType(JSONToken token) { return token.typeandflags&~JSONTokenFlagMask; }
static inline bool IsJSONTokenPartial(JSONToken token) { return token.typeandflags&PartialJSONTokenFlag; }

// Pull API.

typedef bool JSONInputProviderCallbackFunction(JSONParser *parser,void *context);

typedef struct JSONProvider {
	JSONInputProviderCallbackFunction *callback;
	void *context;
	uint8_t *buffer;
	size_t buffersize;
} JSONProvider;

static inline void InitialiseJSONProvider(JSONProvider *self,
JSONInputProviderCallbackFunction *callback,void *context,void *buffer,size_t buffersize)
{
	self->callback=callback;
	self->context=context;
	self->buffer=buffer;
	self->buffersize=buffersize;
}

JSONToken NextJSONTokenWithProvider(JSONParser *self,JSONProvider *provider);

static inline bool IsJSONTokenTruncated(JSONToken token) { return token.typeandflags&TruncatedJSONTokenFlag; }

// Token parsing functions.

bool UnescapeJSONStringToken(JSONToken token,char *unescapedbuffer,size_t buffersize,char **end);
bool UnescapeStringTokenInPlace(JSONToken *token);
bool FastIsJSONStringEqual(JSONToken token,const char *string);
bool FastIsJSONStringEqualWithLength(JSONToken token,const char *string,size_t length);
bool ParseNumberTokenAsInteger(JSONToken token,int *result);
bool ParseNumberTokenAsFloat(JSONToken token,float *result);
bool ParseNumberTokenAsDouble(JSONToken token,double *result);

// Structure parsing functions.

bool ExpectJSONTokenOfTypeWithProvider(JSONParser *self,JSONProvider *provider,int expectedtype,JSONToken *token);
bool SkipJSONValueWithProvider(JSONParser *self,JSONProvider *provider);
bool ExpectAndSkipJSONValueOfTypeWithProvider(JSONParser *self,JSONProvider *provider,int expectedtype);
bool SkipUntilEndOfJSONObjectWithProvider(JSONParser *self,JSONProvider *provider);
bool SkipUntilEndOfJSONArrayWithProvider(JSONParser *self,JSONProvider *provider);
bool SkipUntilJSONObjectKeyWithProvider(JSONParser *self,JSONProvider *provider,const char *key);
bool ExpectAndSkipUntilJSONObjectKeyWithProvider(JSONParser *self,JSONProvider *provider,const char *key);

static inline bool ExpectJSONTokenOfType(JSONParser *self,int expectedtype,JSONToken *token) { return ExpectJSONTokenOfTypeWithProvider(self,NULL,expectedtype,token); }
static inline bool SkipJSONValue(JSONParser *self) { return SkipJSONValueWithProvider(self,NULL); }
static inline bool ExpectAndSkipJSONValueOfType(JSONParser *self,int expectedtype) { return ExpectAndSkipJSONValueOfTypeWithProvider(self,NULL,expectedtype); }
static inline bool SkipUntilEndOfJSONObject(JSONParser *self) { return SkipUntilEndOfJSONObjectWithProvider(self,NULL); }
static inline bool SkipUntilEndOfJSONArray(JSONParser *self) { return SkipUntilEndOfJSONArrayWithProvider(self,NULL); }
static inline bool SkipUntilJSONObjectKey(JSONParser *self,const char *key) { return SkipUntilJSONObjectKeyWithProvider(self,NULL,key); }
static inline bool ExpectAndSkipUntilJSONObjectKey(JSONParser *self,const char *key) { return ExpectAndSkipUntilJSONObjectKeyWithProvider(self,NULL,key); }

#endif
