#include "dm-block-manager.h"


dm_block_t dm_block_location(struct dm_block *b)
{
	return 0;
}

void *dm_block_data(struct dm_block *b)
{
	return NULL;
}

/*----------------------------------------------------------------*/

struct dm_block_manager *dm_block_manager_create(
	struct block_device *bdev, unsigned block_size,
	unsigned max_held_per_thread)
{
	return NULL;
}

void dm_block_manager_destroy(struct dm_block_manager *bm)
{
}

unsigned dm_bm_block_size(struct dm_block_manager *bm)
{
	return 0;
}

dm_block_t dm_bm_nr_blocks(struct dm_block_manager *bm)
{
	return 0;
}

/*----------------------------------------------------------------*/

int dm_bm_read_lock(struct dm_block_manager *bm, dm_block_t b,
		    struct dm_block_validator *v,
		    struct dm_block **result)
{
	return 0;
}

int dm_bm_write_lock(struct dm_block_manager *bm, dm_block_t b,
		     struct dm_block_validator *v,
		     struct dm_block **result)
{
	return 0;
}

int dm_bm_read_try_lock(struct dm_block_manager *bm, dm_block_t b,
			struct dm_block_validator *v,
			struct dm_block **result)
{
	return 0;
}

int dm_bm_write_lock_zero(struct dm_block_manager *bm, dm_block_t b,
			  struct dm_block_validator *v,
			  struct dm_block **result)
{
	return 0;
}

void dm_bm_unlock(struct dm_block *b)
{
}

int dm_bm_flush(struct dm_block_manager *bm)
{
	return 0;
}

void dm_bm_prefetch(struct dm_block_manager *bm, dm_block_t b)
{
}

bool dm_bm_is_read_only(struct dm_block_manager *bm)
{
	return true;
}

void dm_bm_set_read_only(struct dm_block_manager *bm)
{
}

void dm_bm_set_read_write(struct dm_block_manager *bm)
{
}

u32 dm_bm_checksum(const void *data, size_t len, u32 init_xor)
{
	return 0;
}

/*----------------------------------------------------------------*/
