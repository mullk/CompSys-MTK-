#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdalign.h>

#include "record.h"
#include <inttypes.h>
#include "id_query.h"
#include "Eytzinger.h"



// Hjælpefunktion til at printe en record
static void print_record(const struct record *r) {
    printf("%ld: %s %f %f\n", r->osm_id, r->name, r->lon, r->lat);
}


// Main til at køre programmet

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <datafile>\n", argv[0]);
        return EXIT_FAILURE;
    }

// ideen er at programmet skal kaldes med præcis ét argument for eksempel 20000records.tsv
// Hvis man ikke giver filen, stopper programmet og printer en "EXIT_FAILURE".




    int n;
    struct record *records = read_records(argv[1], &n); // fra record.h

// read_records åbner filen og læser alle rækker.
// Den returnerer en array af struct record, og sætter n til antal rækker.
// Nu burde vi have rå data i hukommelsen. 




    // Byg Eytzinger-layout
    struct eytz_data *data = mk_eytzinger(records, n);

// kald til mk_eytzinger 
// Laver et midlertidig sortering, kalder build_eytzinger, og returnerer et færdig eytz_data
// Efter dette burde vi have et cache-venligt søgeindeks klar til eytzinger_lookup.




    int64_t id;
    while (scanf("%" SCNd64, &id) == 1) {   // læs et ID fra stdin
        const struct record *rec = eytzinger_lookup(data, id);
        if (rec) {
            print_record(rec);              // fundet → print record
        } else {
            printf("NOT FOUND\n");          // ikke fundet
        }
    }



    // Ryd op
    free_records(records, n); // frigør de oprindelige records
    free(data->ey);           // frigør eytzinger arrayet
    free(data);               // frigør containeren

    return EXIT_SUCCESS;
}








