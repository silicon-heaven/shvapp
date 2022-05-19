local shv_utils = require('shv_utils')
shv.log_info('This is the root level configuration!')
shv.log_info('my environment is:')
shv.log_info(shv_utils.print_table(_ENV))

shv.log_info("I'm setting something for the child environments!")
something = 123
