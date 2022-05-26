local shv_utils = require('shv_utils')

shv.on_broker_connected(function ()

    shv.rpc_call('test/shvagent/tester/prop1', 'set', '33', function ()
        shv.subscribe('test/shvagent/tester/prop1', 'chng', function (path, new_value)
            if new_value.value ~= '1' then
                error('Test failed. Expected: 1, Got: ' .. new_value.value)
            end
        end)
        shv.rpc_call('test/shvagent/tester/prop1', 'set', '1', function ()
            -- Do nothing, I only care about the subscription
        end)
    end)
end)
