//  hosts2zones.c
//  hosts2zones
//
//  Created by Dr. Rolf Jansen on 2014-07-06.
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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include "binutils.h"
#include "store.h"


#define hcnt 65535

int     hosts_count = 0;
int     zones_count = 0;
FILE   *in, *out;
Node  **domainStore;


void zoneOutput(Node *node)
{
   if (node)
   {
      zoneOutput(node->L);
      zoneOutput(node->R);

      bool  ontop = true;
      int   n, level = 0;
      char *q, *p = node->name;

      for (q = p + node->naml, n = 0; ontop && q > p; q--, n++)
         if (*q && *(q-1) == '.' && (++level >= 3 || level == 2 && n > 6) && findName(domainStore, q, n))
            ontop = false;

      if (ontop && level && !node->value.b)
      {
         fprintf(out, "local-zone: \"%s\" static\n", node->name);
         zones_count++;
      }
   }
}


int main(int argc, const char *argv[])
{
   if (argc >= 3 && (out = fopen(argv[1], "w")))
   {
      domainStore = createTable(hcnt);

      int    inc;
      struct stat st;
      for (inc = 2; inc < argc; inc++)
         if (stat(argv[inc], &st) == noerr && st.st_size > 3 && (in = fopen(argv[inc], "r")))
         {
            char *hosts = allocate(st.st_size + 2, false);
            if (fread(hosts, st.st_size, 1, in) == 1)
            {
               cpy2(&hosts[st.st_size], "\n");                                         // guaranteed end of line + end of string at the end of the read-in data

               bool  iswhite;
               int   dl, ll, wl;
               char *word, *lineend, *nextword, *nextline;
               char *line = hosts + blanklen(hosts);                                   // get the start of the first line by skipping leading blanks + blank lines

               while (line < hosts + st.st_size)
               {
                  ll = linelen(line);
                  lineend   = bskip(line+ll);                                          // skip trailing blanks
                  *lineend  = '\0';                                                    // separate the entry as string

                  nextline  = line + ll+1;                                             // get the start of the next line
                  nextline += blanklen(nextline);                                      // by skipping leading blanks + blank lines

                  if (*line != '#')                                                    // skip comment lines
                  {
                     word = NULL;                                                      // assume an inacceptable entry

                     line[wl = wordlen(line)] = '\0';                                  // pickup the first entry on the line
                     if (line+wl == lineend &&                                         // accept a single domain name per line which
                         0 < (dl = domainlen(line)) && dl < wl)                        // must contain at least 1 non-leading & non-trailing dot
                        word = line, iswhite = false;                                  // simple domain lists are always black lists
                                                                                       // otherwise assume the Hosts file format
                     else if ((iswhite =  cmp8(line, "1.1.1.1")) ||                    // entries starting with 1.1.1.1 shall be white listed
                             /*isblack*/  cmp8(line, "0.0.0.0")  ||                    // 0.0.0.0 or 127.0.0.1 are black list entries
                             /*isblack*/ cmp10(line, "127.0.0.1") && (line+=2))
                        word = line + 8, word += blanklen(word);                       // skip IP address and leading blanks

                     if (word)                                                         // only process simple domain entries or entries in Hosts file format
                        while (*word && *word != '#' && word < nextline)               // Hosts files may contain more than one domain per line
                        {
                           wl = wordlen(word);
                           nextword = word + wl;
                           *nextword++ = '\0';
                           nextword += blanklen(nextword);

                           if (*lowercase(word, wl) &&
                               !cmp10(word, "localhost") &&                            // don't process 'localhost' entries
                               !findName(domainStore, word, wl))                       // if the entry does not exit in the domain store
                           {                                                           // then create a new one for the given domain name
                              Value value = {Simple, .b = iswhite};
                              storeName(domainStore, word, wl, &value);
                              hosts_count++;
                           }

                           word = nextword;
                        }
                  }

                  line = nextline;
               }
            }

            deallocate(VPR(hosts), false);
            fclose(in);
         }

      for (size_t h = 1; h <= hcnt; h++)
         zoneOutput(domainStore[h]);

      releaseTable(domainStore);
      fclose(out);

      printf("Number of Hosts: %d\nNumber of Zones: %d\n", hosts_count, zones_count);

      return (zones_count > 0) ? 0 : 1;
   }
   else
      return 1;
}
