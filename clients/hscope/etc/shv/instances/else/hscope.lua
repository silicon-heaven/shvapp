local shv_utils = require('shv_utils')

shv.on_broker_connected(function ()
    shv.subscribe('test/shvagent/tester/prop1', 'chng', function (path, new_value)
        shv.log_info('path ' .. path)
        shv_utils.print_table(new_value, 'result.')
    end)

    shv.rpc_call('test/shvagent/tester/prop1', 'set', '123', function ()
        -- Do nothing, I only care about the subscription
    end)
end)
