#!/bin/sh

#  void-zones-update.sh
#  void-zones-tools
#
# Shell Script for updating the void zones list by downloading from pre-defined hosts file providers
#
# Created by Dr. Rolf Jansen on 2016-11-16.
# Copyright (c) 2016, projectworld.net. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


### verify the path to the fetch utility
if [ -e "/usr/bin/fetch" ]; then
   FETCH="/usr/bin/fetch"
elif [ -e "/usr/local/bin/fetch" ]; then
   FETCH="/usr/local/bin/fetch"
elif [ -e "/opt/local/bin/fetch" ]; then
   FETCH="/opt/local/bin/fetch"
else
   echo "No fetch utility can be found on the system -- Stopping."
   echo "On Mac OS X, execute either 'sudo port install fetch' or install"
   echo "fetch from source into '/usr/local/bin', and then try again."
   exit 1
fi


### Storage location of the dowloaded void hosts lists
ZONES_DIR="/usr/local/etc/void-zones"
if [ ! -d "$ZONES_DIR" ]; then
   mkdir -p "$ZONES_DIR"
   echo "# white list"          > "$ZONES_DIR/my_void_hosts.txt"
   echo "1.1.1.1 my.white.dom" >> "$ZONES_DIR/my_void_hosts.txt"
   echo ""                     >> "$ZONES_DIR/my_void_hosts.txt"
   echo "# black list"         >> "$ZONES_DIR/my_void_hosts.txt"
   echo "0.0.0.0 my.black.dom" >> "$ZONES_DIR/my_void_hosts.txt"
fi


### Updating the void zones
$FETCH -o "$ZONES_DIR/pgl_void_hosts.txt"  "http://pgl.yoyo.org/as/serverlist.php?hostformat=hosts&showintro=0&useip=0.0.0.0&mimetype=plaintext"
$FETCH -o "$ZONES_DIR/sowc_void_hosts.txt" "http://someonewhocares.org/hosts/zero/hosts"
$FETCH -o "$ZONES_DIR/mvps_void_hosts.txt" "http://winhelp2002.mvps.org/hosts.txt"

if [ ! -f "$ZONES_DIR/pgl_void_hosts.txt" ] ; then
   echo "# No hosts from pgl." > "$ZONES_DIR/pgl_void_hosts.txt"
fi

if [ ! -f "$ZONES_DIR/sowc_void_hosts.txt" ] ; then
   echo "# No hosts from sowc." > "$ZONES_DIR/sowc_void_hosts.txt"
fi

if [ ! -f "$ZONES_DIR/mvps_void_hosts.txt" ] ; then
   echo "# No hosts from mvps." > "$ZONES_DIR/mvps_void_hosts.txt"
fi

/usr/local/bin/hosts2zones /tmp/local-void.zones \
                           "$ZONES_DIR/my_void_hosts.txt" \
                           "$ZONES_DIR/pgl_void_hosts.txt" \
                           "$ZONES_DIR/sowc_void_hosts.txt" \
                           "$ZONES_DIR/mvps_void_hosts.txt" \
  && /bin/mv /tmp/local-void.zones /var/unbound/local-void.zones
