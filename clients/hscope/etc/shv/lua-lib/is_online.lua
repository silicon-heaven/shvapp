local shv_utils = require('shv_utils')
local function do_set_status(set_status, node_mounted)
	if node_mounted == true then
		set_status({
			message = 'Node is up',
			severity = 'ok'
		})
	else
		set_status({
			message = 'Node is down',
			severity = 'error'
		})
	end
end

local function manual_refresh(set_status, path)
	shv.rpc_call('.broker/mounts', 'ls', '', function (result, error)
		if error then
			shv_utils.handle_rpc_error(error, set_status)
			return
		end
		local node_mounted = false

		for _, v in ipairs(result.value) do
			if v.value == "'" .. path .. "'" then -- The mount path strings include single quotes.
				node_mounted = true
			end
		end

		do_set_status(set_status, node_mounted)
	end)
end

return function (set_status, path)
	shv.on_broker_connected(function ()
		shv.subscribe(path, 'mntchng', function (_, value)
			do_set_status(set_status, value.value)
		end)
	end)

	-- Even though we setup a mntchng subscribe, we can still allow the user to refresh the online status manually (for
	-- whatever reason). Also, we need to return SOMETHING, otherwise hscope won't recognize this as a tester node.
	return function ()
		manual_refresh(set_status, path)
	end
end
