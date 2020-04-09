#! /bin/sh

l_time='/usr/bin/time'
time_opt='-f%e\t%U\t%S'
#time_opt='-f%S'
#time_opt="-p"
get_html='wget -q -O /dev/null localhost:1999/fib/'

output_file="output.txt"

rm -f ${output_file}

for i in `seq 0 10000 100000`
do
    echo -n "${i}\t" >> ${output_file}
    { time "${time_opt}" --quiet ${get_html}"${i}" >/dev/null;  }  2>> ${output_file}
done

for i in `seq 100000 100000 10000000`
do
    echo -n "${i}\t" >> ${output_file}
    { time "${time_opt}" --quiet ${get_html}"${i}" >/dev/null;  }  2>> ${output_file}
done


