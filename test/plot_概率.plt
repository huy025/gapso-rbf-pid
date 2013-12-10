#!/usr/bin/gnuplot

output_base_dir='/root/AQM/master-paper/figure/chapter6/'

# 输出eps（无法使用中文）
# set terminal postscript eps enhanced color solid linewidth 2 "Helvetica" 20 
# set output output_base_dir.'grpid-proba.eps'

# 输出pdf（可以使用中文）
set terminal pdfcairo lw 2 font "Time New Roman,10.5"
set output output_base_dir.'grpid-proba.pdf'

data_file1 = '../../pid/test/transfered_data.txt'
data_file2 = '../../rbf-pid/test/transfered_data.txt'
data_file3 = 'transfered_data.txt'

set xrange [0:10000]
set yrange [0:.14]

set xlabel "发包数（个）"

# plot data_file1 using 1:4 t "GRPID概率变化" with line lw 3,\
#     data_file2 using 1:4 t "RBF-PID概率变化" with line lw 2,\
#	 data_file3 using 1:4 t "PID概率变化" with line lw 1

#三行一列
set multiplot layout 3,1

#绘制图1
plot data_file1 using 1:4 t "PID" with lines lt 3
#绘制图2
plot data_file2 using 1:4 t "RBF-PID" with lines lt 3
#绘制图3
plot data_file3 using 1:4 t "GRPID" with lines lt 3

unset multiplot

set output
set terminal wxt

reset

