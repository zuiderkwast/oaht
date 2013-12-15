#define OAHT_KEY_T int
#define OAHT_VALUE_T int
#define OAHT_HASH_T int
#define OAHT_HASH(x) (x + 42)
#define OAHT_EMPTY_KEY 0
#define OAHT_DELETED_KEY -1
#include "oaht.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int main() {
	struct oaht * my_tab = oaht_create();
	/* set */
	my_tab = oaht_set(my_tab, 5, 42);
	my_tab = oaht_set(my_tab, 400, 9);
	/* get */
	assert(oaht_get(my_tab, 400, 999) == 9);
	assert(oaht_get(my_tab,   5, 999) == 42);
	assert(oaht_get(my_tab, 827, 999) == 999);
	{
		/*
		 * Iteration. Since the order of entries is undefined, we check
		 * the sums of keys and values instead of the actual values.
		 */
		size_t i;
		int k, v;
		int sumk = 0, sumv = 0;
		for (i = 0; i = oaht_iter(my_tab, i, &k, &v);) {
			sumk += k;
			sumv += v;
		}
		assert(sumk == 5 + 400);
		assert(sumv == 42 + 9);
	}
	return 0;
}
