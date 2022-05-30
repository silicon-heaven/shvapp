local function do_test()
    if shv.hscope_initializing() then
        shv.log_info("this is the first run")
        set_status({
            message = "Result from init run",
            severity = "good"
        })
    else
        shv.log_info("this is a subsequent run")
        set_status({
            message = "Result from a subsequent run",
            severity = "good"
        })
    end
end

shv.on_hscope_initialized(function ()
    do_test()
end)

return do_test
