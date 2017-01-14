//  binutils.c
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


#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>

#include "binutils.h"

#pragma mark ••• Fencing Memory Allocation Wrappers •••

ssize_t gAllocationTotal = 0;

static inline void countAllocation(ssize_t size)
{
   if (__sync_add_and_fetch(&gAllocationTotal, size) < 0)
   {
      syslog(LOG_ERR, "Corruption of allocated memory detected by countAllocation().");
      exit(EXIT_FAILURE);
   }
}

void *allocate(ssize_t size, bool cleanout)
{
   if (size >= 0)
   {
      allocation *a;

      if (cleanout)
      {
         if ((a = calloc(1, allocationMetaSize + size + sizeof(size_t))) == NULL)
            return NULL;
      }

      else
      {
         if ((a = malloc(allocationMetaSize + size + sizeof(size_t))) == NULL)
            return NULL;

         *(size_t *)((void *)a + allocationMetaSize + size) = 0;   // place a (size_t)0 just above the payload as the upper boundary of the allocation
      }

      countAllocation(size);
      a->size  = size;
      a->check = size | (size_t)a;
      return &a->payload;
   }
   else
      return NULL;
}

void *reallocate(void *p, ssize_t size, bool cleanout, bool free_on_error)
{
   if (size >= 0)
      if (p)
      {
         allocation *a = p - allocationMetaSize;

         if (a->check == (a->size | (size_t)a) && *(ssize_t *)((void *)a + allocationMetaSize + a->size) == 0)
            a->check = 0;
         else
         {
            syslog(LOG_ERR, "Corruption of allocated memory detected by reallocate().");
            exit(EXIT_FAILURE);
         }

         if ((p = realloc(a, allocationMetaSize + size + sizeof(size_t))) == NULL)
         {
            if (free_on_error)
            {
               countAllocation(-a->size);
               free(a);
            }
            return NULL;
         }
         else
            a = p;

         if (cleanout)
            bzero((void *)a + allocationMetaSize + a->size, size - a->size + sizeof(size_t));
         else
            *(ssize_t *)((void *)a + allocationMetaSize + size) = 0;   // place a (size_t)0 just above the payload as the upper boundary of the allocation

         countAllocation(size - a->size);
         a->size  = size;
         a->check = size | (size_t)a;
         return &a->payload;
      }
      else
         return allocate(size, cleanout);

   return NULL;
}

void deallocate(void **p, bool cleanout)
{
   if (p && *p)
   {
      allocation *a = *p - allocationMetaSize;
      *p = NULL;

      if (a->check == (a->size | (size_t)a) && *(ssize_t *)((void *)a + allocationMetaSize + a->size) == 0)
         a->check = 0;
      else
      {
         syslog(LOG_ERR, "Corruption of allocated memory detected by deallocate().");
         exit(EXIT_FAILURE);
      }

      countAllocation(-a->size);
      if (cleanout)
         bzero((void *)a, allocationMetaSize + a->size + sizeof(size_t));
      free(a);
   }
}

void deallocate_batch(unsigned cleanout, ...)
{
   void   **p;
   va_list  vl;
   va_start(vl, cleanout);

   while (p = va_arg(vl, void **))
      if (*p)
      {
         allocation *a = *p - allocationMetaSize;
         *p = NULL;

         if (a->check == (a->size | (size_t)a) && *(ssize_t *)((void *)a + allocationMetaSize + a->size) == 0)
            a->check = 0;
         else
         {
            syslog(LOG_ERR, "Corruption of allocated memory detected by deallocate_batch().");
            exit(EXIT_FAILURE);
         }

         countAllocation(-a->size);
         if (cleanout)
            bzero((void *)a, allocationMetaSize + a->size + sizeof(size_t));
         free(a);
      }

   va_end(vl);
}
