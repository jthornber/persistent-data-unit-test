#include "dm-space-map-core.h"

#include "compat/memory.h"
#include "framework.h"

/*----------------------------------------------------------------*/

struct sm_core {
	struct dm_space_map sm;
	unsigned nr_blocks;
	unsigned nr_free;
	uint16_t *ref_counts;
};

static struct sm_core *to_smc(struct dm_space_map *sm)
{
	return (struct sm_core *) sm;
}

static void check_index_(struct sm_core *smc, dm_block_t b)
{
	T_ASSERT(b < smc->nr_blocks);
}

static void destroy_(struct dm_space_map *sm)
{
	struct sm_core *smc = to_smc(sm);
	free(smc->ref_counts);
	free(smc);
}

static int extend_(struct dm_space_map *sm, dm_block_t extra_blocks)
{
	struct sm_core *smc = to_smc(sm);
	smc->ref_counts = realloc(smc->ref_counts,
	                          sizeof(*smc->ref_counts) * (smc->nr_blocks + extra_blocks));
	T_ASSERT(smc->ref_counts);	
	smc->nr_blocks += extra_blocks;
	return 0;
}


static int get_nr_blocks_(struct dm_space_map *sm, dm_block_t *count)
{
	struct sm_core *smc = to_smc(sm);
	return smc->nr_blocks;
}

static int get_nr_free_(struct dm_space_map *sm, dm_block_t *count)
{
	struct sm_core *smc = to_smc(sm);
	return smc->nr_free;
}

static int get_count_(struct dm_space_map *sm, dm_block_t b, uint32_t *result)
{
	struct sm_core *smc = to_smc(sm);
	check_index_(smc, b);
	return smc->ref_counts[b];
}

static int count_is_more_than_one_(struct dm_space_map *sm, dm_block_t b,
			      int *result)
{
	struct sm_core *smc = to_smc(sm);
	check_index_(smc, b);
	return smc->ref_counts[b] > 1;
}

static int set_count_(struct dm_space_map *sm, dm_block_t b, uint32_t count)
{
	struct sm_core *smc = to_smc(sm);
	check_index_(smc, b);
	if (smc->ref_counts[b]) {
		if (!count)
			smc->nr_free++;
	} else {
		if (count)
			smc->nr_free--;
	}

	smc->ref_counts[b] = count;
	return 0;
}

static int commit_(struct dm_space_map *sm)
{
	return 0;
}

static int inc_block_(struct dm_space_map *sm, dm_block_t b)
{
	struct sm_core *smc = to_smc(sm);
	check_index_(smc, b);
	if (!smc->ref_counts[b])
		smc->nr_free--;
	smc->ref_counts[b]++;

	return 0;
}

static int dec_block_(struct dm_space_map *sm, dm_block_t b)
{
	struct sm_core *smc = to_smc(sm);
	check_index_(smc, b);
	T_ASSERT(smc->ref_counts[b] > 0);
	smc->ref_counts[b]--;
	if (!smc->ref_counts[b])
		smc->nr_free++;

	return 0;
}

static int new_block_(struct dm_space_map *sm, dm_block_t *b)
{
	unsigned i;
	struct sm_core *smc = to_smc(sm);

	for (i = 0; i < smc->nr_blocks; i++)
		if (!smc->ref_counts[i]) {
			*b = i;
			smc->ref_counts[i] = 1;
			smc->nr_free--;
			return 0;
		}

	// If this triggers then nr_free accounting is wrong
	fprintf(stderr, "nr_free = %u\n", (unsigned) smc->nr_free);
	T_ASSERT(!smc->nr_free);
	return -ENOSPC;
}

static int root_size_(struct dm_space_map *sm, size_t *result)
{
	*result = 0;
	return 0;
}

static int copy_root_(struct dm_space_map *sm, void *copy_to_here_le, size_t len)
{
	return 0;
}

static int register_threshold_callback_(struct dm_space_map *sm,
					   dm_block_t threshold,
					   dm_sm_threshold_fn fn,
					   void *context)
{
	return 0;
}

struct dm_space_map *dm_sm_core_create(dm_block_t nr_blocks)
{
	struct sm_core *smc = malloc(sizeof(*smc));
	T_ASSERT(smc);
	smc->nr_blocks = nr_blocks;
	smc->nr_free = nr_blocks;
	smc->ref_counts = zalloc(sizeof(*smc->ref_counts) * nr_blocks);
	T_ASSERT(smc->ref_counts);

	smc->sm.destroy = destroy_;
	smc->sm.extend = extend_;
	smc->sm.get_nr_blocks = get_nr_blocks_;
	smc->sm.get_nr_free = get_nr_free_;
	smc->sm.get_count = get_count_;
	smc->sm.count_is_more_than_one = count_is_more_than_one_;
	smc->sm.set_count = set_count_;
	smc->sm.commit = commit_;
	smc->sm.inc_block = inc_block_;
	smc->sm.dec_block = dec_block_;
	smc->sm.new_block = new_block_;
	smc->sm.root_size = root_size_;
	smc->sm.copy_root = copy_root_;
	smc->sm.register_threshold_callback = register_threshold_callback_;

	return &smc->sm;
}

/*----------------------------------------------------------------*/
