oaht
====

Open addressing hash table in C

Features
--------

* Open addressing AKA closed hashing
* Linear probing
* Using a single contiguous memory space for both header and contents
* Highly configurable, e.g.
  * User-defined key and value types
  * User-defined hash function and hash value type
  * Optional user-defined data inside the hashtable header (e.g. ref-counter)
  * Optional value – when value is omitted, the hashtable is a set
* No divisions or modulos
* Small and readable source code (~300 lines) in standard C.

Notes
-----

The library is implemented as a single header file and all functions are declared `static inline`. This is much more readable and maintailable than macros while being almost as generic. Generics is done by defining configuration macros prior to including the header file. The macros are described in a section below.

The hashtable relies on two special values for keys: `OAHT_EMPTY_KEY` and `OAHT_DELETED_KEY`. The macro `OAHT_EMPTY_KEY` must always be represented as all bits zero (for practical reasons) while `OAHT_DELETED_KEY` may be defined to any non-zero value. These keys cannot be inserted into the hashtable.

The hashtable is resized when it's 2/3 full, to keep the number of collissions low. This and some other design details are modelled after CPython's dict object. (Source code: http://svn.python.org/view/python/trunk/Objects/dictobject.c?view=markup)

Example
-------

```C
#define OAHT_KEY_T int
#define OAHT_VALUE_T int
#define OAHT_HASH_T int
#define OAHT_HASH(x) (x + 42)
#define OAHT_EMPTY_KEY 0
#define OAHT_DELETED_KEY -1
#include "oaht.h"

#include <stdio.h>

int main() {
	struct oaht * my_tab = oaht_create();

	my_tab = oaht_set(my_tab, 5, 42);
	my_tab = oaht_set(my_tab, 400, 9);
	my_tab = oaht_set(my_tab, 827, 42);

	my_tab = oaht_delete(my_tab, 400);

	printf("Values: %d, %d, %d.\n",
	       oaht_get(my_tab, 5, 0),
	       oaht_get(my_tab, 400, 0),
	       oaht_get(my_tab, 827, 0));

	return 0;
}
```

Function reference
------------------

**oaht_create**: Creates an empty hashtable with the minimum initial size.

**oaht_create_presized**: Creates an empty hashtable of a given initial size.

```c
static inline struct oaht *
oaht_create(void)

static inline struct oaht *
oaht_create_presized(OAHT_SIZE_T min_size)
```

**oaht_destroy**: Frees the memory.

```c
static inline void
oaht_destroy(struct oaht *a)
```

**oaht_len**: Returns the number of entries in the hashtable.

```c
static inline OAHT_SIZE_T
oaht_len(struct oaht *a)
```

**oaht_contains**: Check if a key exists. Returns 1 if it does, 0 if it doesn't.

```c
static inline int
oaht_contains(struct oaht *a, OAHT_KEY_T key)
```

**oaht_get**: Fetch a value by its key. If it's not defined, `default_value` is returned. This function does not exist if `OAHT_NO_VALUE` is defined.

```c
static inline OAHT_VALUE_T
oaht_get(struct oaht *a, OAHT_KEY_T key, OAHT_VALUE_T default_value)
```

**oaht_set**: Insert or replace the element at the given key. Returns a pointer to the same memory location or to a new memory location if the memory has been reallocated. (If the hash tables has been reallocated, the old memory has been free'd.) This function does not exist if `OAHT_NO_VALUE` is defined.

```c
static inline struct oaht *
oaht_set(struct oaht *a, OAHT_KEY_T key, OAHT_VALUE_T value)
```

**oaht_delete**: Delete the given key from the hashtable. Returns a pointer to the same memory location or to a new memory location if the memory has been reallocated. (If the hash tables has been reallocated, the old memory has been free'd.)

```c
static inline struct oaht *
oaht_delete(struct oaht *a, OAHT_KEY_T key)
```

**oaht_add**: Exists only when `OAHT_NO_VALUE` is defined. Then, the hashtable is a set. The functions `oaht_get` and `oaht_set` are not defined. Instead, `oaht_add` is defined to add an element (key) to the hashtable. Returns a pointer to the same memory location or to a new memory location if the memory has been reallocated. (If the hash tables has been reallocated, the old memory has been free'd.)

```c
static inline struct oaht *
oaht_add(struct oaht *a, OAHT_KEY_T key)
```

Generics, configuration, tweaking
---------------------------------

Configuration is done by defining macros prior to including `oaht.h`. All configuration macros are optional and have defaults. Though the library is generic, it can only be included once in each compilation unit.

Types of keys, values and hash function:

* `OAHT_KEY_T`: Type of the keys. Defaults to `int`.
* `OAHT_VALUE_T`: Type of the values. Defauts to `void *`.
* `OAHT_SIZE_T`: Type of sizes such as the number of elements in the table. Should be an integer type. Defaults to `unsigned int`.
* `OAHT_HASH(key)`: The hash function. Should take a key of type `OAHT_KEY_T` and return a value of type `OAHT_HASH_T`. Defaults to casting the key to `OAHT_HASH_T`.
* `OAHT_HASH_T`: The type of hashes. This should be the return type of the hash function. Defaults to `int`.
* `OAHT_KEY_EQUALS(a, b)`: Takes two keys of type OAHT_KEY_T and should evaluate to non-zero if they are equal and to zero if they are not equal. Defaults to `a == b`.
* `OAHT_EMPTY_KEY`: A special value of a key that represents an empty slot. This value must not be used as a key. Must be represented with all bits set to zero. Defaults to `0`.
* `OAHT_DELETED_KEY`: A special value of a key that represents a deleted slot. This value must not be used as a key. Defaults to `-1`.
* `OAHT_IS_EMPTY_KEY(key)`: Check if a key is the empty key. Defaults to `key == OAHT_EMPTY_KEY`.
* `OAHT_IS_DELETED_KEY(key)`: Check if a key is the deleted key. Defaults to `key == OAHT_DELETED_KEY`.
* `OAHT_HEADER`: If defined, this is included first in the `struct oaht`. Typical fields may include a type tag and a reference counter. Not defined by default.
* `OAHT_MIN_CAPACITY`: Minimum and initial capacity. Defaults to `8`.
* `OAHT_NO_STORE_HASH`: Unless this macro is defined, the hash value is stored in the hashtable together with the key and the value, to avoid computing the hash more often. If this macro is defined, the hash function is used every time the hash value is needed. Define this macro if you have a very fast hash function (such as taking the key itself as the hash) or to optimize for memory.
* `OAHT_NO_VALUE`: If this macro is defined, no value is stored together with the key and thus the hashtable is a set. The get and set functions are not defined. Instead, an add function is defined. The contains function is always defined.

Allocation macros. These default to malloc/realloc/free but may be defined to use custom allocation functions.

* `OAHT_ALLOC(size)`: Allocate n bytes. Defaults to `malloc(size)`.
* `OAHT_REALLOC(ptr, size, oldsize)`: Reallocate size bytes pointed to by `ptr`. The old size is provided to allow tracking memory usage. Defaults to `realloc(ptr, size)`.
* `OAHT_FREE(ptr, size)`: Free the memory pointed to by `ptr`. The size is provided to allow tracking memory usage. Defaults to `free(ptr)`.

Error handling macros.

* `OAHT_OOM()`: This is called when a memory allocation fails. Must exit the program or longjmp out of the current function. Defaults to `exit(-1)`.

Choosing a hash function
------------------------

As the hashtable uses sizes of powers of 2 and linear probing, a good hash function is essential to minimize collissions. Recommended hash functions include SipHash-2-4 for numeric keys on 64-bit platforms, Spooky hash or City hash for variable-length keys such as strings on x86-64.

Related projects
----------------

* UT_hash: a generic hashtable in C implemented as macros. More generic but less readable and less elegant.
