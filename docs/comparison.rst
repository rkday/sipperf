Comparison with similar projects
================================

The main goals of sipperf are to be:

* high-performance - it should be able to handle hundreds of calls per second on a single, reasonably-powered CPU core
* realistic - it should be able to emulate 
* SIP-focused - while media performance testing is important

SIPp
----

SIPp is the main other SIP performance testing tool I'm familiar with

It has a couple of disadvantages:

* it's focused on SIP dialogs rather than users - so it allows you to define a call scenario, for example (SIP INVITE through to BYE) and run that repeatedly, but not to define a mix of dialogs bound together by a user (a user sends a REGISTER, then makes some calls and sends some text messages, and it also able to answer calls)
* it's quite cluttered - it supports a lot of features which are either not directly relevant to SIP performance testing (like the ability to replay media) or quite niche (like SCTP support)

The main advantages of SIPp over sipperf are:

* it's very user-programmable - 
* it's much more mature
