local shv_utils = require('shv_utils')
print('This is the root level configuration!')
print('my environment is:')
shv_utils.print_table(_ENV)

print("I'm setting something for the child environments!")
something = 123
