#!/usr/bin/gnuplot

output_base_dir='/root/AQM/master-paper/figure/chapter6/'

# 输出eps（无法使用中文）
# set terminal postscript eps enhanced color solid linewidth 2 "Helvetica" 20 
# set output output_base_dir.'grpid-matching-result.eps'

# 输出pdf（可以使用中文）
set terminal pdfcairo lw 2 font "Time New Roman,10.5"
set output output_base_dir.'grpid-matching-result.pdf'

data_file = 'transfered_data.txt'

set xrange [0:1000]
set yrange [0:600]

set xlabel "发包数（个）"

plot data_file using 1:9 t "预测队长（RBF网络输出层值)" with lines lt 3 lc rgbcolor "blue",\
     data_file using 1:2 t "实际队长" with lines lt 3 lc rgbcolor "red"

unset multiplot

set output
set terminal wxt

reset

