local telegram = require('telegram_bot')
local bot = telegram.new("5221162017:AAFDDw7Ovp2XWUth4XZhoa90hSgqnCs9OX4")

shv.on_broker_connected(function()
	bot:sendMessage(5362232370, "Broker connected!")

	shv.subscribe_change('test', function(path, new_value)
		bot:sendMessage(5362232370, string.format("CHANGE: path = %s, new_value = %s", path, new_value))
	end)

	shv.subscribe_change('test', function(path, new_value)
		bot:sendMessage(5362232370, string.format("some other callback: path", path, new_value))
		shv.rpc_call('.broker', 'dir', '', function (value)
			bot:sendMessage(5362232370, string.format("RPC call returned: %s", value))
		end)
	end)
end)

-- subscribe_change(22, function(path, new_value)
-- 	-- fail
-- end)
