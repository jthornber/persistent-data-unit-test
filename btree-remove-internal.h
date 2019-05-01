#ifndef BTREE_REMOVE_INTERNAL_H
#define BTREE_REMOVE_INTERNAL_H

#include "dm-btree-internal.h"

/*----------------------------------------------------------------*/

void delete_at(struct btree_node *n, unsigned index);
void shift(struct btree_node *left, struct btree_node *right, int count);
int rebalance2(struct shadow_spine *s, struct dm_btree_info *info,
	      struct dm_btree_value_type *vt, unsigned left_index);

/*----------------------------------------------------------------*/

#endif

