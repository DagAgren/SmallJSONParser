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
	static const char *tokennames[]=
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

	static const char *json=" {\"key\":  \"value\",	\"alsokey\":\ntrue,\"furtherkey\":[1,2,3]}  ";

	printf("%s\n\n",json);

	printf("Minimal test:\n\n");

	JSONParser parser;
	InitialiseJSONParser(&parser);
	ProvideJSONInput(&parser,json,strlen(json));

	for(;;)
	{
		JSONToken token=NextJSONToken(&parser);

		printf("%s: ",tokennames[JSONTokenType(token)]);
		for(const uint8_t *ptr=token.start;ptr<token.end;ptr++) fputc(*ptr,stdout);
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
		for(const uint8_t *ptr=token.start;ptr<token.end;ptr++) fputc(*ptr,stdout);
		printf("\n");

		if(JSONTokenType(token)==OutOfDataJSONToken || JSONTokenType(token)==ParseErrorJSONToken) break;
	}
	printf("\n");

	printf("Parse test:\n\n");

	input=json;
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

	printf("Unicode test:\n\n");

	static char surrogatejson[]="\"\\uD83D\\uDD30\"";

	InitialiseJSONParser(&parser);
	ProvideJSONInput(&parser,surrogatejson,strlen(surrogatejson));

	for(;;)
	{
		JSONToken token=NextJSONToken(&parser);

		printf("%s: ",tokennames[JSONTokenType(token)]);
		if(JSONTokenType(token)==StringJSONToken)
		{
			UnescapeJSONStringTokenInPlace(&token);
			printf("%s\n",token.start);
		}
		else
		{
			for(const uint8_t *ptr=token.start;ptr<token.end;ptr++) fputc(*ptr,stdout);
			printf("\n");
		}

		if(JSONTokenType(token)==OutOfDataJSONToken || JSONTokenType(token)==ParseErrorJSONToken) break;
	}
	printf("\n");
}
