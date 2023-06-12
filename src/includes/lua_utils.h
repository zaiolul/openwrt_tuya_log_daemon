#ifndef LUA_UTILS_H
#define LUA_UTILS_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdlib.h>

#define SCRIPTS_FOLDER "/usr/bin/tuya_scripts"
#define MAX_SCRIPTS 20

int data_scripts_init();
int data_scripts_run();
int data_scripts_cleanup();
int execute_action(char *payload);
#endif
