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

int lua_do_function_no_arg(lua_State **st, char *function)
{
    if(check_global(st, LUA_TFUNCTION, function) != 0){
        syslog(LOG_WARNING, "No function detected %s", function);
        lua_close(*st);
        return -1;
    } else if (lua_pcall(*st, 0, 0, 0) != 0){
        syslog(LOG_WARNING, "Error running function \"%s\": %s",function,
            lua_tostring(*st, -1));
        return -1;
    }
    return 0;
}

int data_scripts_init()
{
    char full_path[256];
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
            syslog(LOG_NOTICE, "Reached max lua data script count.");
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
            syslog(LOG_WARNING, "Error opening script: %s", full_path);
            lua_close(st);
            st = NULL;
            continue;
        }
        if(check_global(&st, LUA_TFUNCTION, "GetData") != 0){
            syslog(LOG_WARNING, "Script %s must implement GetData function", full_path);
            continue;
        }

        int time_val = DEFAULT_TIMER;
        if(check_global(&st, LUA_TNUMBER, "Timer") > 0){
            time_val = (int)lua_tonumber(st, -1);
        }

        lua_do_function_no_arg(&st, "Init");

        scripts[i].st = st;  
        scripts[i].timer = time_val;
        i ++;
    } 
    return i;
}

int get_action_parts(const char *str, char* file, char* function, char* sep)
{
    char buf[strlen(str) + 1];
    buf[strlen(str)] = '\0';
    strncpy(buf, str, sizeof(buf));
    char* token = strtok(buf, sep);

    if(!token)
        return -1;

    strncpy(file, token, strlen(token));
    token = strtok(NULL, sep);
    strncpy(function, token, strlen(token));
    return 0;
}

void push_table(lua_State *st, cJSON *obj){
    lua_pushstring(st, obj->string);
    !obj->valuestring ? lua_pushnumber(st, obj->valuedouble) : lua_pushstring(st, obj->valuestring);
    lua_settable(st, -3); 
}

int make_table(lua_State *st, cJSON* json)
{
    cJSON* c = json->child; // inputParams
    lua_newtable(st);
    lua_pushstring(st, c->string);
    lua_newtable(st);

    cJSON* args = c->child;
    while(args){
        push_table(st, args);
        args = args->next;
    }
    lua_settable(st, -3);

    c = c->next;
    push_table(st, c);
    return 0;  
}

int execute_action(cJSON *json)
{    
    cJSON *action = cJSON_GetObjectItemCaseSensitive(json, "actionCode");

    char file[100] = {0};
    char function[100] = {0};

    get_action_parts(action->valuestring, file, function, "_");
    //printf("FILE: %s, FUNCTION: %s\n", file, function);
    char full_path[256];
    sprintf(full_path, "%s/%s.lua", SCRIPTS_FOLDER, file);
    //printf("full file path: %s\n", full_path);
    lua_State *st = luaL_newstate();
    luaL_openlibs(st);

    int ret = luaL_dofile(st, full_path);
    if(ret != 0){
        syslog(LOG_INFO, "Can't open action script: %s", full_path);
        lua_close(st);
        return ret;
    }

    if(check_global(&st, LUA_TFUNCTION, function) != 0){
        syslog(LOG_INFO, "No function detected %s:%s", full_path, function);
        lua_close(st);
        return -1;
    }

    lua_do_function_no_arg(&st, "Init");

    make_table(st, json);
    if (lua_pcall(st, 1, 1, 0) != 0){
        syslog(LOG_INFO, "Error running function \"%s\": %s",function,
            lua_tostring(st, -1));
        lua_do_function_no_arg(&st, "Deinit");
        lua_close(st);
        return -1;
    }
    const char* return_string = lua_tostring(st, -1);
    send_report(return_string);

    lua_do_function_no_arg(&st, "Deinit");
    lua_close(st);
    return 0;
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
              syslog(LOG_WARNING, "Error running function \"%s\": %s","GetData", lua_tostring(scripts[i].st, -1));
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
        lua_do_function_no_arg(&(scripts[i].st), "Deinit");
        lua_close(scripts[i].st);
    }
}