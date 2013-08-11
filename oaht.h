/*
 * oaht.h - A generic open addressing hash table
 *
 * Author: Viktor Söderqvist
 * License: MIT
 */
#ifndef OAHT_H
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Allocation macros. Default to malloc, realloc and free */
#ifndef OAHT_ALLOC
	#define OAHT_ALLOC(size) malloc(size)
#endif
#ifndef OAHT_REALLOC
	#define OAHT_REALLOC(ptr, size, oldsize) realloc(ptr, size)
#endif
#ifndef OAHT_FREE
	#define OAHT_FREE(ptr, size) free(ptr)
#endif

/*
 * Out of memory macro. Defaults to exit(-1).
 * Must exit or longjmp out of the current function.
 */
#ifndef OAHT_OOM
	#define OAHT_OOM() exit(-1)
#endif

/* Minimum capacity, must be a power of 2 */
#ifndef OAHT_MIN_CAPACITY
	#define OAHT_MIN_CAPACITY 8
#endif

/* Key type */
#ifndef OAHT_KEY_T
	#define OAHT_KEY_T int
#endif

/* Key equality check, defaults to a == b */
#ifndef OAHT_KEY_EQUALS
	#define OAHT_KEY_EQUALS(a, b) (a == b)
#endif

/* Value type */
#ifndef OAHT_VALUE_T
	#define OAHT_VALUE_T void*
#endif

/* The type of the lengths and indices, should be an integer type */
#ifndef OAHT_SIZE_T
	#define OAHT_SIZE_T unsigned int
#endif

/* The hash type is the return type of the hash function. */
#ifndef OAHT_HASH_T
	#define OAHT_HASH_T int
#endif

/* Hash function */
#ifndef OAHT_HASH
	#define OAHT_HASH(key) (OAHT_HASH_T)key
#endif

/*
 * Macros for the special key values EMPTY and DELETED. An empty key must be
 * encoded as all bits zero.
 */
#ifndef OAHT_EMPTY_KEY
	#define OAHT_EMPTY_KEY (OAHT_KEY_T)0
#endif

#ifndef OAHT_DELETED_KEY
	#define OAHT_DELETED_KEY (OAHT_KEY_T)-1
#endif

#ifndef OAHT_IS_EMPTY_KEY
	#define OAHT_IS_EMPTY_KEY(key) (key == OAHT_EMPTY_KEY)
#endif

#ifndef OAHT_IS_DELETED_KEY
	#define OAHT_IS_DELETED_KEY(key) (key == OAHT_DELETED_KEY)
#endif

/* A key-value pair */
struct oaht_entry {
	#ifndef OAHT_NO_STORE_HASH
	OAHT_HASH_T hash;
	#endif
	OAHT_KEY_T key;
	#ifndef OAHT_NO_VALUE
	OAHT_VALUE_T value;
	#endif
};

/*
 * The hashtable type, optionally prefixed user-defined extra members.
 *
 * There is always at least one EMPTY entry in a table.
 */
struct oaht {
	#ifdef OAHT_HEADER
	OAHT_HEADER
	#endif
	OAHT_SIZE_T fill;          /* the number of used + deleted entries */
	OAHT_SIZE_T used;          /* the number of used entries */
	OAHT_SIZE_T mask;          /* actual length of els - 1 */
	struct oaht_entry els[1];  /* entries, allocated in-place */
};

/* Size to allocate for a struct oaht with mask mask. Used internally. */
static inline size_t
oaht_sizeof(OAHT_SIZE_T mask) {
	return sizeof(struct oaht) + mask * sizeof(struct oaht_entry);
}

/* Used internally */
static inline OAHT_HASH_T
oaht_get_hash_of_entry(struct oaht_entry *e) {
	#ifndef OAHT_NO_STORE_HASH
	return e->hash;
	#else
	return OAHT_HASH(e->key);
	#endif
}

/*
 * Creates an empty hashtable of a given initial size.
 */
static inline struct oaht *
oaht_create_presized(OAHT_SIZE_T min_size) {
	OAHT_SIZE_T size = OAHT_MIN_CAPACITY;
	OAHT_SIZE_T mask;
	struct oaht *a;
	assert(min_size >= 0);
	assert(OAHT_MIN_CAPACITY > 0);
	/* Compute size, the smallest power of 2 >= min_size */
	while (size < min_size) {
		min_size *= 2;
		if (size < OAHT_MIN_CAPACITY) OAHT_OOM(); /* overflow */
	}
	mask = size - 1;
	a = (struct oaht *)OAHT_ALLOC(oaht_sizeof(mask));
	if (!a) OAHT_OOM();
	memset(a, 0, oaht_sizeof(mask));
	a->mask = mask;
	assert(OAHT_IS_EMPTY_KEY(a->els[0].key));
	return a;
}

/*
 * Creates an empty hashtable with the minimum initial size.
 */
static inline struct oaht *
oaht_create(void) {
	return oaht_create_presized(OAHT_MIN_CAPACITY);
}

/*
 * Frees the memory.
 */
static inline void
oaht_destroy(struct oaht *a) {
	OAHT_FREE(a, oaht_sizeof(a->mask));
}

/*
 * Returns the number of entries in the hashtable.
 */
static inline OAHT_SIZE_T
oaht_len(struct oaht *a) {
	return a->used;
}

/*
 * Lookup an entry by its key. Returns a pointer to an entry that can be
 * assigned to, to insert or replace a value in the table. If the returned
 * entry is EMPTY or DELETED, the key is not present in the table.
 *
 * This function is used by many of the other functions (set, get, delete).
 */
static inline struct oaht_entry *
oaht_lookup_helper(struct oaht *a, OAHT_KEY_T key, OAHT_HASH_T hash) {
	OAHT_SIZE_T pos = hash & a->mask; /* initial probe */
	struct oaht_entry *freeslot = NULL;
	assert(!OAHT_IS_EMPTY_KEY(key));
	assert(!OAHT_IS_DELETED_KEY(key));
	/* This will always terminate as there is always one empty entry */
	while (1) {
		if (OAHT_IS_EMPTY_KEY(a->els[pos].key))
			return freeslot ? freeslot : &a->els[pos];
		if (a->els[pos].key == key
		    || (
		    	#ifndef OAHT_NO_STORE_HASH
		    	a->els[pos].hash == hash &&
		    	#endif
		        !OAHT_IS_DELETED_KEY(a->els[pos].key) &&
		        OAHT_KEY_EQUALS(a->els[pos].key, key)
		       )
		   )
			return &a->els[pos];
		if (OAHT_IS_DELETED_KEY(a->els[pos].key) && !freeslot)
			freeslot = &a->els[pos];
		pos = (pos + 1) & a->mask;
	}
}

/*
 * Allocate and copy the contents to a new memory area. Returns a pointer to
 * the new memory. Used internally.
 */
static inline struct oaht *
oaht_resize(struct oaht *a, OAHT_SIZE_T min_size) {
	struct oaht *b = oaht_create_presized(min_size);
	OAHT_SIZE_T i;
	/* copy user-defined header data */
	#ifdef OAHT_HEADER
	memcpy(b, a, offsetof(struct oaht, fill));
	#endif
	/* set the used and fill values as they will be */
	b->used = b->fill = a->used;
	/* copy the entries */
	for (i = 0; i <= a->mask; i++) {
		struct oaht_entry *ea = &a->els[i];
		struct oaht_entry *eb;
		if (OAHT_IS_EMPTY_KEY(ea->key)
		    || OAHT_IS_DELETED_KEY(ea->key))
			continue;
		eb = oaht_lookup_helper(b, ea->key, oaht_get_hash_of_entry(ea));
		assert(OAHT_IS_EMPTY_KEY(eb->key));
		memcpy(eb, ea, sizeof(struct oaht_entry));
	}
	/* Free the memory of the old table */
	OAHT_FREE(a, OAHT_SIZEOF(a->mask));
	return b;
}

/*
 * Check if a key exists. Returns 1 if it does, 0 if it doesn't.
 */
static inline int
oaht_contains(struct oaht *a, OAHT_KEY_T key) {
	struct oaht_entry *e = oaht_lookup_helper(a, key, OAHT_HASH(key));
	return !OAHT_IS_EMPTY_KEY(e->key) && !OAHT_IS_DELETED_KEY(e->key);
}

#ifndef OAHT_NO_VALUE
/* The hashtable has values. Provide get and set functions. */

/*
 * Fetch a value by its key. If it's not defined, default_value is returned.
 */
static inline OAHT_VALUE_T
oaht_get(struct oaht *a, OAHT_KEY_T key, OAHT_VALUE_T default_value) {
	struct oaht_entry *entry = oaht_lookup_helper(a, key, OAHT_HASH(key));
	return OAHT_IS_EMPTY_KEY(entry->key) || OAHT_IS_DELETED_KEY(entry->key)
		? default_value : entry->value;
}

/*
 * Insert or replace the element at the given key. Returns a pointer to the same
 * memory location or to a new memory location if the memory has been
 * reallocated. (If the hash tables has been reallocated, the old memory has
 * been free'd.)
 */
static inline struct oaht *
oaht_set(struct oaht *a, OAHT_KEY_T key, OAHT_VALUE_T value) {
	OAHT_HASH_T hash = OAHT_HASH(key);
	struct oaht_entry *entry = oaht_lookup_helper(a, key, hash);
	if (OAHT_IS_EMPTY_KEY(entry->key)) {
		a->used++;
		a->fill++;
	} else if (OAHT_IS_DELETED_KEY(entry->key))
		a->used++;
	#ifndef OAHT_NO_STORE_HASH
	entry->hash  = hash;
	#endif
	entry->key   = key;
	entry->value = value;
	/* resize if 2/3 full */
	if (a->fill * 3 >= (a->mask + 1) * 2)
		return oaht_resize(a, (a->used > 50000 ? 2 : 4) * a->used);
	return a;
}

#else
/* The hash table is a set. Provide an add function instead of get and set. */

/*
 * Add an element (key) to the set. Returns a pointer to the same
 * memory location or to a new memory location if the memory has been
 * reallocated. (If the hash tables has been reallocated, the old memory has
 * been free'd.)
 *
 * (This is identical to the set function except there is no value.)
 */
static inline struct oaht *
oaht_add(struct oaht *a, OAHT_KEY_T key) {
	OAHT_HASH_T hash = OAHT_HASH(key);
	struct oaht_entry *entry = oaht_lookup_helper(a, key, hash);
	if (OAHT_IS_EMPTY_KEY(entry->key)) {
		a->used++;
		a->fill++;
	} else if (OAHT_IS_DELETED_KEY(entry->key))
		a->used++;
	#ifndef OAHT_NO_STORE_HASH
	entry->hash  = hash;
	#endif
	entry->key   = key;
	/* resize if 2/3 full */
	if (a->fill * 3 >= (a->mask + 1) * 2)
		return oaht_resize(a, (a->used > 50000 ? 2 : 4) * a->used);
	return a;
}

#endif

/*
 * Delete the given key from the hashtable. Returns a pointer to the same
 * memory location or to a new memory location if the memory has been
 * reallocated. (If the hash tables has been reallocated, the old memory has
 * been free'd.)
 */
static inline struct oaht *
oaht_delete(struct oaht *a, OAHT_KEY_T key) {
	struct oaht_entry *entry = oaht_lookup_helper(a, key, OAHT_HASH(key));
	if (!OAHT_IS_EMPTY_KEY(entry->key) && !OAHT_IS_DELETED_KEY(entry->key)) {
		entry->key = OAHT_DELETED_KEY;
		a->used--;
		/* Maybe TODO: resize */
	}
	return a;
}

#define OAHT_H
#endif
