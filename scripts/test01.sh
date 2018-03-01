#!/bin/bash

for i in  www.baidu.com www.google.com 
do
    echo "To ping $i :"
    ping -c 1 $i
done