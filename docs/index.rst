.. sipperf documentation master file, created by
   sphinx-quickstart on Sun May  1 12:44:43 2016.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to sipperf's documentation!
===================================

sipperf is a performance-testing tool for SIP, a VoIP (Voice-over-IP) communication protocol.

It's free software licensed under the `3-clause BSD license <https://github.com/rkday/sipperf/blob/master/LICENSE>`_.

Quickstart
----------

Create a ``users.csv`` file in the format SIPURI,USERNAME,PASSWORD::

    sip:1234@example.com,1234@example.com,secret
    sip:1235@example.com,1235@example.com,secret

Then run ``./sipperf --target sip:localhost --rps 5 --cps 1 --max-calls 1``

You should see output similar to the following::

    2 successful registers
    0 failed registers
    0 calls initiated
    0 calls successfully set up
    0 calls failed


.. toctree::
   :maxdepth: 2
   :hidden:

   self
   options
   performance
   comparison

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: Developer Info

   architecture
