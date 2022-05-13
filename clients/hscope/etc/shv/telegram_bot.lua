local http_requests = require "http.request"
local lib = {}

function lib.sendMessage(self, chat_id, text)
    local uri = string.format('https://api.telegram.org/bot%s/sendMessage', self.bot_token)
    local req = http_requests.new_from_uri(uri)
    req:set_body(string.format('{"chat_id": %d,"text": "%s"}', chat_id, text:gsub('"', '\\"')))
    req.headers:append("content-type", "application/json")

    local headers, stream = req:go()
    local body = assert(stream:get_body_as_string())
    if headers:get ":status" ~= "200" then
        error(body)
    end
end

function lib.getRecentMessages(self)
    local uri = string.format('https://api.telegram.org/bot%s/getUpdates', self.bot_token)
    local req = http_requests.new_from_uri(uri)
    req.headers:append("content-type", "application/json")

    local headers, stream = req:go()
    local body = assert(stream:get_body_as_string())
    if headers:get ":status" ~= "200" then
        error(body)
    end
    return body
end

function lib.new(bot_token)
    local res = {bot_token = bot_token}
    setmetatable(res, {__index = lib})
    return res
end

return lib
