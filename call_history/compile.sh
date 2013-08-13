DEST_DIR=/home/john/GBS-ROOT/local/scratch.i586.0/tmp/tizen-extensions-crosswalk
#rm -rf $DEST_DIR
mkdir -p  $DEST_DIR
cp -ra ../../tizen-extensions-crosswalk/* $DEST_DIR
sudo cp /home/john/GBS-ROOT/local/scratch.i586.0/usr/bin/whereis /home/john/GBS-ROOT/local/scratch.i586.0/usr/bin/which
echo "cd /tmp/tizen-extensions-crosswalk && PATH=$PATH:../gyp ./configure && make" | gbs chroot /home/john/GBS-ROOT/local/scratch.i586.0/ 2>&1 |grep -i -B3 error
if [ $? = 1 ]; then
  sdb -d root on
  sdb -d push $DEST_DIR/out/Default/libtizen_call_history.so /usr/lib/tizen-extensions-crosswalk
  sdb -d push $DEST_DIR/out/Default/libjsc-wrapper.so /usr/lib
  sdb -d push test.html /tmp
  sdb -d shell pkill -9 xwalk
  sdb -d shell TIZEN_PLATFORMLOGGING_MODE=1 TIZEN_DLOG_LEVEL=1 xwalk --remote-debugging-port=9222  --external-extensions-path=/usr/lib/tizen-extensions-crosswalk /tmp/test.html
fi
