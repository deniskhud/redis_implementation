#include<assert.h>
#include<stdlib.h>
#include "hashtable.h"

static void h_init(HTab* htab, size_t n) {
    assert(n > 0 && ((n - 1) & n) == 0);
    htab->tab = (Node**)calloc(n, sizeof(Node*));
    htab->mask = n - 1;
    htab->size = n;
}

//insertion
static void h_insert(HTab* htab, Node* node) {
    size_t pos = node->code & htab->mask;
    Node* next = htab->tab[pos];
    node->next = next;
    htab->tab[pos] = node;
    htab->size++;
}

static Node** h_lookup(HTab* htab, Node* key, bool(*eq)(Node*, Node*)) {
    if (!htab->tab) {
        return NULL;
    }
    size_t pos = key->code & htab->mask;
    Node** from = &htab->tab[pos];      //incoming pointer to the target
    for (Node* current; (current = *from) != NULL; from = &current->next) {
        if (current->code == key->code && eq(current, key)) {
            return from;    //may be a node or slot
        }
    }
    return NULL;
}

//remove a node from the chain
static Node* h_detach(HTab* htab, Node** from) {
    Node* node = *from;     //target node
    *from = node->next;     //update the incoming pointer to the target
    htab->size--;
    return node;
}

const size_t k_rehashing_work = 128;

static void hm_help_rehashing(HMap* hmap) {
    size_t nwork = 0;
    while (nwork < k_rehashing_work && hmap->older.size > 0) {
        //find a free slot
        Node** from = &hmap->older.tab[hmap->migrate_pos];
        if (!*from) {
            hmap->migrate_pos++;
            continue;
        }
        //move the first list item to the newer table
        h_insert(&hmap->newer, h_detach(&hmap->older, from));
        nwork++;
    }
    //discard the old table if done
    if (hmap->older.size == 0 && hmap->older.tab) {
        free(hmap->older.tab);
        hmap->older = HTab{};
    }
}

static void hm_trigger_rehashing(HMap* hmap) {
    assert(hmap->older.tab == NULL);
    // (newer, older) <- (new, newer)
    hmap->older = hmap->newer;
    h_init(&hmap->newer, (hmap->newer.mask + 1) * 2);
    hmap->migrate_pos = 0;
}

Node *hm_lookup(HMap *hmap, Node *key, bool (*eq)(Node *, Node *)) {
    hm_help_rehashing(hmap);
    Node** from = h_lookup(&hmap->newer, key, eq);
    if (!from) {
        from = h_lookup(&hmap->older, key, eq);
    }
    return from ? *from : NULL;
}

const size_t k_max_load_factor = 8;

void hm_insert(HMap* hmap, Node* node) {
    if (!hmap->newer.tab) {
        h_init(&hmap->newer, 4);    //if empty
    }
    h_insert(&hmap->newer, node);  //insert to the newer table

    if (!hmap->older.tab) {     //check whether we need to rehash
        size_t shreshold = (hmap->newer.mask + 1) * k_max_load_factor;
        if (hmap->newer.size >= shreshold) {
            hm_trigger_rehashing(hmap);
        }
    }
    hm_help_rehashing(hmap);    //migrate some keys
}


Node* hm_delete(HMap* hmap, Node* key, bool(*eq)(Node*, Node*)) {
    hm_help_rehashing(hmap);
    if (Node** from = h_lookup(&hmap->newer, key, eq)) {
        return h_detach(&hmap->newer, from);
    }
    if (Node** from = h_lookup(&hmap->older, key, eq)) {
        return h_detach(&hmap->older, from);
    }
    return NULL;
}

void hm_clear(HMap* hmap) {
    free(hmap->newer.tab);
    free(hmap->older.tab);
    *hmap = HMap{};
}

size_t hm_size(HMap* hmap) {
    return hmap->newer.size + hmap->older.size;
}

static bool h_foreach(HTab *htab, bool (*f)(Node *, void *), void *arg) {
    for (size_t i = 0; htab->mask != 0 && i <= htab->mask; ++i) {
        for (Node* node = htab->tab[i]; node != NULL; node = node->next) {
            if (!f(node, arg)) {
                return false;
            }
        }
    }
    return true;
}

void hm_foreach(HMap* hmap, bool (*f)(Node *, void *), void *arg) {
    h_foreach(&hmap->newer, f, arg) && h_foreach(&hmap->older, f, arg);
}




















































