#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "cJSON.h"
#include <stdio.h>
#include <syslog.h>


int write_to_file(char* parameter, char* message, char* filename)
{
    cJSON *json = cJSON_Parse(message);
    char* value = json->child->valuestring;
    char* param = json->child->string;

    if(strcmp(param, parameter) == 0){
        FILE* fptr = fopen(filename, "a");
        if(fptr == NULL){
            syslog(LOG_ERR, "Cannot open file for writing");
            return -1;
        }
        else{
            time_t seconds;
            time(&seconds);

            fprintf(fptr, "Timestamp: %ld, value: %s\n",seconds, value);
            fclose(fptr);
        }
    }
    cJSON_Delete(json);
    return 0;
}

int check_file_ending(char *filename, char *expected_ending)
{
    char buf[strlen(filename) + 1];
    buf[strlen(filename)] = '\0';

    strncpy(buf, filename, sizeof(buf));
    char* token = strtok(buf, ".");
    char last_token[256];

    while(token){
        token = strtok(NULL, ".");
        if(token)
            strcpy(last_token, token);
    }
    return strcmp(last_token, expected_ending);
}

int get_string_parts(const char *str, char *sep, char ***arr)
{
    if(!str){
        return -1;
    }

    char **parts = NULL;
    char buf[strlen(str) + 1];

    buf[strlen(str)] = '\0';
    strncpy(buf, str, sizeof(buf));

    char* token = strtok(buf, sep);
    int count = 0;
    while(token){
        parts = realloc(parts, sizeof(char*) * ++count);
        if(!parts){
            return -1;
        }
        parts[count - 1] = malloc(strlen(token) * sizeof(char) + 1);

        if(!parts[count - 1]){
            return -1;
        }
         
        strncpy(parts[count - 1], token, strlen(token));
        parts[count - 1][strlen(token)] = '\0';
        token = strtok(NULL, sep);
    }
    *arr = parts;
    return count;
}

int free_string_array(char ***arr, int count){
    char** parts = *arr;
    for(int i = 0; i < count; i ++){
        free(parts[i]);
    }
    free(parts);
    return 0;
}

