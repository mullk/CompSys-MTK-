#!/usr/bin/env bash
set -u

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
RED='\033[0;31m'
NC='\033[0m'

# Brug engelsk numerisk format for at sikre korrekt decimaladskillelse
export LC_NUMERIC=C

if [ "$#" -lt 3 ]; then
  echo "Usage: $0 SEARCHWORD DIRECTORY THREADS"
  echo "Example: $0 int src/ 4"
  exit 1
fi

needle="$1"; dir="$2"; threads="$3"; runs=10
out=$(mktemp)

benchmark_program() {
  local program=$1; shift
  local args=("$@")
  local total=0 best=999999 worst=0

  {
    echo -e "\n${YELLOW}==== Resultater for ${program} ====${NC}"
    echo -e "${BLUE}Command:${NC} $program ${args[*]}"
    echo -e "${BLUE}Runs:${NC} $runs"
    echo "-----------------------------------------"

    for i in $(seq 1 "$runs"); do

      # Hent real-tid og konverter evt. komma til punktum
      t=$({ /usr/bin/time -p "$program" "${args[@]}" >/dev/null; } 2>&1 \
          | awk '/^real/{print $2} /real$/{print $1}' | tr ',' '.')

      [ -z "$t" ] && continue
      total=$(awk -v a="$total" -v b="$t" 'BEGIN{printf "%.6f", a+b}')
      (( $(echo "$t < $best" | bc -l) )) && best=$t
      (( $(echo "$t > $worst" | bc -l) )) && worst=$t
      printf "Run %2d: %.6fs\n" "$i" "$t"
    done

    avg=$(awk -v total="$total" -v n="$runs" 'BEGIN{printf "%.6f", total/n}')
    echo "-----------------------------------------"
    printf "${YELLOW}Average runtime:${NC} %8.3fs\n" "$avg"
    printf "${GREEN}Fastest:${NC} %8.2fs   ${RED}Slowest:${NC} %8.2fs\n" "$best" "$worst"
    echo "-----------------------------------------"
  } >>"$out"

  echo "$avg"
}

avg_single=$(benchmark_program ./fauxgrep "$needle" "$dir")
avg_multi=$(benchmark_program ./fauxgrep-mt -n "$threads" "$needle" "$dir")

speedup=$(awk -v s="$avg_single" -v m="$avg_multi" \
              'BEGIN{if(m>0)printf"%.2f",s/m;else print "0"}')

{
  echo -e "\n${YELLOW}==== Sammenligning (Single vs Multi) ====${NC}"
  echo "-----------------------------------------"
  printf "${BLUE}Single-thread average:${NC}      %8.3fs\n" "$avg_single"
  printf "${BLUE}Multi-thread average (${threads} tråde):${NC} %8.3fs\n" "$avg_multi"
  printf "${GREEN}Speedup:${NC} %5.2fx hurtigere\n" "$speedup"
  echo "========================================="
} >>"$out"

clear
cat "$out"
rm -f "$out"
