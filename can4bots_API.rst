Copyright 2012,2013 ThinkerBlox, LLC

CAN4Bots Reference API for C, Ver 0.1
=====================================

This document describes
the reference C API for the CAN4Bots protocol.

The controlling document for CAN4bots used for this implementation is
CAN4Bots Protocol, Ver 0.3

Scope of this Document
======================

This is the controlling specification document for the reference
C-language API to the CAN4Bots protocol.


Table of Contents
=================

- `Overview`_
- `License`_
- `API`_

.. _Overview:

Overview
========

TBW

.. _License:

License
=======

This CAN4Bots API is licensed under the GPL v3 or later.

.. _API:

API
===

API information.

- `Kernel API`_
- `Module Addressed API`_

Naming convention:

- CAN4Bots API types and function names start with 'c4b\_'
- CAN4Bots kernel API types and function names start with 'c4bk\_'

.. _Kernel API:

Kernel API
----------

CAN4Bots requires a minimal real-time kernel.
The CAN4Bots library provides a simple run-to-completion multi-tasking kernel.
Other kernels can be used as long as they can provide an API with equivalent functionality.
The CAN4Bots kernel fuctions are provided via wrapper macros to simplify substituion of
other real time kernels.

.. _Global Tick API:

Global Tick API
...............

CAN4Bots Types
--------------

Referenced Types
................

CAN4Bots references the following system types: ::

    typedef unsigned char  uint8_t;
    typedef unsigned char  bool;
    typedef signed   short int16_t;
    typedef unsigned short uint16_t;
    typedef unsigned int   uint32_t;

Foundation Types
................

Basic types used in the CAN4Bots API: ::

    typedef uint8_t  c4b_nodeid_t;
    typedef uint16_t c4b_opcode1b_t;
    typedef uint16_t c4b_opcode2b_t;
    typedef uint16_t c4b_taskid_t;
    typedef uint16_t c4b_potentialid_t;
    typedef uint32_t c4b_topicid_t;
    typedef uint8_t  c4b_sockid_t;
    typedef uint8_t  c4b_seq_t;
    typedef uint16_t c4b_subscriberid_t;

Structure Types
...............

Structure types used in the CAN4Bots API.

A CAN payload buffer consisting of a length and buffer for up to 8 bytes: ::

    typedef struct c4b_payload {
        uint8_t len;
        uint8_t payload[8];
            } c4b_payload;

The arbitration field for a module-addressed frame: ::

    typedef struct c4b_marb {
        c4b_nodeid_t nf;
        c4b_nodeid_t nt;
        c4b_opcode2b_t opcode;
             } c4b_marb;

The arbitration field for a task-addressed frame: ::
    
    typedef struct c4b_barb {
        c4b_opcode2b_t opcode;
        c4b_taskid_t task;
             } c4b_barb;

The arbitration field for a potential-addressed frame: ::
    
    typedef struct c4b_parb {
        c4b_potentialid_t potential;
        c4b_taskid_t task; }
             c4b_parb;

The arbitration field for a topic-addressed frame: ::
    
    typedef struct c4b_tarb {
        c4b_nodeid_t nf;
        c4b_topicid_t topic;
             } c4b_tarb;

The arbitration field for a streaming frame: ::
    
    typedef struct c4b_sarb { 
        c4b_nodeid_t nf; 
        c4b_nodeid_t nt; 
        c4b_opcode1b_t opcode;
        c4b_sockid_t skf;
        c4b_sockid_t skt;
        c4b_seq_t seqf;
        c4b_seq_t seqt; 
            } c4b_sarb;

.. _Module Addressed API:

Module Addressed API
--------------------

(Incomplete)

Functions:

A function::

    void 
    c4b_mput(uint8 nodeto, uint16 opcode)

Put message.

- void c4b_mputd(uint8 nodeto, uint16 opcode, c4b_payload \*bfr)

  Put message with data payload.

- c4b_payload\* c4b_mwreg(uint8 nodeto, uint16 reg, c4b_payload \*bfr)

  Register write convenience function. Waits for response packet with a blocking,
  non-busy wait.

- c4b_payload\* c4b_mrreg(uint8 nodeto, uint16 reg, c4b_payload \*bfr)

  Register read convenience function. Waits for response packet with a blocking,
  non-busy wait.

- void c4b_getcb(\*(uint8\*)(uint8 \*buflen) callback, uint16 opcode)

  Register a callback function to handle specified opcode.
  Declaration is screwed up.  It should be a pointer to a void function
  taking a pointers to a module address arbitratoin buffer and payload buffer.


