# openwrt_tuya_log_daemon

Replaces ubus logic with embedded lua, allows to execute scripts. Lua scripts must contain `GetData` function for them to be executed and their data sent to the cloud periodically.
Scripts can contain `Init` and `Deinit` functions for logic that needs to be executed at the beginning and the end respectively.

To execute a function in a script on received tuya action, the action name must consist of the script name and function name like this: `script_name`. For example, to execute method `PrintData` in script called `MyScript`, the action name must be `MyScript_PrintData` 
