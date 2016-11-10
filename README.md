## void-zones-tools for FreeBSD
### Prepare a list of void zones that can be readily feed into unbound.

### What is a void zone?

In terms of Unbound, a void zone is a static empty local-zone, i.e. one without any local-data directives, for example:
    
    local-zone: "void.example.com" static
    
With this in place, Unbound would answer any DNS requests for `void.example.com` with `NXDOMAIN`.


### For what are void zones useful?

Void zones are the most straightforward way of blocking ad, tracking and other malware domains.
While the method is similar to the host files approach, the void zone method has 3 advantages:

1. Requests for any subdomain of the void zone result in `NXDOMAIN`.
   In hosts files any subdomain needs ints own entry.

2. In hosts files, each entry does consist of a fully qualified domain and an IP address,
   and DNS requests for a given domain would return the listed IP address. For blocking
   purposes either 127.0.0.1 or 0.0.0.0 is used, which may result in undesired sied effects:
   
   - in the case of 127.0.0.1, developers won't be able to run a test web-server on localhost
   - in the case of 0.0.0.0 ill-behaved software still trie establish a connection, which
      cannot work, but resources are utilized until timeout.
   
   The void zone response is `NXDOMAIN`, and no software would try to establish a connection
   to a non-existing address.

3. The void zone method can be deployed on the gateway, and any number of clients behind the gateway
   gateway do benefit automatically from ad, tracking and malware domain blocking. Hosts files need to
   be deployed on the client. While this is not that difficult on desktop and notebook machines, this
   would easily become a PITA for mobile clients. You would need `root` login on your Androids, iPhone
   or iPad.


### How does this compare to Browser plugins?

1. Browser plugins are destined to one piece of software and not to the whole machine.
   Void zone are active for the whole machine or if deployed on a gateway, even for
   thousands of clients, even for those (Android) which don't allow ad-blocking plugins.

2. Browser plugins are active filters, that means beside the advertised behaviour, they
   are able to do something in the background. This is a matter of trust, which may
   sometimes miserably trapped -- see the WoT incident. Void zones are passive. The actual
   filtering is done by the DNS resolver, here Unbound, which is much less likely of doing
   undesired things behind your back.


### How do I deploy the void zone method on my FreeBSD machine?

