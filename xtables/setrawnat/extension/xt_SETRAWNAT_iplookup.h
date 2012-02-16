#ifndef _XT_SETRAWNAT_IPLOOKUP_H
#define _XT_SETRAWNAT_IPLOOKUP_H

#include <linux/rcupdate.h>
#include <linux/jhash.h>
#include <linux/netfilter/ipset/ip_set_timeout.h>

#define CONCAT(a, b, c)		a##b##c
#define TOKEN(a, b, c)		CONCAT(a, b, c)

#define type_pf_next		TOKEN(TYPE, PF, _elem)

/* Hashing which uses arrays to resolve clashing. The hash table is resized
 * (doubled) when searching becomes too long.
 * Internally jhash is used with the assumption that the size of the
 * stored data is a multiple of sizeof(u32). If storage supports timeout,
 * the timeout field must be the last one in the data structure - that field
 * is ignored when computing the hash key.
 *
 * Readers and resizing
 *
 * Resizing can be triggered by userspace command only, and those
 * are serialized by the nfnl mutex. During resizing the set is
 * read-locked, so the only possible concurrent operations are
 * the kernel side readers. Those must be protected by proper RCU locking.
 */

/* Number of elements to store in an initial array block */
#define AHASH_INIT_SIZE			4
/* Max number of elements to store in an array block */
#define AHASH_MAX_SIZE			(3*AHASH_INIT_SIZE)

/* Max number of elements can be tuned */
#ifdef IP_SET_HASH_WITH_MULTI
#define AHASH_MAX(h)			((h)->ahash_max)

static inline u8
tune_ahash_max(u8 curr, u32 multi)
{
	u32 n;

	if (multi < curr)
		return curr;

	n = curr + AHASH_INIT_SIZE;
	/* Currently, at listing one hash bucket must fit into a message.
	 * Therefore we have a hard limit here.
	 */
	return n > curr && n <= 64 ? n : curr;
}
#define TUNE_AHASH_MAX(h, multi)	\
	((h)->ahash_max = tune_ahash_max((h)->ahash_max, multi))
#else
#define AHASH_MAX(h)			AHASH_MAX_SIZE
#define TUNE_AHASH_MAX(h, multi)
#endif

/* A hash bucket */
struct hbucket {
	void *value;		/* the array of the values */
	u8 size;		/* size of the array */
	u8 pos;			/* position of the first free entry */
};

/* The hash table: the table size stored here in order to make resizing easy */
struct htable {
	u8 htable_bits;		/* size of hash table == 2^htable_bits */
	struct hbucket bucket[0]; /* hashtable buckets */
};

#define hbucket(h, i)		(&((h)->bucket[i]))

/* Book-keeping of the prefixes added to the set */
struct ip_set_hash_nets {
	u8 cidr;		/* the different cidr values in the set */
	u32 nets;		/* number of elements per cidr */
};

/* The generic ip_set hash structure */
struct ip_set_hash {
	struct htable *table;	/* the hash table */
	u32 maxelem;		/* max elements in the hash */
	u32 elements;		/* current element (vs timeout) */
	u32 initval;		/* random jhash init value */
	u32 timeout;		/* timeout value, if enabled */
	struct timer_list gc;	/* garbage collection when timeout enabled */
	struct type_pf_next next; /* temporary storage for uadd */
#ifdef IP_SET_HASH_WITH_MULTI
	u8 ahash_max;		/* max elements in an array block */
#endif
#ifdef IP_SET_HASH_WITH_NETMASK
	u8 netmask;		/* netmask value for subnets to store */
#endif
#ifdef IP_SET_HASH_WITH_RBTREE
	struct rb_root rbtree;
#endif
#ifdef IP_SET_HASH_WITH_NETS
	struct ip_set_hash_nets nets[0]; /* book-keeping of prefixes */
#endif
};

#ifdef IP_SET_HASH_WITH_NETS
#ifdef IP_SET_HASH_WITH_NETS_PACKED
/* When cidr is packed with nomatch, cidr - 1 is stored in the entry */
#define CIDR(cidr)	(cidr + 1)
#else
#define CIDR(cidr)	(cidr)
#endif

#define SET_HOST_MASK(family)	(family == AF_INET ? 32 : 128)

/* Network cidr size book keeping when the hash stores different
 * sized networks */
static void
add_cidr(struct ip_set_hash *h, u8 cidr, u8 host_mask)
{
	u8 i;

	++h->nets[cidr-1].nets;

	pr_debug("add_cidr added %u: %u\n", cidr, h->nets[cidr-1].nets);

	if (h->nets[cidr-1].nets > 1)
		return;

	/* New cidr size */
	for (i = 0; i < host_mask && h->nets[i].cidr; i++) {
		/* Add in increasing prefix order, so larger cidr first */
		if (h->nets[i].cidr < cidr)
			swap(h->nets[i].cidr, cidr);
	}
	if (i < host_mask)
		h->nets[i].cidr = cidr;
}

static void
del_cidr(struct ip_set_hash *h, u8 cidr, u8 host_mask)
{
	u8 i;

	--h->nets[cidr-1].nets;

	pr_debug("del_cidr deleted %u: %u\n", cidr, h->nets[cidr-1].nets);

	if (h->nets[cidr-1].nets != 0)
		return;

	/* All entries with this cidr size deleted, so cleanup h->cidr[] */
	for (i = 0; i < host_mask - 1 && h->nets[i].cidr; i++) {
		if (h->nets[i].cidr == cidr)
			h->nets[i].cidr = cidr = h->nets[i+1].cidr;
	}
	h->nets[i - 1].cidr = 0;
}
#endif

#endif /* _XT_SETRAWNAT_IPLOOKUP_H */

#ifndef HKEY_DATALEN
#if (PF == 4)
#define HKEY_DATALEN	sizeof(__be32)
#elif (PF == 6)
#define HKEY_DATALEN	sizeof(union nf_inet_addr)
#endif
#endif

#define CONCAT(a, b, c)		a##b##c
#define TOKEN(a, b, c)		CONCAT(a, b, c)

/* Hash key wrapper */
#define type_pf_hash		TOKEN(TYPE, PF, _hash)

static inline u32
type_pf_hash(const void *data, u32 initval, u32 htable_bits);

#define HKEY(data, initval, htable_bits)	\
(type_pf_hash(data, initval, htable_bits))

/* Type/family dependent function prototypes */

#define type_pf_data_equal	TOKEN(TYPE, PF, _data_equal)
#define type_pf_data_copy	TOKEN(TYPE, PF, _data_copy)

#define type_pf_elem		TOKEN(TYPE, PF, _elem)
#define type_pf_telem		TOKEN(TYPE, PF, _telem)

#define type_pf_test		TOKEN(TYPE, PF, _test)

static inline u32
type_pf_hash(const void *data, u32 initval, u32 htable_bits)
{
	const struct type_pf_elem *d = data;
	return jhash2((u32 *)(&d->ip), HKEY_DATALEN/sizeof(u32), initval) &
	         jhash_mask(htable_bits);
}

/* Flavour without timeout */

/* Get the ith element from the array block n */

#define ahash_data(n, i)	\
	((struct type_pf_elem *)((n)->value) + (i))

#define ahash_tdata(n, i) \
	(struct type_pf_elem *)((struct type_pf_telem *)((n)->value) + (i))

/* Test whether the element is added to the set */
static int
type_pf_test(struct ip_set *set, void *value)
{
	struct ip_set_hash *h = set->data;
	struct htable *t = h->table;
	struct type_pf_elem *d = value;
	struct hbucket *n;
	const struct type_pf_elem *data;
	int i, ret = 0;
	u32 key, multi = 0;

	key = HKEY(d, h->initval, t->htable_bits);
	n = hbucket(t, key);

	read_lock_bh(&set->lock);
	for (i = 0; i < n->pos; i++) {
		data = ahash_data(n, i);
		if (type_pf_data_equal(data, d, &multi)) {
			type_pf_data_copy(d, data);
			ret = 1;
			break;
        }
	}
	read_unlock_bh(&set->lock);

	return ret;
}

#undef HKEY_DATALEN
#undef HKEY
#undef type_pf_data_equal
#undef type_pf_data_copy

#undef type_pf_elem
#undef type_pf_telem

#undef type_pf_test
