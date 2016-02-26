#include "SmallJSONParser.h"

#include <string.h>

void InitialiseJSONParser(JSONParser *self)
{
	memset(self,0,sizeof(JSONParser));
	self->partialtokentype=OutOfDataJSONToken;
}

void ProvideJSONInput(JSONParser *self,const char *bytes,size_t length)
{
	self->currentbyte=bytes;
	self->end=bytes+length;
}

#define BaseState 0
#define BareSymbolState 1
#define StringState 2
#define StringEscapeState 3

#define ParseError() do { token.typeandflags=ParseErrorJSONToken; token.end=self->currentbyte; return token; } while(0)

JSONToken NextJSONToken(JSONParser *self)
{
	JSONToken token={
		.typeandflags=self->partialtokentype,
		.start=self->currentbyte,
	};
	self->partialtokentype=OutOfDataJSONToken;

	while(self->currentbyte<self->end)
	{
		unsigned char c=*self->currentbyte++;
		switch(self->state)
		{
			case BaseState:
				switch(c)
				{
					case '\t': case ' ': case '\r': case '\n': // Skip white space.
						break;

					case '"':
						token.typeandflags=StringObjectJSONToken;
						token.start=self->currentbyte;
						self->state=StringState;
						break;

					case ':': case ',': // Ignore : and ,
						break;

					case '[':
						token.typeandflags=StartArrayJSONToken;
						token.start=self->currentbyte-1;
						token.end=self->currentbyte;
						return token;

					case ']':
						token.typeandflags=StartArrayJSONToken;
						token.start=self->currentbyte-1;
						token.end=self->currentbyte;
						return token;

					case '{':
						token.typeandflags=StartObjectJSONToken;
						token.start=self->currentbyte-1;
						token.end=self->currentbyte;
						return token;

					case '}':
						token.typeandflags=EndObjectJSONToken;
						token.start=self->currentbyte-1;
						token.end=self->currentbyte;
						return token;

					case '-':
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						token.typeandflags=NumberObjectJSONToken;
						token.start=self->currentbyte-1;
						self->state=BareSymbolState;
						break;

					case 't':
						token.typeandflags=TrueJSONToken;
						token.start=self->currentbyte-1;
						self->state=BareSymbolState;
						break;

					case 'f':
						token.typeandflags=FalseJSONToken;
						token.start=self->currentbyte-1;
						self->state=BareSymbolState;
						break;

					case 'n':
						token.typeandflags=NullJSONToken;
						token.start=self->currentbyte-1;
						self->state=BareSymbolState;
						break;

					default:
						ParseError();
				}
			break;

			case BareSymbolState:
				switch(c)
				{
					case '\t': case ' ': case '\r': case '\n': case ',': case ':':
						token.end=self->currentbyte-1;
						self->state=BaseState;
						return token;
					case ']': case '}':
						token.end=self->currentbyte-1;
						self->state=BaseState;
						self->currentbyte--;
						return token;
					default:
						if(c<=31 || c>=127) ParseError();
						break;
				}
			break;

			case StringState:
				switch(c)
				{
					case '\\':
						self->state=StringEscapeState;
						break;
					case '"':
						token.end=self->currentbyte-1;
						self->state=BaseState;
						return token;
					default:
						if(c<=31 || (c>=127 && c<=191)) ParseError();
						break;
				}
			break;

			case StringEscapeState:
				switch(c)
				{
					case '"': case '\\': case '/': case 'b':
					case 'f': case 'n': case 'r': case 't': case 'u':
						self->state=StringState;
					break;
					default: ParseError();
				}
			break;
		}
	}

	self->partialtokentype=token.typeandflags;
	token.typeandflags|=PartialJSONTokenFlag;
	token.end=self->currentbyte;

	return token;
}

JSONToken NextJSONTokenWithProvider(JSONParser *self,JSONProvider *provider)
{
	size_t bufferposition=0;
	int flags=0;
	JSONToken token;

	for(;;)
	{
		// Get the next possibly partial token.
		token=NextJSONToken(self);

		// If we get a complete token and have not yet copied any data,
		// we can just return the token without copying and exit early.
		if(!IsJSONTokenPartial(token) && bufferposition==0) return token;

		if(JSONTokenType(token)!=OutOfDataJSONToken)
		{
			// Otherwise, calculate how much more data we can fit into the
			// buffer, and set the truncated flag if we can't fit all of it.
			size_t bytestocopy=token.end-token.start;
			if(bufferposition+bytestocopy>provider->buffersize)
			{
				bytestocopy=provider->buffersize-bufferposition;
				flags|=TruncatedJSONTokenFlag;
			}

			// Copy this fragment into the buffer.
			memcpy(&provider->buffer[bufferposition],token.start,bytestocopy);
			bufferposition+=bytestocopy;

			// If this was the last fragment, exit the loop and return the
			// data we have collected.
			if(!IsJSONTokenPartial(token)) break;
		}

		// Otherwise, try to fetch more data through the callback. If
		// it signals no more data is available, set the partial flag,
		// exit the loop and return as much data as we managed to collect.
		if(!provider->callback(self,provider->context))
		{
			flags|=PartialJSONTokenFlag;
			break;
		}
	}

	return (JSONToken){
		.typeandflags=JSONTokenType(token)|flags,
		.start=&provider->buffer[0],
		.end=&provider->buffer[bufferposition],
	};
}

