#!/usr/bin/gnuplot
data_file2 = '/root/AQM/gapso-rbf-pid/test/transfered_data.txt'
set xrange [0:1000]
set yrange [0:600]
#绘制图2
plot data_file2 using 1:9 t "netout" with lines lt 3 lc rgbcolor "blue",\
     data_file2 using 1:2 t "netout" with lines lt 3 lc rgbcolor "red"
reset
pause -1
