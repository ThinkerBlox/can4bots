Copyright 2012,2013 ThinkerBlox, LLC

CAN4Bots Protocol, Ver. 0.3
===========================

This document describes a CAN-based protocol for use in small, low-cost
robots.
The protocol supports a variety of robot-oriented communication
mechanisms that enable behavior-based robots and publish-subscribe
messaging architectures.

Scope of this Document
======================

This is the controlling specification document for the protocol.
The reader should be familiar with the basic CAN specification.

Associated API's are documented in:

- CAN4Bots Reference API for C
- CAN4Bots Architecture-Specific Bootloader Commands
- CAN4Bots Xfeature XML Schema


Table of Contents
=================

- `License`_
- `Overview`_
- `CAN Bus Background Information`_
- `Message Formats`_

  - `Module Addressed`_

    - `Safe (E-Stop)`_ Op Code
    - `Set Mode Op Code`_
    - `Mode Report Op Code`_
    - `Global Tick Op Code`_
    - `Bus Probe Op Code`_
    - `Bus Announce Op Code`_
    - `Set Addr by ID`_ Op Code
    - `Bootloader Command`_ Op code
    - `Write Register`_ Op Code
    - `Write Confirm`_ Op Code
    - `Read Register`_ Op Code
    - `Read Result`_ Op Code

  - `Task Addressed`_     
  - `Potential Addressed`_ 
  - `Topic Addressed`_    
  - `Socket Stream`_       
  - `Payload Fragmentation`_

- `Implementation Options`_

  - `Required Functions`_
  - `Optional Module Addressed Functions`_

    - `Optional Global Tick`_
    - `Optional Pre-Defined Registers`_
    - `Feature Mask`_
    - `Bootloader`_

  - `Optional Task Addressed Functions`_
  - `Optional Topic Addressed Functions`_
  - `Optional Potential Addressed Functions`_
  - `Optional Socket Services`_

    - `Error Logging Service`_
    - `Self Documentation Service`_
    - `Named Register Discovery`_
    - `Named Service Discovery`_
    - `Console`_

- `State Tables`_

- `Tricks and Tips`_

- `Implementation Notes`_
  
  - `P Field Values`_
  - `Parsimonius Identifier Assignment`_

.. _License:

License
=======

CAN4Bots is licensed under the GPL V3 or later.

.. _Overview:

Overview
========

CAN4Bots is a protocol intended to facilitate the implementation
of behavior-based robots using multiple, modest-size microcontrollers
communicating over a CAN bus.
CAN4Bots provides multiple addressing views into the robot topology.

- Hardware module addressing provides a view of the robot that mirrors
  its physical network topology.
- Task addressing provides a view that mirrors the software topology
  and enables the principals of subsumption.
- Topic addressing enables a publish/subscribe messaging paradigm.
- Potential addressing enables motor schema control paradigms based on 
  the summation of potential vectors.
- Streaming sockets allow a connected protocol that is a very
  light-weight analog to Berkeley sockets.

Not all of these functions must be implemented on every hardware
module.
Only the core functions of hardware module addressing are required
for the most basic implementation.
The core functions can be implemented with very modest resources.
See `Implementation Options`_ for details.

CAN4Bots is native to wired CAN, but nothing precludes encapsulating CAN4Bots
within other protocols, such as IP, serial, or RF protocols.

.. _CAN Bus Background Information:

CAN Bus Background Information
==============================

CAN was developed by Bosch 
for automtive vehicle-area networking.
It is highly robust, and suitable for sending short
messages in real time.
The implementation of the CAN physical and data link
layers are quite complex.
Fortunately, CAN controller electronics that implement
the complex aspects of CAN are easily 
available at low cost because of the vast size of
the automotive market.

This document assumes a basic understanding of CAN.
A good reference for the CAN specification is often found
in the data sheets for CAN controller chips, for example
the Microchip MCP2515 CAN controller, or microcontroller
chips containing a CAN peripheral, such as the Atmel ATMega32m1,
or STMicro STM32F103.

Happily, a grossly simplified view of the basic CAN frame 
is sufficient for understading the details of CAN4Bots.
If we ignore protocol overhead for synchronization, 
media access, error control, framing, etc, and look
only at the bits transferred, we see a very simple
frame consisting of two major fields, a Message Identifier
(MID) and a data field. [#]_

Here is a basic CAN frame, with much interesting
protocol overhead ignored:

+------+---------+--------------+ 
| Name | MID     | Data         |
+------+---------+--------------+ 
| Size | 29 bits | 0 to 8 Bytes |
+------+---------+--------------+ 

The MID serves two key functions.  
As indicated by its name, it serves to identify a
particular type of message.  
It also serves to resolve media access and
establish message priority.
The CAN physical layer allows multiple, synchronized
transmitters to simultaneously send MID information
without corruption.
CAN capitalizes on this feature to arbitrate
media access.  
Simply put, if multiple transmitters begin a
frame at the same time, the first transmitter
to send a '1' in the MID field
loses arbitration and immediately
backs off. 
Unlike many networking protocols, CAN transmitters
synchronize the start of frame transmission so that
all transmitters attempting to access the media
are simultaneously sending synchronized 
MID fields.  [#]_

This leads to some requirements and implications for
any protocol built on top of CAN:

- The higher level protocol must ensure that no
  two transmitters can send the same MID at
  the same time.  

- The higher level protocol must be cognizant
  of the fact that the choice of MID value
  also sets message priority.

In CAN 1.0, the MID field is 11 bits.  
In CAN 2.0A, the MID field is either 11 or 29 bits.
In CAN 2.0B, the MID field must be 29 bits.

CAN4Bots uses CAN 2.0B 29 bit MID fields ONLY.

In both CAN 1.0 and 2.0 the data field can be
anywhere from 0 to 8 bytes.

It should be noted also that since the MID field is
delivered along with the data, certain parts of
the message content can reasonably be carried in the MID
field.

CAN4Bots runs at a speed of 500 kilobits/sec. 
At that speed, a maximum-length CAN frame takes
about 262 microseconds.
The resulting bandwidth is about 3800 frames per second, 
assuming the worst case of 8 bytes of data per frame.


.. [#] MID + Data is a very gross simplification of
       the actual frame.  You have been warned.

.. [#] Imagine "Rock, Paper, Scisors" with 2^29
       unique tokens.


.. _Message Formats:

Message Formats
===============

CAN4Bots is built upon CAN 2.0B and therefore ALWAYS uses 29 bit MID fields.
There are five different message classes which are distinguished by the
three most significant bits of the MID field. 
CAN4Bots defines the name of
this three bit subfield of the MID field as the
'P field'.
The remaining 26 bits of the CAN MID are defined by each format.

+-------+---------------+---------------+
| Name  | P             |               |
+-------+---------------+---------------+
| Desc  | Message Class | Class defined |
+-------+---------------+---------------+
| Width | 3             | 26            |
+-------+---------------+---------------+

Since CAN assigns message priority based on the
values of the MID field, the value of 
the P field sets the relative priority of each
message class.

The message classes are:

+------+------------------------+
| P    | Message Class          |
+======+========================+
| 000b | `Module Addressed`_    |
+------+------------------------+
| 001b | `Task Addressed`_      |
+------+------------------------+
| 010b | (reserved)             |
+------+------------------------+
| 011b | `Potential Addressed`_ |
+------+------------------------+
| 100b | (reserved)             |
+------+------------------------+
| 101b | `Topic Addressed`_     |
+------+------------------------+
| 110b | `Socket Stream`_       |
+------+------------------------+
| 111b | (reserved)             |
+------+------------------------+

.. _Module Addressed:

Module-Addressed Format
-----------------------

Messages can be addressed to a specific hardware module using
the module-addressed format.
This class of message is therefore hardware-topology-centric.
The P field is 000b, therefore this is the highest priority
class of traffic.

The MID field of a module-addressed message contains 'to' and 'from'
node addresses and an op code.

+-------+------+-----------+---------+---------+
| Name  | P    | Nf        | Nt      | Opc     |
+-------+------+-----------+---------+---------+
| Desc  | 000b | Node from | Node to | Op Code |
+-------+------+-----------+---------+---------+
| Width | 3    | 7         | 7       | 12      | 
+-------+------+-----------+---------+---------+

Since the Nf and Nt fields are 7 bits, there are a total
of 128 hardware node addresses available. 
Four hardware node addresses are reserved, leaving 124
that can be used for hardware modules in the application.

+-----------+----------------------------------------------+
| Node Addr | Function                                     |
+-----------+----------------------------------------------+
| 0x00      | Reserved for the debug host, not to be       |
|           | assigned to a hardware node on the bus. [#]_ |
+-----------+----------------------------------------------+
| 0x01      | Broadcast address.                           |
+-----------+----------------------------------------------+
| 0x7e      | Null address. [#]_                           |
+-----------+----------------------------------------------+
| 0x7f      | Hard 'factory' reset address. [#]_           |
+-----------+----------------------------------------------+

.. [#] Communications with a debug host over CAN4Bots
       is accomplished by having some node on the bus
       proxy communications addressed to node 0 over
       whatever connection is used to reach the host.

.. [#] The Null address is used to send data to no node
       in particular.  Recipients match on the op code.

.. [#] An unconfigured module MUST be preprogrammed to
       hardware address 0x7f. 
       This enables modules to be added to the bus
       one at a time, and 
       immediately moved to an application address as
       part of configuration.
       Alternatively, use `Set Addr by ID`_.

The Op Code field determines the function of the message.
From zero to 8 data payload bytes may be contained in each message.

+-----------------+-------------------------+----------------------------+
| Op Code         | Function                | Data                       |
+-----------------+-------------------------+----------------------------+
| 0000_0000_0000b | `Safe (E-Stop)`_        | No payload allowed.        |
+-----------------+-------------------------+----------------------------+
| 0000_0000_0001b | Reserved                |                            |
+-----------------+-------------------------+----------------------------+
| 0000_0000_0010b | `Set Mode Op Code`_     | 1 byte mode value          |
|                 |                         |                            |
|                 |                         | - 0x1 = Soft reset         |
|                 |                         | - 0x2 = Sleep              |
|                 |                         | - 0x3 = Idle               |
|                 |                         | - 0x4 = Run                |
|                 |                         | - 0xfe = Enter Bootloader  |
|                 |                         | - 0xff = no change         |
+-----------------+-------------------------+----------------------------+
| 0000_0000_0011b | `Mode Report Op Code`_  | 2 bytes, new/old modes     |
+-----------------+-------------------------+----------------------------+
| 0000_0000_0100b | `Global Tick Op Code`_  | 1 + 3 = 4 bytes            |
+-----------------+-------------------------+----------------------------+
| 0001_XXXX_XXXXb | Application             |                            |
| 001X_XXXX_XXXXb | defined Op codes [#]_   |                            |
+-----------------+-------------------------+----------------------------+
| 0111_1110_1100b | `Bus Probe Op Code`_    | 8 bytes                    |
+-----------------+-------------------------+----------------------------+
| 0111_1110_1101b | `Bus Announce Op Code`_ | 8 bytes (Module ID)        |
+-----------------+-------------------------+----------------------------+
| 0111_1110_1110b | Reserved                |                            |
+-----------------+-------------------------+----------------------------+
| 0111_1110_1111b | `Set Addr by ID`_       | 8 bytes (Module ID)        |
+-----------------+-------------------------+----------------------------+
| 0111_1111_XXXXb | `Bootloader Command`_   | 0 to 8 byte payload        |
+-----------------+-------------------------+----------------------------+
| 100X_XXXX_XXXXb | `Write Register`_       | 1 to 8 bytes [#]_          |
+-----------------+-------------------------+----------------------------+
| 101X_XXXX_XXXXb | `Write Confirm`_        | 1 to 8 bytes               |
+-----------------+-------------------------+----------------------------+
| 110X_XXXX_XXXXb | `Read Register`_        | none                       |
+-----------------+-------------------------+----------------------------+
| 111X_XXXX_XXXXb | `Read Result`_          | 1 to 8 bytes               |
+-----------------+-------------------------+----------------------------+

.. [#] The application-defined op code space is reserved for
       functionality defined by the module.

.. [#] Up to 512 registers may be defined by a module.
       The maximum size of a register is 8 bytes. 

.. _Safe (E-Stop):

Safe (E-Stop) Op Code
.....................

The ``Safe`` op code, also known as ``E-Stop`` or ``Emergency Stop``
MUST put the module immediately into the safest possible configuration.
In most cases, this is to come to a complete stop and shut down
all actuators.  
The only exception is that actuators that can be dangerous if 
completely released and are not capable of creating pinching
hazzards should be put in a braking condition.

When sending the ``E-Stop`` command, the transmitting
module MUST spoof the master host address Nf=0x00
in order to guarantee that  E-Stop wins 
CAN bus arbitration.
The destination MUST be the broadcast address Nt=0x01.
Since having both Nf and Nt equal to 0 is nonsensical,
and the ``E-Stop`` opcode is 0, this guarantees that
``E-Stop`` will win arbitration since it has the highest
priority valid MID possible in CAN4Bots.

The E-Stop frame value is shown here: 

+------+-----------+-----------+-----------------+
| P    | Nf        | Nt        | Op Code         |
+------+-----------+-----------+-----------------+
| 000b | 000_0000b | 000_0001b | 0000_0000_0000b |
+------+-----------+-----------+-----------------+

The module MUST NOT exit the "safe" condition until it
is reset by hardware or by a ``Set Mode: Soft Reset`` command.

If by some chance 
multiple nodes send ``E-Stop`` simultaneously, they will
not conflict or cause an error as
NO DATA PAYLOAD is ALLOWED on ``E-Stop``.
Multiple simultaneous ``E-Stops`` are indistinguishable from
a single ``E-Stop``.

Since CAN does not guarantee delivery, ``E-Stop`` SHOULD
be sent a recommended minimum of 3 times.

.. _Set Mode Op Code:
.. _Mode Report Op Code:

Set Mode and Mode Report Op Codes
.................................

``Set Mode`` updates the module's operating mode.  
The module MUST reply with a ``Report Mode``.

``Set Mode`` has a single byte payload that is a sub-opcode.

+------+--------------------+----------------------------------------------------------------+
| 0x1  | Soft reset         | Restart the module firmware. Normally ends up in 'Idle' state. |
+------+--------------------+----------------------------------------------------------------+
| 0x2  | Sleep              | (Optional) Enter low-power mode. [#]_                          |
+------+--------------------+----------------------------------------------------------------+
| 0x3  | Idle               | Passive listening mode.                                        |
+------+--------------------+----------------------------------------------------------------+
| 0x4  | Run                | Normal operations.                                             |
+------+--------------------+----------------------------------------------------------------+
| 0xfe | Enter Bootloader   | Stop normal operations. Respond only to bootloader commands.   |
|      |                    | Must exit bootloader state with a Soft Reset.                  |
+------+--------------------+----------------------------------------------------------------+
| 0xff | no change          | Does not update module state.  Module still replies with       |
|      |                    | 'Report Mode' message. Used to elicit report of current state. |
+------+--------------------+----------------------------------------------------------------+

``Report Mode`` has a two byte payload that consists of the new operating state and the previous
operating state.  
The module MUST issue a ``Report Mode`` after it executes a ``Set Mode`` op code.

.. [#] A module that does not implement Sleep state should simply go Idle.


.. _Global Tick Op Code:

Global Tick Op Code
...................

The ``Global Tick`` op code is used to synchronize the time marker called the "global tick"
which is syncrhronized across the entire robot.
The global tick is precise to 1 mSec and accurate to +/-5 mSec when synchronized.
The global tick is represented in an unsigned 24 bit integer, which is sufficient
to represent approximately 4 hours. 
Since the global tick will wrap modulo 2^24 any time more than 1 hour in the past or 
in the future relative to the current global tick is invalid and ignored.
The net result is that modules can communicate time deltas of about one hour 
when timestamping events.

``Global Tick`` messages contain one of four sub op codes:

+-------------+-------------------+-----------------------------------------+-------------+
| Sub Op Code | Name              | Function                                | Data        |
+-------------+-------------------+-----------------------------------------+-------------+
| 0x00        | Tick              | Canonical global tick sent by provider. | 3 byte tick |
+-------------+-------------------+-----------------------------------------+-------------+
| 0x01        | Poll              | Call for vote on value and provider.    | none        |
+-------------+-------------------+-----------------------------------------+-------------+
| 0x02        | Vote, not sync'ed | Vote from non-syncronized module.       | 3 byte tick |
+-------------+-------------------+-----------------------------------------+-------------+
| 0x03        | Vote, sync'ed     | Vote from a synchronized module.        | 3 byte tick |
+-------------+-------------------+-----------------------------------------+-------------+

All ``Global Tick`` messages are sent to broadcast, Nt = 0x01.

``Tick`` (0x00): A module that has been elected the provider sends the global tick once every 200 mSec. 
It is guaranteed to be monotonically increasing, modulo 2^24. 
Any *unsynchronized* module that receives the tick sets its own copy of the global tick to the
received value, goes to synchronized state, and updates it's own copy of the global tick
every 1 mSec using its own timer.

Any *synchronized* module that receives the tick computes a drift.
If the drift is > 5 mSec, the module goes to the unsynchronized state.
If the drift is <= 5 mSec, the module checks to see if the  provider has a lower module number, 
and if so, issues a ``Poll``.
In any case, if the drift is <= 5 mSec, a synchronized module gradually advances or retards its own copy
of the global tick to bring it into synchrony with the global tick from the provider.
While adjusting for drift, the module's copy of the global tick MUST remain monotonically increasing.

``Poll`` (0x01) is a call for a vote on the current value of the global tick and on 
a module to be the provider.
The provider is nominally the module with the lowest address from among those capable of
providing the tick.  The debug host does not provide the tick.
If a module is synchronized, is capable of providing the tick, and has a lower address than
the module that sent a tick, the module should issue a ``Poll`` in an attempt to become
the provider.
If any module goes 500 mSec or more without seeing a ``Tick`` or a ``Poll``, it may assume that there is no
provider and issue a ``Poll``.
Modules SHOULD AVOID issuing a ``Poll`` if a polling sequence has already started.

On reciept of a ``Poll``, a module MUST issue a ``Vote`` (0x02 or 0x03).
The opcode MUST reflect the module's synchronization state.
The data MUST be the module's global tick value at the time of reciept of the ``Poll`` message.
The module MUST capture the value of its local copy of the global tick as soon as
possible after reciept of the ``Poll`` in order to use the value during computation
of a new global tick during vote tally.

On reciept of a ``Vote``, each module starts computing a tally.
If a ``Vote`` from a lower module address arrives, the receiving module can cease
computing the tally since it will not be elected provider.
After 50 mSec from receipt of the ``Poll``, if a module has not seen a ``Vote`` from
a lower numbered module it assumes the role of provider.
The newly elected provider computes the global tick as follows:

- If any synchronized votes were received, all unsynchronized votes MUST be discarded.
- Among the remaining votes, any values +/- 1 hour from the mean value of all votes MUST be discarded. 
- Of the now remaining votes, the maximum (modulo 2^24) is the new elected tick.
  After adjustment to account for the delay since the issuance of the ``Poll``
  command, the elected tick value becomes the new global tick value.
  The elected provider issues the new ``Tick`` immediately, and every 200 mSec thereafter.

If a ``Poll`` is received while a vote tally is being computed, the tally is
cancelled and the process is restarted using tick values associated with the new ``Poll``.

Note the following behaviors:

- A newly reset module joining a running bus will see a global tick within 200 mSec in most cases.
- If the provider drops off the bus, a ``Poll`` will cause a new provider to be elected within about 550 mSec.
- If newly reset module joins a running bus with an address lower than the current provider, it
  will issue a ``Poll`` as soon as it is synchronized, and will likely become the provider.
- At boot, when there is no provider, some module will ``Poll`` for a vote talley in about 500 mSec, and since
  no module is synchronized, the global tick will be selected from among the unsychronized values.
- If, due to packet loss, two modules assume the role of provider, the lower numbered module will
  issue a ``Poll`` at the first instance of a ``Tick`` from the higher numbered module, restarting
  voting.
- If, due to race conditions, a ``Poll`` is issued while a tally from a previous ``Poll`` is still
  being computed, the earlier tally is discarded.
- Eventually the system will stabalize with the lowest numbered module providing the global tick.

A synchronized global tick will be within +/- 5 mSec of
nominal, so the maximum possible skew between synchronized modules is 10 mSec.
The maximum time delta that can be communicated using the global tick is 2^22 mSsec,
or about +/- one hour.
Some possible uses:

- Apply a time stamp to sensor readings.
- Schedule an actuation for some time in the future.
- Schedule coordinated movement of several actuators for some time in the future.

The global is only valid while the module is synchronized, and should not be used if
the module is unsynchronized.

.. _Bus Probe Op Code:
.. _Bus Announce Op Code:

Bus Probe and Bus Announce Op Codes
...................................

``Bus Probe`` and ``Bus Announce`` perform module discovery and serve as a module address diagnostic.
Every module must have a unique address in order for CAN4Bots to work correctly.  
Unless every module has a unique address, CAN arbitration clashes may result in lost
packets and transmission errors.
``Bus Probe`` and ``Bus Announce`` can discover and disambiguate modules in 
the presense of duplicate module addresses.

The ``Bus Probe`` payload is a set of 8 arbitrarily chosen 8 bit values.
It will normally be sent to the broadcast address (0x01), but MAY
be addressed to a specific module address.
Every module receiving a Bus Probe MUST respond with a ``Bus Announce``.

The ``Bus Announce`` payload is the 64 bit Module ID, which is guaranteed to be globally unique.
The 'to node' (Nt) field is spoofed using a checksum of the module ID and a seed
from ``Bus Probe`` payload.
The first byte of the ``Bus Probe`` payload is chosen as the first seed, and
the module sends a ``Bus Announce``.  In the event that the message clashes with
a ``Bus Announce`` from another module, both modules will detect the transmission
error. 
If an error is detected, the ``Bus Announce`` is retried using the next seed from
the ``Bus Probe`` payload. [#]_
If all 8 seeds are consumed without a successful transmission, no further retries take place.


Since the Nt address is spoofed, a module collecting ``Bus Announce`` messages will
need to match on the ``Bus Announce`` opcode.
In most cases, modules with duplicate module addresses will eventually hash to a different
spoofed Nf address, and transmit in the clear.
The collecting module can use the Nf address and Module ID to identify
modules and diagnose address problems. 

`Set Addr by ID`_ can be used to correct address problems.

After clearing known address problems, ``Bus Probe`` SHOULD be repeated with new
seeds until ``Bus Announce`` completes without transmission errors.

.. [#] Most CAN controllers will automatically retry.  In this case, 
       the software should monitor for errors and abort automatic
       retries so that a new Bus Announce can be computed.

.. _Set Addr by ID:

Set Address by ID Op Code
.........................

The ``Set Addr by ID`` op code is an alternate way to write the node address.
The payload is the Module Id.
The Nt SHOULD be broadcast (0x01).
The Nf address is spoofed to the new desired module address setting.
A module receiving a ``Set Addr by ID`` copies the Nf
address into it's module address if and only if the
module id matches it's own globally unique module id.

.. _Bootloader Command:

Bootloader Command Op Code
..........................

The CAN4Bots bootloader enables updating code in hardware modules over the bus.
Bootload mode is entered by sending a ``Set Mode`` op code with the mode value of ``Enter Bootloader``
The bootloader is exited by sending a ``Set Mode`` op code with a mode value of ``Soft Reset``
While in bootloader mode only ``Bootloader Command`` opcodes are recognized.
All other CAN4Bots traffic is silently ignored.

Since CAN does not guarantee delivery, the bootloader commands
are defined in command/ack pairs.
Since a command may get through and be completed, and the ack lost,
it is guaranteed to be harmless to repeat any command.
Some host-based application is presumed to exist which acts as a master
while programming a hardware module.

The `bootloader`_ is further defined under `Implementation Options`_.

+------------+---------------------+----------------------------------------------+-----+
| sub opcode | Function            | Payload                                      | Len |
+------------+---------------------+----------------------------------------------+-----+
|       0x0  | Architecture Query  | none                                         | 0   |
+------------+---------------------+----------------------------------------------+-----+
|       0x8  | Architecture Info   | Processor architecture code. [#]_            | 1-8 |
+------------+---------------------+----------------------------------------------+-----+
|       0x1  | Bank Select         | Bank address (one byte)                      | 2   |
+------------+---------------------+----------------------------------------------+-----+
|       0x9  | Bank Sel Ack        | Bank address (one byte), buffer len: (two)   | 3   |
+------------+---------------------+----------------------------------------------+-----+
|       0x2  | Buffer Fetch        | page address (two bytes)                     | 2   |
+------------+---------------------+----------------------------------------------+-----+
|       0xa  | Buffer Fetch Ack    | page address (two bytes)                     | 2   |
+------------+---------------------+----------------------------------------------+-----+
|       0x3  | Buffer Put          | page address (two bytes)                     | 2   |
+------------+---------------------+----------------------------------------------+-----+
|       0xb  | Buffer Put Ack      | page address (two bytes)                     | 2   |
+------------+---------------------+----------------------------------------------+-----+
|       0x4  | Buffer checksum     | none                                         | 0   |
+------------+---------------------+----------------------------------------------+-----+
|       0xc  | Buffer Checksum Ack | 2 bytes                                      | 2   |
+------------+---------------------+----------------------------------------------+-----+
|       0x5  | Buffer Read         | data offset (two bytes)                      | 2   |
+------------+---------------------+----------------------------------------------+-----+
|       0xd  | Buffer Read Ack     | data offset (two bytes), data (4 bytes)      | 6   |
+------------+---------------------+----------------------------------------------+-----+
|       0x6  | Buffer Write        | data offset (two bytes), data (4 bytes)      | 6   |
+------------+---------------------+----------------------------------------------+-----+
|       0xe  | Buffer Write Ack    | data offset (two bytes), data (4 bytes)      | 6   |
+------------+---------------------+----------------------------------------------+-----+
|       0x7  | Reserved            | n/a                                          | n/a |
+------------+---------------------+----------------------------------------------+-----+
|       0xf  | Error               | sub-op (1), error code (1), offending data   | 4-8 |
+------------+---------------------+----------------------------------------------+-----+

.. [#] Refer to: CAN4Bots Architecture-Specific Bootloader Commands 

.. _Write Register:
.. _Write Confirm:
.. _Read Register:
.. _Read Result:

Register Read/Write Op Codes
............................

Up to 512 registers may be defined by a module.
The maximum size of a register is 8 bytes. 
Size and content are defined by the module.

Certain register addresses are pre-defined.
All register addresses from 0x000 to 0x01F are reserved.  
Registers from 0x020 to 0x1FF are application defined.

Pre-defined register key:

- R: Read-only
- R/W: Read/Write, MAY be volatile
- R/W*: Read/Write, and MUST be backed by non-volatile storage

+---------+-------------------+-------------------------------------------+-------------+------+----------+
| Address | Name              | Description                               | Size, bytes | R/W  | Required |
+=========+===================+===========================================+=============+======+==========+
| 0x000   | Module ID         | vendor id, product id, serial number [#]_ | 2+2+4 = 8   | R    | Y        |
+---------+-------------------+-------------------------------------------+-------------+------+----------+
| 0x001   | Firmware Checksum | Checksum of the firmware. Algorithm TBD.  | 4           | R    | Y        |
+---------+-------------------+-------------------------------------------+-------------+------+----------+
| 0x002   | Module Address    | The hardware module address.              | 1           | R/W* | Y        |
+---------+-------------------+-------------------------------------------+-------------+------+----------+
| 0x003   | Module Name       | The module name.                          | 8           | R/W* | Y        |
+---------+-------------------+-------------------------------------------+-------------+------+----------+
| 0x004   | Max Reg           | Highest used regster numbers:             | 2+2 = 4     | R    | Y        |
|         |                   | pre-defined, application.                 |             |      |          |
+---------+-------------------+-------------------------------------------+-------------+------+----------+
| 0x005   | `Feature Mask`_   | bit mask of provided features             | 8           | R    | N        |
+---------+-------------------+-------------------------------------------+-------------+------+----------+

.. [#] The (vendor id, product id, serial number) triple MUST
       be globally unique.
       Registered vendor id's are issued by CAN4Bots.org at no cost
       on a non-discrimniatory basis. Before selling a module or
       sharing a module design, you SHOULD register a vendor id. 
       0x0000 to 0x000f are reserved for experimental testing and
       can be used freely in your own designs. There is no guarantee
       against vendor id collisions in the range 0x0000-0x000f. 
       The vendor is responsible for ensuring that the (Vendor id,
       product id, serial number) triple is globally unique.

``Write Register`` updates the contents of a register.
Excess data is ignored on ``Write Register``  
Actual data written is reported by ``Write Confirm``

``Read Register`` solicits the contents of a register, which are
returned in a ``Read Result``

.. _Task Addressed:

Task-Addressed Format
---------------------

Messages can be addressed to a task identifier index.
The task can reside in any hardware module and multiple
tasks can reside in a single hardware module.
A task may be a behavior, a sensor driver, or an actuator
driver.
Task-addressed messages follow the logical topology of the
software modules without regard to physical topology.
Task-addressed messages enable classical behavior-based
robot functionality.

Task addressed messages may contain a payload of up
to 8 data bytes.  
Fragmented payloads are NOT supported in task-addressed
messages.

The assignment of task identifiers is arbitrary, and
is left to the application. 
As the task id field is 13 bits, up to 8192 tasks can
be addressed.  
If a naive assignment of the task address field is 
used for CAN MID
matching then a hardware module running several 
addressable tasks may quickly run out of match registers.
Judicious selection of task identifiers with respect to
the actual mapping to physical hardware modules will
reduce the number of CAN match registers needed to
accomodate task-addressed messages. 

+-------+------+---------+-----------------+--------------+
| Name  | P    | Opc     | Task            | Payload      |
+-------+------+---------+-----------------+--------------+
| Desc  | 001b | Op Code | Task identifier | Payload      |
+-------+------+---------+-----------------+--------------+
| Width | 3    | 13 [#]_ | 13              | 0 to 8 bytes |
+-------+------+---------+-----------------+--------------+

.. [#] Op Codes 0_0000_XXXX_XXXX reserved for arbitration.

Arbitration among behaviors to control subsumption is
an important, performance-limiting task, so high-performance
arbitration is enabled through dedicated messages.
For arbitration messages, the Opc field is further 
subdivided.

+----------+--------------+------------+------------+-----------+-----------+
| Name     | Arb-op       | SeqQ       | SeqR       | Active    | Run       |
+----------+--------------+------------+------------+-----------+-----------+
| Result   | 000_0000     | Reqest Seq | Result Seq | bool [#]_ | bool [#]_ |
+----------+--------------+------------+------------+-----------+-----------+
| Confirm  | 000_0001     | Reqest Seq | Result Seq | bool      | bool      |
+----------+--------------+------------+------------+-----------+-----------+
| Request  | 000_0010     | Reqest Seq | Result Seq | bool      | bool      |
+----------+--------------+------------+------------+-----------+-----------+
| Reserved | 000_0011     |            |            |           |           |
+----------+--------------+------------+------------+-----------+-----------+
| Width    | 7            | 2          | 2          | 1         | 1         |
+----------+--------------+------------+------------+-----------+-----------+

.. [#] Active is a boolean that is true when a behavior desires to run.

.. [#] Run is a boolean that is true when a behavior has won arbitration.

Subsumption arbitration is accomplished as follows:

- An arbiter somwhere on the bus matches on arbitration request
  messages by looking for P=001b and Opc=0_0000_XXXX_XXXX.
- A behavior reflects changes in it's desire to run by sending
  an ``Arbitration Request`` with 'Active' set appropriately, 
  incrementing 'Request Seq' (modulo 4) from the last arbitration
  request. 
- The arbiter responds with ``Arbitration Result`` messages, 
  one to each effected behavior,
  setting the 'Run' bit according to the result of arbitration
  for that behavior.
  The 'Result Seq' is incremented (module 4) from the last 
  'Result Seq' sent to that task id.
- Any behavior receiving an ``Arbitration Result`` MUST
  confirm with an ``Arbitration Confirm`` message, setting the
  'Run' bit in confirmation of the ``Arbitration Result`` 
  copying 'Result Seq', and incrementing 'Request Seq' (modulo 4).

Note that a behavior will often be activated/deactivated with an
``Arbitration Result`` message without having sent an ``Arbitration 
Request`` since arbitration is the result of requests 
from multiple behaviors.

.. _Potential Addressed:

Potential-Addressed Format
--------------------------

The potential-addressed format is used to facilitate 
the implementation of
*potential field* and *motor schema* robot behaviors.
For background, see work on potential fields by O. Khatib 
and work on motor schema by R. Arkin. [#]_

The potential-addressed format is used to transmit potential 
vector values
from producer behaviors to the consuming mixer.
The mixer should match on the P field and potential ID.
The behavior id serves to resolve CAN arbitration, and
also identifies to the mixer the source of the payload.
Both the behavior and the mixer MUST know, a priori, the total
size of the potential data type.
Potentials that are more than 8 bytes must be transmitted
as described in `Payload Fragmentation`_.

+-------+------+--------------+-------------+-----------+
| Name  | P    | Pot          | Beh         | Data      |
+-------+------+--------------+-------------+-----------+
| Desc  | 011b | Potential id | Behavior id | payload   | 
+-------+------+--------------+-------------+-----------+
| Width | 3    | 13           | 13          | 1-8 bytes |
+-------+------+--------------+-------------+-----------+

.. [#] In very terse summary, behaviors competing to 
       control an actuator each send a potential to a mixer,
       where the arriving potentials are blended to
       arrive at a setting for the actuator.
       A good introduction can be found in the book:
       "Behavior Based Robotics", by R. Arkin.

.. _Topic Addressed:

Topic-Addressed Format
----------------------

The topic-addressed format supports a publish-subscribe
communications mechanism.
The Nf field guarantees that arbitration will resolve
when two different hardware modules publish to the
same topic simultaneously.

The assignment of topic identifiers is arbitrary, and
is left to the application. 
As the topic id field is 19 bits, up to 2^19 topics can
be addressed.  
If a naive assignment of the topic identifier field is 
used for CAN message id
matching then a hardware module subscribing to several 
topics may quickly run out of match registers.
Judicious selection of topic identifiers with respect to
the actual mapping to physical hardware modules will
reduce the number of CAN match registers needed to
accomodate topic-addressed messages. 

+-------+------+-----------+-------------+
| Name  | P    | Nf        | Topic       |
+-------+------+-----------+-------------+
| Desc  | 101b | Node From | Topic ident |
+-------+------+-----------+-------------+
| Width | 3    | 7         | 19          |
+-------+------+-----------+-------------+

Both the publisher and the subscriber MUST know, a priori, the total
size ofthe topic data type.
Topic data types that are more than 8 bytes long must be transmitted
as described in `Payload Fragmentation`_.

.. _Socket Stream:

Socket-addressed connected streaming
------------------------------------

Socket streaming provides a simple, light-weight connected 
byte-streaming protocol inspired by Berkeley sockets.
Each physical hardware node MAY have up to eight sockets.
A single socket can accept multiple connections, as long as the each connection source can be distinguished
by a unique Nf:Skf tuple.

Since this is a connected protocol, in-order delivery is guaranteed.

+-------+------+-----------+---------+---------+-------------+----------+----------+-------------+-----------+
| Name  | P    | Nf        | Nt      | Opc     | Skf         | Skt      | Seq      | Ack         | Data      |
+-------+------+-----------+---------+---------+-------------+----------+----------+-------------+-----------+
| Desc  | 110b | Node From | Node To | Op code | Socket From | Socket To| Sequence | Acknowledge | Payload   |
+-------+------+-----------+---------+---------+-------------+----------+----------+-------------+-----------+
| Width | 3    | 7         | 7       | 2       | 3           | 3        | 2        | 2           | 0-8 Bytes |
+-------+------+-----------+---------+---------+-------------+----------+----------+-------------+-----------+

Op Codes:

- 0 : Disconnect
- 1 : Connect 
  
  - If no data is present, connect to socket Skt.
  - If data is present, connect by name and assign Skt locally, ignoring received Skt.

- 2 : Transfer data
- 3 : Service lookup

Skf, "socket from" is the socket number on the transmitting module.

Skt, "socket to" is the socket number on the receiving module.

Seq, "sequence" is incremented (modulo 4) with every frame transmitted
on a connection.

Ack, "acknowledge" is a copy of the last received Seq.

Socket Connection Set-Up
........................

A connection can be addresed to a particular socket, or to
a service name.

To set up a connection by socket number:

- Initiating module sets Nf and Skt to convenient values, Nt to 
  the target module, and Skt to the target socket on the target
  module.  The data field must be empty.  Seq == Ack == 0. 
  The initiating module goes to the *connecting* state.
- On reciept of a ``connect``, the target module either
  accepts or rejects the connection.

  - To accept, the target module goes to *connected* state.
    It constructs a ``connect`` packet with Nf and Skf
    set to it's own module and socket number, and Nt and
    Skt to the Nf and Skf from the received connect.
  - To reject, the target module replies with a ``disconnect``
    packet, with Nf and Skf set to it's own module and
    socket number and Nt and Skt set to Nf and Skf from
    the received connect.

- When the initiating module receives the response, it
  identifies the relevant connection by Nt:Skt:Nf.

  - If the response is a ``connect``, the initiator goes
    from *connecting* to *connected* state.
    The connection is now ready for bi-directional,
    streaming data flow.
  - If the response is a ``disconnect``, the initiator
    goes from *connecting* to *disconnected* state.


To set up a connection by service name:

- Initiating module sets Nf and Skt to convenient values, Nt to 
  the target module, and Skt to 0.
  The data field is set to the service name on the target
  module.  Seq == Ack == 0. 
  The initiating module goes to the *connecting* state.
- On reciept of a ``connect``, the target module either
  accepts or rejects the connection.
  If the module can not provide the named service, 
  the target module MUST reject the connection.

  - To accept, the target module goes to *connected* state.
    It constructs a ``connect`` packet with Nf 
    set to it's own module number, and Skf set to a
    convenient socket number.  Nt and Skt are set
    to the Nf and Skf from the received connect.
  - To reject, the target module replies with a ``disconnect``
    packet, with Nf set to it's own module
    number and Nt and Skt set to Nf and Skf from
    the received connect.

- When the initiating module receives the response, it
  identifies the relevant connection by Nt:Skt:Nf.

  - If the response is a ``connect``, the initiator goes
    from *connecting* to *connected* state.
    It MUST use the Skf set by the target module 
    as Skt for traffic on the connection.
    The connection is now ready for bi-directional,
    streaming data flow.
  - If the response is a ``disconnect``, the initiator
    goes from *connecting* to *disconnected* state.

Socket Connection Tear-Down
...........................

Either module may initiate a disconnection by sennding
a ``disconnect`` message with Nf:Skf:Nt:Skt set according
to the connection.
On sending a ``disconnect``, the connection goes to *disconnected*
state.
On receiving a ``disconnect``, the connection goes to *disconnected*
state.
No reply is needed to complete a disconnection.

Data Transfer
.............

Data transfer packets have Nf:Skf:Nt:Skt set according to 
the connection.
0 to 8 bytes of data can be included in the payload packet.
Zero-length data packets are allowed, and have no meaning to
the byte stream, but participate in acknowledgement.

With every frame transfered, the Seq is incremented (modulo 4), and
Ack is set to the last received Seq.
After a timeout, unacknowledged packets are repeated in order
to guarantee delivery.  
After some number of timeouts, the socket is disconnected.

Since Seq and Ack are only 2 bits, there MUST be no more than
a single packet in flight, since having multiple packets in
flight risks the "lost ACK problem". 

Service Name Lookup
...................

A simple name lookup service can be optionally provided. 
The querying node sets Nf, Skf to convenient values, 
sets Nt to 'broadcast' (0x01),
and skt, Seq, and Ack to 0.
The data is set to the case-senstiveASCII value of the
service name, which must by 8 or fewer bytes long.

Receiving nodes which implement the named service respond with
a ``service lookup`` op code, setting
Nt to the received Nf, Skt to the received Skf, Nf to its own address,
Skt = 0, Seq = 0, and Ack to 1.
The service name is returned in the data.
Note that the socket providing the service is not specified.
The client should connect by name allowing the socket to be set
at connection time.

Note that the querying node will receive a response from every
module implementing the service. 

If not implemented, ``service lookup`` op codes MUST be silently ignored.
Since CAN does not guarantee delivery, retries are allowed and must be
harmless.

Spurious Data Packets
.....................

If a data packet arrives for a socket that is not in the
*connected* state, the receiving module MUST reply
with a ``disconnect`` sent to the originating Skf:Skt.
This situation may arise from:

- A module misses a ``disconnect`` message and still has
  it's end of the socket in the connected state when the
  other end has already disconnected.
- A module misses the ``connect`` that completes a 
  connection, and the missed ``connect`` is followed
  by data transfer on the socket.

In both cases, the connection will be torn down.  
In the case of the missed ``disconnect``, this has the
net effect of retrying the missed ``disconnect``. 
In the case of the missed connect, the partially 
constructed connection will be torn down without
any automatic retry mechanism. Retrying 
the connection is the responsibility of the application.

.. _Payload Fragmentation:

Payload Fragmentation
---------------------

The data types sent as potentials or as topics can be large.
Since CAN messages are limited to a maximum of eight data bytes large,
payloads must be fragmented into multiple CAN packets.  
Fragmented payloads are sent without guaranteed delivery.
If any fragment of a payload is missed, the entire multi-packet
message is silently discarded. 

The first byte of a fragment's data field is envelope information, so
each fragment transmits a maximum of 7 payload bytes.
The envelope byte consists of a 5 bit fragment number, and a 3 bit
sequence number.
The maximum size of a fragmented payload is limited to 2^5 fragments,
or 7 * 32 = 224 bytes.

+-------+-----------------+----------+
| Name  | Frag            | Seq      |
+-------+-----------------+----------+
| Desc  | Fragment number | Sequence |
+-------+-----------------+----------+
| Width | 5               | 3        |
+-------+-----------------+----------+

The sequence number is incremented (modulo 8) with each transmission of
a fragmented payload.  The fragment number starts with zero
on the first fragment of the transmission of a new payload. 
The receiving module assembles the payload from the received
fragements.
Only if all N fragments are received with the same sequence
number is the payload valid.  Otherwise the entire multi-frame
transmission is silently discarded.  

Since CAN provides reliable error control via a frame-level checksum, 
there is no further checksum on a fragmented payload.
For a given source, if the sequence number advances before
all the fragments of a payload are received, the incomplete
payload is invalid.
For topics, the source is identified by the module address (Nf).
For potentials, the source is a identified by the behavior id.

.. Implementation Options`_ 

Implementation Options
======================

.. _Required Functions:

Required Functions
------------------

All implementations must support module addressed communications
with a core subset of opcodes and registers.
The required core is enough to discover and identify the module,
perform basic set-up,
start and stop it's basic function, and put it into 'safe mode'.

The following are REQUIRED to be implemented in any CAN4Bots module:

- Module addressed communications
- Module addressed opcode `Safe (E-Stop)`_
- Module addressed `Set Mode Op Code`_ and `Mode Report Op Code`_. Set mode
  MUST support:

  - Soft Reset
  - Idle
  - Run
  - No Change

- Module addressed `Bus Probe Op Code`_ and `Bus Announce Op Code`_
- Module addressed `Set Addr by ID`_
- Module addressed opcodes `Write Register`_, `Write Confirm`_, `Read Register`_,
  and `Read Result`_
- The following registers must be implemented:

  - Module Id
  - Firmware Checksum
  - Module Address
  - Module Name
  - Max Reg

The module MUST respond to it's own address and to the broadcast address.
In most CAN controllers, this will consume two match registers.  

A minimal CAN4Bots module consists of the above 
requirements plus application firmware controlled by application-defined
registers.

.. _Optional Module Addressed Functions:

Optional Module Addressed Functions
-----------------------------------

- `Optional Global Tick`_
- `Optional Pre-Defined Registers`_
- `Bootloader`_
   
.. _Optional Global Tick:

Optional Global Tick
....................

The `Global Tick Op Code`_ implements time synchronization among modules.
It can be used to time-stamp sensor readings, or to schedule actuations.

A module MAY choose to implement a 'receive only' version of the global tick,
and not participate in voting or as a provider.  This is NOT RECOMMENDED. 
A module that implements any part of the global tick functionality SHOULD
implement all of the polling and voting as well.

.. _Optional Pre-Defined Registers:

Optional Pre-Defined Registers
..............................

Register addresses from 0x000 to 0x01F are reserved for predefined registers.

.. _Feature Mask:

Feature Mask Register 0x004
+++++++++++++++++++++++++++

The Feature Mask register is an optional, read-only 64-bit map of implemented features.
Any undefined bits are reserved.

+-----------------------------------------+-----------------------+
| Feature                                 | Mask                  |
+=========================================+=======================+
| Behavior addressed messaging            | 0x0000_0000_0000_0001 |
+-----------------------------------------+-----------------------+
| Potential addressed messaging           | 0x0000_0000_0000_0002 |
+-----------------------------------------+-----------------------+
| Topic addressed messaging               | 0x0000_0000_0000_0004 |
+-----------------------------------------+-----------------------+
| Socket streams                          | 0x0000_0000_0000_0008 |
+-----------------------------------------+-----------------------+
| Bootloader                              | 0x0000_0000_0000_0010 |
+-----------------------------------------+-----------------------+
| XML `Self Documentation Service`_       | 0x0000_0000_0000_0020 |
+-----------------------------------------+-----------------------+
| `Named Register Discovery`_             | 0x0000_0000_0000_0040 |
+-----------------------------------------+-----------------------+
| `Named Service Discovery`_              | 0x0000_0000_0000_0080 |
+-----------------------------------------+-----------------------+

.. _Bootloader:

Optional Bootloader
...................

Bootloading over the CAN bus is OPTIONAL.

The bootload command opcodes generic functions to fetch, store, and verify
blocks of memory.
Programmable memories are modelled as *banks* made up of *pages*.
The programming model assumes a page buffer in SRAM is used as 
a buffer for programming operations.
A non-volatile page can be copied to the buffer, the contents of
the buffer can be written to a non-volatile page, and the contents
of the buffer can be checksummed.

An ``Architecture Query`` op code allows the bootloader host-side software
to identify the device.  
Device-specific implementations of the generic bootload commands 
perform the actual programming operations.

A bootloader SHOULD attempt to detect and block dangerous operations, such as:

- Writes that will overwrite the bootloader itself.
- Writes that will overwrite parmanent data, such as the
  module serial number.
- Writes to so-called "fuse bits" that may render the 
  processor inoperable.

See: CAN4Bots Architecture-Specific Bootloader Commands 
for device-specific details.

Since CAN does not guarantee delivery, all bootload sub op code are
arranged in command-acknowlegement pairs.
It is always safe to retry any bootloader command.

Bootloader sub op codes:

+------------+---------------------+----------------------------------------------+-----+
| sub opcode | Function            | Payload                                      | Len |
+------------+---------------------+----------------------------------------------+-----+
|       0x0  | Architecture Query  | none                                         | 0   |
+------------+---------------------+----------------------------------------------+-----+
|       0x8  | Architecture Info   | Processor architecture code.                 | 1-8 |
+------------+---------------------+----------------------------------------------+-----+

``Architecture Query`` solicits a data frame identifying the programmable device in question.
``Architecture Info`` returns identification information in the form of a catalog index.

+------------+---------------------+----------------------------------------------+-----+
| sub opcode | Function            | Payload                                      | Len |
+------------+---------------------+----------------------------------------------+-----+
|       0x1  | Bank Select         | Bank address:one byte                        | 2   |
+------------+---------------------+----------------------------------------------+-----+
|       0x9  | Bank Sel Ack        | Bank address:one byte, buffer len: two bytes | 3   |
+------------+---------------------+----------------------------------------------+-----+

``Bank Select`` sets the major memory block to be prgrammed/verified.
The meaning of the bank number is defined by the architecture-specific bootloader.
``Bank Sel Ack`` confirms selection of the bank, and returns the size of the SRAM 
buffer in bytes.

+------------+---------------------+----------------------------------------------+-----+
| sub opcode | Function            | Payload                                      | Len |
+------------+---------------------+----------------------------------------------+-----+
|       0x2  | Buffer Fetch        | page address: two bytes                      | 2   |
+------------+---------------------+----------------------------------------------+-----+
|       0xa  | Buffer Fetch Ack    | page address: two bytes                      | 2   |
+------------+---------------------+----------------------------------------------+-----+

``Buffer Fetch`` copies a page from non-volatile memory into the SRAM buffer.  
The payload contains the page number.
``Buffer Fetch Ack`` confirms that the page has been copied to the SRAM buffer.

+------------+---------------------+----------------------------------------------+-----+
| sub opcode | Function            | Payload                                      | Len |
+------------+---------------------+----------------------------------------------+-----+
|       0x3  | Buffer Put          | page address: two bytes                      | 2   |
+------------+---------------------+----------------------------------------------+-----+
|       0xb  | Buffer Put Ack      | page address: two bytes                      | 2   |
+------------+---------------------+----------------------------------------------+-----+

``Buffer Put`` copies a page from the SRAM buffer to non-volatile memory.
The payload contains the page number.
``Buffer Put Ack`` confirms that the page has been copied to non-volatile memory.

+------------+---------------------+----------------------------------------------+-----+
| sub opcode | Function            | Payload                                      | Len |
+------------+---------------------+----------------------------------------------+-----+
|       0x4  | Buffer checksum     | none                                         | 0   |
+------------+---------------------+----------------------------------------------+-----+
|       0xc  | Buffer Checksum Ack | 2 bytes                                      | 2   |
+------------+---------------------+----------------------------------------------+-----+

``Buffer Checksum`` computes a checksum of the contents of the SRAM buffer.
The checksum algorithm is TBD.
``Buffer Checksum Ack`` returns the computed checksum.

+------------+---------------------+----------------------------------------------+-----+
| sub opcode | Function            | Payload                                      | Len |
+------------+---------------------+----------------------------------------------+-----+
|       0x5  | Buffer Read         | data offset: two bytes                       | 2   |
+------------+---------------------+----------------------------------------------+-----+
|       0xd  | Buffer Read Ack     | data offset: two bytes, data: 4 bytes        | 6   |
+------------+---------------------+----------------------------------------------+-----+

``Buffer Read`` requests data from the SRAM buffer at a specified byte offset.
``Buffer Read Ack`` returns four bytes of data from the SRAM buffer.

+------------+---------------------+----------------------------------------------+-----+
| sub opcode | Function            | Payload                                      | Len |
+------------+---------------------+----------------------------------------------+-----+
|       0x6  | Buffer Write        | data offset: two bytes, data: 4 bytes        | 6   |
+------------+---------------------+----------------------------------------------+-----+
|       0xe  | Buffer Write Ack    | data offset: two bytes, data: 4 bytes        | 6   |
+------------+---------------------+----------------------------------------------+-----+

``Buffer Write`` puts four bytes of data into the SRAM buffer at a specified byte offset.
``Buffer Write Ack`` confirms the write by returning the contents of the buffer after update.

+------------+---------------------+-------------------------------------------------+-----+
| sub opcode | Function            | Payload                                         | Len |
+------------+---------------------+-------------------------------------------------+-----+
|       0xf  | Error               | op code (1), error code (1), offending data (n) | 4-8 |
+------------+---------------------+-------------------------------------------------+-----+

``Error`` is sent instead of an ``Ack`` when an operation can not be completed.
The payload consists of the op code from the failed request, a one byte error code,
and the data payload from the failed request. 

.. _Optional Task Addressed Functions:

Optional Task Addressed
-----------------------

Task-addressed messaging is entirely optional.
A module MAY participate in arbitration without supporting other
task-addressed messaging.
A module MAY participate in task-addressed messaged receive-only,
transmit-only, or both transmit and receive.

.. _Optional Topic Addressed Functions:

Optional Topic Addressed
------------------------

Topic-addressed messaging is optional.
A module MAY publish topics without subscribing.
A module MAY subscribe to topics without publishing.

.. _Optional Potential Addressed Functions:

Optional Potential Addressed
----------------------------

Potential-addressed messaging is optional.
A module MAY publish potentials without subscribing.
A module MAY subscribe to potentials without publishing.

.. _Optional Socket Services:

Optional Socket Services
------------------------

Pre-defined socket services MAY be implemented.
pre-defined services are discovered by using normal name lookup functions.

- `Error Logging Service`_
- `Self Documentation Service`_
- `Named Register Discovery`_
- `Named Service Discovery`_
- `Console`_

.. _Error Logging Service:

Error Logging Service
.....................

A node MAY implement an error log under the reserved name 'errorlog'.
If implemented, this service SHOULD accept an arbitrary number of connections.
Clients MAY send ASCII data to the errorlog service.
The errorlog service SHOULD interpret newline characters in the data stream as
separators between log entries.  
The errorlog service SHOULD interleave log data at newline boundaries, and not between.
The errorlog service SHOULD prepend the module node number (Nf field) to log entries.
The actual spooling or other dispositive handling of the log data is application defined.

.. _Self Documentation Service: 

Self Documentation Service
..........................

A module MAY implement a self-documentation feature under
the reserved name 'Xfeature'.
On a completed connection, the module SHOULD immediately start streaming an 
XML document that is compliant with the CAN4Bots Xfeature XML schema.
After streaming the document, the module SHOULD immediately disconnect.

.. _Named Register Discovery:

Named Register Discovery
........................

A module MAY implement a register discovery service under the reserved name 'regname'.
On a completed connection, the module SHOULD immediately start streaming an ASCII 
text document in the format below mapping register numbers to names and format.
After streaming the document, the module SHOULD immediately disconnect.

The document contains sufficient information to dynamically construct
a basic UI for editing module-defined registers.
Each line of the register name document consists of:

``<decimal regnum> <space> <reg name> <optional format> <newline>``

- The register number in decimal representation.
- A single space character.
- The register name, with an ABSOLUTE MAXUMUM of 16 characters.
  The register name MUST be unique with a module.
- Optionally, a register display format specification.
- A single newline character as a line terminator.
 
The display format specification is a sequence of bit-field
width and format specifers. 
The field width is 1 or 2 decimal digits.  
The format specifier is a single alphabetic character as follows:

- i = signed decimal integer, of size implied by field width.
- I = unsigned integer, of size implied by field width.
- X = unsigned hex integer, of size implied by field width.
- B = unsigned binary string, of size implied by field width.
- s = string, number of 8 bit characters implied by field width.  
  Field width must be a multiple of 8.
- F = signed 32 bit float. Field width must be 32 bits.
- D = signed 64 bit float. Field width must be 64 bits.
- s = signed fixed point, of size implied by field width.
- S = unsigned fixed point, of size implied by field width.

Example: Imagine a 32 bit register that holds two 16 bit,
unsigned, fixed-point fractions representing the clockwise and
counter-clockwise motiong limits for a servo. 
In the example below, this register has been assigned to
register number 37, and named "s01_lim_cw_ccw".
The format specification indicates that it is a total of
32 bits wide, and consists of two 16 bit fields, each of
which are unsigned fixed point numbers.: ::

    37 s01_lim_cw_ccw S16S16

The document MUST NOT contain any other extraneous text in order to enable 
very simple parsers with minimal error checking.

.. _Named Service Discovery:

Named Service Discovery
.......................

A module MAY implement a named service doscovery service under the reserved name 'services'.
On a completed connection, the module SHOULD immediately start streaming an ASCII 
text document 
in the format below
listing service names.
After streaming the document, the module SHOULD immediately disconnect.

Each line of the named service document consists of a service name and a newline.
The service name has an ABSOLUTE MAXIMUM of 8 characters.

.. _Console:

Console Service
...............

A module MAY implement a serial debug interface with application-defined 
functionality under the reserved name 'console'. 
The console service SHOULD implement simple, tty-style interaction with
application code running on the module.

.. _State Tables:

State Tables
============

Protocol state transition tables.

TBW


.. _Tricks and Tips:

Tricks and Tips
===============

Module Discovery
----------------

To discover modules on the bus, assuming not conflicting module addresses:

- Broadcast a 'Read Register' of the module number register.
- All nodes will respond with their assigned module number.
- Note that delivery is not guaranteed.  May need to repeat.

To resolve module address conflicts, see discussion in
`Bus Probe Op Code`_.


Watch Dog
---------

To implement a watch dog:

- Assign a topic identifier to the watch dog.

- Let one or more observer module(s) match the watch dog topic.
  The observer module is responsible for handling time-out
  conditions.
  Redundant observers solves the problem of 'who watches the watchmen'.

- All modules can publish to the watch dog topic. 
  Since publishing to a topic doesn't consume any
  CAN match registers, a publish-only topic interface
  light weight.

.. _Implementation Notes:

Implementation Notes
====================

.. _P Field Values:

P Field Values
--------------

The P field values have been chosen to satisfy several conflicting goals.
The CAN protocol specifies that the arbitration field establishes message priority.
Left-most zeroes win.  
This is why P fields and other sub-fields of the arbitration field
have been assigned to ensure that high priority messages will win arbitration.

At conflict with assigning CAN message identifiers arbitrarily is the simple
fact that no micro controller has unlimited match registers.  
Since match registers are a scarce resource, message identifiers must be
assigned with some intelligence in order to enable parsimonious use of
match registers.

The highest priority messages are in the module-addressed message class.  
Prime examples of high priority module addressed messages are
the Safe (E-Stop) command that is used to put all modules
into safe mode, and the 'Set Mode' command. 
The module-addressed message class is given the
P field of 000b to ensure the highest priority for these messages.

Module addressed messages will consume two match registers, one for the
assigned module address, and one to match the broadcast address 0x01.
Socket streaming messages can match on the same 'Node To' field, but is the lowest
priority traffic.  Socket streaming is assigned the P field of 110b.
This enables Socket streaming traffic and module-addressed traffic
to share a match register, by matching XX0b in the P field. 
The P fields 010b and 110b are reserved to ensure that no other traffic will
match that P field pattern.

Behavior arbitration is high priority traffic, so Task-Addressed
traffic is assigned the P field 001b.

Potentials are most often used for motor control, so potential-addressed 
messages are given
a higher priority P field than topic-addressed traffic.
Potentials have a P field of 011b, topics have a P field of 101b.

.. _Parsimonius Identifier Assignment:

Parsimonious Identifier Assignment
----------------------------------

As discussed in `P Field Values`_, module addressed and socket streaming
messaging can both be accomplished using the same two match registers in 
the CAN peripheral.
Careful assignment of Task ID's, Topic ID's and, and Potential ID's is
necessary in order to accomplish message filtering entirely in hardware 
with whatever match registers remain.
It is always possible to fall back to software filtering, by setting
the match registers to accept more traffic than is strictly necessary,
and discarding extraneous messages. 
This comes at the expense of increasing the processor burden to 
handle the extra interrupts and discriminate among the arriving messages.
Clearly, it is preferable to do all matching in hardware.

Conceptually, Task ID's, Topic ID's, and Potential ID's can be assigned
without regard to module topology.
In practice, manimizing the number of match registers consumed requires
that identifiers be assigned in groups corresoponding to physical
CAN interfaces.
Ideally, all of the identifiers of a particular class that are destined
for a particular CAN interface will have a common value for some 
subset of the identifier bits, and only that set of identifiers will 
share that common bit pattern.
The common bit pattern becomes the match pattern for that class of
traffic.

For example:

- Let there be three modules, A,B, and C.
- Let there be five topics, T1, T2, T3, T4, T5
- Let A receive T1 and T2
- Let B receive T2 and T3
- Let C receive T3, T4, and T5

There are 3 topic groups, and each group overlaps.
The following assignment of topic identifies allows each module to use a single 
match register to accept all desired topics and reject all other topics:

+--------+------+
| Topics | ID   | 
+--------+------+
| T1     | 1000 | 
+--------+------+
| T2     | 1101 |
+--------+------+
| T3     | 0110 |
+--------+------+
| T4     | 0010 |
+--------+------+
| T5     | 0011 |
+--------+------+

+--------+------------+----------------+
| Module | Match Mask | Topics Matched |
+--------+------------+----------------+
| A      | 1X0X       | T1, T2         |
+--------+------------+----------------+
| B      | 01XX       | T2, T3         |
+--------+------------+----------------+
| C      | 0X1X       | T3, T4, T5     |
+--------+------------+----------------+

In practice, identifier assignment can be done in relatively simple
software, given
only the partitioning of software units among hardware modules.
 
