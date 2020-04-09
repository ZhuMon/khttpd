#! /bin/bash
url="fib/"

if [[ $#>"0" ]]; then
    k="$1"
else
    k="10"
fi

url="${url}${k}"

make >> /dev/null
if [[ $? -ne 0 ]]; then
    return
fi

sudo rmmod khttpd
sudo insmod khttpd.ko port=1999
wget localhost:1999/"${url}"

