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
	static const char *json=" {\"key\":  \"value\",	\"alsokey\":\ntrue,\"furtherkey\":[1,2,3]}  ";

	printf("%s\n\n",json);

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

	printf("Minimal test:\n\n");

	for(;;)
	{
		JSONToken token=NextJSONToken(&parser);

		printf("%s: ",tokennames[JSONTokenType(token)]);
		for(const char *ptr=token.start;ptr<token.end;ptr++) fputc(*ptr,stdout);
		printf("\n");

		if(JSONTokenType(token)==OutOfDataJSONToken || JSONTokenType(token)==ParseErrorJSONToken) break;
	}
	printf("\n");

	printf("Provider test:\n\n");

	InitialiseJSONParser(&parser);

	const char *input=json;
	char buffer[128];
	JSONProvider annoyingprovider;
	InitialiseJSONProvider(&annoyingprovider,AnnoyingInputProvider,&input,buffer,sizeof(buffer));

	for(;;)
	{
		JSONToken token=NextJSONTokenWithProvider(&parser,&annoyingprovider);

		printf("%s: ",tokennames[JSONTokenType(token)]);
		for(const char *ptr=token.start;ptr<token.end;ptr++) fputc(*ptr,stdout);
		printf("\n");

		if(JSONTokenType(token)==OutOfDataJSONToken || JSONTokenType(token)==ParseErrorJSONToken) break;
	}
	printf("\n");

	printf("Parse test:\n\n");

	input=json;
	InitialiseJSONProvider(&annoyingprovider,AnnoyingInputProvider,&input,buffer,sizeof(buffer));

	JSONToken token;
	if(ExpectAndScanJSONObjectForKeyWithProvider(&parser,&annoyingprovider,"furtherkey"))
	{
		printf("Found key.\n");

		if(ExpectJSONTokenOfTypeWithProvider(&parser,&annoyingprovider,StartArrayJSONToken,&token))
		{
			printf("Found expected array.\n");

			if(SkipJSONValueWithProvider(&parser,&annoyingprovider))
			{
				printf("Skipped value.\n");

				if(ExpectJSONTokenOfTypeWithProvider(&parser,&annoyingprovider,NumberObjectJSONToken,&token))
				{
					printf("Found expected number.\n");
				}
				else
				{
					printf("Failed to find expected number.\n");
				}
			}
			else
			{
				printf("Failed to skip value.\n");
			}
		}
		else
		{
			printf("Failed to find expected array.\n");
		}
	}
	else
	{
		printf("Failed to find key.\n");
	}
}
