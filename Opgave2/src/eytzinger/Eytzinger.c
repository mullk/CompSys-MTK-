#ifndef EYTZINGER_INCLUDED
#define EYTZINGER_INCLUDED

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdalign.h>

#include "record.h"
#include "id_query.h"
#include "Eytzinger.h"


void build_eytzinger(struct index_record *sorted, struct index_record *ey, int k, int *i, int n) {
    if (k <= n) {
        build_eytzinger(sorted, ey, 2*k, i, n);         // Gå ned til venstre barn (indeks 2*k).
        ey[k] = sorted[(*i)++];                         // Når vi er "tilbage" fra venstre barn, tager vi næste element fra sorted og putter det ind i den aktuelle node ey[k]. (*i)++ betyder: brug værdien af *i, og øg derefter tælleren med 1.
        build_eytzinger(sorted, ey, 2*k+1, i, n);       // Til sidst bygger vi højre barn (indeks 2*k+1).
    }
}

// *sorted er en pointer til et array af struct index_record, som er sorteret efter key-feltet.
// sorted: din input-array, sorteret på nøglefeltet (f.eks. osm_id).
// ey: output-arrayet i Eytzinger-layout, 1-indekseret (dvs. ey[1] er roden).
// k: den nuværende node-position i Eytzinger-arrayet (starter med k=1).
// i: en tæller, der holder styr på hvor langt vi er i den sorterede array. Det er en pointer, fordi du vil opdatere tallet på tværs af rekursive kald.
// n: antal elementer.


const struct record* eytzinger_lookup(struct eytz_data *data, int64_t x) {
    int k = 1;        // Start i roden [1]
    int last = 0;     // Holder styr på bedste kandidat (mindste key >= x)

    while (k <= data->n) {

        __builtin_prefetch(&data->ey[k*2], 0, 1);

        
        // Bed CPU’en om at forudindlæse venstre barn i cachen.
        // __builtin_prefetch(&data->ey[k*2], 0, 1);
        //   0 = vi vil læse (ikke skrive)
        //   1 = “locality hint”: vi forventer snart at bruge dataen
        // Prefetching gør, at når vi senere hopper til børnene, ligger deres data ofte allerede i cachen.

        int cond = (data->ey[k].key >= x);
        last = cond ? k : last;

        // er key >= x? Hvis ja, så er denne node en mulig kandidat. så gemmer vi den i last.

        k = 2*k + !cond;

        // Hvis cond er true (key >= x), gå venstre (2*k).
        // Hvis cond er false (key < x), gå højre (2*k+1).
    }

    // Når vi er faldet ud af træet (k > n), står last tilbage
    // som indekset til den bedste kandidat (lower_bound).
    if (last == 0) {
        return NULL; // ingen element >= x
    }

    if (data->ey[last].key != x) {
        return NULL; // nøgle eksisterer ikke, kun en større nøgle
    }


    return data->ey[last].rec; // Returnér pointer til den originale record
}

#endif // EYTZINGER_INCLUDED // Hvis last stadig er 0, fandt vi ingen nøgle >= x, så returnér













