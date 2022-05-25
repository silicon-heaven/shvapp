local shv_utils = require('shv_utils')

shv.on_broker_connected(function ()
    shv.rpc_call('nonexisting', 'nope', '', function (result, error)
        shv_utils.print_table(error, 'error.')
    end)
end)
