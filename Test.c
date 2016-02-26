#include "SmallJSONParser.h"
#include <stdio.h>
#include <string.h>

static bool AnnoyingInputProvider(JSONParser *self,void *context)
{
	const char **input=context;
	if(**input==0) return false;
	ProvideJSONInput(self,*input,1);
	(*input)++;
	return true;
}

int main()
{
	static const char *json="{\"key\":\"value\",\"alsokey\":true,\"furtherkey\":[1,2,3]}";

	printf("%s\n",json);

	JSONParser parser;
	InitialiseJSONParser(&parser);
	ProvideJSONInput(&parser,json,strlen(json));

	static const char *tokennames[]=
	{
		[StringObjectJSONToken]="String",
		[NumberObjectJSONToken]="Number",
		[StartObjectJSONToken]="Start object",
		[EndObjectJSONToken]="End object",
		[StartArrayJSONToken]="Start array",
		[EndArrayJSONToken]="End array",
		[TrueJSONToken]="True",
		[FalseJSONToken]="False",
		[NullJSONToken]="Null",
		[OutOfDataJSONToken]="Out of data",
		[ParseErrorJSONToken]="Parse error",
	};


	for(;;)
	{
		JSONToken token=NextJSONToken(&parser);

		printf("%s: ",tokennames[JSONTokenType(token)]);
		for(const char *ptr=token.start;ptr<token.end;ptr++) fputc(*ptr,stdout);
		printf("\n");

		if(JSONTokenType(token)==OutOfDataJSONToken || JSONTokenType(token)==ParseErrorJSONToken) break;
	}

	const char *input=json;
	for(;;)
	{
		char buffer[128];
		JSONToken token=NextJSONTokenWithInputProvider(&parser,
		buffer,sizeof(buffer),AnnoyingInputProvider,&input);

		printf("%s: ",tokennames[JSONTokenType(token)]);
		for(const char *ptr=token.start;ptr<token.end;ptr++) fputc(*ptr,stdout);
		printf("\n");

		if(JSONTokenType(token)==OutOfDataJSONToken || JSONTokenType(token)==ParseErrorJSONToken) break;
	}

}
