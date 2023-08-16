# openwrt_tuya_log_daemon

Connects to Tuya cloud platform, can send and receive data. 

`main` contains base connection logic, microcontroller controlling through UBUS with instructions from the cloud. Also fetches data from system through UBUS and sends it to the cloud.

`lua-scripts` replaces ubus logic with embedded lua, allows to execute scripts. Lua scripts must contain `GetData` function for them to be executed and their data sent to the cloud periodically.
Scripts can contain `Init` and `Deinit` functions for logic that needs to be executed at the beginning and the end respectively.


