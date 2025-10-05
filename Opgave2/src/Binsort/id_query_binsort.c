#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include "record.h"
#include "id_query.h"

struct binary_sort_data {
    struct record *rs;
    int n;
};

int comp_record_ID_asc(const void *elem1, const void *elem2) {
    const struct record *record_1 = elem1;
    const struct record *record_2 = elem2;

    if (record_1->osm_id > record_2->osm_id) return 1;
    if (record_1->osm_id < record_2->osm_id) return -1;
    return 0;
}

struct binary_sort_data* mk_binary_sort(struct record* rs, int n) {
    struct binary_sort_data *data = malloc(sizeof(*data));
    //sætter struct naive_data's fields
    data->rs =rs; 
    data->n =n;
    //data peger på structen.
    qsort(rs, n, sizeof(struct record), comp_record_ID_asc);
    return data;
} 

void free_binary_sort(struct binary_sort_data* data) {
    free(data);
}

const struct record* binary_lookup(struct binary_sort_data *data, int64_t needle) {
    int low = 0;
    int high = data->n - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        // Check om x er i midten
        if (data->rs[mid].osm_id == needle) 
            return &data ->rs[mid]; 
            // "mid" kan ikke direkte returneres, da det er en int.
            // En pointer til struct record returneres,
            // da det matcher retur-typen.
        // Hvis x er større, ignorer venstre halvdel
        if (data->rs[mid].osm_id < needle)
            low = mid + 1;
        // Hvis x er mindre, ignorer højre halvdel
        else
            high = mid - 1;
    }
    // Hvis vi kommer herned, findes ID'et ikke.
    return NULL;
}
    //Binær søgning i C: https://www.geeksforgeeks.org/dsa/binary-search/

int main(int argc, char** argv) {
    return id_query_loop(argc, argv,
                         (mk_index_fn)mk_binary_sort,
                         (free_index_fn)free_binary_sort,
                         (lookup_fn)binary_lookup);
}
