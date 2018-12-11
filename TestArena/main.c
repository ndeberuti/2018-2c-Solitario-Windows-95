#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/collections/list.h>

bool _is_last_token(char* next, int _)
{
		return next[0] != '\0';
}

t_list* _string_split(char* text, char* separator)
{
	t_list* substrings = list_create();
	int size = 0;

	char *text_to_iterate = string_duplicate(text);

	char *next = text_to_iterate;
	char *str = text_to_iterate;

	while(_is_last_token(next, size))
	{
		char* token = strtok_r(str, separator, &next);
		if(token == NULL)
		{
			break;
		}
		str = NULL;
		size++;
		list_add(substrings, string_duplicate(token));
	};

	if (next[0] != '\0')
	{
		size++;
		list_add(substrings, string_duplicate(next));
	}

	size++;

	free(text_to_iterate);
	return substrings;
}


int main()
{
	printf("First Test - Print %%d to a string, then print that to stdout\n\n");

	char* message = calloc(1, sizeof(char) * 65);
	sprintf(message, "\t message %s %%d\n\n", "number");

	printf(message, 1);
	printf(message, 2);

	printf("Second Test - Split string and get number of strings in char**\n\n");

	char* stringToSplit = "hello\nhow\nare\nyou\nmy\nnigga";
	char* stringToSplit2 = "this\nis\na\nmagic\nstring\nthat\nis\nlonger\nthan\nthe\nother\none";

	t_list* stringArray = _string_split(stringToSplit, "\n");
	t_list* stringArray2 = _string_split(stringToSplit2, "\n");

	int stringArraySize = list_size(stringArray);
	int stringArraySize2 = list_size(stringArray2);

	printf("\t El tamaño del primer string es: %d\n", stringArraySize);
	printf("\t El tamaño del segundo string es: %d", stringArraySize2);

	getchar();
}

