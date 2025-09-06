#!/bin/bash

if [ $# -eq 0 ]; then
    echo "No arguments provided"
    exit 1
fi

sum=0
count=0

for num in "$@"; do
    sum=$((sum + num))
    count=$((count + 1))
done

average=$(echo "scale=2; $sum / $count" | bc)
echo "Numbers: $count"
echo "Average: $average"
