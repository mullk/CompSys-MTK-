#!/bin/bash

SEQ_PROGRAM=./fhistogram
MT_PROGRAM=./fhistogram-mt
DIR=testdir
RUNS=10
THREADS=(1 2 3 4)


if [[ ! -x "$SEQ_PROGRAM" ]]; then
  echo "Fejl: $SEQ_PROGRAM blev ikke fundet eller er ikke eksekverbart."
  exit 1
fi
if [[ ! -x "$MT_PROGRAM" ]]; then
  echo "Fejl: $MT_PROGRAM blev ikke fundet eller er ikke eksekverbart."
  exit 1
fi
if [[ ! -d "$DIR" ]]; then
  echo "Fejl: mappen '$DIR' findes ikke."
  exit 1
fi

echo "Starter benchmark af histogram-programmer i mappen '$DIR' ($RUNS gentagelser pr. test)"
echo "-------------------------------------------------------------"

base_time=0

for n in "${THREADS[@]}"; do
  total=0.0

  if [[ $n -eq 1 ]]; then
    echo -n "📘 Sekventiel version (fhistogram) – 1 tråd → "
    for ((i=1; i<=RUNS; i++)); do
      t=$( { /usr/bin/time -p $SEQ_PROGRAM $DIR > /dev/null; } 2>&1 | grep real | awk '{print $2}' )
      total=$(echo "$total + $t" | bc -l)
    done
  else
    echo -n "🧵 Multitrådet version (fhistogram-mt) – $n tråde → "
    for ((i=1; i<=RUNS; i++)); do
      t=$( { /usr/bin/time -p $MT_PROGRAM -n $n $DIR > /dev/null; } 2>&1 | grep real | awk '{print $2}' )
      total=$(echo "$total + $t" | bc -l)
    done
  fi

  avg=$(echo "scale=3; $total / $RUNS" | bc -l)
  if [[ $n -eq 1 ]]; then
    base_time=$avg
    echo "Gennemsnitlig tid: ${avg}s (baseline)"
  else
    speedup=$(echo "scale=2; $base_time / $avg" | bc -l)
    echo "Gennemsnitlig tid: ${avg}s → Speedup: ${speedup}x"
  fi
done

echo "-------------------------------------------------------------"
echo "Benchmark færdig!"
