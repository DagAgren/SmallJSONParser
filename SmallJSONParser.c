#include "SmallJSONParser.h"

#include <string.h>

void InitialiseJSONParser(JSONParser *self)
{
	memset(self,0,sizeof(JSONParser));
	self->partialtokentype=OutOfDataJSONToken;
}

void ProvideJSONInput(JSONParser *self,const void *bytes,size_t length)
{
	self->currentbyte=bytes;
	self->end=self->currentbyte+length;
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
		uint8_t c=*self->currentbyte++;
		switch(self->state)
		{
			case BaseState:
				switch(c)
				{
					case '\t': case ' ': case '\r': case '\n': // Skip white space.
						break;

					case '"':
						token.typeandflags=StringJSONToken;
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
						token.typeandflags=EndArrayJSONToken;
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
						token.typeandflags=NumberJSONToken;
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
			// Use the buffer size -1, so that a terminating zero byte can
			// be added later by the unescaping function.
			size_t bytestocopy=token.end-token.start;
			if(bufferposition+bytestocopy>provider->buffersize-1)
			{
				bytestocopy=provider->buffersize-1-bufferposition;
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




static int HexDigit(uint8_t c)
{
	if(c>='0' && c<='9') return c-'0';
	else if(c>='a' && c<='f') return c-'a'+10;
	else if(c>='A' && c<='F') return c-'A'+10;
	else return -1;
}

bool UnescapeJSONStringToken(JSONToken token,char *unescapedbuffer,char **end)
{
	const uint8_t *src=token.start;
	char *dest=unescapedbuffer;

	while(src<token.end)
	{
		uint8_t c1=*src++;
		if(c1=='\\')
		{
			if(src==token.end) return false;

			uint8_t c2=*src++;
			switch(c2)
			{
				case '"': case '\\': case '/': *dest++=c2; break;
				case 'b': *dest++='\b'; break;
				case 'f': *dest++='\f'; break;
				case 'n': *dest++='\n'; break;
				case 'r': *dest++='\r'; break;
				case 't': *dest++='\t'; break;
				case 'u':
					if(src+4>token.end) return false;
					int h0=HexDigit(*src++);
					int h1=HexDigit(*src++);
					int h2=HexDigit(*src++);
					int h3=HexDigit(*src++);
					if(h0<0 || h1<0 || h2<0 || h3<0) return false;

					int code=(h0<<12)|(h1<<8)|(h2<<4)|h3;

					// If we encounter a high surrogate, look for a pair.
					if(code>=0xd800 && code<=0xdbff)
					{
						// Check if there is enough space for another unicode
						// escape, and that it starts with \u...
						if(src+6<=token.end && src[0]=='\\' && src[1]=='u')
						{
							int h0=HexDigit(src[2]);
							int h1=HexDigit(src[3]);
							int h2=HexDigit(src[4]);
							int h3=HexDigit(src[5]);

							// And is followed by four valid hex digits...
							if(h0>=0 && h1>=0 && h2>=0 && h3>=0)
							{
								// Specifying a low surrogate.
								int code2=(h0<<12)|(h1<<8)|(h2<<4)|h3;
								if(code2>=0xdc00 && code2<=0xdfff)
								{
									// If so, adjust the code accordingly.
									code=((code&0x3ff)<<10)+(code2&0x3ff)+0x10000;
									// And skip the escape code.
									src+=6;
								}
							}
						}
					}

					if(code<128)
					{
						*dest++=code;
					}
					else if(code<2048)
					{
						*dest++=0xc0|(code>>6);
						*dest++=0x80|(code&0x3f);
					}
					else if(code<65536)
					{
						*dest++=0xe0|(code>>12);
						*dest++=0x80|((code>>6)&0x3f);
						*dest++=0x80|(code&0x3f);
					}
					else
					{
						*dest++=0xf0|(code>>18);
						*dest++=0x80|((code>>12)&0x3f);
						*dest++=0x80|((code>>6)&0x3f);
						*dest++=0x80|(code&0x3f);
					}
				break;
			}
		}
		else
		{
			*dest=c1;
		}
	}

	*dest=0;

	return true;
}

bool UnescapeJSONStringTokenInPlace(JSONToken *token)
{
	return UnescapeJSONStringToken(*token,(char *)token->start,(char **)&token->end);
}

bool FastIsJSONStringEqual(JSONToken token,const char *string)
{
	return FastIsJSONStringEqualWithLength(token,string,strlen(string));
}

bool FastIsJSONStringEqualWithLength(JSONToken token,const char *string,size_t length)
{
	size_t testlength=token.end-token.start;
	if(testlength!=length) return false;
	return memcmp(token.start,string,testlength)==0;
}

bool ParseNumberTokenAsInteger(JSONToken token,int *result)
{
	int value=0;
	bool negative=false;
	const uint8_t *ptr=token.start;

	if(ptr==token.end) return false;

	if(*ptr=='-')
	{
		negative=true;
		ptr++;
		if(ptr==token.end) return false;
	}

	while(ptr<token.end)
	{
		uint8_t c=*ptr++;
		if(c<'0' || c>'9') return false;
		value=value*10+c-'0';
	}

	if(negative) *result=-value;
	else *result=value;

	return true;
}

bool ParseNumberTokenAsFloat(JSONToken token,float *result)
{
	size_t length=token.end-token.start;
	char buffer[length+1];
	memcpy(buffer,token.start,length);
	buffer[length]=0;

	char *end;
	*result=strtof(buffer,&end);

	return end!=(const char *)token.start;
}

bool ParseNumberTokenAsDouble(JSONToken token,double *result)
{
	size_t length=token.end-token.start;
	char buffer[length+1];
	memcpy(buffer,token.start,length);
	buffer[length]=0;

	char *end;
	*result=strtod(buffer,&end);

	return end!=(const char *)token.start;
}




static bool ParseBraces(JSONParser *self,JSONProvider *provider,int level);

bool ExpectJSONTokenOfTypeWithProvider(JSONParser *self,JSONProvider *provider,int expectedtype,JSONToken *token)
{
	if(provider) *token=NextJSONTokenWithProvider(self,provider);
	else *token=NextJSONToken(self);

	return JSONTokenType(*token)==expectedtype;
}

bool SkipJSONValueWithProvider(JSONParser *self,JSONProvider *provider)
{
	return ParseBraces(self,provider,0);
}

bool ExpectAndSkipJSONValueOfTypeWithProvider(JSONParser *self,JSONProvider *provider,int expectedtype)
{
	JSONToken token;
	if(provider) token=NextJSONTokenWithProvider(self,provider);
	else token=NextJSONToken(self);

	int type=JSONTokenType(token);
	if(type!=expectedtype) return false;
	else if(type==StartObjectJSONToken || type==StartArrayJSONToken) return ParseBraces(self,provider,1);
	else return true;
}

bool SkipUntilEndOfJSONObjectWithProvider(JSONParser *self,JSONProvider *provider)
{
	return ParseBraces(self,provider,1);
}

bool SkipUntilEndOfJSONArrayWithProvider(JSONParser *self,JSONProvider *provider)
{
	return ParseBraces(self,provider,1);
}

bool SkipUntilJSONObjectKeyWithProvider(JSONParser *self,JSONProvider *provider,const char *key)
{
	size_t keylength=strlen(key);
	for(;;)
	{
		JSONToken token;
		if(!ExpectJSONTokenOfTypeWithProvider(self,provider,StringJSONToken,&token)) return false;

		if(FastIsJSONStringEqualWithLength(token,key,keylength)) return true;

		if(!SkipJSONValueWithProvider(self,provider)) return false;
	}
}

bool ExpectAndSkipUntilJSONObjectKeyWithProvider(JSONParser *self,JSONProvider *provider,const char *key)
{
	JSONToken token;
	if(!ExpectJSONTokenOfTypeWithProvider(self,provider,StartObjectJSONToken,&token)) return false;

	return SkipUntilJSONObjectKeyWithProvider(self,provider,key);
}

static bool ParseBraces(JSONParser *self,JSONProvider *provider,int level)
{
	for(;;)
	{
		JSONToken token;
		if(provider) token=NextJSONTokenWithProvider(self,provider);
		else token=NextJSONToken(self);

		int type=JSONTokenType(token);
		if(type==StartObjectJSONToken || type==StartArrayJSONToken)
		{
			level++;
		}
		else if(type==EndObjectJSONToken || type==EndArrayJSONToken)
		{
			level--;
			if(level<0) return false;
		}
		else if(type==OutOfDataJSONToken || type==ParseErrorJSONToken)
		{
			return false;
		}

		if(level==0) return true;
	}
}
