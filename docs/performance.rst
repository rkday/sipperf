Performance tips
================

Here are some tips on getting the best performance out of sipperf.

DNS
---

sipperf doesn't have any built-in DNS caching, so it does DNS lookups for each call, and may be slowed down by waiting for DNS responses from the network.

Two tips for getting around this problem are:

* install dnsmasq - this adds a local DNS server that caches DNS lookups, giving faster response times
* use SIP URIs that minimise the number of DNS lookups - "sip:example.com" will need three or four DNS lookups (NAPTR, one or two SRV lookups, and an A record lookup), whereas "sip:example.com:5060;transport=tcp" will just need the A record lookup - and "sip:1.2.3.4" won't need any

Scaling to multiple cores
-------------------------

Because sipperf is a single-threaded process, it doesn't make use of multiple CPU cores. If you have multiple CPU cores and want to use them, the best way to do so is to shard your performance testing:

* split your subscriber base into N equal parts, and put each part in its own CSV file
* start N instances of sipperf, each using one CSV file, and each running 1/Nth of your load

