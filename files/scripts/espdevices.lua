local cjson = require "cjson"
local ubus = require "ubus"

local conn



function Init()
    conn = ubus.connect()
end

function Deinit()
    conn:close()
end


function ChangeState(tab)
    
    local status = conn:call("esp_control", tab["inputParams"]["state"],
        {port = tab["inputParams"]["port"], pin = tab["inputParams"]["gpio"]})
    if(status == nil) then
        error("Ubus method failed: check arguments and state function")
    end
    local objs = {}
    objs["esp_response"] = cjson.encode({  msg = status["msg"] ,response = status["response"]})
   
    --print(cjson.encode(objs))
    return cjson.encode(objs)
    
end

function GetDevices(tab)
    local status = conn:call("esp_control", "devices",{})
    local full_json = {}
    local objs = {}
    if(status ~= nil) then
        for k, v in pairs(status["devices"]) do
            objs[k] = cjson.encode({ port = v["port"], pid = v["pid"], vid = v["vid"]})
        end
        full_json["device_list"] = objs;
    else
            full_json["device_list"] = {"No devices found"}
    end
    --print(cjson.encode(full_json))
    return cjson.encode(full_json);
end
