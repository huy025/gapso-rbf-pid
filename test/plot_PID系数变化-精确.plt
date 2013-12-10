#!/usr/bin/gnuplot

output_base_dir='/root/AQM/master-paper/figure/chapter6/'

# 输出eps（无法使用中文）
# set terminal postscript eps enhanced color solid linewidth 2 "Helvetica" 20 
# set output output_base_dir.'grpid-kp-ki-kd.eps'

# 输出pdf（可以使用中文）
set terminal pdfcairo lw 2 font "Time New Roman,10.5"
set output output_base_dir.'grpid-kp-ki-kd-precise.pdf'

data_file = '/root/AQM/gapso-rbf-pid/test/transfered_data.txt'

set xrange [2000:10000]
#set yrange [0:0.1]

set xlabel "发包数（个）"

#plot data_file using 1:5 t "kp" with lines lt 3 lc rgbcolor "blue"
#     data_file using 1:6 t "ki" with lines lt 3 lc rgbcolor "red",\
#     data_file using 1:7 t "kd" with lines lt 3 lc rgbcolor "green"

#三行一列
set multiplot layout 3,1

#绘制图1
plot data_file using 1:5 t "比例系数kp" with lines lt 3
#绘制图2
plot data_file using 1:6 t "积分系数ki" with lines lt 3
#绘制图3
plot data_file using 1:7 t "微分系数kd" with lines lt 3

unset multiplot

unset multiplot

set output
set terminal wxt

reset

