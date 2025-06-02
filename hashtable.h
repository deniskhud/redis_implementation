#pragma once

#include <stddef.h>
#include <stdint.h>

//hash node
struct Node {
    Node* next = NULL;
    uint64_t code = 0;
};

struct HTab {
    Node** tab = NULL;  //arr of slots
    size_t mask = 0;    //power of 2 arr size, n^2 - 1
    size_t size = 0;    //num of keys
};

struct HMap {
    HTab newer;
    HTab older;
    size_t migrate_pos = 0;
};

Node* hm_lookup(HMap* hmap, Node* key, bool(*eq)(Node* , Node* ));
void hm_insert(HMap* hmap, Node* node);
Node* hm_delete(HMap* hmap, Node* key, bool(*eq)(Node*, Node*));
size_t hm_size(HMap* hmap);
void hm_clear(HMap* hmap);
size_t hm_size(HMap* hmap);
void hm_foreach(HMap* hmap, bool(*f)(Node* , void*), void* arg);
