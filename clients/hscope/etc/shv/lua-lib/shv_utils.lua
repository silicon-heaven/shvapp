local utils = {
}

function utils.print_table(table, key_prefix)
	if key_prefix == nil then
		key_prefix = ''
	end
	for k, v in pairs(table) do
		if type(v) == "table" then
			utils.print_table(v, key_prefix .. k .. '.')
		else
			shv.log_info(string.format('%s%s=%s', key_prefix, k, v))
		end
	end
end

return utils
