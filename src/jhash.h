#ifndef __JHASH_H
#define __JHASH_H

#include <inttypes.h>

/* jhash.h: Jenkins hash support.
 *
 * Copyright (C) 2006. Bob Jenkins (bob_jenkins@burtleburtle.net)
 * Copyright (C) 2009-2010 Jozsef Kadlecsik (kadlec@blackhole.kfki.hu)
 *
 * http://burtleburtle.net/bob/hash/
 * http://git.netfilter.org/ipset/plain/kernel/include/linux/jhash.h
 *
 */

#define jhash_size(n) ((uint32_t)1<<(n))
#define jhash_mask(n) (jhash_size(n)-1)
#define rot(x,k)      (((x)<<(k)) | ((x)>>(32-(k))))

#define jhash_mix(a, b, c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c, 16); c += b; \
  b -= a;  b ^= rot(a, 19); a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

#define jhash_final(a, b, c) \
{ \
  c ^= b; c -= rot(b, 14); \
  a ^= c; a -= rot(c, 11); \
  b ^= a; b -= rot(a, 25); \
  c ^= b; c -= rot(b, 16); \
  a ^= c; a -= rot(c, 4);  \
  b ^= a; b -= rot(a, 14); \
  c ^= b; c -= rot(b, 24); \
}

#define JHASH_INITVAL   0xdeadbeef

#define M1  0xff
#define M2  0xffff
#define M3  0xffffff

/* jhash - hash an arbitrary key
 * @k: sequence of bytes as key
 * @length: the length of the key
 * @initval: the previous hash, or an arbitray value
 *
 * The generic version, hashes an arbitrary sequence of bytes.
 * No alignment or length assumptions are made about the input key.
 *
 * Returns the hash value of the key. The result depends on endianness.
 */

static inline
uint32_t jhash (const void *key, uint32_t length, uint32_t initval)
{
  uint32_t a, b, c;
  const uint32_t *k = (const uint32_t *) key;

  /* Set up the internal state */
  a = b = c = JHASH_INITVAL + length + initval;

  /* All but the last block: affect some 32 bits of (a,b,c) */
  while (length > 12) {
    a += k[0];
    b += k[1];
    c += k[2];
    jhash_mix (a, b, c);
    length -= 12;
    k += 3;
  }

  switch (length) {
    case 12: c += k[2];      b += k[1];      a += k[0];      break;
    case 11: c += k[2] & M3; b += k[1];      a += k[0];      break;
    case 10: c += k[2] & M2; b += k[1];      a += k[0];      break;
    case  9: c += k[2] & M1; b += k[1];      a += k[0];      break;
    case  8:                 b += k[1];      a += k[0];      break;
    case  7:                 b += k[1] & M3; a += k[0];      break;
    case  6:                 b += k[1] & M2; a += k[0];      break;
    case  5:                 b += k[1] & M1; a += k[0];      break;
    case  4:                                 a += k[0];      break;
    case  3:                                 a += k[0] & M3; break;
    case  2:                                 a += k[0] & M2; break;
    case  1:                                 a += k[0] & M1; break;
    case  0: return c;
  }

  jhash_final (a,b,c)
  return c;
}

#endif // __JHASH_H
