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

static struct btree_node *create_leaf(uint32_t value_size, unsigned nr_entries)
{
	unsigned i, k;
	struct btree_node *n = zalloc(BLOCK_SIZE);
	struct node_header *hdr = &n->header;

	hdr->flags = LEAF_NODE;
	hdr->nr_entries = nr_entries;
	hdr->max_entries = calc_max_entries(value_size, BLOCK_SIZE);
	hdr->value_size = value_size;

	k = rnd(1000);
	for (i = 0; i < nr_entries; i++) {
		k += 1 + rnd(100);
		n->keys[i] = k;

		memcpy(value_ptr(n, i), &k, min(n->header.value_size, sizeof(k)));
	}

	return n;
}

static void free_node(struct btree_node *n)
{
	free(n);
}

//--------------------------------------------------------

static void do_delete_at(unsigned nr_entries, unsigned entry)
{
	struct btree_node *n;
	uint64_t k;
	unsigned i;

	n = create_leaf(sizeof(uint64_t), nr_entries);
	k = n->keys[entry];

	delete_at(n, entry);

	for (i = 0; i < nr_entries - 1; i++) {
		T_ASSERT_NOT_EQUAL(n->keys[i], k);
		T_ASSERT(!memcmp(n->keys + i, value_ptr(n, i), sizeof(uint64_t)));
	}

	T_ASSERT(n->header.nr_entries == nr_entries - 1);
	node_is_well_formed(n);

	free_node(n);
}

static void test_delete_at(void *context)
{
	unsigned i;
	unsigned nr_entries, max_entries = calc_max_entries(sizeof(uint64_t), BLOCK_SIZE);

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

#define T(path, desc, fn) register_test(ts, "/btree/remove/" path, desc, fn)

static struct test_suite *delete_at_tests(void)
{
	struct test_suite *ts = test_suite_create(NULL, NULL);
	if (!ts) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	T("delete_at", "delete_at()", test_delete_at);

	return ts;
}

//--------------------------------------------------------

void btree_tests(struct list_head *suites)
{
	list_add(&delete_at_tests()->list, suites);
}

//--------------------------------------------------------
