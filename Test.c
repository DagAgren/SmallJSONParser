#include "SmallJSONParser.h"
#include <stdio.h>
#include <string.h>

static void DumpJSON(JSONParser *parser);
static void DumpJSONWithProvider(JSONParser *parser,JSONProvider *provider);
static bool AnnoyingInputProvider(JSONParser *self,void *context);

int main()
{

	static const char *testcases[]=
	{
		" {\"key\":  \"value\",	\"alsokey\":\ntrue,\"furtherkey\":[1,2,3],\n\"longkey\": \"veryveryveryveryveryverylongstring\"}  ",
		"\"\\uD83D\\uDD30\"",
	};

	printf("Minimal test:\n\n");

	for(int i=0;i<sizeof(testcases)/sizeof(testcases[0]);i++)
	{
		const char *json=testcases[i];
		printf("%s\n\n",json);

		JSONParser parser;
		InitialiseJSONParser(&parser);
		ProvideJSONInput(&parser,json,strlen(json));
		DumpJSON(&parser);
		printf("\n");
	}

	printf("Provider test:\n\n");

	for(int i=0;i<sizeof(testcases)/sizeof(testcases[0]);i++)
	{
		const char *json=testcases[i];
		printf("%s\n\n",json);

		JSONParser parser;
		InitialiseJSONParser(&parser);

		JSONProvider annoyingprovider;
		const char *input=json;
		char buffer[32];
		InitialiseJSONProvider(&annoyingprovider,AnnoyingInputProvider,&input,buffer,sizeof(buffer));

		DumpJSONWithProvider(&parser,&annoyingprovider);
		printf("\n");
	}

	printf("Parse test:\n\n");

	JSONParser parser;
	InitialiseJSONParser(&parser);

	JSONProvider annoyingprovider;
	const char *input=testcases[0];
	char buffer[32];
	InitialiseJSONProvider(&annoyingprovider,AnnoyingInputProvider,&input,buffer,sizeof(buffer));

	JSONToken token;
	if(ExpectAndSkipUntilJSONObjectKeyWithProvider(&parser,&annoyingprovider,"furtherkey"))
	{
		printf("Found key.\n");

		if(ExpectJSONTokenOfTypeWithProvider(&parser,&annoyingprovider,StartArrayJSONToken,&token))
		{
			printf("Found expected array.\n");

			if(SkipJSONValueWithProvider(&parser,&annoyingprovider))
			{
				printf("Skipped value.\n");

				if(ExpectJSONTokenOfTypeWithProvider(&parser,&annoyingprovider,NumberJSONToken,&token))
				{
					printf("Found expected number.\n");

					int value;
					if(ParseNumberTokenAsInteger(token,&value))
					{
						if(value==2)
						{
							printf("Parsed value as correct integer %d.\n",value);
						}
						else
						{
							printf("Parsed value as incorrect integer %d.\n",value);
						}
					}
					else
					{
						printf("Failed to parse number as integer.\n");
					}
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
	printf("\n");
}




static const char *TokenNames[]=
{
	[StringJSONToken]="String",
	[NumberJSONToken]="Number",
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

static void DumpJSON(JSONParser *parser)
{
	for(;;)
	{
		JSONToken token=NextJSONToken(parser);

		printf("%s",TokenNames[JSONTokenType(token)]);

		if(IsJSONTokenPartial(token)) printf(" (partial)");

		printf(": ");

		if(JSONTokenType(token)==StringJSONToken)
		{
			char buf[SizeOfUnescapingBufferForJSONStringToken(token)];
			UnescapeJSONStringToken(token,buf,NULL);
			printf("%s\n",buf);
		}
		else
		{
			for(const uint8_t *ptr=token.start;ptr<token.end;ptr++) fputc(*ptr,stdout);
			printf("\n");
		}

		if(JSONTokenType(token)==OutOfDataJSONToken) break;
		if(JSONTokenType(token)==ParseErrorJSONToken) exit(1);
	}
}

static void DumpJSONWithProvider(JSONParser *parser,JSONProvider *provider)
{
	for(;;)
	{
		JSONToken token=NextJSONTokenWithProvider(parser,provider);

		printf("%s",TokenNames[JSONTokenType(token)]);

		if(IsJSONTokenPartial(token)) printf(" (partial)");
		if(IsJSONTokenTruncated(token)) printf(" (truncated)");

		printf(": ");

		if(JSONTokenType(token)==StringJSONToken)
		{
			char buf[SizeOfUnescapingBufferForJSONStringToken(token)];
			UnescapeJSONStringToken(token,buf,NULL);
			printf("%s\n",buf);
		}
		else
		{
			for(const uint8_t *ptr=token.start;ptr<token.end;ptr++) fputc(*ptr,stdout);
			printf("\n");
		}

		if(JSONTokenType(token)==OutOfDataJSONToken) break;
		if(JSONTokenType(token)==ParseErrorJSONToken) exit(1);
	}
}

static bool AnnoyingInputProvider(JSONParser *self,void *context)
{
	const char **input=context;
	if(**input==0) return false;
	ProvideJSONInput(self,*input,1);
	(*input)++;
	return true;
}
