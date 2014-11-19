LIBTOOL="libtool --tag=CC --silent"
$LIBTOOL --mode=compile cc   -O2 -I/usr/include/lua5.1 -I/usr/local/include/libfreenect -c src/vision.c
$LIBTOOL --mode=compile cc   -O2 -I/usr/include/lua5.1 -I/usr/local/include/libfreenect -c src/device.c
$LIBTOOL --mode=compile cc   -O2 -I/usr/include/lua5.1 -I/usr/local/include/libfreenect -c src/run.c
$LIBTOOL --mode=link cc   -O2 -I/usr/include/lua5.1  -I/usr/local/include/libfreenect /usr/local/lib/libfreenect.a -lusb-1.0 -rpath /usr/local/lib/lua/5.1 -o libvision.la vision.lo device.lo run.lo
mv .libs/libvision.so.0.0.0 vision.so
