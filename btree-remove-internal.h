#ifndef BTREE_REMOVE_INTERNAL_H
#define BTREE_REMOVE_INTERNAL_H

#include "dm-btree-internal.h"

/*----------------------------------------------------------------*/

void delete_at(struct btree_node *n, unsigned index);
void shift(struct btree_node *left, struct btree_node *right, int count);

/*----------------------------------------------------------------*/

#endif

