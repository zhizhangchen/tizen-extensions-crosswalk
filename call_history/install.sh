sdb -d push /home/john/GBS-ROOT/local/repos/tizen2.1/i586/RPMS/tizen-extensions-crosswalk-1.0.0-0.i586.rpm /tmp
sdb -d shell rpm -ivh --force /tmp/tizen-extensions-crosswalk-1.0.0-0.i586.rpm
sdb -d shell pkill -9 xwalk
sdb -d shell xwalk --remote-debugging-port=9222  --external-extensions-path=/usr/lib/tizen-extensions-crosswalk test.html
