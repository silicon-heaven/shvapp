local shv_utils = require("shv_utils")
return function (set_status, path_to_agent, filesystem_path)
	if path_to_agent == nil then
		error("path_to_agent mustn't be null")
	end

	if filesystem_path == nil then
		error("filesystem_path mustn't be null")
	end

	return function ()
		shv.rpc_call(path_to_agent, "runScript", string.format([[df %s | sed -n -r '1n;s/[^ ]+ +[^ ]+ +[^ ]+ +[^ ]+ +([^ ]+)%% +[^\n ]+/\1/;p']], filesystem_path), function (result)
			local percent = tonumber(result.value)
			shv_utils.print_table(result.meta)
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
