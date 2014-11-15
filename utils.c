#include <utils.h>

char** get_tokens(const char* string)
{
	static char* tokens[16] = {};
	
	for (int k = 0; k < 16; k++) {
		if (tokens[k] == NULL) {
			tokens[k] = (char*) calloc(TOKEN_SIZE, sizeof(char));
		}
		memset(tokens[k], 0, TOKEN_SIZE);
	}

	char* str_dup = strdup(string);
	char* token = strtok(str_dup, " ");
	
	if (token) {
		snprintf(tokens[0], TOKEN_SIZE, "%s", token);
	}
	for (int i = 1; token; i++) {
    	token = strtok(NULL, " ");
    	if (token) {
    		snprintf(tokens[i], TOKEN_SIZE, "%s", token);
    	}
    	else {
    		tokens[i][0] = '\0';
    	}
	}

	free(str_dup);
	return tokens;
}