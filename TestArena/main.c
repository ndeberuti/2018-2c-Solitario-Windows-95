#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/collections/list.h>


t_list* _string_split(char* text, int n, char* separator)
{
	bool _is_last_token(char* next, int index)
	{
		return next[0] != '\0' && index < (n - 1);
	}

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

	free(message);

	printf("Second Test - Split string and get number of strings in char**\n\n");

	char* stringToSplit = "hello,how,are,you,my,nigga";
	char* stringToSplit2 = "this,is,a,magic,string,that,is,longer,than,the,other,one";

	t_list* stringArray = _string_split(stringToSplit, 6, ",");
	t_list* stringArray2 = _string_split(stringToSplit2, 20, ",");
	t_list* stringArray3 = _string_split(stringToSplit2, 8, ",");

	int stringArraySize = list_size(stringArray);
	int stringArraySize2 = list_size(stringArray2);
	int stringArraySize3 = list_size(stringArray3);

	printf("\t El tamaño del primer string dividido es: %d\n", stringArraySize);
	printf("\t El tamaño del segundo string dividido es: %d", stringArraySize2);
	printf("\t El tamaño del segundo string dividido es: %d", stringArraySize3);

	printf("\tEl contenido del primer array dividido es:");
	showStringArray(stringArray, stringArraySize);
	printf("\n\n");

	printf("\tEl contenido del segundo array dividido es:");
	showStringArray(stringArray2, stringArraySize2);
	printf("\n\n");

	printf("\tEl contenido del tercer array dividido es:");
	showStringArray(stringArray3, stringArraySize3);
	printf("\n\n");

	list_destroy_and_destroy_elements(stringArray, free);
	list_destroy_and_destroy_elements(stringArray2, free);
	list_destroy_and_destroy_elements(stringArray3, free);


	printf("\n\nThird Test - Create a list of integers and add all the values to a string; then print them");

	t_list* porongasGordas = list_create();
	list_add(porongasGordas, 10);
	list_add(porongasGordas, 50);
	list_add(porongasGordas, 25);
	list_add(porongasGordas, 55);

	int tamaniosPorongasGordas = list_size(porongasGordas);
	int tamanioPorongaGorda = 0;

	char* porongasGordasStr = string_new();
	char* porongaGorda = NULL;

	tamanioPorongaGorda = (int) list_get(porongasGordas, 0);
	porongaGorda = string_from_format("%d", tamanioPorongaGorda);
	string_append(&porongasGordasStr, porongaGorda);

	free(porongaGorda);

	for(int i = 1; i < tamaniosPorongasGordas; i++)
	{
		tamanioPorongaGorda = (int) list_get(porongasGordas, i);
		porongaGorda = string_from_format(", %d", tamanioPorongaGorda);
		string_append(&porongasGordasStr, porongaGorda);

		free(porongaGorda);
	}

	printf("\n\n\tTamanios de porongas gordas en cm: %s", porongasGordasStr);
	free(porongasGordasStr);
	list_destroy(porongasGordas);

	printf("\n\nPress a key to exit...");


	getchar();
}

void showStringArray(t_list* array, int arraySize)
{
	char* string = NULL;

	for(int i = 0; i < arraySize; i++)
	{
		string = (char*) list_get(array, i);
		printf("\n\t\t%s", string);
	}
}

