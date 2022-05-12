local shv_utils = require('shv_utils')
local hdspace = require('hdspace')
print('my environment is:')
shv_utils.print_table(_ENV)

return function (callback)
	print("hdspace", "=", hdspace)
	hdspace(function (value)
		callback(value)
	end)
end
