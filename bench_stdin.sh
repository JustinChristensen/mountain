#!/usr/bin/env bash

stdin=${STDIN:-"./stdin"}
end=${1:-28}

top_kb_s=0
top_time=0
top_bufsiz=0

for i in $(seq 1 $end); do
    time=$(BUFSIZ="$i" "$stdin" < /dev/random)
    bufsiz=$((1 << $i))
    kb_s=$(($bufsiz * 1000000 / $time))     # bytes * 1000 / ns * 1000000000 = KB/s

    if [[ $kb_s -gt $top_kb_s ]]; then
        top_time="$time"
        top_bufsiz="$bufsiz"
        top_kb_s="$kb_s"
    fi
done

printf "%d bytes in %ld nanoseconds at %d KB/s\n" "$top_bufsiz" "$top_time" "$top_kb_s"
