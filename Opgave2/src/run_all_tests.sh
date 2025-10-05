#!/bin/bash
# ------------------------------------------------------------
#  Build og performance-test af alle query-programmer
# ------------------------------------------------------------

set -e  # stop scriptet ved første fejl

echo "=== Bygger alle programmer ==="
make clean
make id_query_eytzinger
make id_query_naive
make id_query_binsort
make id_query_indexed
make coord_query_naive
make coord_query_kdtree
echo "✅ Build færdig"
echo ""

# Funktion til pænt output
run_test() {
    local exe=$1
    local datafile=$2
    local inputfile=$3

    echo ""
    echo "▶️  Kører: $exe  (data: $datafile, input: $inputfile)"
    echo "----------------------------------------------"
    { time ./$exe $datafile < $inputfile > /dev/null; } 2>&1
    echo "----------------------------------------------"
}

# ------------------------------------------------------------
#  Lille datasæt (20.000 records)
# ------------------------------------------------------------
echo ""
echo "=============================================="
echo "=== TEST: Lille datasæt (20.000 records) ==="
echo "=============================================="

# ID queries
run_test id_query_naive     data/20000records.tsv data/random_ids.txt
run_test id_query_indexed   data/20000records.tsv data/random_ids.txt
run_test id_query_binsort   data/20000records.tsv data/random_ids.txt
run_test id_query_eytzinger data/20000records.tsv data/random_ids.txt

# Coordinate queries
run_test coord_query_naive  data/20000records.tsv data/random_coords.txt
run_test coord_query_kdtree data/20000records.tsv data/random_coords.txt


# ------------------------------------------------------------
#  Større test (Bigfiles)
# ------------------------------------------------------------
echo ""
echo "=============================================="
echo "=== TEST: Større datasæt (Bigfiles) ==="
echo "=============================================="

# ID queries
run_test id_query_naive     data/20000records.tsv data/Bigrandom_ids.txt
run_test id_query_indexed   data/20000records.tsv data/Bigrandom_ids.txt
run_test id_query_binsort   data/20000records.tsv data/Bigrandom_ids.txt
run_test id_query_eytzinger data/20000records.tsv data/Bigrandom_ids.txt

# Coordinate queries
run_test coord_query_naive  data/20000records.tsv data/Bigcoords.txt
run_test coord_query_kdtree data/20000records.tsv data/Bigcoords.txt

echo ""
echo "✅ Alle tests færdige!"
