return function (set_status, path_to_agent, filesystem_path)
	if path_to_agent == nil then
		error("path_to_agent mustn't be null")
	end

	if filesystem_path == nil then
		error("filesystem_path mustn't be null")
	end

	return function ()
		shv.rpc_call(path_to_agent, "runScript", string.format([[df %s | sed -n -r '1n;s/[^ ]+ +[^ ]+ +[^ ]+ +[^ ]+ +([^ ]+)%% +[^\n ]+/\1/;p']], filesystem_path), function (result)
			local percent = tonumber(shv.cpon_to_string(result))
			local res = {
				message = tostring(percent) .. "% usage"
			}

			res.severity = "good"
			if percent >= 75 then
				res.severity = "warn"
			end
			if percent >= 90 then
				res.severity = "error"
			end

			set_status(res)
		end)
	end
end
