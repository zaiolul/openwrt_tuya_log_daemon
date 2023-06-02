#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "lua_utils.h"
#include "tuya_utils.h"
#include <time.h>

struct script_data{
    lua_State *st;
    int timer;
    time_t previous_time;
};

struct script_data scripts[MAX_SCRIPTS] = {0};

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

int data_scripts_init()
{
    
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
        scripts[i].st = NULL;

        if(i == MAX_SCRIPTS){
            syslog(LOG_INFO, "Reached max lua data script count.");
            break;
        }    
        if(ent->d_type != DT_REG)
            continue;
        if(check_file_ending(ent->d_name, "lua") != 0){
            continue;   
        }
            
        sprintf(full_path, "%s/%s", SCRIPTS_FOLDER, ent->d_name);
        
        scripts[i].st = luaL_newstate();
        luaL_openlibs(scripts[i].st);
        if(luaL_dofile(scripts[i].st, full_path) != 0)
            continue;
        
        lua_getglobal(scripts[i].st, "GetData");
        if(!lua_isfunction(scripts[i].st, -1)){
            syslog(LOG_INFO, "Script %s does not contain GetData function.", full_path);
            lua_close(scripts[i].st);
            scripts[i].st = NULL;
            continue;
        }
        lua_getglobal(scripts[i].st, "Timer");
        
        scripts[i].timer = (int)lua_tonumber(scripts[i].st, -1);
        //lua_pop(scripts[i].st, -1);
        printf("timer value: %d\n", scripts[i].timer);
        i ++;
    } 
    
    return i;
}

int data_scripts_run()
{
    for(int i = 0; i < MAX_SCRIPTS; i ++){
       
        if(!scripts[i].st)
            continue;

        time_t current = time(NULL);
        printf("----%ld %ld diff: %ld\n\n", current, scripts[i].previous_time,current- scripts[i].previous_time);
        if(current - scripts[i].previous_time < scripts[i].timer)
            continue;
        scripts[i].previous_time = current;
        
        lua_getglobal(scripts[i].st, "GetData");
        if (lua_pcall(scripts[i].st, 0, 1, 0) != 0)
            printf("error running function : %s",
                lua_tostring(scripts[i].st, -1));
        const char* report = lua_tostring(scripts[i].st, -1);
        send_report(report);
        lua_pop(scripts[i].st, 1);
    }
}

int data_scripts_cleanup()
{
    for(int i = 0; i < MAX_SCRIPTS; i ++){
        if(!scripts[i].st)
            continue;
        lua_close(scripts[i].st);
    }
}




