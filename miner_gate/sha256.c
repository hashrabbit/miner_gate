/*
This file is part of the ARM-Crypto-Lib.
Copyright (C) 2008  Daniel Otte (daniel.otte@rub.de)
6      This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * \file        sha256.h
 * \author  Daniel Otte
 * \date    2006-05-16
 * \license     GPLv3 or later
* 
 */
#define LITTLE_ENDIAN

#include <stdint.h>
#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include "asic.h"
#include "spond_debug.h"
#include <string.h>

#ifndef SHA256_H_
#define SHA256_H_

/** \fn void sha256_init(sha256_ctx_t *state)
 * \brief initialize a SHA-256 context
* 
 * This function sets a ::sha256_ctx_t to the initial values for hashing.
 * \param state pointer to the SHA-256 hashing context
 */
void sha256_init(sha256_ctx_t *state);

/** \fn void sha256_nextBlock (sha256_ctx_t* state, const void* block)
 * \brief update the context with a given block
* 
 * This function updates the SHA-256 hash context by processing the given block
 * of fixed length.
 * \param state pointer to the SHA-256 hash context
 * \param block pointer to the block of fixed length (512 bit = 64 byte)
 */
void sha256_nextBlock(sha256_ctx_t *state, const void *block);

/** \fn void sha256_lastBlock(sha256_ctx_t* state, const void* block, uint16_t
* length_b)
 * \brief finalize the context with the given block
* 
 * This function finalizes the SHA-256 hash context by processing the given
* block
 * of variable length.
 * \param state pointer to the SHA-256 hash context
 * \param block pointer to the block of fixed length (512 bit = 64 byte)
 * \param length_b the length of the block in bits
 */
void sha256_lastBlock(sha256_ctx_t *state, const void *block,
                      uint16_t length_b);

/** \fn void sha256_ctx2hash(sha256_hash_t* dest, const sha256_ctx_t* state)
 * \brief convert the hash state into the hash value
 * This function reads the context and writes the hash value to the destination
 * \param dest pointer to the location where the hash value should be written
 * \param state pointer to the SHA-256 hash context
 */
void sha256_ctx2hash(void *dest, const sha256_ctx_t *state);

/** \fn void sha256(sha256_hash_t* dest, const void* msg, uint32_t length_b)
 * \brief simple SHA-256 hashing function for direct hashing
* 
 * This function automatically hashes a given message of arbitary length with
 * the SHA-256 hashing algorithm.
 * \param dest pointer to the location where the hash value is going to be
* written to
 * \param msg pointer to the message thats going to be hashed
 * \param length_b length of the message in bits
 */
void sha256(void *dest, const void *msg, uint32_t length_b);

#endif /*SHA256_H_*/

/**
 * rotate x right by n positions
 */
static uint32_t rotr32(uint32_t x, uint8_t n) {
  return ((x >> n) | (x << (32 - n)));
}

static uint32_t rotl32(uint32_t x, uint8_t n) {
  return ((x << n) | (x >> (32 - n)));
}

/*************************************************************************/

// #define CHANGE_ENDIAN32(x) (((x)<<24) | ((x)>>24) | (((x)& 0x0000ff00)<<8) |
// (((x)& 0x00ff0000)>>8))
/*
static
uint32_t change_endian32(uint32_t x){
        return (((x)<<24) | ((x)>>24) | (((x)& 0x0000ff00)<<8) | (((x)&
0x00ff0000)>>8));
}
*/

/* sha256 functions as macros for speed and size, cause they are called only
 * once */

#define CH(x, y, z) (((x) & (y)) ^ ((~(x)) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define SIGMA_0(x) (rotr32((x), 2) ^ rotr32((x), 13) ^ rotl32((x), 10))
#define SIGMA_1(x) (rotr32((x), 6) ^ rotr32((x), 11) ^ rotl32((x), 7))
#define SIGMA_a(x) (rotr32((x), 7) ^ rotl32((x), 14) ^ ((x) >> 3))
#define SIGMA_b(x) (rotl32((x), 15) ^ rotl32((x), 13) ^ ((x) >> 10))

const uint32_t k[] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
  0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
  0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
  0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
  0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
  0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void load_endian32_changed(uint8_t *dest, uint8_t *src, uint16_t words) {
  while (words--) {
    *dest++ = src[3];
    *dest++ = src[2];
    *dest++ = src[1];
    *dest++ = src[0];
    src += 4;
  }
}

/**
 * block must be, 512, Bit = 64, Byte, long !!!
 */
void sha2_small_common_nextBlock(sha2_small_common_ctx_t *state,
                                 const void *block) {
  uint32_t w[16], wx;
  uint8_t i;
  uint32_t a[8], t1, t2;

/* init w */
#if defined LITTLE_ENDIAN
  load_endian32_changed((uint8_t *)w, (uint8_t *)block, 16);
#elif defined BIG_ENDIAN
  memcpy((void *)w, block, 64);
#endif
  /*
          for (i=16; i<64; ++i){
                  w[i] = SIGMA_b(w[i-2]) + w[i-7] + SIGMA_a(w[i-15]) + w[i-16];
          }
  */
  /* init working variables */
  memcpy((void *)a, (void *)(state->h), 8 * 4);

  DBG(DBG_HASH, "STARTING 64 STAGES\n");
  /* do the, fun stuff, */
  for (i = 0; i < 64; ++i) {
    DBG(DBG_HASH, "w = \n%08x %08x %08x %08x   %08x %08x %08x %08x   %08x %08x "
                  "%08x %08x   %08x %08x %08x %08x\n",
        w[0], w[1], w[2], w[3], w[4], w[5], w[6], w[7], w[8], w[9], w[10],
        w[11], w[12], w[13], w[14], w[15]);

    DBG(DBG_HASH, "state = \n%08x %08x %08x %08x   %08x %08x %08x %08x\n", a[0],
        a[1], a[2], a[3], a[4], a[5], a[6], a[7]);

    if (i < 16) {
      wx = w[i];
    } else {
      wx = SIGMA_b(w[14]) + w[9] + SIGMA_a(w[1]) + w[0];
      memmove(&(w[0]), &(w[1]), 15 * 4);
      w[15] = wx;
    }
    t1 = a[7] + SIGMA_1(a[4]) + CH(a[4], a[5], a[6]) + k[i] + wx;
    t2 = SIGMA_0(a[0]) + MAJ(a[0], a[1], a[2]);
    memmove(&(a[1]), &(a[0]),
            7 * 4); /* a[7]=a[6]; a[6]=a[5]; a[5]=a[4]; a[4]=a[3]; a[3]=a[2];
                       a[2]=a[1]; a[1]=a[0]; */
    a[4] += t1;
    a[0] = t1 + t2;
  }

  /* update, the, state, */
  for (i = 0; i < 8; ++i) {
    state->h[i] += a[i];
  }
  state->length += 1;
}

void sha2_small_common_lastBlock(sha2_small_common_ctx_t *state,
                                 const void *block, uint16_t length_b) {
  static uint8_t lb[512 / 8]; /* local block */
  static uint64_t len;
  while (length_b >= 512) {
    sha2_small_common_nextBlock(state, block);
    length_b -= 512;
    block = (uint8_t *)block + 64;
  }
  len = state->length * 512 + length_b;
  memset(lb, 0, 64);
  memcpy(lb, block, (length_b + 7) / 8);

  /* set the final one bit */
  lb[length_b / 8] |= 0x80 >> (length_b & 0x7);
  /* pad with zeros */
  if (length_b >= 512 - 64) { /* not enouth space for 64bit length value */
    sha2_small_common_nextBlock(state, lb);
    memset(lb, 0, 64);
  }
/* store the 64bit length value */
#if defined LITTLE_ENDIAN
  /* this is now rolled up */
  uint8_t i;
  i = 7;
  do {
    lb[63 - i] = ((uint8_t *)&len)[i];
  } while (i--);
#elif defined BIG_ENDIAN
  uint8_t i;
  i = 7;
  do {
    lb[i] = ((uint8_t *)&len)[i];
  } while (i--);
#endif
  sha2_small_common_nextBlock(state, lb);
}

/* sha256.c */
/*
This file is part of the ARM-Crypto-Lib.
Copyright (C) 2006-2010  Daniel Otte (daniel.otte@rub.de)
    This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * \file                sha256.c
 * \author              Daniel Otte
 * \date                16.05.2006
* 
 * \par License:
 *      GPL
* 
 * \brief SHA-256 implementation.
* 
* 
 */

#include <stdint.h>
#include <string.h> /* for memcpy, memmove, memset */
//#include "sha256.h"

#define BIG_ENDIAN

#if defined LITTLE_ENDIAN
#elif defined BIG_ENDIAN
#else
#error specify endianess!!!
#endif

void sha2_small_common_nextBlock(sha2_small_common_ctx_t *state,
                                 const void *block);
void sha2_small_common_lastBlock(sha2_small_common_ctx_t *state,
                                 const void *block, uint16_t length_b);

uint32_t sha256_init_vector[] = { 0x6A09E667, 0xBB67AE85, 0x3C6EF372,
                                  0xA54FF53A, 0x510E527F, 0x9B05688C,
                                  0x1F83D9AB, 0x5BE0CD19 };

/*************************************************************************/
/**
 * \brief \c sh256_init initialises a sha256 context for hashing.
 * \c sh256_init c initialises the given sha256 context for hashing
 * @param state pointer to a sha256 context
 * @return none
 */
void sha256_init(sha256_ctx_t *state) {
  state->length = 0;
  memcpy(state->h, sha256_init_vector, 8 * 4);
}

/*************************************************************************/
void sha256_nextBlock(sha256_ctx_t *state, const void *block) {
  sha2_small_common_nextBlock(state, block);
}

/*************************************************************************/
void sha256_lastBlock(sha256_ctx_t *state, const void *block,
                      uint16_t length_b) {
  sha2_small_common_lastBlock(state, block, length_b);
}
/*************************************************************************/

/**
 * \brief function to process the last block being hashed
 * @param state Pointer to the context in which this block should be processed.
 * @param block Pointer to the message wich should be hashed.
 * @param length is the length of only THIS block in BITS not in bytes!
 *  bits are big endian, meaning high bits come first.
 *      if you have a message with bits at the end, the byte must be padded with
 * zeros
 */

/*************************************************************************/

/*
 * length in bits!
 */
void sha256(void *dest, const void *msg,
            uint32_t length_b) { /* length could be choosen longer but this is
                                    for */
  static sha256_ctx_t s;
  sha256_init(&s);
  while (length_b >= SHA256_BLOCK_BITS) {
    sha256_nextBlock(&s, msg);
    msg = (uint8_t *)msg + SHA256_BLOCK_BITS / 8;
    length_b -= SHA256_BLOCK_BITS;
  }
  sha256_lastBlock(&s, msg, length_b);
  sha256_ctx2hash(dest, &s);
}

/*************************************************************************/

void sha256_ctx2hash(void *dest, const sha256_ctx_t *state) {
#ifdef LITTLE_ENDIAN
  uint8_t i, j, *s = (uint8_t *)(state->h);
  i = 8;
  do {
    j = 3;
    do {
      *((uint8_t *)dest) = s[j];
      dest = (uint8_t *)dest + 1;
    } while (j--);
    s += 4;
  } while (--i);
#else
  memcpy(dest, state->h, 32);
#endif
}

void sha256(void *dest, const void *msg, uint32_t length_b);
void sha256_ctx2hash(void *dest, const sha256_ctx_t *state);
void sha256_lastBlock(sha256_ctx_t *state, const void *block,
                      uint16_t length_b);
void sha256_init(sha256_ctx_t *state);

void compute_hash(const unsigned char *midstate, unsigned int mrkle_root,
                  unsigned int timestamp, unsigned int difficulty,
                  unsigned int winner_nonce, unsigned char *hash) {
  static unsigned char data[64];
  static unsigned char hash1[32];
  unsigned int *data32 = (unsigned int *)data;
  data32[0] = (mrkle_root);
  data32[1] = (timestamp);
  data32[2] = (difficulty);
  data32[3] = (winner_nonce);
  sha256_ctx_t s;
  sha256_init(&s);
  memcpy(s.h, midstate, 8 * 4);

  s.length = 1;
  sha256_lastBlock(&s, data, 16 * 8);
  sha256_ctx2hash(hash1, &s);

  sha256(hash, hash1, 256);
}

int get_leading_zeroes(const unsigned char *hash) {
  int i = 0;
  int leading = 0;
  for (i = 0; i < 32 * 8; i++) {
    unsigned char c = hash[31 - i];
    // printf("%x-",c);
    if (c & 0x80) {
      leading += 0;
      break;
    }
    if (c & 0x40) {
      leading += 1;
      break;
    }
    if (c & 0x20) {
      leading += 2;
      break;
    }
    if (c & 0x10) {
      leading += 3;
      break;
    }

    if (c & 0x08) {
      leading += 4;
      break;
    }
    if (c & 0x04) {
      leading += 5;
      break;
    }
    if (c & 0x02) {
      leading += 6;
      break;
    }
    if (c & 0x01) {
      leading += 7;
      break;
    }

    leading += 8;
  }
  return leading;
}
