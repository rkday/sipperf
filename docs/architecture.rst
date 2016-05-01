Architecture
============

sipperf uses the libre SIP library to handle all its SIP work (e.g. listening for SIP messages, message parsing, maintaining the various SIP state machines and timers that handle retransmissions).

All its work is done on a single thread - the main thread running libre_main. This thread processes both incoming SIP messages and timers - before calling into libre_main, sipperf sets up various recurring timers to do work (for example, a recurring timer to start calls).

libre is relatively efficient - it uses epoll on Linux for good signaling performance, and the custom version of libre used in sipperf includes a timer heap for good timer performance (which is being contributed back into libre).

Control flow
------------

* main() does various setup, such as parsing arguments and reading the CSV file containing user details
* it then starts the user registration timer
* it then calls libre_main(0, which continues running until program exit
* periodically, the user registration timer will fire and register some users
* when all users are registered, the user registration timer starts the call timer, and stops itself
* libre takes care of re-registering users when their registrations expire - we don't need a separate timer for that
* periodically, the user registration timer will fire and set up some calls
* when the required number of calls have been set up, the call timer will start the cleanup timer and stop itself
* the cleanup timer will attempt a graceful shutdown of the libre_main event loop (e.g. waiting for all outstanding traffic to finish)
* if the cleanup timer fires too many times, it will do a hard shutdown instead
* after the cleanup timer shuts down the event loop, sipperf exits

Representing users
------------------

Statistics
----------

Statistics reporting
~~~~~~~~~~~~~~~~~~~~
