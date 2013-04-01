Copyright 2012,2013 ThinkerBlox, LLC

CAN4Bots Over Serial Channels
=============================

ASCII encoding and encapsulation of CAN4Bots to enable
transport over simple serial links.
Also useful for hex dump style displays.

Form
----

- Everything is in ASCII to avoid problems with character-oriented serial links.
- Each CAN4Bots frame is a single line, terminated with a newline.
- Each frame is checksummed using simple sum.  The least significatn 5 bits are
  encoded from 'A-Z0-5'. 
- Each frame is split into three fields by the pipe (|) character.
- Period (.) separates subfields of the arbitration field.
- The data field is encoded in hex.  It is an even number of characters from 0 to 16.
- The arbitration field of each class has its own format.

Module-Addressed
----------------

M.xx.yy.zzzz|<data>|c\\n

- xx is Nf
- yy is Nt
- zzzz is Opc


Task-Addressed
--------------

B.xxx.yyy|<data>|c\\n

- xxx is Opcode
- yyy is task identifier

Potential-Addressed
-------------------

P.xxx.yyy|<data>|c\\n

- xxx is Potential Id
- yyy is task identifier

Topic-Addressed
---------------

T.xx.yyyyy|<data>|c\\n

- xx is Nf
- yyyyy is Topic identifier


Socket-Streaming
----------------

Sv.xxv.yyw.z|<data>|c\\n

- v is one of:
  - C : Connect
  - D : Disconnect
  - T : Transfer
  - L : Lookup
- xx is Nf
- v is Skf
- yy is Nt
- w is Skt
- z is the concatenation of Seqf:Seqt 

Linux Tools
-----------

Define tools on top of or similar to cantools in Linux.
