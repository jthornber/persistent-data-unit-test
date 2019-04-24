#include "units.h"

#include "dm-btree.h"

#include <stdio.h>

//--------------------------------------------------------

static void test_noop(void *context)
{

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

	T("noop", "noop test, FIXME: remove", test_noop);

	return ts;
}

void btree_tests(struct list_head *suites)
{
	list_add(&remove_tests()->list, suites);
}

//--------------------------------------------------------
