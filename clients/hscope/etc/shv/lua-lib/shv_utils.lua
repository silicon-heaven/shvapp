local utils = {
}

function utils.print_table(table)
	for k, v in pairs(table) do
		print(string.format('%s=%s', k, v))
	end
end

return utils
