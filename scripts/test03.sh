#/bin/bash
echo "Current User is :"
echo $(ps | grep "$$" | awk '{print $2}')


sum=0
for I in {1..50}; do
    sum=$(($sum+2*$I))
    echo $sum
done
echo "the sum is $sum"