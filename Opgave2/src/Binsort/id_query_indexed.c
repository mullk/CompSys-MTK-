#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include "record.h"
#include "id_query.h"

struct index_record {
    int64_t osm_id;
    const struct record *record;
};

struct indexed_data {
    struct index_record *irs;
    int n;
};
//Forståelse af datastrukturen og structs: 
//https://chatgpt.com/share/68de35d7-81a4-800f-95cd-567ca7bf42fe

struct indexed_data* mk_indexed(struct record* rs, int n) {
    struct indexed_data *data = malloc(sizeof(struct indexed_data));
    //Struct indexed data allokeres
    data ->irs = malloc(n * sizeof(struct index_record));
    data ->n = n;
    //data -> irs. irs er en pointer til struct index_records.
    for (int i = 0; i < n; i++) { 
        data->irs[i].record = &rs[i];
        data->irs[i].osm_id =rs[i].osm_id; 
        //Pointer til indexeret data returneres.
    }
    return data;
}

void free_indexed(struct indexed_data* data) {
    //FRIGIVER POINTER
    free(data->irs);
    free(data);
}

const struct record* lookup_indexed(struct indexed_data *data, int64_t needle) {
    for (int i = 0; i < data->n; i++) { 
        if (data->irs[i].osm_id == needle) { 
            return data->irs[i].record; 
            //feltet record i index record returneres, 
            //som peger på struct record.
            //record pointer returneres, hvis osm.id == needle
        }
    }
    return NULL;
}

int main(int argc, char** argv) {
    return id_query_loop(argc, argv,
                         (mk_index_fn)mk_indexed,
                         (free_index_fn)free_indexed,
                         (lookup_fn)lookup_indexed);
}
