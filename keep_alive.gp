set title "make check"
set xlabel "times"
set ylabel "requests/sec"
set terminal png font " Times_New_Roman,12 "
set key left top
set output "keep_alive.png"

plot \
"cmp_keep_alive" using 1:2 with linespoints linewidth 2 title "close", \
"cmp_keep_alive" using 1:3 with linespoints linewidth 2 title "keep alive"
