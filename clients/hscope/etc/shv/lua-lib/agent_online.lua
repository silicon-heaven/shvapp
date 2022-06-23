return function (set_status, path_to_agent)
	local function do_test()
		shv.rpc_call(path_to_agent, 'runScript', [["echo"]], function (result, error)
			if result then
				set_status({
					message = 'Agent is up',
					severity = 'ok'
				})
			else
				set_status({
					message = 'Agent is down',
					severity = 'error'
				})
			end
		end)
	end

	shv.add_timer(do_test, 1000 * 60 * 5) -- Every five minutes

	return do_test
end
