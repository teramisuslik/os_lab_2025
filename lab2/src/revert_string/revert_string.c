#include "revert_string.h"
#include <string.h>

void RevertString(char *str)
{
	if (str == NULL) return;

	int len = strlen(str);
	int i = 0;
	int j = len - 1;

	while (i < j){
		char box = str[j];
		str[j] = str[i];
		str[i] = box;
		i++;
		j--;
	}
	return;

}

