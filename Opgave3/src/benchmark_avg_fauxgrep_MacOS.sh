#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 PROGRAM ARGS..."
  echo "Example: $0 ./fauxgrep-mt -n 4 needle testdir/"
  exit 1
fi

PROGRAM="$1"
shift
ARGS=( "$@" )

RUNS=10
total=0

echo "Running: $PROGRAM ${ARGS[*]}"
echo "Runs: $RUNS"

for i in $(seq 1 $RUNS); do
  # Programmet køres:
  # Bemærk: tidsoutput kommer på stderr:
  t=$({ /usr/bin/time -p "$PROGRAM" "${ARGS[@]}" > /dev/null; } 2>&1 | awk '/^real/ {print $2}')

  if [ -z "$t" ]; then
    echo "Run $i: failed to capture time output"
    exit 2
  fi

  # Printer run tid
  printf "Run %2d: %ss\n" "$i" "$t"

  # Summer tider med awk for at undgå floating-point problemer i bash
  total=$(awk -v a="$total" -v b="$t" 'BEGIN{printf "%.6f", a + b}')
done

avg=$(awk -v total="$total" -v n="$RUNS" 'BEGIN{printf "%.6f", total / n}')
echo "--------------------------"
printf "Average (real): %ss\n" "$avg"
