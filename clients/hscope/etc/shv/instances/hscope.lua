local shv_utils = require('shv_utils')
local bot = require('telegram_bot').new('5223967314:AAF8ypBAV07IUIxW7e6TLHBYLZWcSObXyuk')
shv.log_info('This is the root level configuration!')
shv.log_info('my environment is:')
shv.log_info(shv_utils.print_table(_ENV))

shv.log_info("I'm setting something for the child environments!")
something = 123

local enabled_paths = {}

shv.on_broker_connected(function ()
    shv.rpc_call(".broker/currentClient", "mountPoint", "", function (value)
        shv.subscribe(value.value .. "/" .. cur_shv_path, "chng", function (path, new_value)
            if enabled_paths[path] then
                if new_value.value.severity.value ~= 'ok' then
                    -- bot:sendMessage(CHAT_ID, string.format('Test failure occurred.\nPath: %s\nSeverity: %s\nMessage: %s', path, new_value.value.severity.value, new_value.value.message.value))
                end
            end

            enabled_paths[path] = true
        end)
    end)
end)
