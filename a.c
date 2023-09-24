#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

void parse_path(const char *path, char ***parts, int *count) {
    char *token;
    char *copy = strdup(path); // Make a copy to avoid modifying the original string
    char **result = NULL;
    int i = 0;

    token = strtok(copy, "/");
    while (token != NULL) {
        result = realloc(result, (i + 1) * sizeof(char *));
        result[i] = strdup(token);
        i++;
        token = strtok(NULL, "/");
    }

    free(copy);

    *parts = result;
    *count = i;
}

int main(int argc, char * argv[]){
    const char *path = "/home/linux/tutorial.txt";
    char **parts;
    int count;

    parse_path(path, &parts, &count);

    for (int i = 0; i < count; i++) {
        printf("%s\n", parts[i]);
        free(parts[i]); // Free each string
    }    

    free(parts); // Free the array of strings

    return 0;
}