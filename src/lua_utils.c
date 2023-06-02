#include "lua_utils.h"
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "tuya_utils.h"

lua_State *st[MAX_SCRIPTS] = {0};


int check_file_ending(char *filename, char *expected_ending)
{
    printf("Checking file: %s\n", filename);
    char buf[strlen(filename) + 1];
    buf[strlen(filename)] = '\0';

    strncpy(buf, filename, sizeof(buf));
    char* token = strtok(buf, ".");
    char last_token[256];

    while(token){
         printf( " %s\n", token );
        token = strtok(NULL, ".");
        if(token)
            strcpy(last_token, token);
    }
    return strcmp(last_token, expected_ending);
}

int data_scripts_init()
{
    printf("starting lua init\n");
    char full_path[256];
    //strncpy(full_path, path, sizeof(full_path));
    DIR* dir = opendir(SCRIPTS_FOLDER);
    if(!dir){
        syslog(LOG_ERR, "Can't find or open Lua script folder");
        return -1;
    }
    struct dirent *ent;
    int i = 0;

    while((ent = readdir(dir))){
        if(i == MAX_SCRIPTS){
            syslog(LOG_INFO, "Reached max lua data script count.");
            break;
        }
            
        if(ent->d_type != DT_REG)
            continue;
        if(check_file_ending(ent->d_name, "lua") != 0)
            continue;
        sprintf(full_path, "%s/%s", SCRIPTS_FOLDER, ent->d_name);
        //printf("Found lua script: %s\n", full_path);
        st[i] = luaL_newstate();
        luaL_openlibs(st[i]);
        if(luaL_dofile(st[i], full_path) != 0){
            printf("Error opening lua script");
            continue;
        }
        
        lua_getglobal(st[i], "GetData");
        if(!lua_isfunction(st[i], lua_gettop(st[i]))){
            printf("No GetData in file\n");
            lua_close(st[i]);
            continue;
        }
        i ++;
    } 
    return i;
}

int data_scripts_run()
{
    for(int i = 0; i < MAX_SCRIPTS; i ++){
        if(!st[i])
            continue;
        lua_getglobal(st[i], "GetData");
        if (lua_pcall(st[i], 0, 1, 0) != 0)
            printf("error running function : %s",
                lua_tostring(st[i], -1));
        const char* report = lua_tostring(st[i], -1);
        printf("got string: %s", report);
        send_report(report);
        lua_pop(st[i], 1);
    }
}

int data_scripts_cleanup()
{
    for(int i = 0; i < MAX_SCRIPTS; i ++){
        if(!st[i])
            continue;
        lua_close(st[i]);
    }
}




