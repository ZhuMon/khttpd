set title "request -> response"
set xlabel "offset"
set ylabel "time(s)"
set terminal png font " Times_New_Roman,12 "
set output "pt5_khttpd.png"

plot \
"pt5_khttpd.txt" using 1:2 with linespoints linewidth 2 title "protocol5", \
"pt5_khttpd.txt" using 1:3 with linespoints linewidth 2 title "khttpd", \
