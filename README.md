# openwrt_tuya_log_daemon

Replaces ubus logic with embedded lua, allows to execute scripts. Lua scripts must contain `GetData` function for them to be executed and their data sent to the cloud periodically.
Scripts can contain `Init` and `Deinit` functions for logic that needs to be executed at the beginning and the end respectively.
