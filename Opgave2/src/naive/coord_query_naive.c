#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#include "record.h"
#include "coord_query.h"
// Meningen med denne fil er at lave en naiv søgning efter koordinater,
// ved at gennemgå alle poster og finde den med den korteste afstand til de givne koordinater.


// IDEEN BAG SAMMENHÆNGEN MELLEM DENNE FIL OG COORD_QUERY.C https://chatgpt.com/share/68d2809a-81cc-800c-a326-ca262236afac

//data fås fra struct naive_data* mk_naive(struct record* rs, int n)
struct naive_data {
  struct record *rs;
  int n;
};

//data skabes ved coord_query.c, kald: struct record *rs = read_records(argv[1], &n;
struct naive_data* mk_naive(struct record* rs, int n) {

//sikkerhedstjek
  assert(rs != NULL);
  assert(n >= 0);

  //https://chatgpt.com/share/68d28da2-c2b0-800c-a4be-72950eccb873 peger på data
  struct naive_data* data = malloc(sizeof(struct naive_data));
  if (data == NULL){
    return NULL;
  }
  //sætter struct naive_data's fields
  data->rs = rs;
  data->n = n;

  return data;
}

/// @brief Frigiver ens data
/// @param data kaldes i coord_query.c, ved kald: free_index(index);
/// Dermed frigives den data, som blev returneret i mk_naive
void free_naive(struct naive_data* data) {
    if (data) {
        fprintf(stderr, "free_naive: data=%p\n", (void*)data);
        free(data);
    }
}



/// @brief lon og lat fås i coord_query.c ved while (getline(&line, &line_len, stdin) != -1)
/// Her modtages straten af array af naive_data som kørges igennem efter korteste distance.
const struct record* lookup_naive(struct naive_data *data, double lon, double lat) {
  //Her regnes det tætteste for at have den som reference for senere udskiftning.

  if (data == NULL || data->n == 0) {
      return NULL;
  }


  const struct record *closest = &data->rs[0];
  double my_dist =  (closest->lon - lon) * (closest->lon - lon) +
                    (closest->lat - lat) * (closest->lat - lat);
  //https://chatgpt.com/share/68d2985f-9c58-800c-8c25-a201c53a6165

  //Her tjekkes alle koordinater efter, om de er tættest.
  for (int i = 1; i < data->n; i++){
    double dx = data->rs[i].lon - lon;
    double dy = data->rs[i].lat - lat;
    double temp_dist = (dx*dx + dy*dy);

    //Sætter pointeren til den nyeste tætteste 
    if (temp_dist < my_dist){
      my_dist = temp_dist;
      closest = &data->rs[i];
    }
  }


  return closest;

}

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_naive,
                          (free_index_fn)free_naive,
                          (lookup_fn)lookup_naive);
}
