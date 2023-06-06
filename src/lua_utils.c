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
#include "cJSON.h"

#define DEFAULT_TIMER 5

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

int check_global(lua_State **st, int type, const char *g)
{
    int ret = 0;
    lua_getglobal(*st, g);
    switch(type){
        case LUA_TFUNCTION:
            ret = lua_isfunction(*st, -1);
            break;
        case LUA_TNUMBER:
            return lua_isnumber(*st, -1); 
        default:
            return -1;
    }
    if(!ret && type == LUA_TFUNCTION){
        lua_close(*st);
        *st = NULL;
        return -1;
    }
    return 0;
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
    lua_State *st = NULL;

    while((ent = readdir(dir))){

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
        
        st = luaL_newstate();
        luaL_openlibs(st);
        if(luaL_dofile(st, full_path) != 0){
            syslog(LOG_INFO, "Error opening script: %s", full_path);
            lua_close(st);
            st = NULL;
            continue;
        }
        if(check_global(&st, LUA_TFUNCTION, "Init") != 0 ||
            check_global(&st, LUA_TFUNCTION, "Deinit") != 0 ||
            check_global(&st, LUA_TFUNCTION, "GetData") != 0){
            syslog(LOG_INFO, "Script %s must implement global Init, Deinit and GetData functions", full_path);
            continue;
        }

        int time_val = DEFAULT_TIMER;
        if(check_global(&st, LUA_TNUMBER, "Timer") > 0){
            time_val = (int)lua_tonumber(st, -1);
        }

        lua_getglobal(st, "Init");
        if (lua_pcall(st, 0, 0, 0) != 0){
            syslog(LOG_INFO, "Error running function : %s", lua_tostring(st, -1));
            continue;
        }
        lua_settop(st, 0);
        scripts[i].st = st;  
        scripts[i].timer = time_val;
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
        if(current - scripts[i].previous_time < scripts[i].timer)
            continue;
        scripts[i].previous_time = current;
        
        lua_getglobal(scripts[i].st, "GetData");
        if (lua_pcall(scripts[i].st, 0, 1, 0) != 0){
            syslog(LOG_INFO, "Error running function : %s", lua_tostring(scripts[i].st, -1));
            continue;
        }
        const char* report = lua_tostring(scripts[i].st, -1);
        send_report(report);
        lua_pop(scripts[i].st, -1);
    }
}

int data_scripts_cleanup()
{
    for(int i = 0; i < MAX_SCRIPTS; i ++){
        if(!scripts[i].st)
            continue;
        lua_getglobal(scripts[i].st, "Deinit");
        if (lua_pcall(scripts[i].st, 0, 0, 0) != 0){
            syslog(LOG_INFO, "Error running function : %s", lua_tostring(scripts[i].st, -1));
        }
            
        lua_close(scripts[i].st);
    }
}





