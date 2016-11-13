//  binutils.h
//  hosts2zones
//
//  Created by Dr. Rolf Jansen on 2016-08-13.
//  Copyright (c) 2016 projectworld.net. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
//  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
//  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
//  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
//  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
//  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#define noerr 0

typedef unsigned char uchar;
typedef unsigned int  uint;
typedef long long     llong;


#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

   #define b2_0 0
   #define b2_1 1

   #define b4_0 0
   #define b4_1 1
   #define b4_2 2
   #define b4_3 3

   #define b8_0 0
   #define b8_1 1
   #define b8_2 2
   #define b8_3 3
   #define b8_4 4
   #define b8_5 5
   #define b8_6 6
   #define b8_7 7

   #define swapInt16(x) _swapInt16(x)
   #define swapInt32(x) _swapInt32(x)
   #define swapInt64(x) _swapInt64(x)

   #if defined(__i386__) || defined(__x86_64__)

      static inline uint16_t _swapInt16(uint16_t x)
      {
         __asm__("rolw $8,%0" : "+q" (x));
         return x;
      }

      static inline uint32_t _swapInt32(uint32_t x)
      {
         __asm__("bswapl %0" : "+q" (x));
         return x;
      }

   #else

      static inline uint16_t _swapInt16(uint16_t x)
      {
         uint16_t z;
         uint8_t *p = (uint8_t *)&x;
         uint8_t *q = (uint8_t *)&z;
         q[0] = p[1];
         q[1] = p[0];
         return z;
      }

      static inline uint32_t _swapInt32(uint32_t x)
      {
         uint32_t z;
         uint8_t *p = (uint8_t *)&x;
         uint8_t *q = (uint8_t *)&z;
         q[0] = p[3];
         q[1] = p[2];
         q[2] = p[1];
         q[3] = p[0];
         return z;
      }

   #endif

   #if defined(__x86_64__)

      static inline uint64_t _swapInt64(uint64_t x)
      {
         __asm__("bswapq %0" : "+q" (x));
         return x;
      }

   #else

      static inline uint64_t _swapInt64(uint64_t x)
      {
         uint64_t z;
         uint8_t *p = (uint8_t *)&x;
         uint8_t *q = (uint8_t *)&z;
         q[0] = p[7];
         q[1] = p[6];
         q[2] = p[5];
         q[3] = p[4];
         q[4] = p[3];
         q[5] = p[2];
         q[6] = p[1];
         q[7] = p[0];
         return z;
      }

   #endif

#else

   #define b2_0 1
   #define b2_1 0

   #define b4_0 3
   #define b4_1 2
   #define b4_2 1
   #define b4_3 0

   #define b8_0 7
   #define b8_1 6
   #define b8_2 5
   #define b8_3 4
   #define b8_4 3
   #define b8_5 2
   #define b8_6 1
   #define b8_7 0

   #define swapInt16(x) (x)
   #define swapInt32(x) (x)
   #define swapInt64(x) (x)

#endif


#if defined(__x86_64__)

   #include <x86intrin.h>

   static const __m128i nul16 = {0x0000000000000000ULL, 0x0000000000000000ULL};  // 16 bytes with nul
   static const __m128i lfd16 = {0x0A0A0A0A0A0A0A0AULL, 0x0A0A0A0A0A0A0A0AULL};  // 16 bytes with line feed
   static const __m128i col16 = {0x3A3A3A3A3A3A3A3AULL, 0x3A3A3A3A3A3A3A3AULL};  // 16 bytes with colon ':' limit
   static const __m128i vtl16 = {0x7C7C7C7C7C7C7C7CULL, 0x7C7C7C7C7C7C7C7CULL};  // 16 bytes with vertical line '|' limit
   static const __m128i blk16 = {0x2020202020202020ULL, 0x2020202020202020ULL};  // 16 bytes with inner blank limit
   static const __m128i obl16 = {0x2121212121212121ULL, 0x2121212121212121ULL};  // 16 bytes with outer blank limit
   static const __m128i dot16 = {0x2E2E2E2E2E2E2E2EULL, 0x2E2E2E2E2E2E2E2EULL};  // 16 bytes with dots '.'

   // Drop-in replacement for strlen(), utilizing some builtin SSSE3 instructions
   static inline int strvlen(const char *str)
   {
      if (!str || !*str)
         return 0;

      unsigned bmask;
      if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_loadu_si128((__m128i *)str), nul16)))
         return __builtin_ctz(bmask);

      for (int len = 16 - (intptr_t)str%16;; len += 16)
         if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&str[len]), nul16)))
            return len + __builtin_ctz(bmask);
   }

   static inline int linelen(const char *line)
   {
      if (!line || !*line)
         return 0;

      unsigned bmask;
      if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_loadu_si128((__m128i *)line), nul16))
                | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_loadu_si128((__m128i *)line), lfd16)))
         return __builtin_ctz(bmask);

      for (int len = 16 - (intptr_t)line%16;; len += 16)
         if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&line[len]), nul16))
                   | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&line[len]), lfd16)))
            return len + __builtin_ctz(bmask);
   }

   static inline int taglen(const char *tag)
   {
      if (!tag || !*tag)
         return 0;

      unsigned bmask;
      if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_loadu_si128((__m128i *)tag), nul16))
                | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_loadu_si128((__m128i *)tag), col16)))
         return __builtin_ctz(bmask);

      for (int len = 16 - (intptr_t)tag%16;; len += 16)
         if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&tag[len]), nul16))
                   | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&tag[len]), col16)))
            return len + __builtin_ctz(bmask);
   }

   static inline int fieldlen(const char *field)
   {
      if (!field || !*field)
         return 0;

      unsigned bmask;
      if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_loadu_si128((__m128i *)field), nul16))
                | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_loadu_si128((__m128i *)field), vtl16)))
         return __builtin_ctz(bmask);

      for (int len = 16 - (intptr_t)field%16;; len += 16)
         if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&field[len]), nul16))
                   | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&field[len]), vtl16)))
            return len + __builtin_ctz(bmask);
   }

   static inline int domainlen(const char *domain)
   {
      if (!domain || !*domain)
         return 0;

      unsigned bmask;
      if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_loadu_si128((__m128i *)domain), nul16))
                | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_loadu_si128((__m128i *)domain), dot16)))
         return __builtin_ctz(bmask);

      for (int len = 16 - (intptr_t)domain%16;; len += 16)
         if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&domain[len]), nul16))
                   | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&domain[len]), dot16)))
            return len + __builtin_ctz(bmask);
   }

   static inline int wordlen(const char *word)
   {
      if (!word || !*word)
         return 0;

      unsigned bmask;
      if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(blk16, _mm_max_epu8(blk16, _mm_loadu_si128((__m128i *)word)))))
         return __builtin_ctz(bmask);      // ^^^^^^^ unsigned comparison (a >= b) is identical to a == maxu(a, b) ^^^^^^^

      for (int len = 16 - (intptr_t)word%16;; len += 16)
         if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(blk16, _mm_max_epu8(blk16, _mm_load_si128((__m128i *)&word[len])))))
            return len + __builtin_ctz(bmask);
   }

   static inline int blanklen(const char *blank)
   {
      if (!blank || !*blank)
         return 0;

      unsigned bmask;
      if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_loadu_si128((__m128i *)blank), nul16))
                | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(obl16, _mm_min_epu8(obl16, _mm_loadu_si128((__m128i *)blank)))))
         return __builtin_ctz(bmask);      // ^^^^^^^ unsigned comparison (a <= b) is identical to a == minu(a, b) ^^^^^^^

      for (int len = 16 - (intptr_t)blank%16;; len += 16)
         if (bmask = (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((__m128i *)&blank[len]), nul16))
                   | (unsigned)_mm_movemask_epi8(_mm_cmpeq_epi8(obl16, _mm_min_epu8(obl16, _mm_load_si128((__m128i *)&blank[len])))))
            return len + __builtin_ctz(bmask);
   }


   // String copying from src to dst.
   // m: Max. capacity of dst, including the final nul.
   //    A value of 0 would indicate that the capacity of dst matches the size of src (including nul)
   // l: On entry, src length or 0, on exit, the length of src, maybe NULL
   // Returns the length of the resulting string in dst.
   static inline int strmlcpy(char *dst, const char *src, int m, int *l)
   {
      int k, n;

      if (l)
      {
         if (!*l)
            *l = strvlen(src);
         k = *l;
      }
      else
         k = strvlen(src);

      if (!m)
         n = k;
      else
         n = (k < m) ? k : m-1;

      switch (n)
      {
         default:
            if ((intptr_t)dst&0xF || (intptr_t)src&0xF)
               for (k = 0; k  < n>>4<<1; k += 2)
                  ((uint64_t *)dst)[k] = ((uint64_t *)src)[k], ((uint64_t *)dst)[k+1] = ((uint64_t *)src)[k+1];
            else
               for (k = 0; k  < n>>4; k++)
                  _mm_store_si128(&((__m128i *)dst)[k], _mm_load_si128(&((__m128i *)src)[k]));
         case 8 ... 15:
            if ((k = n>>4<<1) < n>>3)
               ((uint64_t *)dst)[k] = ((uint64_t *)src)[k];
         case 4 ... 7:
            if ((k = n>>3<<1) < n>>2)
               ((uint32_t *)dst)[k] = ((uint32_t *)src)[k];
         case 2 ... 3:
            if ((k = n>>2<<1) < n>>1)
               ((uint16_t *)dst)[k] = ((uint16_t *)src)[k];
         case 1:
            if ((k = n>>1<<1) < n)
               dst[k] = src[k];
         case 0:
            ;
      }

      dst[n] = '\0';
      return n;
   }

#else

   #define strvlen(s) strlen(s)

   static inline int linelen(const char *line)
   {
      if (!line || !*line)
         return 0;

      int l;
      for (l = 0; line[l] && line[l] != '\n'; l++)
         ;
      return l;
   }

   static inline int taglen(const char *tag)
   {
      if (!tag || !*tag)
         return 0;

      int l;
      for (l = 0; tag[l] && tag[l] != ':'; l++)
         ;
      return l;
   }

   static inline int fieldlen(const char *field)
   {
      if (!field || !*field)
         return 0;

      int l;
      for (l = 0; field[l] && field[l] != '|'; l++)
         ;
      return l;
   }

   static inline int domainlen(const char *domain)
   {
      if (!domain || !*domain)
         return 0;

      int l;
      for (l = 0; domain[l] && domain[l] != '|'; l++)
         ;
      return l;
   }

   static inline int wordlen(const char *word)
   {
      if (!word || !*word)
         return 0;

      int l;
      for (l = 0; (uchar)word[l] > ' '; l++)
         ;
      return l;
   }

   static inline int blanklen(const char *blank)
   {
      if (!blank || !*blank)
         return 0;

      int l;
      for (l = 0; blank[l] && (uchar)blank[l] <= ' '; l++)
         ;
      return l;
   }


   // String copying from src to dst.
   // m: Max. capacity of dst, including the final nul.
   //    A value of 0 would indicate that the capacity of dst matches the size of src (including nul)
   // l: On entry, src length or 0, on exit, the length of src, maybe NULL
   // Returns the length of the resulting string in dst.
   static inline int strmlcpy(char *dst, const char *src, int m, int *l)
   {
      int k, n;

      if (l)
      {
         if (!*l)
            *l = (int)strlen(src);
         k = *l;
      }
      else
         k = (int)strlen(src);

      if (!m)
         n = k;
      else
         n = (k < m) ? k : m-1;

      strlcpy(dst, src, m);
      return n;
   }

#endif


// forward skip white space  !!! s MUST NOT be NULL !!!
static inline char *skip(char *s)
{
   for (;;)
      if ((uchar)*s <= ' ')
         s++;
      else
         return s;
}

// backward skip white space  !!! s MUST NOT be NULL !!!
static inline char *bskip(char *s)
{
   for (;;)
      if ((uchar)*--s > ' ')
         return s+1;
}

static inline char *trim(char *s)
{
   *bskip(s+strvlen(s)) = '\0';
   return skip(s);
}


static inline char *lowercase(char *s, int n)
{
   if (s)
   {
      char c, *p = s;
      for (int i = 0; i < n && (c = *p); i++)
         if ('A' <= c && c <= 'Z')
            *p++ = c + 0x20;
         else
            p++;
   }
   return s;
}

static inline char *uppercase(char *s, int n)
{
   if (s)
   {
      char c, *p = s;
      for (int i = 0; i < n && (c = *p); i++)
         if ('a' <= c && c <= 'z')
            *p++ = c - 0x20;
         else
            p++;
   }
   return s;
}


#pragma mark ••• Fencing Memory Allocation Wrappers •••

// void pointer reference
#define VPR(p) (void **)&(p)

typedef struct
{
   ssize_t size;
   size_t  check;
   char    payload[16];
// size_t  zerowall;       // the allocation routines allocate sizeof(size_t) extra space and set this to zero
} allocation;

#define allocationMetaSize (offsetof(allocation, payload) - offsetof(allocation, size))

extern ssize_t gAllocationTotal;

void *allocate(ssize_t size, bool cleanout);
void *reallocate(void *p, ssize_t size, bool cleanout, bool free_on_error);
void deallocate(void **p, bool cleanout);
void deallocate_batch(bool cleanout, ...);
