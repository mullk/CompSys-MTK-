#!/bin/bash
# Benchmark script for fhistogram and fhistogram-mt

SEQ_PROGRAM=./fhistogram
MT_PROGRAM=./fhistogram-mt
DIR=testdir
RUNS=10
THREADS=(1 2 3 4)

# Sørg for at vi bruger engelsk format for decimaler
export LC_ALL=C

if [[ ! -x $SEQ_PROGRAM ]]; then
  echo "Error: $SEQ_PROGRAM not found or not executable"
  exit 1
fi

if [[ ! -x $MT_PROGRAM ]]; then
  echo "Error: $MT_PROGRAM not found or not executable"
  exit 1
fi

if [[ ! -d $DIR ]]; then
  echo "Error: directory '$DIR' not found"
  exit 1
fi

echo "Starter benchmark af histogram-programmer i mappen '$DIR' ($RUNS gentagelser pr. test)"
echo "-------------------------------------------------------------"

# Kør sekventiel version én gang for reference
for n in "${THREADS[@]}"; do
  total=0.0
  if [[ $n -eq 1 ]]; then
    echo -n "📘 Sekventiel version (fhistogram) – 1 tråd → "
    for ((i=1; i<=RUNS; i++)); do
      t=$(/usr/bin/time -p $SEQ_PROGRAM $DIR 2>&1 | grep real | awk '{print $2}' | tr ',' '.')
      total=$(echo "$total + $t" | bc -l)
    done
    avg=$(echo "$total / $RUNS" | bc -l)
    echo "$avg sek."
  else
    echo -n "⚙️  Multithreaded version (fhistogram-mt -n $n) → "
    total=0.0
    for ((i=1; i<=RUNS; i++)); do
      t=$(/usr/bin/time -p $MT_PROGRAM -n $n $DIR 2>&1 | grep real | awk '{print $2}' | tr ',' '.')
      total=$(echo "$total + $t" | bc -l)
    done
    avg=$(echo "$total / $RUNS" | bc -l)
    echo "$avg sek."
  fi
done

echo "-------------------------------------------------------------"
echo "Benchmark færdig ✅"
