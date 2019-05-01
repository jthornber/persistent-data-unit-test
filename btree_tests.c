#include "framework.h"
#include "units.h"

#include "compat/cmp.h"

#include "dm-btree.h"
#include "btree-remove-internal.h"
#include "dm-space-map-core.h"
#include "dm-transaction-manager.h"

#include <stdio.h>
#include <string.h>

// FIXME: do endian conversions
 
//--------------------------------------------------------

#define BLOCK_SIZE 4096

static unsigned rnd(unsigned end)
{
	return rand() % end;
}

//--------------------------------------------------------

struct fixture {
	dm_block_t nr_blocks;
	struct block_device bdev;
	struct dm_block_manager *bm;
	struct dm_space_map *sm;
	struct dm_transaction_manager *tm;

	struct dm_btree_info info;
	struct dm_block_validator *validator;
};

static FILE *create_block_file_(unsigned block_size, dm_block_t nr_blocks)
{
	unsigned i;
	FILE *f = tmpfile();
	T_ASSERT(f);

	uint8_t data[block_size];
	memset(data, 0, sizeof(data));
	for (i = 0; i < nr_blocks; i++)
		fwrite(data, block_size, 1, f);
	rewind(f);

	return f;
}

static void *create_tm_()
{
	struct fixture *fix = malloc(sizeof(*fix));
	T_ASSERT(fix);

	fix->nr_blocks = 10240;
	fix->bdev.file = create_block_file_(BLOCK_SIZE, fix->nr_blocks);

	fix->bm = dm_block_manager_create(&fix->bdev, BLOCK_SIZE, 10);
	T_ASSERT(fix->bm);

	fix->sm = dm_sm_core_create(fix->nr_blocks);
	T_ASSERT(fix->sm);

	fix->tm = dm_tm_create(fix->bm, fix->sm);
	T_ASSERT(fix->tm);

	fix->info.tm = fix->tm;
	fix->info.levels = 1;
	fix->info.value_type.context = NULL;
	fix->info.value_type.size = sizeof(uint64_t);
	fix->info.value_type.inc = NULL;
	fix->info.value_type.dec = NULL;
	fix->info.value_type.equal = NULL;

	return fix;
}

static void destroy_tm_(void *context)
{
	struct fixture *fix = context;
	dm_tm_destroy(fix->tm);
	dm_sm_destroy(fix->sm);
	dm_block_manager_destroy(fix->bm);
	fclose(fix->bdev.file);
	free(fix);
}

//--------------------------------------------------------

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

typedef int (*blk_fn)(void *, struct dm_block *);

static struct dm_block_validator *null_validator_()
{
	return NULL;
}

static dm_block_t with_new_block(struct dm_transaction_manager *tm,
                                 blk_fn fn, void *context)
{
	int r;
	dm_block_t b;
	struct dm_block *blk;

	T_ASSERT(!dm_tm_new_block(tm, null_validator_(), &blk));
	r = fn(context, blk);
	T_ASSERT(!r);
	b = dm_block_location(blk);
	dm_tm_unlock(tm, blk);

	return b;
}

struct ci_params {
	unsigned nr_entries;
	unsigned key_start;
};

static int create_internal__(void *context, struct dm_block *blk)
{
	unsigned i, k;
	struct ci_params *params = context;
	struct btree_node *n = dm_block_data(blk);
	struct node_header *hdr = &n->header;

	hdr->flags = INTERNAL_NODE;
	hdr->nr_entries = params->nr_entries;
	hdr->max_entries = calc_max_entries(sizeof(__le64), BLOCK_SIZE);
	hdr->value_size = sizeof(__le64);

	T_ASSERT(params->nr_entries <= hdr->max_entries);
	k = params->key_start;
	for (i = 0; i < params->nr_entries; i++) {
		k += rnd(10);
		n->keys[i] = k;

		memset(value_ptr(n, i), k, n->header.value_size);
		k++;
	}

	return 0;
}

static dm_block_t create_internal(struct dm_transaction_manager *tm,
                                  unsigned nr_entries, unsigned key_start)
{
	struct ci_params params = {nr_entries, key_start};
	return with_new_block(tm, create_internal__, &params);
}

struct cl_params {
	uint32_t value_size;
	unsigned nr_entries;
	unsigned key_start;
};

static int create_leaf__(void *context, struct dm_block *blk)
{
	unsigned i, k;
	struct cl_params *params = context;
	struct btree_node *n = dm_block_data(blk);
	struct node_header *hdr = &n->header;

	hdr->flags = LEAF_NODE;
	hdr->nr_entries = params->nr_entries;
	hdr->max_entries = calc_max_entries(params->value_size, BLOCK_SIZE);
	hdr->value_size = params->value_size;

	T_ASSERT(params->nr_entries <= hdr->max_entries);
	k = params->key_start;
	for (i = 0; i < params->nr_entries; i++) {
		k += rnd(10);
		n->keys[i] = k;

		memset(value_ptr(n, i), k, n->header.value_size);
		k++;
	}

	return 0;
}

static dm_block_t create_leaf(struct dm_transaction_manager *tm,
                              uint32_t value_size, unsigned nr_entries,
                              unsigned key_start)
{
	struct cl_params params = {value_size, nr_entries, key_start};
	return with_new_block(tm, create_leaf__, &params);
}

static bool create_leaves(struct dm_transaction_manager *tm,
                          uint32_t value_size, unsigned count,
                          unsigned *nr_entries, dm_block_t *result)
{
	unsigned i, key_start = 0;

	for (i = 0; i < count; i++) {
		result[i] = create_leaf(tm, value_size, nr_entries[i], key_start);
		if (nr_entries[i]) {
			struct dm_block *blk;
			struct btree_node *n;

			T_ASSERT(!dm_tm_read_lock(tm, result[i], null_validator_(), &blk));
			n = dm_block_data(blk);
			key_start = n->keys[nr_entries[i] - 1] + 1;
			dm_tm_unlock(tm, blk);
		}
	}

	return true;
}

//--------------------------------------------------------

static void do_delete_at(struct dm_transaction_manager *tm,
                         unsigned nr_entries, unsigned entry)
{
	dm_block_t b = create_leaf(tm, sizeof(uint64_t), nr_entries, 0);
	struct btree_node *n;
	struct dm_block *blk;

	T_ASSERT(!dm_tm_read_lock(tm, b, null_validator_(), &blk));

	n = dm_block_data(blk);
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

	dm_tm_unlock(tm, blk);
}

static void test_delete_at(void *context)
{
	struct fixture *fix = context;
	unsigned i;
	unsigned nr_entries;
	unsigned max_entries = calc_max_entries(sizeof(uint64_t), BLOCK_SIZE);

	// Try across a variety of populated nodes
	for (nr_entries = 1; nr_entries < max_entries; nr_entries++) {
		// delete the first entry
		do_delete_at(fix->tm, nr_entries, 0);

		// delete the last entry
		do_delete_at(fix->tm, nr_entries, nr_entries - 1);

		// delete a few random entries
		for (i = 0; i < 5; i++)
			do_delete_at(fix->tm, nr_entries, rnd(nr_entries));
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

static void do_shift(struct dm_transaction_manager *tm,
                     unsigned nr_left, unsigned nr_right, int s)
{
	unsigned nr_entries[2] = {nr_left, nr_right};
	dm_block_t bs[2];
	struct dm_block *blks[2];
	struct btree_node *nodes[2];
	uint64_t *before_keys, *after_keys;
	unsigned nr_keys;

	T_ASSERT(create_leaves(tm, sizeof(uint64_t), 2, nr_entries, bs));

	T_ASSERT(!dm_tm_read_lock(tm, bs[0], null_validator_(), blks + 0)); 
	T_ASSERT(!dm_tm_read_lock(tm, bs[1], null_validator_(), blks + 1)); 

	nodes[0] = dm_block_data(blks[0]);
	nodes[1] = dm_block_data(blks[1]);

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

	dm_tm_unlock(tm, blks[0]);
	dm_tm_unlock(tm, blks[1]);
}

static void test_shift(void *context)
{
	struct fixture *fix = context;
	unsigned max_entries = calc_max_entries(sizeof(uint64_t), BLOCK_SIZE);

	do_shift(fix->tm, 32, 43, 12);
	do_shift(fix->tm, 32, 43, -12);

	do_shift(fix->tm, 100, 0, 50);
	do_shift(fix->tm, 0, 100, -50);

	do_shift(fix->tm, max_entries, 20, 50);
	do_shift(fix->tm, 20, max_entries, -50);
}

//--------------------------------------------------------

static void step_(struct shadow_spine *spine, dm_block_t b,
                  struct dm_btree_value_type *v)
{
	T_ASSERT(!shadow_step(spine, b, v));
}

static void test_rebalance2_merge(void *context)
{
#if 0
	int r;
	dm_block_t b;
	struct shadow_spine spine;
	struct fixture *fix = context;

	init_shadow_spine(&spine, &fix->info);
	b = create_internal(fix->tm, 70, 0);
	step_(&spine, b, &fix->info.value_type);

	b = create_leaf(fix,  
	
	r = rebalance2(&spine, &fix->info, &fix->info.value_type, 0);
	T_ASSERT(!r);
#endif
}

static void test_rebalance2_shift(void *context)
{
}

//--------------------------------------------------------

#define T(path, desc, fn) register_test(ts, "/btree/remove/" path, desc, fn)

static struct test_suite *remove_tests(void)
{
	struct test_suite *ts = test_suite_create(create_tm_, destroy_tm_);
	if (!ts) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	T("delete_at", "delete_at()", test_delete_at);
	T("shift", "shift()", test_shift);
	//T("rebalance2", "rebalance2()", test_rebalance2);

	return ts;
}

//--------------------------------------------------------

void btree_tests(struct list_head *suites)
{
	list_add(&remove_tests()->list, suites);
}

//--------------------------------------------------------
