#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include "record.h"
#include "id_query.h"

struct naive_data {
  struct record *rs; //Peger på array med records. 
  int n; 
}; 
  //Array med records tilgås vha. record.h. 
  //rs er en pointer til et array, som indeholder records
struct naive_data* mk_naive(struct record* rs, int n) {

  struct naive_data *data = malloc(sizeof(struct naive_data));
//data peger på struct naive_data, hvor records (n) kan hentes.

  if (data == NULL) {
    printf("Allocation Failed\n");
    exit(1);
  } //Hvis malloc fejler og der ikke er hukommelse tilbage.

  data->rs =rs; //variabel for pointer til alle records
  data->n =n; //variabel for pointer til antallet af records
  return data;
  //https://www.geeksforgeeks.org/c/dynamic-memory-allocation-in-c-using-malloc-calloc-free-and-realloc/
}

void free_naive(struct naive_data* data) {
  free(data);//Frigør allokkeret hukommelse tilbage til hukommelsen. 
}

const struct record* lookup_naive(struct naive_data *data, int64_t needle) {
  for (int i = 0; i < data->n; i++) { //peger på (antal records) og iterer igennem alle records.
        if (data->rs[i].osm_id == needle) { //Tjekker om record (osm.id) matcher indtastede ID.
            return &data->rs[i];
        }
    }//Lineær søgning i C: https://www.geeksforgeeks.org/c/c-c-program-for-linear-search/
    return NULL; //Hvis ID ikke findes, terminerer løkken og returnerer NULL.
}

int main(int argc, char** argv) {
  return id_query_loop(argc, argv,
                    (mk_index_fn)mk_naive,
                    (free_index_fn)free_naive,
                    (lookup_fn)lookup_naive);
}
