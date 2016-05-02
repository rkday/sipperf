Developing sipperf
==================

Getting the source
------------------

The sipperf source code is hosted on Github at https://github.com/rkday/sipperf.

You can check the code out by running::

    git clone --recursive https://github.com/rkday/sipperf

Building sipperf
----------------

sipperf is built with CMake::

    cmake .
    make

This produces a ``sipperf`` binary in the current directory.

Understanding the code
----------------------

For a high-level overview, see the architecture documentation (:doc:`architecture`).

After that, the `header files`_ are well-commented and should help you understand what code does what.

If something still isn't clear to you, drop me an email at rkd@rkd.me.uk!

Writing documentation
---------------------

.. _header files: https://github.com/rkday/sipperf/tree/master/include
