local cjson = require "cjson"
local ubus = require "ubus"
Timer = 5000
local function makeJsonString(key, val)
    local tb = {}
    tb[key] = {value = val};
    return cjson.encode(tb);
end

function GetData()
    local conn = ubus.connect()
    if not conn then
        error("Failed to connect to ubusd")
    end
    local status = conn:call("system", "info", {})
    conn:close()
    return makeJsonString("uptime", tostring(status["uptime"]));
end

