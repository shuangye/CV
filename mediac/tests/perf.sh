#!/system/bin/sh

# this script is for Android


client=0
mediac='/data/rk_backup/mediac'

while true;
do
    /data/rk_backup/mediac 0 close
    /data/rk_backup/mediac 1 close
    sleep 1   
              
    /data/rk_backup/mediac 0 open
    /data/rk_backup/mediac 1 open
    /data/rk_backup/mediac 0 start
    /data/rk_backup/mediac 1 start
    sleep 10
done
