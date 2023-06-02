local cjson = require "cjson"
local ubus = require "ubus"


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
    return makeJsonString("free_memory", tostring(status["memory"]["free"]))
end