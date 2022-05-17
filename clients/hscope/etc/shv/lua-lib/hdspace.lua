return function (callback, path_to_agent, filesystem_path)
	if path_to_agent == nil then
		path_to_agent = "test/shvagent"
	end

	if filesystem_path == nil then
		filesystem_path = ""
	end

	return function ()
		shv.rpc_call(path_to_agent, "runScript", string.format([[df -h %s | sed -n -r '1n;s/[^ ]+ +([^ ]+) +([^ ]+) +([^ ]+) +([^ ]+) +([^\n ]+)/\5 \2\/\1 \4/;p']], filesystem_path), callback)
	end
end
