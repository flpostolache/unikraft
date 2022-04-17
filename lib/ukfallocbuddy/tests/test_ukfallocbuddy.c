#include <uk/test.h>
#include <uk/fallocbuddy.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


static struct uk_falloc *generate(void *fa_meta, void *start_zone, unsigned long len,
									__paddr_t *new_start, __sz fa_meta_size, unsigned long *pages)
{
	__vaddr_t dm_off;
	__paddr_t start = (__paddr_t)start_zone;
	struct uk_falloc *fa = malloc(uk_fallocbuddy_size());

	uk_fallocbuddy_init(fa);
	start = PAGE_ALIGN_UP(start + fa_meta_size);
	*pages = (len - fa_meta_size) >> PAGE_SHIFT;
	dm_off = start;
	fa->addmem(fa, fa_meta, start, *pages, dm_off);
	*new_start = start;
	return fa;
}


UK_TESTCASE(ukfallocbuddy, test_init)
{
	// test if the init function works

	struct uk_falloc *fa;

	fa = malloc(uk_fallocbuddy_size());
	UK_TEST_EXPECT_SNUM_EQ(uk_fallocbuddy_init(fa), 0);
	free(fa);
}


UK_TESTCASE(ukfallocbuddy, test_add_memory)
{
	// test if the addmem functionality works

	__sz len = 10000;
	void *start_zone = malloc(len);
	__vaddr_t dm_off;
	__paddr_t start = (__paddr_t)start_zone;
	unsigned long pages = len >> PAGE_SHIFT;
	__sz fa_meta_size = uk_fallocbuddy_metadata_size(pages);
	void *fa_meta = malloc(fa_meta_size);
	struct uk_falloc *fa = malloc(uk_fallocbuddy_size());
	int addmem_result;

	uk_fallocbuddy_init(fa);

	start = PAGE_ALIGN_UP(start + fa_meta_size);
	pages = (len - fa_meta_size) >> PAGE_SHIFT;
	dm_off = start;
	addmem_result = fa->addmem(fa, fa_meta, start, pages, dm_off);
	UK_TEST_EXPECT_SNUM_EQ(addmem_result, 0);
	free(start_zone);
	free(fa_meta);
	free(fa);
}

UK_TESTCASE(ukfallocbuddy, test_check_variables)
{
	__paddr_t start;
	__sz len = 10000;
	unsigned long pages = len >> PAGE_SHIFT;
	__sz fa_meta_size =  uk_fallocbuddy_metadata_size(pages);
	void *start_zone = malloc(len);
	void *fa_meta = malloc(fa_meta_size);
	struct uk_falloc *fa = generate(fa_meta, start_zone, len, &start, fa_meta_size, &pages);

	UK_TEST_EXPECT_SNUM_EQ(fa->free_memory, 8192);
	UK_TEST_EXPECT_SNUM_EQ(fa->total_memory, 8192);
	free(start_zone);
	free(fa_meta);
	free(fa);
}

UK_TESTCASE(ukfallocbuddy, test_simple_falloc)
{
	// test a simple allocation

	__paddr_t start;
	__sz len = 10000;
	unsigned long pages = len >> PAGE_SHIFT;
	__sz fa_meta_size =  uk_fallocbuddy_metadata_size(pages);
	void *start_zone = malloc(len);
	void *fa_meta = malloc(fa_meta_size);
	struct uk_falloc *fa = generate(fa_meta, start_zone, len, &start, fa_meta_size, &pages);
	int falloc_result;

	falloc_result = fa->falloc(fa, (__paddr_t *)start, 1, 0);
	UK_TEST_EXPECT_SNUM_EQ(falloc_result, 0);
	free(start_zone);
	free(fa_meta);
	free(fa);
}

UK_TESTCASE(ukfallocbuddy, test_alloc_from_range)
{

	// test allocation after a falloc

	__paddr_t start, min, max;
	__sz len = 50000;
	unsigned long pages = len >> PAGE_SHIFT;
	__sz fa_meta_size =  uk_fallocbuddy_metadata_size(pages);
	void *start_zone = malloc(len);
	void *fa_meta = malloc(fa_meta_size);
	struct uk_falloc *fa = generate(fa_meta, start_zone, len, &start, fa_meta_size, &pages);
	int falloc_from_range_result;

	min = start + 4096;
	max = start + 20000;
	fa->falloc(fa, (__paddr_t *)start, 1, FALLOC_FLAG_ALIGNED);
	falloc_from_range_result = fa->falloc_from_range(fa, (__paddr_t *)start, 2, 0, min, max);
	UK_TEST_EXPECT_SNUM_EQ(falloc_from_range_result, 0);
	free(start_zone);
	free(fa_meta);
	free(fa);
}

UK_TESTCASE(ukfallocbuddy, test_alloc_large)
{

	// test allocation with too many pages

	__paddr_t start;
	__sz len = 50000;
	unsigned long pages = len >> PAGE_SHIFT;
	__sz fa_meta_size =  uk_fallocbuddy_metadata_size(pages);
	void *start_zone = malloc(len);
	void *fa_meta = malloc(fa_meta_size);
	struct uk_falloc *fa = generate(fa_meta, start_zone, len, &start, fa_meta_size, &pages);
	int falloc_result;

	falloc_result = fa->falloc(fa, (__paddr_t *)start, 600, 0);
	UK_TEST_EXPECT_SNUM_EQ(falloc_result, -EFAULT);
	free(start_zone);
	free(fa_meta);
	free(fa);
}

UK_TESTCASE(ukfallocbuddy, test_alloc_over)
{

	// test allocation over an already allocated zone

	__paddr_t start, end;
	__sz len = 50000;
	unsigned long pages = len >> PAGE_SHIFT;
	__sz fa_meta_size =  uk_fallocbuddy_metadata_size(pages);
	void *start_zone = malloc(len);
	void *fa_meta = malloc(fa_meta_size);
	struct uk_falloc *fa = generate(fa_meta, start_zone, len, &start, fa_meta_size, &pages);
	int falloc_from_range_result;

	end = start + 8192;
	fa->falloc(fa, (__paddr_t *)start, 2, 0);
	falloc_from_range_result = fa->falloc_from_range(fa, (__paddr_t *)start, 2, 0, start, end);
	UK_TEST_EXPECT_SNUM_EQ(falloc_from_range_result, -ENOMEM);
	free(start_zone);
	free(fa_meta);
	free(fa);
}

UK_TESTCASE(ukfallocbuddy, test_alloc_over_2)
{

	// test allocation of two frames between an already allocated one

	__paddr_t start;
	__sz len = 50000;
	unsigned long pages = len >> PAGE_SHIFT;
	__sz fa_meta_size =  uk_fallocbuddy_metadata_size(pages);
	void *start_zone = malloc(len);
	void *fa_meta = malloc(fa_meta_size);
	struct uk_falloc *fa = generate(fa_meta, start_zone, len, &start, fa_meta_size, &pages);
	int falloc_from_range_result;

	fa->falloc_from_range(fa, (__paddr_t *)start, 1, 0, start + 4097, start + 8192);
	falloc_from_range_result = fa->falloc_from_range(fa, (__paddr_t *)start, 2, 0, start, start + 12288);
	UK_TEST_EXPECT_SNUM_EQ(falloc_from_range_result, 0);
	free(start_zone);
	free(fa_meta);
	free(fa);
}
uk_testsuite_register(ukfallocbuddy, NULL);
