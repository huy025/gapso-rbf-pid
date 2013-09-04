#!/bin/bash
#this is a bash script to read logdata about TCP connection
#2006-4-18,lianlinjiang,BUAA-CSE
echo "read logdata about queue connection......"
touch queuedata_rbfgrad.txt
rm queuedata_rbfgrad.txt
touch queuedata_rbfgrad.txt
i=0
while [ "$i" -le "30000" ]
do
insmod /root/AQM/gapso-rbf-pid/queuedata/seqfile_queuedata_rbfgrad.ko queue_array_count=$i
cat /proc/data_seq_file >> queuedata_rbfgrad.txt
rmmod seqfile_queuedata_rbfgrad
i=$[$i + 40]
done
echo "read logdata succed! pelease see queuedata.txt"
