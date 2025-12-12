#!/bin/bash
err=0
file_1=$(tr '\n' ';' < $1)
file_2=$(tr '\n' ';' < $2)
IFS=';'

printf "Comparing files generated from %s...\n" $3

counter1=0
for inst1 in $file_1; do
    counter2=0
    for inst2 in $file_2; do
        if [ ! -z $inst1 ]; then
            if [ ! -z $inst2 ]; then
                if [ $counter1 -eq $counter2 ]; then
                    if [ $inst1 != $inst2 ]; then
                        printf "[%i: %s; %s]\n" $counter1 $inst1 $inst2
                        err=1
                    fi
                fi
            fi
        fi
        ((counter2+=1))
    done
    ((counter1+=1))
done

if [ $err -eq 0 ]; then
    printf "No errors found.\n"
fi