CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=gnu11 -g -Icommon
LDFLAGS=-lm

# programmer vi vil bygge
PROGRAMS = id_query_naive \
           coord_query_naive \
           id_query_binsort \
           id_query_eytzinger \
           coord_query_kdtree \
           random_ids

.PHONY: all clean test ../src.zip

all: $(PROGRAMS)

# --- fælles kode ---
COMMON = common/record.c 


# --- programmer ---

# --- ID QUERIES ---

id_query_naive: common/record.c common/id_query.c binsort/id_query_naive.c
	$(CC) $(CFLAGS) -Icommon -o $@ $^ -lm

id_query_indexed: common/record.c common/id_query.c binsort/id_query_indexed.c
	$(CC) $(CFLAGS) -Icommon -o $@ $^ -lm

id_query_binsort: common/record.c common/id_query.c binsort/id_query_binsort.c
	$(CC) $(CFLAGS) -Icommon -o $@ $^ -lm

id_query_eytzinger: common/record.c common/id_query.c eytzinger/Eytzinger.c eytzinger/id_query_eytzinger.c eytzinger/EytzingerMain.c
	$(CC) $(CFLAGS) -Icommon -o $@ $^ -lm



# --- COORDINATE QUERIES ---

coord_query_naive: common/record.c common/coord_query.c naive/coord_query_naive.c
	$(CC) $(CFLAGS) -Icommon -o $@ $^ -lm

coord_query_kdtree: common/record.c common/coord_query.c kdtree/coord_query_kdtree.c
	$(CC) $(CFLAGS) -Icommon -o $@ $^ -lm

# --- ekstra targets ---
test: $(PROGRAMS)
	@set e; for prog in $(PROGRAMS); do echo ./$$prog; ./$$prog; done

clean:
	rm -f $(PROGRAMS) *.o

planet-latest-geonames.tsv:
	wget https://github.com/OSMNames/OSMNames/releases/download/v2.0.4/planet-latest_geonames.tsv.gz
	gunzip -f planet-latest_geonames.tsv.gz

../src.zip:
	make clean
	cd .. && zip -r src.zip src

.SECONDARY:
