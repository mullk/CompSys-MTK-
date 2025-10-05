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

#define K 2  // dimension

/// @brief Struct som organisere lat og lon for hver node)
typedef struct {
    double x;
    double y;
} Point;

/// @brief KDNnoden indeholder Point samt reference til den record* den dannes ud fra.
typedef struct KDNode {
    Point pt;
    struct record* rec;
    struct KDNode* left;
    struct KDNode* right;
} KDNode;

/// @brief Sammenligner afstand mellem søgte query og aktuelle punkt
/// @param x1 aktuelle x
/// @param y1 aktuelle y
/// @param x2 query x
/// @param y2 query y
/// @return afstanden mellem punkter
double distanceSquared(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return dx*dx + dy*dy;
}

/// @brief hjælpefunktion for qsort
/// @param a vi sætter a til struct record** som dereferences, så det peger på struct record*
/// @param b struct record**
/// @return ens, større eller mindre relativt til x
int compareX(const void* a, const void* b) {
    struct record* r1 = *(struct record**)a;
    struct record* r2 = *(struct record**)b;
    return (r1->lon < r2->lon) ? -1 : (r1->lon > r2->lon);
}

/// @brief hjælpefunktion for qsort
/// @param a struct record**
/// @param b struct record**
/// @return ens, større eller mindre relativt til y
int compareY(const void* a, const void* b) {
    struct record* r1 = *(struct record**)a;
    struct record* r2 = *(struct record**)b;
    return (r1->lat < r2->lat) ? -1 : (r1->lat > r2->lat);
}

/// @brief Oprette ny node
/// @param rec information fra record
/// @return en ny node med fields
KDNode* newNode(struct record* rec) {
    KDNode* node = malloc(sizeof(KDNode));
    node->rec = rec;
    node->pt.x = rec->lon;
    node->pt.y = rec->lat;
    node->left = node->right = NULL;
    return node;
}

///BRUGT TIL AT FORSTÅ; HVORDAN MAN BYGGER ET TRÆ: https://chatgpt.com/share/68d58657-3b2c-800c-9838-4d9d73276000
/// @brief udformer et K2 tree ud fra qsort samt pointere mellem parent og child.
/// @param recs Struct record** altså et array af pointeres til structs
/// @param n antallet af elementer
/// @param depth dybden i træet
/// @return en pointer til roden af træet
KDNode* buildKDTree(struct record** recs, int n, int depth) {
    if (n <= 0) return NULL;

    int axis = depth % K;
    if (axis == 0) qsort(recs, n, sizeof(struct record*), compareX);
    else qsort(recs, n, sizeof(struct record*), compareY);

    int median = n / 2;
    KDNode* node = newNode(recs[median]);
    node->left = buildKDTree(recs, median, depth + 1);
    node->right = buildKDTree(recs + median + 1, n - median - 1, depth + 1);

    return node;
}

/// @brief Modtager data og konstruere træet ud fra hjælpefunktioner
/// @param rs struct record* rs fra coord_query.c
/// @param n antallet af elementer
/// @return en pointer til root
KDNode* mk(struct record* rs, int n) {
    assert(rs != NULL);
    assert(n >= 0);

    //Her allokeres plad til n pointers i et array af pointeres
    struct record** rec_ptrs = malloc(n * sizeof(struct record*));
    if (!rec_ptrs) return NULL;
    //Her sættes pointere for hver struct
    for (int i = 0; i < n; i++) {
        rec_ptrs[i] = &rs[i];
    }

    KDNode* root = buildKDTree(rec_ptrs, n, 0);
    free(rec_ptrs);
    return root;
}

/// @brief Træet frigives i hukommelsen
/// @param root pointer til roden af træet
void freeKDTree(KDNode* root) {
    if (!root) return;
    freeKDTree(root->left);
    freeKDTree(root->right);
    free(root);
}

/// @brief Her tjekkes afstanden mellem den aktuelle node og den, 
/// som er aktuelt tættest, hvis den er mere tæt, så udskiftes de
const struct record* lookup_tree(Point query, KDNode* node, int depth, const struct record* closest)
{
    if (!node) return closest;

    double node_dist = distanceSquared(node->pt.x, node->pt.y, query.x, query.y);
    double closest_dist = distanceSquared(closest->lon, closest->lat, query.x, query.y);

    if (node_dist < closest_dist) closest = node->rec;

    int axis = depth % K;
    double di = (axis == 0 ? node->pt.x - query.x : node->pt.y - query.y);

    if (di < 0) {
    closest = lookup_tree(query, node->left, depth+1, closest);
    //sikre at det tætteste punkt ikke ligger i splittet mod den anden side.
    //FORSTÅELSE: https://chatgpt.com/share/68d82975-1348-800c-9c04-bc6936a8201a
    if (di*di < closest_dist)
        closest = lookup_tree(query, node->right, depth+1, closest);
    } else {
        closest = lookup_tree(query, node->right, depth+1, closest);
        if (di*di < closest_dist)
            closest = lookup_tree(query, node->left, depth+1, closest);
    }
    return closest;
}

/// @brief kaldefunktion for lookop, som organisere data
const struct record* lookup(void* index, double lon, double lat) {
    if (!index) return NULL;

    KDNode* root = (KDNode*)index;
    Point query = {lon, lat};
    return lookup_tree(query, root, 0, root->rec);
}

/// @brief Indhenter værdier for funktionskald fra coord_query.c
/// @param argc antallet af argumenter
/// @param argv en tekst, som indeholder filen, der ønskes
/// @return returnere værdierne af funktionkald til coord_query.c
int main(int argc, char** argv) {
    return coord_query_loop(argc, argv,
                            (mk_index_fn)mk,
                            (free_index_fn)freeKDTree,
                            (lookup_fn)lookup);
}

///SAMMENSÆTNING AF coord_query.c og coord_query_kdtree.c https://chatgpt.com/share/68dbbb08-ebb0-800c-b311-50ba85dbeb18
