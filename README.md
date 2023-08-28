### [ACTION REQUIRED] Your GitHub account, cyclaero, will soon require 2FA
Here is the deal: https://obsigna.com/articles/1693258424.html

 
## void-zones-tools for FreeBSD
### Prepare a list of void zones that can be readily fed into *Unbound*.

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

The *void-zones-tools* made it into the FreeBSD ports and it can be installed from source or from the binary package repository:

    # cd /usr/ports/dns/void-zones-tools
    # make install clean
    
or

    # pkg install void-zones-tools


Of course, it is also possible to build it from the source of the present github repository. Either clone the `void-zones-tools` project using *Git* or check it out with *Subversion*:

    # git clone https://github.com/cyclaero/void-zones-tools.git

or

    # svn checkout https://github.com/cyclaero/void-zones-tools.git/trunk/ void-zones-tools
    
Enter the directory of the working copy of the `void-zones-tools` and build & install the tools:

    # cd void-zones-tools
    # make install clean
    

The tools consist of the *Hosts* file converter & consolidator `hosts2zones` and the shell script
`void-zones-update.sh` which is used to download suitable *Hosts* files from 8 pre-defined providers:

* [PGL - Ad blocking with ad server hostnames and IP addresses](http://pgl.yoyo.org/adservers/)
* [SomeOneWhoCares - How to make the internet not suck (as much)](http://someonewhocares.org/hosts/zero/)
* [MVPS - A detailed guide for using the MVPS HOSTS file](http://winhelp2002.mvps.org/)
* [AdAway - Hosts](https://github.com/AdAway/AdAway/)
* [DNS-BH – Malware Domain Blocklist](http://www.malwaredomains.com/)
* [FadeMind - UncheckyAds](https://github.com/FadeMind/hosts.extras/)
* [WindowsSpyBlocker - Spy Hosts](https://github.com/crazy-max/WindowsSpyBlocker/)


The tools are placed by the above command sequence into `/usr/local/bin/`.

On the first run of `void-zones-update.sh`, a directory is created at `/usr/local/etc/void-zones/`,
which serves as the storage location for the downloaded *Hosts* files and/or *Domain* listings. In
addition a template for a custom white/black list `my_void_hosts.txt` is placed into that directory,
and this may be used for whitelisting some zones that are inadvertently part of the downloaded *Hosts* files,
or for blacklisting additional zones, which are missing from the downloads. Now execute said shell script:

    # void-zones-update.sh
    # nano /usr/local/etc/void-zones/my_void_hosts.txt
    
    # white list
    1.1.1.1 my.white.dom

    # black list
    0.0.0.0 my.black.dom

For whitelisting use the IP address `1.1.1.1`, and for blacklisting `0.0.0.0` shall be used.

The downloaded *Hosts* files are  placed into `/usr/local/etc/void-zones/` as well:

    # ls -l /usr/local/etc/void-zones

    total 1876
    -rw-r--r--  1 root  wheel   13722 Jan 31  2017 away_void_hosts.txt
    -rw-r--r--  1 root  wheel  640858 Aug 17 19:16 jdom_void_list.txt
    -rw-r--r--  1 root  wheel  497673 Aug  7 11:07 mvps_void_hosts.txt
    -rw-r--r--  1 root  wheel   60257 Aug 21 05:43 pgl_void_hosts.txt
    -rw-r--r--  1 root  wheel  376421 Aug 20 14:40 sowc_void_hosts.txt
    -rw-r--r--  1 root  wheel     618 Aug 22 09:29 ucky_void_host.txt
    -rw-r--r--  1 root  wheel    9977 Aug 22 09:29 w10telm_void_hosts.txt
    -rw-r--r--  1 root  wheel     886 Aug 22 09:29 w7telm_void_hosts.txt
    -rw-r--r--  1 root  wheel    1142 Aug 22 09:29 w81telm_void_hosts.txt

And finally the `void-zones-update.sh` compiles (converts & consolidates) all *Hosts* files
and *Domain* listings into one single `local-void.zones` include file, and moves this into
`/var/unbound/` for direct usage with *Unbound*:

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

In order to facilitate inclusion of listings which are not part of the automated updating, 3 additional input files are
passed by `void-zones-update.sh` to the conversion tool `hosts2zones`:

    x_void_list.txt
    y_void_list.txt
    z_void_list.txt
    
This mechanism can be used to include for example the [Disconnect.me](https://github.com/chrisaljoudi/uBlock/issues/1406)
listings to the `hosts2zones` processing by executing the following command before updating the other zones:

    # fetch -o - \
            https://s3.amazonaws.com/lists.disconnect.me/simple_ad.txt \
            https://s3.amazonaws.com/lists.disconnect.me/simple_malware.txt \
            https://s3.amazonaws.com/lists.disconnect.me/simple_tracking.txt \
            https://s3.amazonaws.com/lists.disconnect.me/simple_malvertising.txt \
            > /usr/local/etc/void-zones/x_void_list.txt

Said command would place the respective lists joined together into `/usr/local/etc/void-zones/x_void_list.txt`, and on the
next run of `void-zones-update.sh` that one would be  converted & consolidated & included into the `local-void.zones` for
filtering by *Unbound*. In the case these additional files are missing, the tool simply ignores these parameters.
