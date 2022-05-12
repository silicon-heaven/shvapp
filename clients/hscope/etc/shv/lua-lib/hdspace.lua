return function (filesystem_path)
	if filesystem_path == nil then
		filesystem_path = ""
	end

	return function (callback)
		shv.rpc_call("test/shvagent", "runScript", string.format([[df -h %s | sed -n -r '1n;s/[^ ]+ +([^ ]+) +([^ ]+) +([^ ]+) +([^ ]+) +([^\n ]+)/\5 \2\/\1 \4/;p']], filesystem_path), callback)
	end
end
