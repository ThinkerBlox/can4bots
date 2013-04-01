// Copyright 2012,2013 ThinkerBlox, LLC
//
// Licensed under GPL v3 or later

// To Do:
//  ____ Think about time-outs and spec behavior of each call.
//  ____ Add error returns where appropriate.
//  ____ Check base types for signed/unsighed versus use as error codes.
//  ____ Evaluate all for lookup requirements and consistency requirements.
//  ____ Sketch out psuedo-code implementation for each call.
//  ____ Need some mechanism to handle biult-in registers and also
//       manage flash storage. Probably need kernel functions for flash.

///////////////////////////////////
#if 0

New Init:

-Per module init structure
-Per task init structure
  -Per behavior init structure?
-Per topic init structure
-Per potential init structure
-Socket pool init structure

-Allocate storage in the structure
-Some kind of end-of-list sentinal for all those structs

#endif

/* 
  Naming:
   c4b_ CAN4Bots
   c4bp_ CAN4Bots private
   c4bk_ CAN4Bots kernel
   c4bcb_ CAN4Bots callback 

*/

// Foundation types

typedef signed   char  int8_t;
typedef unsigned char  uint8_t;
typedef unsigned char  bool;
typedef signed   short int16_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

// CAN4Bots types


typedef uint8_t  c4b_nodeid_t;
typedef uint16_t c4b_opcode1b_t;
typedef uint16_t c4b_opcode2b_t;
typedef uint16_t c4b_taskid_t;
typedef uint16_t c4b_potentialid_t;
typedef uint32_t c4b_topicid_t;
typedef int8_t   c4b_sockid_t;
typedef uint8_t  c4b_seq_t;

typedef struct c4b_payload_t {
    uint8_t len;
    uint8_t payload[8];
        } c4b_payload_t;

typedef struct c4b_moduleid_t {
    uint16_t vendor;
    uint16_t product;
    uint32_t serialnum;
        } c4b_moduleid_t;

typedef struct c4b_mmid_t {
    c4b_nodeid_t   nf;
    c4b_nodeid_t   nt;
    c4b_opcode2b_t opcode;
        } c4b_mmid_t;

typedef struct c4b_bmid_t {
    c4b_opcode2b_t opcode;
    c4b_taskid_t   task;
        } c4b_bmid_t;

typedef struct c4b_barbstate_t {
    bool         active;
    bool         run;
        } c4b_barbstate_t;

typedef struct c4b_pmid_t {
    c4b_potentialid_t potential;
    c4b_taskid_t      task;
        } c4b_pmid_t;

typedef struct c4b_pmsg_t {
    c4b_pmid_t mid;
    uint8_t    bfr[];
        } c4b_pmsg_t;

typedef struct c4b_tmid_t {
    c4b_nodeid_t  nf;
    c4b_topicid_t topic;
        } c4b_tmid_t;

typedef struct c4b_tmsg_t {
    c4b_tmid_t mid;
    uint8_t    bfr[];
        } c4b_tmsg_t;

typedef struct c4b_smid_t { 
    c4b_nodeid_t   nf; 
    c4b_nodeid_t   nt; 
    c4b_opcode1b_t opcode;
    c4b_sockid_t   skf;
    c4b_sockid_t   skt;
    c4b_seq_t      seqf;
    c4b_seq_t      seqt; 
        } c4b_smid_t;

typedef struct c4b_taskinit_t {
    void            (*receivecallback)(c4b_bmid_t*, c4b_payload_t*);
    c4b_taskid_t    task;
    c4b_barbstate_t arbstate;
        } c4b_taskinit_t;

typedef struct c4b_potentialinit_t {
    void              (*receivecallback)(c4b_pmid_t*, void*);
    c4b_potentialid_t potential;
    uint8_t           payloadlen;
        } c4b_potentialinit_t;

typedef struct c4b_topicinit_t {
    void          (*subscribercallback)(c4b_tmid_t*, void*);
    c4b_topicid_t topic;
    uint8_t       payloadlen;
        } c4b_topicinit_t;

#if 0
// FIXME: Move these callback up to the init structure and out of here.
void
c4bcb_sconnectreqest(c4b_nodeid_t nf, c4b_sockid_t skf, c4b_sockid_t skt);
    // Called on a reqest to connect by absolute socket number.
    // Complete the connection by issuing a c4b_connect() 
    // The initiating end will trap out the connection completion
    // so that the application doesn't need to get involved beyond
    // polling for completion.
    // to reject, call c4b_sreject();

void
c4bcb_sconnectservice(c4b_nodeid_t nf, c4b_sockid_t skf, char *servicename);
    // Call on a request to connect by service name.
    // Call c4b_connect() with socket numbers filled in, or
    // call c4b_sreject();

bool
c4bcb_slookupservice(char *servicename);
    // Return TRUE if the service name is implemented by this node.

#endif

typedef struct c4b_socketinit_t {
    void    (*connectsocketcallback)(c4b_nodeid_t /* nf */, c4b_sockid_t /* skf */, c4b_sockid_t /* skt */);
    void    (*connectservicecallback)(c4b_nodeid_t /* nf */, c4b_sockid_t /* skf */, char* /* service name */);
    bool    (*lookupservicecallback)(char* /* service name */);
    uint8_t numports;      // number of ports to create.  0 means no socket protocol supported.
    uint8_t numbfrframes;  // number of frames in buffer pool
    uint8_t bfrframelen;   // length of each frame in buffer pool (payload only, doesn't count overhead)
        } c4b_socketinit_t;

typedef struct c4b_bufferinit_t {
    uint8_t nummodule;    // Number of receive buffers for module addressed messagees.
    uint8_t numtask;      // Number of receive buffers for task addressed messagees.
    uint8_t numpotential; // Number of receive buffers for task addressed messagees.
    uint8_t numtopic;     // Number of receive buffers for task addressed messagees.
        } c4b_bufferinit_t;

typedef struct c4b_init_t {
    uint16_t            version;
    c4b_bufferinit_t	*buffers;
    c4b_taskinit_t      *tasks;      // Array -- last entry has NULL callback. task id's are unique.
    c4b_potentialinit_t *potentials; // Array -- last entry has NULL callback. potential id's are unique.
    c4b_topicinit_t     *topics;     // Array -- last entry has NULL callback. topic id's grouped together.
    c4b_socketinit_t    *sockets;    // Only one of these.
        } c4b_init_t;

// Status and error structure.
typedef struct c4b_status_t {
    uint32_t	ma_overruns;
        } c4b_status_t;

// Reuiqred to be provided by user, the linker finds them:
void
c4bcb_mreceive(c4b_mmid_t*, c4b_payload_t*);
    // safe and softreset function codes are trapped out
    // all other function codes are passed through.
    // After handling the function code, callback should post
    // a Report Mode opcode.

void
c4bcb_msafe();
    // Called when a 'safe' function code is received.

void
c4bcb_mregisterwrite(uint16_t reg, c4b_payload_t*);

void
c4bcb_mregisterread(uint16_t reg, c4b_payload_t*);


//////////////////////////////////////
int16_t
c4b_init(c4b_init_t initializer);
    // Initialize.  
    // version specifies an API version.
    // Allocates space with malloc()
   
void
c4b_poll();
    // Called from application thread. 
    // Callback functions will be dispatched from
    // c4b_poll(). 
    // The application must periodically call
    // c4b_poll() to allow the CAN4Bots library
    // to complete driver "bottom halves" and
    // call callback functions.

c4b_status_t *
c4b_status(c4b_status_t *status);
    // Returns a copy of the current status.

///////////////////////////////////////////////////////////////
// Module-addressed messaging.

// CAN controller provides one level of buffering.
// One static buffer can handle any callbacks, since the 
// callback returns to c4b_poll().
//

void 
c4b_mput(c4b_nodeid_t nodeto, c4b_opcode2b_t opcode);

void 
c4b_mputd(c4b_nodeid_t nodeto, c4b_opcode2b_t opcode, c4b_payload_t *bfr);

// Convenience functions.

c4b_payload_t* 
c4b_mwregb(c4b_nodeid_t nodeto, uint16_t reg, c4b_payload_t *bfr);
    // Write a register on another module.
    // Blocking.
    // Returns the payload actually written.

c4b_payload_t* 
c4b_mrregb(c4b_nodeid_t nodeto, uint16_t reg, c4b_payload_t *bfr);
    // Read a register on another module.
    // Blocking.

void
c4b_mpanicstop();
    // Broadcast E-Stop to all modules.

void
c4b_msetmoduleaddress(c4b_moduleid_t *module, c4b_nodeid_t newaddr);
    // Presumeably, this comes from the debug host over the umbilical anyway.
    // But for completeness...

// inconvenience functions:

void
c4b_mputraw(c4b_mmid_t* mid, c4b_payload_t* payload);
    // Raw version of the module-addressed message put function.

// Consider non-blocking reg read/write that take a call-back or set a completion flag.
// Non-blocking version should have the completion call-back as a parameter.

///////////////////////////////////////////////////////////////////
// Task Addressed

void 
c4b_bput(c4b_taskid_t task, c4b_opcode2b_t opcode); 

void 
c4b_bputd(c4b_taskid_t task, c4b_opcode2b_t opcode, c4b_payload_t *bfr);

void
c4bcb_breceivebitbucket(c4b_bmid_t* mid, c4b_payload_t* payload);
    // Since there are cases where a behavior will be given a
    // taskid in order to arbitrate, but doesn't actually receive
    // any task-addressed messages, some kind of no-op recieve
    // callback is necessary.  This one is pre-defined for convenience.

c4b_barbstate_t *
c4b_barbstatelookup(c4b_taskid_t task);
    // Return a pointer to the arbitration state for a 
    // given task.  It is safe to take a permanent copy
    // of this pointer.

void
c4b_barbitrate(c4b_barbstate_t *arbstate, bool active);
    // Call with a new value in active to invoke arbitration.
    // Updates arbitration state.
    // Guarantees that value of active is communicated to
    // arbiter.
    // case active == arbstate->active:
    //   no-op.
    // case active == False & arbstate->active == True:
    //   immediately set *run = False
    //   deliver active to arbiter
    // case active == True * arbstate->active == False:
    //   deliver active to arbiter
    // In either case, the value of arbstate->run will be 
    // updated in response to arbitration results from the
    // arbiter, which may change the value of arbstate->run
    // many times while active == True wihtout
    // any further calls to c4b_barbtitrate()


/////////////////////////////////////////////////////////////
// Potential Addressed messaging
void
c4b_pputd (c4b_potentialid_t potential, c4b_taskid_t behavior, uint8_t len, void *payload);
    // Automatically creates a fragmented payload if needed.
    // Potentials that are transmitted out of the module and never 
    // received do not need to be preregistered.


//////////////////////////////////////////////////////////////////////////
// Topic addressed messaging
void
c4b_tpublish (c4b_nodeid_t nodefrom, c4b_topicid_t topic, uint8_t len, void *payload);
    // Automatically fragments the payload if required.



/////////////////////////////////////////////////////////////////////
// Streaming Sockets

c4b_sockid_t 
c4b_ssockalloc();
    // returns an available socket number, or -1.

uint16_t
c4b_ssockstat(c4b_sockid_t mysock);
    // Report status of socket.
    //   disconnected
    //   connecting
    //   connected

void
c4b_sconnect(c4b_sockid_t mysock, c4b_nodeid_t nt, c4b_sockid_t skt);
    // Non-blocking.  Caller must poll socket status.

void
c4b_sconnectsvc(c4b_sockid_t mysock, c4b_nodeid_t nt, char *servicename);
    // Non-blocking. Caller must pole socket status.

int16_t
c4b_sconnectb(c4b_sockid_t mysock, c4b_nodeid_t nt, c4b_sockid_t skt);
    // Blocking.  
    // returns other-end socket number or negative error code.

int16_t
c4b_sconnectsvcb(c4b_sockid_t mysock, c4b_nodeid_t nt, char *servicename);
    // Blocking. 
    // returns other-end socket number or negative error code.

void
c4b_sreject(c4b_nodeid_t nt, c4b_sockid_t skt);
    // Reject an incoming connection.

void
c4b_sdisc(c4b_sockid_t mysock);
    // Disconnect by local socket number.

uint8_t
c4b_sget(c4b_sockid_t mysock, uint8_t buflen, void *bfr);
    // returns: 
    //   negative error code
    //     <n> : receive overrun
    //   zero if nothing there (non-blocking)
    //   number of bytes returned, which will not be more than buflen.

uint8_t
c4b_sput(c4b_sockid_t mysock, uint8_t buflen, void *bfr);
    // returns number of characters actually queued.
    // is non-blocking.

void
c4b_sputb(c4b_sockid_t mysock, uint8_t buflen, void *bfr);
    // Blocking.  will not return until entire buffer queued.


