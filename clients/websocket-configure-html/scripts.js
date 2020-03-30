var configurationList = new Array();
var statusList = new Array();

var aptTest = "\
#!/bin/bash \n\
TEST=`ps -Af | grep apt | grep -v grep | wc -l` \n\
if [ $TEST -gt 0 ]; then \n\
  echo Packages is updating 1>&2  \n\
fi \n\
"

var pingDns = "\
#!/bin/bash \n\
ping -c 5 8.8.8.8 \n\
"
statusList.push({ "name": "Check DNS:", "id": "pingDns", "script": pingDns});

var sdCard = "\
#!/bin/bash \n\
m=`sudo fdisk -l | grep /dev/mmcblk0p1 | wc -l` \n\
if [ $m -gt 0 ]; then \n\
  size_sd=`sudo fdisk -l | grep /dev/mmcblk0: | sed 's/ //g; s/,/:/g; s/i//g; s/B//g' |cut -d: -f2` \n\
  size_part=`sudo fdisk -l | grep /dev/mmcblk0p1 | sed -e's/  */:/g' | cut -d: -f5` \n\
  if [ $size_sd != $size_part ]; then \n\
    echo WARNING: SD card not using all available space 1>&2 \n\
    df /dev/mmcblk0p1 \n\
  fi \n\
  m=`sudo mount | grep /dev/mmcblk0p1 | wc -l` \n\
  if [ $m -eq 0 ]; then \n\
    sudo mount /mnt/sd 2>/dev/null \n\
    if [ $? -gt 0 ]; then \n\
      echo Creating FS on SD card. \n\
      sudo mkfs.ext4 -F /dev/mmcblk0p1 \n\
    fi \n\
  fi \n\
  m=`sudo mount | grep /dev/mmcblk0p1 | wc -l` \n\
  if [ $m -gt 0 ]; then \n\
    if [ ! -e /mnt/sd/shvjournal ]; then \n\
      echo Creating shvjournal on SD card. \n\
      sudo mkdir /mnt/sd/shvjournal \n\
      sudo chown -R shv. /mnt/sd/shvjournal \n\
    fi \n\
    ls /mnt/sd/shvjournal \n\
    df /dev/mmcblk0p1 \n\
  fi \n\
else \n\
  echo Error: SD not inserted 1>&2 \n\
fi \n\
"

statusList.push({ "name": "SD Card:", "id": "sdCard", "script": sdCard});


var deviceId = "\
#!/bin/bash \n\
if [ ! -z $DEVICE_ID ]; then \n\
  /usr/bin/i2c_pwr_mgr -u $DEVICE_ID \n\
fi \n\
/usr/bin/i2c_pwr_mgr -i user_id | cut -d: -f2- \n\
"

configurationList.push({ "name": "Device ID:", "id": "deviceId", "script": deviceId, "env_name": "DEVICE_ID"});


var hwWatchDog = "\
#!/bin/bash \n\
if [ ! -z $WATCHDOG_TIME ]; then \n\
  /usr/bin/i2c_pwr_mgr -w $WATCHDOG_TIME \n\
fi \n\
/usr/bin/i2c_pwr_mgr -i watchdog_time | cut -d: -f2- \n\
"
configurationList.push({ "name": "HW Watchdog Time:", "id": "hwWatchDog", "script": hwWatchDog, "env_name": "WATCHDOG_TIME"});

var pppdapn = "\
#!/bin/bash \n\
apn=`sudo cat /etc/chatscripts/pap | grep AT+CGDCONT | cut -d, -f3` \n\
apn=${apn:1:-1} \n\
if [ ! -z $PPPD_APN ]; then \n\
  sudo sed  -i \\\"s/$apn/$PPPD_APN/g\\\" /etc/chatscripts/pap \n\
fi \n\
apn=`sudo cat /etc/chatscripts/pap | grep AT+CGDCONT | cut -d, -f3` \n\
apn=${apn:1:-1} \n\
echo $apn \n\
"
configurationList.push({ "name": "PPPD APN:", "id": "pppdapn", "script": pppdapn, "env_name": "PPPD_APN"});

var shvBroker = "\
#!/bin/bash \n\
host=`grep -l 'server' /etc/shv/shvbroker/shvbroker.conf | xargs grep '\\\"host\\\"' | tail -n 1 | sed 's/,//g; s/ //g' | cut -d: -f2` \n\
port=`grep -l 'server' /etc/shv/shvbroker/shvbroker.conf | xargs grep '\\\"port\\\"' | tail -n 1 | sed 's/ //g' | cut -d: -f2` \n\
echo $host:$port \n\
"
configurationList.push({ "name": "Shv broker connection:", "id": "shvBroker", "script": shvBroker, "env_name": "ADDRESS"});

var shvAgent = "\
#!/bin/bash \n\
host=`grep -l 'server' /etc/shv/shvagent.conf | xargs grep '\\\"host\\\"' | tail -n 1 | sed 's/,//g; s/ //g' | cut -d: -f2` \n\
port=`grep -l 'server' /etc/shv/shvagent.conf | xargs grep '\\\"port\\\"' | tail -n 1 | sed 's/ //g' | cut -d: -f2` \n\
echo $host:$port \n\
"
configurationList.push({ "name": "Shv agent connection:", "id": "shvAgent", "script": shvAgent, "env_name": "ADDRESS"});

var enable = "\
#!/bin/bash \n\
if [ ! -z $APP ]; then \n\
  NUM=$(echo $APP | sed 's/[^0-9]*//g') \n\
  NAME=$(echo $APP | sed 's/[0-9]*//g') \n\
  if [ -e /mnt/sd/shvjournal/ ]; then \n\
    if [ ! -e /mnt/sd/shvjournal/${APP} ]; then \n\
      mkdir /mnt/sd/shvjournal/${APP} \n\
    fi \n\
  fi \n\
  if [ ! -z $NUM ]; then \n\
    sudo systemctl enable ${NAME}@$NUM 1>/dev/null  2>&1 \n\
    sudo systemctl start ${NAME}@$NUM \n\
    echo `systemctl is-enabled ${NAME}@$NUM`:`systemctl is-active ${NAME}@$NUM` \n\
  else \n\
    sudo systemctl enable $APP 1>/dev/null 2>&1 \n\
    sudo systemctl start $APP \n\
    echo `systemctl is-enabled $APP`:`systemctl is-active $APP` \n\
  fi \n\
fi \n\
"
var start = "\
#!/bin/bash \n\
if [ ! -z $APP ]; then \n\
  NUM=$(echo $APP | sed 's/[^0-9]*//g') \n\
  NAME=$(echo $APP | sed 's/[0-9]*//g') \n\
  if [ -e /mnt/sd/shvjournal/ ]; then \n\
    if [ ! -e /mnt/sd/shvjournal/${APP} ]; then \n\
      mkdir /mnt/sd/shvjournal/${APP} \n\
    fi \n\
  fi \n\
  if [ ! -z $NUM ]; then \n\
    sudo systemctl start ${NAME}@$NUM \n\
    echo `systemctl is-enabled ${NAME}@$NUM`:`systemctl is-active ${NAME}@$NUM` \n\
  else \n\
    sudo systemctl start $APP \n\
    echo `systemctl is-enabled $APP`:`systemctl is-active $APP` \n\
  fi \n\
fi \n\
"

var disable = "\
#!/bin/bash \n\
if [ ! -z $APP ]; then \n\
  NUM=$(echo $APP | sed 's/[^0-9]*//g') \n\
  NAME=$(echo $APP | sed 's/[0-9]*//g') \n\
  if [ ! -z $NUM ]; then \n\
    sudo systemctl stop ${NAME}@$NUM \n\
    sudo systemctl disable ${NAME}@$NUM 1>/dev/null  2>&1 \n\
    sudo systemctl stop ${NAME}@$NUM \n\
    echo `systemctl is-enabled ${NAME}@$NUM`:`systemctl is-active ${NAME}@$NUM` \n\
  else \n\
    sudo systemctl stop $APP \n\
    sudo systemctl disable $APP 1>/dev/null  2>&1 \n\
    sudo systemctl stop $APP \n\
    echo `systemctl is-enabled $APP`:`systemctl is-active $APP` \n\
  fi \n\
fi \n\
"

var stop = "\
#!/bin/bash \n\
if [ ! -z $APP ]; then \n\
  NUM=$(echo $APP | sed 's/[^0-9]*//g') \n\
  NAME=$(echo $APP | sed 's/[0-9]*//g') \n\
  if [ ! -z $NUM ]; then \n\
    sudo systemctl stop ${NAME}@$NUM \n\
    echo `systemctl is-enabled ${NAME}@$NUM`:`systemctl is-active ${NAME}@$NUM` \n\
  else \n\
    sudo systemctl stop $APP \n\
    echo `systemctl is-enabled $APP`:`systemctl is-active $APP` \n\
  fi \n\
fi \n\
"

var upgrade = "\
#!/bin/bash \n\
cd /home/shv \n\
wget http://shv.elektroline.cz/repo/pool/main/s/shvsysconf/shvsysconf-predator_1.3_all.deb \n\
dpkg -i --force-all shvsysconf-predator_1.3_all.deb \n\
sudo systemctl start shv-upgrade \n\
"

var app1 = "\
#!/bin/bash \n\
n=$( expr $(cat /etc/shv/shvbroker/fstab.cpon|wc -l) - 1) \n\
m=$( expr $(cat /etc/shv/shvbroker/fstab.cpon|wc -l) - 2) \n\
cat /etc/shv/shvbroker/fstab.cpon | tail -n $n | head -n $m > /tmp/1 \n\
cat /tmp/1 | sed 's/ //g; s/,//g; s/{//g; s/}//g' > /tmp/2 \n\
for line in `cat /tmp/2`; do \n\
    id=`echo $line | cut -d':' -f1` \n\
    id=${id:1:-1} \n\
    app=`echo $line | cut -d':' -f3` \n\
    app=${app:5:-1} \n\
    out=${id}:${app} \n\
    id_num=$(echo $id | sed 's/[^0-9]*//g') \n\
    id_name=$(echo $id | sed 's/[0-9]*//g') \n\
    if [ ! -z $id_num ]; then \n\
      service_id=${id_name}@$id_num \n\
    else \n\
      service_id=${id_name} \n\
    fi \n\
    enabled=`systemctl is-enabled $service_id 2>/dev/null` \n\
    if [ -z $enabled ]; then \n\
      enabled='disabled' \n\
    fi \n\
    actived=`systemctl is-active $service_id 2>/dev/null` \n\
    out=${out}:${enabled}:${actived}, \n\
    echo $out \n\
done \n\
"


var app2 = "\
#!/bin/bash \n\
lst[0]='shvdiscon:discon'\n\
lst[1]='shvanca@16:anca1'\n\
lst[2]='shvanca@17:anca2'\n\
lst[3]='shvanca@18:anca3'\n\
lst[4]='shvanca@19:anca4'\n\
lst[5]='shvanca@20:anca5'\n\
lst[6]='shvanca@21:anca6'\n\
lst[7]='shvanca@22:anca7'\n\
lst[8]='shvanca@23:anca8'\n\
lst[9]='shvanca@24:anca9'\n\
lst[10]='shvanca@25:anca10'\n\
lst[11]='shvanca@26:anca11'\n\
lst[12]='shvanca@27:anca12'\n\
lst[13]='shvanca@28:anca13'\n\
lst[14]='shvanca@29:anca14'\n\
lst[15]='shvanca@30:anca15'\n\
lst[16]='shvanca@31:anca16'\n\
lst[17]='shvandi@32:andi1'\n\
lst[18]='shvandi@33:andi2'\n\
lst[19]='shviolca@34:iolca1'\n\
lst[20]='shviolca@35:iolca2'\n\
lst[21]='shvdisconiolca@34:discon1'\n\
lst[22]='shvdisconiolca@35:discon2'\n\
lst[23]='shvconv:conv'\n\
lst[24]='shviolca:iolca'\n\
lst[25]='shvdevmgr:devmgr'\n\
lst[26]='shvtbusswitch:tbusSwitch'\n\
lst[27]='shvpredatorio:predatorio'\n\
lst[28]='shvyicgps:yicgps'\n\
lst[29]='shvbrccat:brccat'\n\
lst[30]='shvagent:agent'\n\
for item in ${lst[*]}\n\
do\n\
    service_id=`echo ${item} | cut -d':' -f1`\n\
    TMP=`ls /etc/systemd/system/multi-user.target.wants/ | grep $service_id`\n\
    app=`echo ${service_id} | sed -e 's/@//'`\n\
    out=$item\n\
    if [ -z $TMP ]; then\n\
        out=${out}':disabled:inactive'\n\
    else\n\
        out=${out}':enabled:'`systemctl is-active $service_id 2>/dev/null`\n\
    fi\n\
    echo ${out},\n\
done\n\
"