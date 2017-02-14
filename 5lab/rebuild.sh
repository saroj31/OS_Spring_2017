mknod "/dev/kyouko3" c 500 127
rmmod mymod
make
insmod mymod.ko
gcc user.c

