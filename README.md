## void-zones-tools for FreeBSD
### Prepare a list of void zones that can be readily feed into *Unbound*.

### What is a void zone?

In terms of *Unbound*, a void zone is a static empty local-zone, i.e. one without any local-data directives, for example:
    
    local-zone: "void.example.com" static
    
With this in place, *Unbound* would answer any DNS requests for `void.example.com` with `NXDOMAIN`.


### For what are void zones useful?

Void zones are the most straightforward way of blocking ad, tracking and other malware domains.
While the method is similar to the *Hosts* file approach, the void zone method got 3 advantages:

1. Requests for any subdomain of the void zone result in `NXDOMAIN`.
   In *Hosts* files every subdomain needs its own entry.

2. In *Hosts* files, each entry does consist of an IP address and a fully qualified domain,
   so DNS requests for a given domain would return the listed IP address. For blocking
   purposes either `127.0.0.1` or `0.0.0.0` is used, which may result in undesired side effects:
   
   - in the case of `127.0.0.1`, developers won't be able to run a test web-server on localhost,
     and any software is supposed to try to establish a connection to localhost, and this 
     usually ends up in a timeout error. 
   - in the case of `0.0.0.0` ill-behaved software still tries to establish a connection, which
      cannot work, but resources are utilized until timeout.
   
   The void zone response is `NXDOMAIN`, and no software would try to establish a connection
   to a non-existing address.

3. The void zone method can be deployed on the gateway, and any number of clients behind the
   gateway do benefit automatically from ad, tracking and malware domain blocking. While *Hosts*
   files need to be deployed on any single machine. Even if this is not that difficult on
   desktops and notebooks, this may easily become a PITA for mobile clients. You would need
   `root` login on your Androids, iPhones and/or iPads.


### How does this compare to Browser Plugins?

1. Browser plugins are destined to one piece of software and not to the whole machine.
   Void zones are active for the whole machine or in the case of a gateway, for any
   number of clients, and even for those (Android) which don't allow ad-blocking plugins.

2. Browser plugins are active filters, that means, beside the advertised behaviour, they
   are able to do something in the background. This is a matter of trust, which may
   sometimes miserably trapped -- see the WoT incident. Void zones are passive. The actual
   filtering is done by the DNS resolver, here *Unbound*, which is much less likely of doing
   undesired things behind your back.


### How do I deploy the void zone method on my FreeBSD machine?

On the FreeBSD machine, either clone the present `void-zones-tools` project using *Git* or check it out with *Subversion*:

    # git clone https://github.com/cyclaero/void-zones-tools.git

or

    # svn checkout https://github.com/cyclaero/void-zones-tools.git/trunk/ void-zones-tools
    
Enter the directory of the working copy of the `void-zones-tools` and build & install the tools:

    # cd void-zones-tools
    # make install clean CDEFS="-march=native"
    
The tools consist of the *Hosts* file converter & consolidator `hosts2zones` and the shell script
`void-zones-update.sh` which is used to download suitable *Hosts* files from 6 pre-defined providers:

* [PGL - Ad blocking with ad server hostnames and IP addresses](http://pgl.yoyo.org/adservers/)
* [SomeOneWhoCares - How to make the internet not suck (as much)](http://someonewhocares.org/hosts/zero/)
* [MVPS - A detailed guide for using the MVPS HOSTS file](http://winhelp2002.mvps.org/)
* [MDL - Malware Domain List](http://www.malwaredomainlist.com/)
* [AdAway - Hosts](https://github.com/AdAway/AdAway/)
* [FadeMind - UncheckyAds & Telemetry](https://github.com/FadeMind/hosts.extras/)
* [DNS-BH – Malware Domain Blocklist](http://www.malwaredomains.com/)


The tools are placed by the above command sequence into `/usr/local/bin/`.

On the first run of `void-zones-update.sh`, a directory is created at `/usr/local/etc/void-zones/`,
which serves as the storage location for the downloaded *Hosts* files and/or *Domain* listings. In
addition a template for a custom white/black list `my_void_hosts.txt` is placed into that directory,
and this may be used for whitelisting some zones that are inadvertantly part of the downloaded *Hosts* files,
or for blacklisting addtional zones, which are missing from the downloads. Now execute said shell script:

    # void-zones-update.sh
    # nano /usr/local/etc/void-zones/my_void_hosts.txt
    
    # white list
    1.1.1.1 my.white.dom

    # black list
    0.0.0.0 my.black.dom

For whitelisting use the IP address `1.1.1.1`, and for blacklisting `0.0.0.0` shall be used.

The downloaded *Hosts* files are  placed into `/usr/local/etc/void-zones/` as well:

    # ls -l /usr/local/etc/void-zones

    total 1310
    -rw-r--r--  1 root  wheel   13783 Nov  1 10:16 away_void_hosts.txt
    -rw-r--r--  1 root  wheel  249280 Nov 11 20:47 jdom_void_list.txt
    -rw-r--r--  1 root  wheel   40097 Nov 11 04:02 mdl_void_hosts.txt
    -rw-r--r--  1 root  wheel  502430 Oct 20 17:09 mvps_void_hosts.txt
    -rw-r--r--  1 root  wheel    1157 Nov 13 18:43 my_void_hosts.txt
    -rw-r--r--  1 root  wheel   58060 Oct 14 10:39 pgl_void_hosts.txt
    -rw-r--r--  1 root  wheel  359132 Nov 12 15:50 sowc_void_hosts.txt
    -rw-r--r--  1 root  wheel    4024 Nov 13 20:16 telm_void_hosts.txt
    -rw-r--r--  1 root  wheel    1124 Nov 13 20:16 ucky_void_host.txt

And finally the `void-zones-update.sh` compiles (consolidates/converts) all *Hosts* files
into one single `local-void.zones` include file, and moves this into `/var/unbound/` for
direct usage with *Unbound*:

    # head /var/unbound/local-void.zones
    
    local-zone: "clk.cloudyisland.com" static
    local-zone: "click.silvercash.com" static
    local-zone: "oascentral.pressdemocrat.com" static
    local-zone: "s29.cnzz.com" static
    local-zone: "www.spywarespy.com" static
    local-zone: "republika.onet.pl" static
    local-zone: "preview.msn.com" static
    local-zone: "pos.baidu.com" static
    ...

For using the void zones method, of course *Unbound* must be up and running already on the given FreeBSD machine.
Then edit the configuration file `/var/unbound/unbound.conf` in order to activate ad, tracking, malware and telemetry domain
filtering by *Unbound*:

    # nano /var/unbound/unbound.conf

**Before** any forwarder directives, e.g. `forward-zone:` or `include: /var/unbound/forward.conf` add the following line:

    include: /var/unbound/local-void.zones

Then restart *Unbound*:

    # service local_unbound restart

For future updates execute the following command sequence which may be placed into a cron job:

    # /usr/local/bin/void-zones-update.sh && /usr/sbin/service local_unbound restart
