local shv_utils = require("shv_utils")
return function (set_status, path_to_agent, hostname, port)
	if path_to_agent == nil then
		error("path_to_agent mustn't be null")
	end

	if hostname == nil then
		error("hostname mustn't be null")
	end

	if port == nil then
		error("port mustn't be null")
	end
	local do_test = function ()
		shv.rpc_call(path_to_agent, "runScript", string.format([[telnet %s %s | grep -oa hello]], hostname, port), function (result, error)
			if error then
				shv_utils.handle_rpc_error(error, set_status)
				return
			end
			if result.value == "hello\n" then
				set_status({
					message = 'Eyas OK',
					severity = 'ok'
				})
			else
				set_status({
					message = 'Eyas unavailable',
					severity = 'error'
				})
			end
		end)
	end

	shv.add_timer(do_test, 1000 * 60 * 5) -- Every five minutes

	return do_test
end
