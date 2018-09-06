#include "../stdio.h"
#include "../common.h"
#include "map.h"

typedef struct map_t {
    void **keys;
    void **vals;
    size_t count;
    size_t space;
    int (*equal)(void *x, void *y);
} map_t;

void increase_map_space(map_t *m) {
    m->space *= 2;
    m->keys = realloc(m->keys, sizeof(char **) * m->space);
    m->vals = realloc(m->vals, sizeof(void **) * m->space);
}

#define INITIAL_SPACE 8
map_t *map_create(int (*equal)(void *x, void *y)) {
    map_t *m = malloc(sizeof (map_t));
    m->keys = malloc(sizeof (char **) * INITIAL_SPACE);
    m->vals = malloc(sizeof (void **) * INITIAL_SPACE);
    m->count = 0;
    m->space = INITIAL_SPACE;
    m->equal = equal;
    return m;
}

void map_put(map_t *m, void *key, void *val) {
    if(m->count == m->space) {
        increase_map_space(m);
    }
    for(size_t i = 0; i < m->count; i++) {
        if(m->equal(m->keys[i], key)) {
            m->vals[i] = val;
            return;
        }
    }
    m->keys[m->count] = key;
    m->vals[m->count] = val;
    m->count++;
}

void *map_get(map_t *m, void *key) {
    if(key == NULL) return NULL;

    for(size_t i = 0; i < m->count; i++) {
        if(m->equal(m->keys[i], key)) {
            return m->vals[i];
        }
    }
    return NULL;
}

void *map_delete(map_t *m, void *key) {
    for(size_t i = 0; i < m->count; i++) {
        if(m->equal(m->keys[i], key)) {
            void *ret = m->vals[i];
            m->count--;
            m->keys[i] = m->keys[m->count];
            m->vals[i] = m->vals[m->count];
            return ret;
        }
    }
    return NULL;
}

void map_destroy(map_t *m) {
    //memset(m->keys, 0, m->space);
    //memset(m->vals, 0, m->space);
    free(m->keys);
    free(m->vals);
    //memset(m, 0, sizeof (map_t));
    free(m);
}

struct map_iterator {
    map_t *map;
    size_t offset;
};

map_iterator *iterate_map(map_t *m) {
    if(m->count == 0) {
        return NULL;
    }
    map_iterator *it = malloc(sizeof (struct map_iterator));
    it->map = m;
    it->offset = 0;
    return it;
}

map_iterator *map_iterator_next(map_iterator *mi) {
    mi->offset++;
    if(mi->offset == mi->map->count) {
        mi->map = NULL;
        return NULL;
    }
    return mi;
}

struct map_pair map_iterator_values(map_iterator *mi) {
    struct map_pair mp;
    if(!mi->map) {
        PANIC("Invalid iterator.");
    }
    mp.key = mi->map->keys[mi->offset];
    mp.val = mi->map->vals[mi->offset];
    return mp;
}

void destroy_map_iterator(map_iterator *mi) {
    if(!mi) return;
    //printf("Freeing map at: %p\n", mi);
    //memset(mi, 0, sizeof (map_iterator));
    free(mi);
}

int ptr_eq(void *x, void *y) {
    return x == y;
}

map_t *map_reverse(map_t *m) {
//    map_iterator *mi = iterate_map(m);
    map_t *revmap = map_create(ptr_eq);
//    do {
//        struct map_pair mp = map_iterator_values(mi);
//        printf("Putting %x -> %s\n", mp.val, (char *)mp.key);
//        map_put(revmap, mp.val, mp.key);
//        map_iterator_next(mi);
//    } while (map_iterator_next(mi));
//
//    destroy_map_iterator(mi);

    if(m == NULL) return NULL;

    for(size_t i = 0; i < m->count; i++) {
//        printf("Putting %x -> %s\n", m->vals[i], (char *)m->keys[i]);
        map_put(revmap, m->vals[i], m->keys[i]);
    }
    
    return revmap;
}
