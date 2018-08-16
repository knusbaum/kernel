#ifndef MAP_H
#define MAP_H

/**
 * Map does not own the keys or values, and will not 
 * attempt to copy, modify, or free them. Keys and values 
 * must be kept alive as long as they are held in the map,
 * or until map_destroy is called.
 */

typedef struct map_t map_t;
typedef struct map_iterator map_iterator;

struct map_pair {
    void *key;
    void *val;
};

map_t *map_create(int (*equal)(void *x, void *y));

void map_put(map_t *m, void *key, void *val);
void *map_get(map_t *m, void *key);
void *map_delete(map_t *m, void *key);

void map_destroy(map_t *m);

map_iterator *iterate_map(map_t *m);
map_iterator *map_iterator_next(map_iterator *mi);
struct map_pair map_iterator_values(map_iterator *mi);
void destroy_map_iterator(map_iterator *mi);

#endif
