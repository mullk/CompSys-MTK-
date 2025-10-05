#ifndef EYTZINGER_H
#define EYTZINGER_H

#include "record.h"

struct index_record {
    int64_t key;
    const struct record *rec;
};

struct eytz_data {
    int n;
    struct index_record *ey;
};

void build_eytzinger(struct index_record *sorted,
                     struct index_record *ey,
                     int k, int *i, int n);

const struct record* eytzinger_lookup(struct eytz_data *data, int64_t x);

struct eytz_data* mk_eytzinger(struct record *records, int n);

int cmp_index_record(const void *a, const void *b);

#endif
