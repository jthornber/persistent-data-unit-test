#include "framework.h"
#include "units.h"

#include "compat/cmp.h"

#include "dm-btree.h"
#include "btree-remove-internal.h"

#include <stdio.h>
#include <string.h>

// FIXME: do endian conversions
 
//--------------------------------------------------------

#define BLOCK_SIZE 4096

static void *zalloc(size_t s)
{
	void *ptr = malloc(s);
	if (ptr)
		memset(ptr, 0, s);

	return ptr;
}

static unsigned rnd(unsigned end)
{
	return rand() % end;
}

static void node_is_well_formed(struct btree_node *n)
{
	unsigned i;
	uint64_t k, last;
	struct node_header *hdr = &n->header;

	T_ASSERT((hdr->flags == INTERNAL_NODE) || (hdr->flags == LEAF_NODE));
	T_ASSERT(hdr->nr_entries <= hdr->max_entries);

	for (i = 0; i < hdr->nr_entries; i++) {
		k = n->keys[i];
		if (i > 0)
			T_ASSERT(k > last);
		last = k;
	}
}

static struct btree_node *create_leaf(uint32_t value_size, unsigned nr_entries,
                                      unsigned key_start)
{
	unsigned i, k;
	struct btree_node *n = malloc(BLOCK_SIZE);
	struct node_header *hdr = &n->header;

	hdr->flags = LEAF_NODE;
	hdr->nr_entries = nr_entries;
	hdr->max_entries = calc_max_entries(value_size, BLOCK_SIZE);
	hdr->value_size = value_size;

	T_ASSERT(nr_entries <= hdr->max_entries);
	k = key_start;
	for (i = 0; i < nr_entries; i++) {
		k += rnd(10);
		n->keys[i] = k;

		memset(value_ptr(n, i), k, n->header.value_size);
		k++;
	}

	return n;
}

static bool create_leaves(uint32_t value_size, unsigned count,
                          unsigned *nr_entries, struct btree_node **result)
{
	unsigned i, key_start = 0;

	for (i = 0; i < count; i++) {
		result[i] = create_leaf(value_size, nr_entries[i], key_start);
		if (!result[i])
			return false;
		if (nr_entries[i])
			key_start = result[i]->keys[nr_entries[i] - 1] + 1;
	}

	return true;
}

static void free_node(struct btree_node *n)
{
	free(n);
}

//--------------------------------------------------------

static void do_delete_at(unsigned nr_entries, unsigned entry)
{
	struct btree_node *n = create_leaf(sizeof(uint64_t), nr_entries, 0);
	uint64_t k = n->keys[entry];
	unsigned i, j;

	delete_at(n, entry);

	for (i = 0; i < nr_entries - 1; i++) {
		T_ASSERT_NOT_EQUAL(n->keys[i], k);
		for (j = 0; j < sizeof(uint64_t); j++)
			T_ASSERT_EQUAL((uint8_t) n->keys[i], *((uint8_t *) value_ptr(n, i)));
	}

	T_ASSERT(n->header.nr_entries == nr_entries - 1);
	node_is_well_formed(n);

	free_node(n);
}

static void test_delete_at(void *context)
{
	unsigned i;
	unsigned nr_entries;
	unsigned max_entries = calc_max_entries(sizeof(uint64_t), BLOCK_SIZE);

	// Try across a variety of populated nodes
	for (nr_entries = 1; nr_entries < max_entries; nr_entries++) {
		// delete the first entry
		do_delete_at(nr_entries, 0);

		// delete the last entry
		do_delete_at(nr_entries, nr_entries - 1);

		// delete a few random entries
		for (i = 0; i < 5; i++)
			do_delete_at(nr_entries, rnd(nr_entries));
	}
}

//--------------------------------------------------------

static bool collect_keys(unsigned nr_nodes, struct btree_node **nodes,
                         unsigned *nr_keys, uint64_t **result)
{
	unsigned node, i, count = 0;

	// count keys
	for (node = 0; node < nr_nodes; node++)
		count += nodes[node]->header.nr_entries;

	*nr_keys = count;

	*result = malloc(sizeof(uint64_t) * count);
	if (!result)
		return false;

	count = 0;
	for (node = 0; node < nr_nodes; node++) {
		struct btree_node *n = nodes[node];
		for (i = 0; i < n->header.nr_entries; i++)
			(*result)[count++] = n->keys[i];
	}

	return true;
}

static bool keys_equal(unsigned nr_keys, uint64_t *lhs, uint64_t *rhs)
{
	unsigned i;

	for (i = 0; i < nr_keys; i++)
		if (lhs[i] != rhs[i])
			return false;

	return true;
}

static void free_keys(uint64_t *keys)
{
	free(keys);
}

static void do_shift(unsigned nr_left, unsigned nr_right, int s)
{
	unsigned nr_entries[2] = {nr_left, nr_right};
	struct btree_node *nodes[2];
	uint64_t *before_keys, *after_keys;
	unsigned nr_keys;

	T_ASSERT(create_leaves(sizeof(uint64_t), 2, nr_entries, nodes));

	T_ASSERT(collect_keys(2, nodes, &nr_keys, &before_keys));
	shift(nodes[0], nodes[1], s);
	T_ASSERT(collect_keys(2, nodes, &nr_keys, &after_keys));

	// nr_entries updated
	T_ASSERT(nodes[0]->header.nr_entries == nr_entries[0] - s);
	T_ASSERT(nodes[1]->header.nr_entries == nr_entries[1] + s);

	// all keys and values must be present after the shift
	T_ASSERT(keys_equal(nr_keys, before_keys, after_keys));
	free_keys(before_keys);
	free_keys(after_keys);

	node_is_well_formed(nodes[0]);
	node_is_well_formed(nodes[1]);
}

static void test_shift(void *context)
{
	unsigned max_entries = calc_max_entries(sizeof(uint64_t), BLOCK_SIZE);

	do_shift(32, 43, 12);
	do_shift(32, 43, -12);

	do_shift(100, 0, 50);
	do_shift(0, 100, -50);

	do_shift(max_entries, 20, 50);
	do_shift(20, max_entries, -50);
}

//--------------------------------------------------------

#define T(path, desc, fn) register_test(ts, "/btree/remove/" path, desc, fn)

static struct test_suite *remove_tests(void)
{
	struct test_suite *ts = test_suite_create(NULL, NULL);
	if (!ts) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	T("delete_at", "delete_at()", test_delete_at);
	T("shift", "shift()", test_shift);

	return ts;
}

//--------------------------------------------------------

void btree_tests(struct list_head *suites)
{
	list_add(&remove_tests()->list, suites);
}

//--------------------------------------------------------
