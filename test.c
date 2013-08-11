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
	my_tab = oaht_set(my_tab, 5, 42);
	my_tab = oaht_set(my_tab, 400, 9);
	assert(oaht_get(my_tab, 400, 999) == 9);
	assert(oaht_get(my_tab,   5, 999) == 42);
	assert(oaht_get(my_tab, 827, 999) == 999);
	return 0;
}
