local shv_utils = require('shv_utils')

shv.on_broker_connected(function ()
    shv.subscribe('test', 'mntchng', function (path, value)
        shv.log_info(string.format('mntchng, path: %s', path))
        shv_utils.print_table(value, 'mntchng.')
    end)
end)
