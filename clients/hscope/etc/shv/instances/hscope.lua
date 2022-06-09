local shv_utils = require('shv_utils')
local bot = require('telegram_bot').new('5223967314:AAF8ypBAV07IUIxW7e6TLHBYLZWcSObXyuk')
shv.log_info('This is the root level configuration!')
shv.log_info('my environment is:')
shv.log_info(shv_utils.print_table(_ENV))

shv.log_info("I'm setting something for the child environments!")
something = 123

local function ends_with(str, ending)
   return ending == "" or str:sub(-#ending) == ending
end

local last_severity = {}

shv.on_broker_connected(function ()
    shv.rpc_call(".broker/currentClient", "mountPoint", "", function (value)
        shv.subscribe(value.value .. "/" .. cur_shv_path, "chng", function (path, new_value)
            if ends_with(path, '/status') and last_severity[path] then
                if new_value.value.severity.value ~= last_severity[path] then
                    local msg =
                        new_value.value.severity.value == 'ok' and 'Test was fixed.' or 'Test failure occurred.'
                    -- bot:sendMessage(CHAT_ID, string.format('%s\nPath: %s\nSeverity: %s\nMessage: %s', msg, path, new_value.value.severity.value, new_value.value.message.value))
                end
            end

            last_severity[path] = new_value.value.severity.value
        end)
    end)
end)
