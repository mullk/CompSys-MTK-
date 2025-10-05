# --- Kompileringsindstillinger ---
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=gnu11 -g

# --- Fælles kode ---
COMMON = common/record.c

# --- Programmer ---

# --- ID QUERIES ---
id_query_naive: $(COMMON) common/id_query.c naive/id_query_naive.c
	$(CC) $(CFLAGS) -Icommon -o $@ $^ -lm

id_query_indexed: $(COMMON) common/id_query.c indexed/id_query_indexed.c
	$(CC) $(CFLAGS) -Icommon -o $@ $^ -lm

id_query_binsort: $(COMMON) common/id_query.c binsort/id_query_binsort.c
	$(CC) $(CFLAGS) -Icommon -o $@ $^ -lm

id_query_eytzinger: $(COMMON) common/id_query.c eytzinger/Eytzinger.c eytzinger/id_query_eytzinger.c eytzinger/EytzingerMain.c
	$(CC) $(CFLAGS) -Icommon -o $@ $^ -lm


# --- COORDINATE QUERIES ---
coord_query_naive: $(COMMON) common/coord_query.c coord_query_naive.c
	$(CC) $(CFLAGS) -Icommon -o $@ $^ -lm

coord_query_kdtree: $(COMMON) common/coord_query.c kdtree/coord_query_kdtree.c
	$(CC) $(CFLAGS) -Icommon -o $@ $^ -lm


# --- Random ID-generator ---
random_ids: common/record.c random_ids.c
	$(CC) $(CFLAGS) -Icommon -o $@ $^ -lm


# --- Rengør projektet ---
clean:
	rm -f id_query_naive id_query_indexed id_query_binsort id_query_eytzinger \
	coord_query_naive coord_query_kdtree random_ids *.o
