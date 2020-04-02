#!$SHELL
if [ $#<2 ]; then
    url=$1
else
    url="fib"
fi

echo "${url}"

make
sudo rmmod khttpd
sudo insmod khttpd.ko port=1999
wget localhost:1999/"${url}"

