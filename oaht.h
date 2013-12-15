/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2013 Viktor Söderqvist
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * oaht.h - A generic open addressing hash table
 */

#ifndef OAHT_H
#include <stdlib.h>
#include <stddef.h> /* offsetof */
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
 * Macros for the special key values EMPTY and DELETED. If an empty key is
 * represented by a repeted byte, define OAHT_EMPTY_KEY_BYTE to this value
 * to allow memset to be used in the initialization of an empty hashtable.
 */
#ifndef OAHT_EMPTY_KEY
	#define OAHT_EMPTY_KEY (OAHT_KEY_T)0
	#define OAHT_EMPTY_KEY_BYTE 0
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

/*
 * Generics: prefix to use instead of 'oaht'. Defaults to oaht.
 */
#ifndef OAHT_PREFIX
	#define OAHT_PREFIX oaht
#endif

/*
 * Macros to expand the prefix.
 */
#undef OAHT_XXNAME
#define OAHT_XXNAME(prefix, name) prefix ## name
#undef OAHT_XNAME
#define OAHT_XNAME(prefix, name) OAHT_XXNAME(prefix, name)
#undef OAHT_NAME
#define OAHT_NAME(name) OAHT_XNAME(OAHT_PREFIX, name)

/* A key-value pair */
struct OAHT_NAME(_entry) {
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
struct OAHT_PREFIX {
	#ifdef OAHT_HEADER
	OAHT_HEADER
	#endif
	OAHT_SIZE_T fill;                /* num used + deleted entries */
	OAHT_SIZE_T used;                /* the number of used entries */
	OAHT_SIZE_T mask;                /* actual length of els - 1 */
	struct OAHT_NAME(_entry) els[1]; /* entries, allocated in-place */
};

/* Size to allocate for a struct oaht with mask mask. Used internally. */
static inline size_t
OAHT_NAME(_sizeof)(OAHT_SIZE_T mask) {
	return sizeof(struct OAHT_PREFIX) +
		mask * sizeof(struct OAHT_NAME(_entry));
}

/* Create a duplicate */
static inline struct OAHT_PREFIX *
OAHT_NAME(_clone)(struct OAHT_PREFIX *a) {
	OAHT_SIZE_T size = OAHT_NAME(_sizeof)(a->mask);
	struct OAHT_PREFIX *clone = (struct OAHT_PREFIX *)OAHT_ALLOC(size);
	return memcpy(clone, a, size);
}

/* Used internally */
static inline OAHT_HASH_T
OAHT_NAME(_get_hash_of_entry)(struct OAHT_NAME(_entry) *e) {
	#ifndef OAHT_NO_STORE_HASH
	return e->hash;
	#else
	return OAHT_HASH(e->key);
	#endif
}

/*
 * Creates an empty hashtable of a given initial size.
 */
static inline struct OAHT_PREFIX *
OAHT_NAME(_create_presized)(OAHT_SIZE_T min_size) {
	OAHT_SIZE_T size = OAHT_MIN_CAPACITY;
	OAHT_SIZE_T mask;
	struct OAHT_PREFIX *a;
	assert(min_size >= 0);
	assert(OAHT_MIN_CAPACITY > 0);
	/* Compute size, the smallest power of 2 >= min_size */
	while (size < min_size) {
		min_size *= 2;
		if (size < OAHT_MIN_CAPACITY) OAHT_OOM(); /* overflow */
	}
	mask = size - 1;
	a = (struct OAHT_PREFIX *)OAHT_ALLOC(OAHT_NAME(_sizeof)(mask));
	if (!a) OAHT_OOM();
	#ifdef OAHT_EMPTY_KEY_BYTE
	# if OAHT_EMPTY_KEY_BYTE == 0
	memset(a, 0, OAHT_NAME(_sizeof)(mask));
	# else
	memset(a, 0, offsetof(struct OAHT_PREFIX, mask));
	memset(a + offsetof(struct OAHT_PREFIX, mask),
	       OAHT_EMPTY_KEY_BYTE,
	       OAHT_NAME(_sizeof)(mask) -
	           offsetof(struct OAHT_PREFIX, mask));
	# endif
	#else
	memset(a, 0, offsetof(struct OAHT_PREFIX, mask));
	{
		OAHT_SIZE_T i;
		for (i = 0; i < size; i++)
			a->els[i].key = OAHT_EMPTY_KEY;
	}
	#endif
	a->mask = mask;
	assert(OAHT_IS_EMPTY_KEY(a->els[0].key));
	return a;
}

/*
 * Creates an empty hashtable with the minimum initial size.
 */
static inline struct OAHT_PREFIX *
OAHT_NAME(_create)(void) {
	return OAHT_NAME(_create_presized)(OAHT_MIN_CAPACITY);
}

/*
 * Frees the memory.
 */
static inline void
OAHT_NAME(_destroy)(struct OAHT_PREFIX *a) {
	OAHT_FREE(a, OAHT_NAME(_sizeof)(a->mask));
}

/*
 * Returns the number of entries in the hashtable.
 */
static inline OAHT_SIZE_T
OAHT_NAME(_len)(struct OAHT_PREFIX *a) {
	return a->used;
}

/*
 * A function to iterate over the keys and values. Start by passing pos = 0.
 * Pass the return value as i to get the next entry. When 0 is returned, there
 * is no more entry to get.
 *
 * If a non-zero value is returned, k and v are assigned to point to a key and
 * a value in the hashtable.
 */
static inline OAHT_SIZE_T
OAHT_NAME(_iter)(struct OAHT_PREFIX *a, OAHT_SIZE_T pos, OAHT_KEY_T *k, OAHT_VALUE_T *v) {
	for (; pos <= a->mask; pos++) {
		if (OAHT_IS_EMPTY_KEY(a->els[pos].key) ||
			OAHT_IS_DELETED_KEY(a->els[pos].key))
			continue;
		*k = a->els[pos].key;
		*v = a->els[pos].value;
		return pos + 1;
	}
	/* There are no more entries. */
	return 0;
}

/*
 * Lookup an entry by its key. Returns a pointer to an entry that can be
 * assigned to, to insert or replace a value in the table. If the returned
 * entry is EMPTY or DELETED, the key is not present in the table.
 *
 * This function is used by many of the other functions (set, get, delete).
 */
static inline struct OAHT_NAME(_entry) *
OAHT_NAME(_lookup_helper)(struct OAHT_PREFIX *a, OAHT_KEY_T key, OAHT_HASH_T hash) {
	OAHT_SIZE_T pos = hash & a->mask; /* initial probe */
	struct OAHT_NAME(_entry) *freeslot = NULL;
	assert(!OAHT_IS_EMPTY_KEY(key));
	assert(!OAHT_IS_DELETED_KEY(key));
	/* This will always terminate as there is always one empty entry */
	while (1) {
		if (OAHT_IS_EMPTY_KEY(a->els[pos].key))
			return freeslot ? freeslot : &a->els[pos];
		if (OAHT_KEY_EQUALS(a->els[pos].key, key)
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
static inline struct OAHT_PREFIX *
OAHT_NAME(_resize)(struct OAHT_PREFIX *a, OAHT_SIZE_T min_size) {
	struct OAHT_PREFIX *b = OAHT_NAME(_create_presized)(min_size);
	OAHT_SIZE_T i;
	/* copy user-defined header data */
	#ifdef OAHT_HEADER
	memcpy(b, a, offsetof(struct OAHT_PREFIX, fill));
	#endif
	/* set the used and fill values as they will be */
	b->used = b->fill = a->used;
	/* copy the entries */
	for (i = 0; i <= a->mask; i++) {
		struct OAHT_NAME(_entry) *ea = &a->els[i];
		struct OAHT_NAME(_entry) *eb;
		if (OAHT_IS_EMPTY_KEY(ea->key)
		    || OAHT_IS_DELETED_KEY(ea->key))
			continue;
		eb = OAHT_NAME(_lookup_helper)(b, ea->key, OAHT_NAME(_get_hash_of_entry)(ea));
		assert(OAHT_IS_EMPTY_KEY(eb->key));
		memcpy(eb, ea, sizeof(struct OAHT_NAME(_entry)));
	}
	/* Free the memory of the old table */
	OAHT_FREE(a, OAHT_SIZEOF(a->mask));
	return b;
}

/*
 * Check if a key exists. Returns 1 if it does, 0 if it doesn't.
 */
static inline int
OAHT_NAME(_contains)(struct OAHT_PREFIX *a, OAHT_KEY_T key) {
	struct OAHT_NAME(_entry) *e = OAHT_NAME(_lookup_helper)(a, key, OAHT_HASH(key));
	return !OAHT_IS_EMPTY_KEY(e->key) && !OAHT_IS_DELETED_KEY(e->key);
}

#ifndef OAHT_NO_VALUE
/* The hashtable has values. Provide get and set functions. */

/*
 * Fetch a value by its key. If it's not defined, default_value is returned.
 */
static inline OAHT_VALUE_T
OAHT_NAME(_get)(struct OAHT_PREFIX *a, OAHT_KEY_T key, OAHT_VALUE_T default_value) {
	struct OAHT_NAME(_entry) *entry =
		OAHT_NAME(_lookup_helper)(a, key, OAHT_HASH(key));
	return OAHT_IS_EMPTY_KEY(entry->key) || OAHT_IS_DELETED_KEY(entry->key)
		? default_value : entry->value;
}

/*
 * Insert or replace the element at the given key. Returns a pointer to the same
 * memory location or to a new memory location if the memory has been
 * reallocated. (If the hash tables has been reallocated, the old memory has
 * been free'd.)
 */
static inline struct OAHT_PREFIX *
OAHT_NAME(_set)(struct OAHT_PREFIX *a, OAHT_KEY_T key, OAHT_VALUE_T value) {
	OAHT_HASH_T hash = OAHT_HASH(key);
	struct OAHT_NAME(_entry) *entry =
		OAHT_NAME(_lookup_helper)(a, key, hash);
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
		return OAHT_NAME(_resize)(a, (a->used > 50000 ? 2 : 4) * a->used);
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
static inline struct OAHT_PREFIX *
OAHT_NAME(_add)(struct OAHT_PREFIX *a, OAHT_KEY_T key) {
	OAHT_HASH_T hash = OAHT_HASH(key);
	struct OAHT_NAME(_entry) *entry =
		OAHT_NAME(_lookup_helper)(a, key, hash);
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
		return OAHT_NAME(_resize)(a, (a->used > 50000 ? 2 : 4) * a->used);
	return a;
}

#endif

/*
 * Delete the given key from the hashtable. Returns a pointer to the same
 * memory location or to a new memory location if the memory has been
 * reallocated. (If the hash tables has been reallocated, the old memory has
 * been free'd.)
 */
static inline struct OAHT_PREFIX *
OAHT_NAME(_delete)(struct OAHT_PREFIX *a, OAHT_KEY_T key) {
	struct OAHT_NAME(_entry) *entry =
		OAHT_NAME(_lookup_helper)(a, key, OAHT_HASH(key));
	if (!OAHT_IS_EMPTY_KEY(entry->key) && !OAHT_IS_DELETED_KEY(entry->key)) {
		entry->key = OAHT_DELETED_KEY;
		a->used--;
		/* Maybe TODO: resize */
	}
	return a;
}

#define OAHT_H
#endif
