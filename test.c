#define OAHT_KEY_T int
#define OAHT_VALUE_T int
#define OAHT_HASH_T int
#define OAHT_HASH(x) (x - 5)
#define OAHT_EMPTY_KEY 0
#define OAHT_DELETED_KEY -1
#include "oaht.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/** helper; creates a small table with 2 elements */
static struct oaht * create_5_42_400_9(void) {
	struct oaht * ht = oaht_create();
	ht = oaht_set(ht, 5, 42);
	ht = oaht_set(ht, 400, 9);
	return ht;
}

void get_test(void) {
	struct oaht * tab = create_5_42_400_9();
	/* make sure the create stuff works */
	assert(oaht_len(tab) == 2);
	/* get existing */
	assert(oaht_get(tab, 400, 999) == 9);
	assert(oaht_get(tab,   5, 999) == 42);
	/* get non-existing */
	assert(oaht_get(tab, 827, 999) == 999);
	/* free the stuff we used */
	oaht_destroy(tab);
}

/** Iteration */
void iter_test(void) {
	struct oaht * tab = create_5_42_400_9();
	/*
	 * Since the order of entries is undefined, we check
	 * the sums of keys and values instead of the actual values.
	 */
	int k, v;
	int sumk = 0, sumv = 0, cnt = 0;
	size_t i;
	for (i = 0; i = oaht_iter(tab, i, &k, &v);) {
		cnt++;
		sumk += k;
		sumv += v;
	}
	assert(cnt == 2);
	assert(sumk == 5 + 400);
	assert(sumv == 42 + 9);

	/* clean up */
	oaht_destroy(tab);
}

/* Iteration over empty dict */
void iter_empty_test(void) {
	struct oaht * tab = oaht_create();
	int k, v;
	assert(oaht_iter(tab, 0, &k, &v) == 0);
	oaht_destroy(tab);
}

static inline int map_to_some_odd_number(int i) {
	return ((i * 234) & 0xfffff) + 1;
}

void large_table_test(void) {
	int i, n = 1000;
	struct oaht * ht = oaht_create();
	for (i = 0; i < n; i++) {
		int k = map_to_some_odd_number(i);
		ht = oaht_set(ht, k, i);
	}
	assert(oaht_len(ht) == 1000);
	assert(oaht_get(ht, map_to_some_odd_number(0), -1) == 0);
	assert(oaht_get(ht, map_to_some_odd_number(5), -1) == 5);
	oaht_destroy(ht);
}

int main() {
	get_test();
	iter_test();
	iter_empty_test();
	large_table_test();
	return 0;
}
