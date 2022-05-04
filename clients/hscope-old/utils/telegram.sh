#!/bin/bash
#
#  This script sends whatever is piped to it as a message to the specified Telegram bot
#
message=$1
apiToken=894962268:AAHoTaHnlNWDfe3xJWN_RHjn6rxvYQSaGQI
# example:
# apiToken=123456789:AbCdEfgijk1LmPQRSTu234v5Wx-yZA67BCD
userChatId=144823520
# example:
# userChatId=123456789
update_id=996097068

sendTelegram() {
        curl -s \
        -X POST \
        https://api.telegram.org/bot$apiToken/sendMessage \
        -d text="$message" \
        -d chat_id=$userChatId
}

getUpdates() {
        curl -s \
        -X POST \
        https://api.telegram.org/bot$apiToken/getUpdates \
        -d timeout=100 \
        -d offset=$update_id \

}

if  [[ -z "$message" ]]; then
        getUpdates
else
        sendTelegram
fi

