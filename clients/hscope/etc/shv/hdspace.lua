return function (callback)
	shv.rpc_call("test/shvagent", "runScript", [[df -h | sed -n -r '1n;s/[^ ]+ +([^ ]+) +([^ ]+) +([^ ]+) +([^ ]+) +([^\n ]+)/\5 \2\/\1 \4/;p']], callback)
end
