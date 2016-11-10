//  store.c
//  hosts2zones
//
//  Created by Dr. Rolf Jansen on 2014-08-01.
//  Copyright (c) 2014 projectworld.net. All rights reserved.
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
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <tmmintrin.h>
#include <sys/stat.h>

#include "binutils.h"
#include "store.h"


#pragma mark ••• Value Data householding •••

static inline void releaseValue(Value *value)
{
   switch (-value->kind)   // dynamic data, has to be released
   {
      case Simple:
      case Data:
         deallocate(VPR(value->p), false);
         break;

      case String:
         deallocate(VPR(value->s), false);
         break;

      case Dictionary:
         releaseTable((Node **)value->p);
         break;
   }
}


#pragma mark ••• AVL Tree •••

static int balanceNode(Node **node)
{
   int   change = 0;
   Node *o = *node;
   Node *p, *q;

   if (o->B == -2)
   {
      if (p = o->L)                    // make the static analyzer happy
         if (p->B == +1)
         {
            change = 1;                // double left-right rotation
            q      = p->R;             // left rotation
            p->R   = q->L;
            q->L   = p;
            o->L   = q->R;             // right rotation
            q->R   = o;
            o->B   = +(q->B < 0);
            p->B   = -(q->B > 0);
            q->B   = 0;
            *node  = q;
         }

         else
         {
            change = p->B;             // single right rotation
            o->L   = p->R;
            p->R   = o;
            o->B   = -(++p->B);
            *node  = p;
         }
   }

   else if (o->B == +2)
   {
      if (q = o->R)                    // make the static analyzer happy
         if (q->B == -1)
         {
            change = 1;                // double right-left rotation
            p      = q->L;             // right rotation
            q->L   = p->R;
            p->R   = q;
            o->R   = p->L;             // left rotation
            p->L   = o;
            o->B   = -(p->B > 0);
            q->B   = +(p->B < 0);
            p->B   = 0;
            *node  = p;
         }

         else
         {
            change = q->B;             // single left rotation
            o->R   = q->L;
            q->L   = o;
            o->B   = -(--q->B);
            *node  = q;
         }
   }

   return change != 0;
}


static int pickPrevNode(Node **node, Node **exch)
{                                       // *exch on entry = parent node
   Node *o = *node;                     // *exch on exit  = picked previous value node

   if (o->R)
   {
      *exch = o;
      int change = -pickPrevNode(&o->R, exch);
      if (change)
         if (abs(o->B += change) > 1)
            return balanceNode(node);
         else
            return o->B == 0;
      else
         return 0;
   }

   else if (o->L)
   {
      Node *p = o->L;
      o->L = NULL;
      (*exch)->R = p;
      *exch = o;
      return p->B == 0;
   }

   else
   {
      (*exch)->R = NULL;
      *exch = o;
      return 1;
   }
}


static int pickNextNode(Node **node, Node **exch)
{                                       // *exch on entry = parent node
   Node *o = *node;                     // *exch on exit  = picked next value node

   if (o->L)
   {
      *exch = o;
      int change = +pickNextNode(&o->L, exch);
      if (change)
         if (abs(o->B += change) > 1)
            return balanceNode(node);
         else
            return o->B == 0;
      else
         return 0;
   }

   else if (o->R)
   {
      Node *q = o->R;
      o->R = NULL;
      (*exch)->L = q;
      *exch = o;
      return q->B == 0;
   }

   else
   {
      (*exch)->L = NULL;
      *exch = o;
      return 1;
   }
}


// CAUTION: The following recursive functions must not be called with name == NULL.
//          For performace reasons no extra error cheking is done.

Node *findTreeNode(const char *name, Node *node)
{
   if (node)
   {
      int ord = strcmp(name, node->name);

      if (ord == 0)
         return node;

      else if (ord < 0)
         return findTreeNode(name, node->L);

      else // (ord > 0)
         return findTreeNode(name, node->R);
   }

   else
      return NULL;
}

int addTreeNode(const char *name, ssize_t naml, Value *value, Node **node, Node **passed)
{
   static const Value zeros = {0, {.i = 0}};

   Node *o = *node;

   if (o == NULL)                         // if the hash/name is not in the tree
   {                                      // then add it into a new leaf
      if (o = allocate(sizeof(Node), true))
         if (o->name = allocate(naml+1, false))
         {
            strcpy(o->name, name);
            o->naml = naml;
            if (value)
               o->value = *value;
            *node = *passed = o;          // report back the new node into which the new value has been entered
            return 1;                     // add the weight of 1 leaf onto the balance
         }
         else
            deallocate(VPR(o), false);

      *passed = NULL;
      return 0;                           // Out of Memory situation, nothing changed
   }

   else
   {
      int change;
      int ord = strcmp(name, o->name);

      if (ord == 0)                       // if the name is already in the tree then
      {
         releaseValue(&o->value);         // release the old value - if kind is empty then releaseValue() does nothing
         o->value = (value) ? *value      // either store the new value or
                            :  zeros;     // zero-out the value struct
         *passed = o;                     // report back the node in which the value was modified
         return 0;
      }

      else if (ord < 0)
         change = -addTreeNode(name, naml, value, &o->L, passed);

      else // (ord > 0)
         change = +addTreeNode(name, naml, value, &o->R, passed);

      if (change)
         if (abs(o->B += change) > 1)
            return 1 - balanceNode(node);
         else
            return o->B != 0;
      else
         return 0;
   }
}

int removeTreeNode(const char *name, Node **node)
{
   Node *o = *node;

   if (o == NULL)
      return 0;                              // not found -> recursively do nothing

   else
   {
      int change;
      int ord = strcmp(name, o->name);

      if (ord == 0)
      {
         int    b = o->B;
         Node  *p = o->L;
         Node  *q = o->R;

         if (!p || !q)
         {
            releaseValue(&(*node)->value);
            deallocate_batch(false, VPR((*node)->name),
                                    VPR(*node), NULL);
            *node = (p > q) ? p : q;
            return 1;                        // remove the weight of 1 leaf from the balance
         }

         else
         {
            if (b == -1)
            {
               if (!p->R)
               {
                  change = +1;
                  o      =  p;
                  o->R   =  q;
               }
               else
               {
                  change = +pickPrevNode(&p, &o);
                  o->L   =  p;
                  o->R   =  q;
               }
            }

            else
            {
               if (!q->L)
               {
                  change = -1;
                  o      =  q;
                  o->L   =  p;
               }
               else
               {
                  change = -pickNextNode(&q, &o);
                  o->L   =  p;
                  o->R   =  q;
               }
            }

            o->B = b;
            releaseValue(&(*node)->value);
            deallocate_batch(false, VPR((*node)->name),
                                    VPR(*node), NULL);
            *node = o;
         }
      }

      else if (ord < 0)
         change = +removeTreeNode(name, &o->L);

      else // (ord > 0)
         change = -removeTreeNode(name, &o->R);

      if (change)
         if (abs(o->B += change) > 1)
            return balanceNode(node);
         else
            return o->B == 0;
      else
         return 0;
   }
}

void releaseTree(Node *node)
{
   if (node)
   {
      if (node->L)
         releaseTree(node->L);
      if (node->R)
         releaseTree(node->R);

      releaseValue(&node->value);
      deallocate_batch(false, VPR(node->name),
                              VPR(node), NULL);
   }
}


#pragma mark ••• Hash Table •••

// Essence of MurmurHash3_x86_32()
//
//  Original at: https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
//
//  Quote from the Source:
//  "MurmurHash3 was written by Austin Appleby, and is placed in the public
//   domain. The author hereby disclaims copyright to this source code."
//
// Many thanks to Austin!

static inline uint mmh3(const char *name, ssize_t naml)
{
   int    i, n   = (int)(naml/4);
   uint   k1, h1 = (uint)naml;    // quite tiny (0.2 %) better distribution by seeding the name length
   uint  *quads  = (uint *)(name + n*4);
   uchar *tail   = (uchar *)quads;

   for (i = -n; i; i++)
   {
      k1  = quads[i];
      k1 *= 0xCC9E2D51;
      k1  = (k1<<15)|(k1>>17);
      k1 *= 0x1B873593;

      h1 ^= k1;
      h1  = (h1<<13)|(h1>>19);
      h1  = h1*5 + 0xE6546B64;
   }

   k1 = 0;
   switch (naml & 3)
   {
      case 3: k1 ^= (uint)(tail[2] << 16);
      case 2: k1 ^= (uint)(tail[1] << 8);
      case 1: k1 ^= (uint)(tail[0]);
         k1 *= 0xCC9E2D51;
         k1  = (k1<<15)|(k1>>17);
         k1 *= 0x1B873593;
         h1 ^= k1;
   };

   h1 ^= naml;
   h1 ^= h1 >> 16;
   h1 *= 0x85EBCA6B;
   h1 ^= h1 >> 13;
   h1 *= 0xC2B2AE35;
   h1 ^= h1 >> 16;

   return h1;
}


// Table creation and release
Node **createTable(uint n)
{
   Node **table = allocate((n+1)*sizeof(Node *), true);
   if (table)
      *(uint *)table = n;
   return table;
}

void releaseTable(Node *table[])
{
   if (table)
   {
      uint i, n = *(uint *)table;
      for (i = 1; i <= n; i++)
         releaseTree(table[i]);
      deallocate(VPR(table), false);
   }
}


// Storing and retrieving values by name
Node *findName(Node *table[], const char *name, ssize_t naml)
{
   if (name && *name)
   {
      if (naml < 0)
         naml = strvlen(name);
      uint  n = *(uint *)table;
      return findTreeNode(name, table[mmh3(name, naml)%n + 1]);
   }
   else
      return NULL;
}

Node *storeName(Node *table[], const char *name, ssize_t naml, Value *value)
{
   if (name && *name)
   {
      if (naml < 0)
         naml = strvlen(name);
      uint  n = *(uint *)table;
      Node *passed;
      addTreeNode(name, naml, value, &table[mmh3(name, naml)%n + 1], &passed);
      return passed;
   }
   else
      return NULL;
}

void removeName(Node *table[], const char *name, ssize_t naml)
{
   if (name && *name)
   {
      if (naml < 0)
         naml = strvlen(name);
      uint  tidx = mmh3(name, naml) % *(uint*)table + 1;
      Node *node = table[tidx];
      if (node)
         if (!node->L && !node->R)
         {
            releaseValue(&node->value);
            deallocate_batch(false, VPR(node->name),
                                    VPR(table[tidx]), NULL);
         }
         else
            removeTreeNode(name, &table[tidx]);
   }
}
