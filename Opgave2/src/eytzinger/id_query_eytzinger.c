#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdalign.h>

#include "record.h"
#include "id_query.h"
#include "Eytzinger.h"








// Comparator til qsort
int cmp_index_record(const void *a, const void *b) {
    const struct index_record *ra = a;
    const struct index_record *rb = b;
    if (ra->key < rb->key) return -1;
    if (ra->key > rb->key) return 1;
    return 0;
}


// Returnerer -1 hvis ra->key < rb->key = a kommer før b.
// Returnerer 1 hvis ra->key > rb->key = a kommer efter b.
// Returnerer 0 hvis de er ens.



// Wrapper der bygger hele eytz_data
struct eytz_data* mk_eytzinger(struct record *records, int n) {

// Funktionens formål er at tage de rå records (fra din .tsv-fil) og bygge et færdigt Eytzinger-layout, pakket ind i en struct eytz_data.
// Returnerer en pointer til denne nye eytz_data.


    // 1. Lav en midlertidig array
    struct index_record *sorted = malloc(n * sizeof(struct index_record));
    for (int i = 0; i < n; i++) {
        sorted[i].key = records[i].osm_id;
        sorted[i].rec = &records[i];
    }

// Her reserverer vi hukommelse til en array af index_record. 
// n er antal records, så vi får plads nok til dem alle.
// Denne array skal bruges som et mellemtrin til at holde (key, rec) par.

// Vi gennemløber alle records.
// sorted[i].key = records[i].osm_id; gemmer ID’et (osm_id) som søgenøglen.
// sorted[i].rec = &records[i]; gemmer en pointer tilbage til den originale record.







    // 2. Sortér arrayet
    qsort(sorted, n, sizeof(struct index_record), cmp_index_record);

// qsort er C’s standard quicksort-funktion fra <stdlib.h>.
// Den tager fire argumenter:
// sorted som pointer til arrayet der skal sorteres.
// n som antal elementer i arrayet.
// sizeof(struct index_record) som er størrelsen af hvert element (så qsort kan hoppe rundt i hukommelsen).
// cmp_index_record som er vores egen comparator-funktion, der fortæller hvordan to elementer skal sammenlignes.







    // 3. Allokér eytz_data
    struct eytz_data *data = malloc(sizeof(struct eytz_data));
    data->n = n;
    data->ey = aligned_alloc(64, (n+1) * sizeof(struct index_record));


// Her reserverer du hukommelse til selve eytz_data structen 
// Her gemmer du hvor mange records du har, så eytzinger_lookup senere ved, hvornår den er “ude af træet”.

// n+1 → fordi du bruger 1-indeksering (dvs. ey[0] er ubrugt, roden starter i ey[1]).
// aligned_alloc(64, …) → sørger for at arrayet starter på en 64-byte grænse, som passer til en CPU cacheline.
// Det betyder, at når CPU’en henter data, får den hele cacheline præcist tilpasset dine noder → bedre cacheudnyttelse.






    // 4. Kald din egen funktion build_eytzinger
    int i = 0;
    build_eytzinger(sorted, data->ey, 1, &i, n);


// Nu bygger du selve Eytzinger-layoutet.
// Parametre:
// sorted = din midlertidige array, sorteret efter key.
// data->ey = det færdige array, hvor layoutet skal ligge.
// 1 = startposition i ey. Vi starter i roden (ey[1]).
// &i = pointer til tælleren, så den kan opdateres rekursivt.
// n = antal elementer.


    free(sorted); // sorted-arrayet behøver vi ikke længere
    return data;
}
