local cjson = require "cjson"
local ubus = require "ubus"

local conn

local function makeJsonString(key, val)
    local tb = {}
    tb[key] = {value = val};
    return cjson.encode(tb);
end

function Init()
    conn = ubus.connect()
end

function Deinit()
    conn:close()
end

function GetData()
    if not conn then
        error("Failed to connect to ubusd")
    end
    local status = conn:call("system", "info", {})
    return makeJsonString("uptime", tostring(status["uptime"]));
end

