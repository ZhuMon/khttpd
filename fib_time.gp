set title "request -> response"
set xlabel "offset"
set ylabel "time(s)"
set terminal png font " Times_New_Roman,12 "
set output "fib_time.png"

plot \
"output.txt" using 1:2 with linespoints linewidth 2 title "real time", \
"output.txt" using 1:3 with linespoints linewidth 2 title "user time", \
"output.txt" using 1:4 with linespoints linewidth 2 title "sys time"
