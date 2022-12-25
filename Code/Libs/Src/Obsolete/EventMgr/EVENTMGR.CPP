//****************************************************************************
//*                                                                          *
//*  EVENTMGR.CPP: DA COM event/message/notification manager                 *
//*                                                                          *
//*  Source compatible with 32-bit 80386 C/C++                               *
//*                                                                          *
//*  V1.00 of 2-Mar-97: Initial                                              *
//*                                                                          *
//*  32-bit protected-mode source compatible with Watcom 10.5/MSC 9.0        *
//*                                                                          *
//*  Author: John Miles                                                      *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  Copyright (C) 1997 Digital Anvil, Inc.                                  *
//*                                                                          *
//****************************************************************************

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dacom.h"                    // DA component manager
#include "eventmgr.h"
#include "stddat.h"
#include "HeapObj.h"
#include "TComponent.h"
#include "System.h"

HINSTANCE  hInstance;                 // DLL instance handle

ICOManager *DACOM;                    // Handle to component manager

C8 *interface_name = "IEventManager"; // Interface name used for registration

//
// By default, the event queue is fixed at 32 entries -- changing the
// queue depth requires recompilation of the event manager
//

#define EV_QSIZE 32

//****************************************************************************
//*                                                                          *
//*  Internal structures                                                     *
//*                                                                          *
//****************************************************************************

//
// EVENT queue entry 
//

class EVENT
{
public:
   S32   message_type;  // MESSAGE_TYPE.index value
   S32   parm0;         // Event-specific parameter block
   S32   parm1;
   S32   parm2;
   S32   parm3;
};

//
// Message name storage class
//
// Names are stored as literal ASCII strings limited to 64 characters in
// size, with an associated 8-bit checksum which is used as a hash key for
// fast lookup
//

class MESSAGE_TYPE
{
public:
   //
   // Data and methods required by HashPool template
   //

   U32           hash_key;  // Hash key for this entry

   MESSAGE_TYPE *hash_next; // Next/prev pointers in each hash bucket,
   MESSAGE_TYPE *hash_prev; // organized for access speed

   MESSAGE_TYPE *next;      // Next/prev pointers in allocation list or  
   MESSAGE_TYPE *prev;      // free list, depending on entry's status

   S32           index;     // Index of this entry in linear array

   //
   // HashPool hash function derives 8-bit key from string by
   // adding all ASCII character values modulo 256
   //

   static U32 hash(const void *object);

   //
   // HashPool search comparison function -- returns TRUE if match found
   //

   inline BOOL32 compare(const void *object)
      {
      return !strcmp(name, (C8 *) object);
      }

   //
   // HashPool initialization function -- used when allocating new entry
   //

   inline void initialize(const void *object)
      {
      strcpy(name, (C8 *) object);
      notify_list = -1;
      }

   //
   // HashPool shutdown function -- called when unlinking entry from
   // pool
   //

   inline void shutdown(void)
      {
      }

   //
   // HashPool diagnostic display function
   //

   void display(void)
      {
      char buff[128];
      wsprintf(buff,"Entry #%d = [%s], key=0x%X\n",index, name, hash_key);
      OutputDebugString(buff);
      }

   //
   // User data
   //

   C8  name[64];           // 64-character ASCII message name
   S32 user_tag;           // User value associated with this message name
   S32 notify_list;        // Head of notification-request list for this event
};

inline U32 MESSAGE_TYPE::hash(const void *object)
{
#if 1
	U32 sum;

	_asm
	 {
	 mov ebx,object
	 xor eax,eax
	 xor edx,edx

	 ALIGN 16
chksum:
	 mov dl,BYTE PTR [ebx]
	 add eax,edx
	 inc ebx
	 test edx,edx
	 jnz chksum

	 and eax,0ffh
	 mov sum,eax
	 }
#else
	U8 sum;

	//
	// Equivalent C version for portability
	//

	U8 *ptr = (U8 *) object;
	sum     = 0;

	while (*ptr)
	   {
		sum += *(ptr++);
	   }

	sum &= 0xff;

#endif

	return sum;
}

//
// Notification request storage class
//
// Clients may request callbacks when events occur, by registering the request
// with the event manager.  The notification request structures are stored 
// in a linear array which serves as a pool of linked-list entries for
// each individual message type.
//
// Notifications are requested with the following call:
//
//    HNOTIFY notify(void *instance,     // "this" for instance, etc.
//                   void *callback,     // Function to call
//                   C8   *message,      // Message 
//                   S32   parm0,        // 4 parameters (must match if not -1)
//                   S32   parm1,        
//                   S32   parm2,        
//                   S32   parm3,        
//                   S32   context);     // User data to be passed to callback
//
//
// Notification callbacks must take the following form:
//
//    void __cdecl call_me(void *instance,   // C++ instance, etc.
//                         S32   parm0,      // Event parameters
//                         S32   parm1,
//                         S32   parm2,
//                         S32   parm3,
//                         S32   context,             // User data from requestor
//                         S32   message_identifier)  // User data for message
//

class NOTIFY_REQUEST
{
public:
   //
   // Data and methods required by Pool template
   //

   NOTIFY_REQUEST *next;    // Next/prev pointers in allocation list or  
   NOTIFY_REQUEST *prev;    // free list, depending on entry's status

   S32             index;   // Index of this entry in linear array, or -1 if
                            // unallocated
   //
   // Pool initialization function -- used when allocating new entry
   //

   inline void initialize(const void *object)
      {
      }

   //
   // Pool shutdown function -- called when unlinking entry from
   // pool
   //

   inline void shutdown(void)
      {
      }

   //
   // Pool diagnostic display function
   //

   void display(void)
      {
      }

   //
   // User data
   //

   NOTIFY_CALLBACK callback;  // Notification function to be called

   const void *instance;      // "this" parameter passed to receiver
   S32         parm0;         // All parms != -1U must match to trigger 
   S32         parm1;         // notification
   S32         parm2;
   S32         parm3;
   S32         context;       // User data associated with receiver's request

   S32         message_type;  // Request was made for this message type

   S32         next_request;  // Next request in event notify chain
   S32         prev_request;  // Previous request in event notify chain
};

struct DACOM_NO_VTABLE EVMGR : public ISystemComponent, IEventManager, IEventSink
{
	BEGIN_DACOM_MAP_INBOUND(EVMGR)
	DACOM_INTERFACE_ENTRY(IEventManager)
	DACOM_INTERFACE_ENTRY(ISystemComponent)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY(IEventSink)
	END_DACOM_MAP()


	GENRESULT init (AGGDESC * info);

    void * operator new (size_t size)
    {
		return HEAP->ClearAllocateMemory(size, "EVMGR instance");
	}

   //
   // Message names are stored in a hash table which is 
   // expandable on demand in blocks of 32 entries
   //

   HashPool <MESSAGE_TYPE, 32> message_types;

   //
   // Event notification requests are stored in an expandable array
   // 

   Pool <NOTIFY_REQUEST, 32> notify_requests;

   //
   // Events are queued in a linear list of size QSIZE
   //
   
   EVENT queue[EV_QSIZE];
   S32   head;
   S32   tail;

   //
   // Shadow list maintained for test_event() function -- test_event()
   // will not be able to "see" an event until after at least one serve() 
   // call has been executed.  This ensures a coherent view of the event
   // queue among all objects that call test_event() during any given frame.
   //

   EVENT shadow        [2] [EV_QSIZE];
   S32   shadow_events [2];
   S32   current_shadow;

   EVMGR::EVMGR()
      {
      head = tail = 0;

      shadow_events[0] =  0;
      shadow_events[1] =  0;
      current_shadow   =  0;
      }

   //
   // IEventManager methods
   //

   virtual void    COMAPI serve              (void);

   virtual HNOTIFY COMAPI notify             (const void     *instance,
                                   NOTIFY_CALLBACK            callback,
                                              const C8       *message,
                                              S32             parm0   = -1,    
                                              S32             parm1   = -1,    
                                              S32             parm2   = -1,    
                                              S32             parm3   = -1,
                                              S32             context =  0);
                                                    
   virtual void    COMAPI cancel             (HNOTIFY      handle);

   virtual void    COMAPI cancel             (const void  *instance,
                                   NOTIFY_CALLBACK         callback = NULL,
                                              const C8    *message  = NULL,
                                              S32          parm0    = -1,
                                              S32          parm1    = -1,
                                              S32          parm2    = -1,
                                              S32          parm3    = -1,
                                              S32          context  = -1); 

   virtual BOOL32  COMAPI test_event         (const C8 *message,
                                              S32       parm0   = -1,
                                              S32       parm1   = -1,
                                              S32       parm2   = -1,
                                              S32       parm3   = -1);
                                                     
   virtual void    COMAPI post_event         (const C8 *message,
                                              S32       parm0    = -1,
                                              S32       parm1    = -1,
                                              S32       parm2    = -1,
                                              S32       parm3    = -1);
                                                     
   virtual void    COMAPI send_event         (const C8 *message,
                                              S32       parm0    = -1,
                                              S32       parm1    = -1,
                                              S32       parm2    = -1,
                                              S32       parm3    = -1);

   virtual S32     COMAPI register_message_identifier
                                             (const C8    *message,
                                              S32          message_identifier);

   virtual BOOL32  COMAPI fetch_event        (S32          entry_number,  
                                              C8         **message_name       = NULL,
                                              S32         *message_identifier = NULL,
                                              S32         *parm0              = NULL,
                                              S32         *parm1              = NULL,
                                              S32         *parm2              = NULL,
                                              S32         *parm3              = NULL);

   //
   // IAggregateComponent methods
   // 

   virtual GENRESULT COMAPI Initialize (void);

   //
   // ISystemComponent methods
   // 

   //
   // Begin / end frame update period
   //
   // All applications should call begin_frame() at the beginning of each
   // game frame, perform all necessary rendering and display actions,
   // and then call end_frame() to display the rendered frame properly.
   //

   DEFMETHOD_(void,Update) (void);

   // 
   // IEventSink methods
   //
   
   virtual void    COMAPI SendEvent         (const C8    *message,
                                              S32          parm0    = -1,
                                              S32          parm1    = -1,
                                              S32          parm2    = -1,
                                              S32          parm3    = -1)
   {
		post_event(message, parm0, parm1, parm2, parm3);
   }
   
   //
   // Private (internal) methods
   // 

   void                   dispatch_event     (EVENT *event, S32 current);

};

//--------------------------------------------------------------------------
//-----------------------StartUp code---------------------------------------
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//  
void main (void)
{
}
//--------------------------------------------------------------------------
//  
void SetDllHeapMsg (void)
{
	DWORD dwLen;
	char buffer[260];
	
	dwLen = GetModuleFileName(hInstance, buffer, sizeof(buffer));
 
	while (dwLen > 0)
	{
		if (buffer[dwLen] == '\\')
		{
			dwLen++;
			break;
		}
		dwLen--;
	}

	SetDefaultHeapMsg(buffer+dwLen);
}

//****************************************************************************
//*                                                                          *
//*  DLLMain() called on startup/shutdown                                    *
//*                                                                          *
//****************************************************************************
//
BOOL COMAPI DllMain(HINSTANCE hinstDLL,  //)
                    DWORD     fdwReason,
                    LPVOID    lpvReserved)
{
   IComponentFactory *server;

   switch (fdwReason)
      {
      //
      // DLL_PROCESS_ATTACH: Create object server component and register it 
      // with DACOM manager
      //

      case DLL_PROCESS_ATTACH:

         hInstance = hinstDLL;

		 HEAP = HEAP_Acquire();
		 SetDllHeapMsg();

         server = new DAComponentFactory2<DAComponentAggregate<EVMGR>, AGGDESC>;

         if (server == NULL)
            {
            break;
            }

         DACOM = DACOM_Acquire();

         if (DACOM != NULL)
            {
            DACOM->RegisterComponent(server, interface_name);
            }

         server->Release();
         break;

      //
      // DLL_PROCESS_DETACH: Release DACOM manager instance
      //

      case DLL_PROCESS_DETACH:

         if (DACOM != NULL)
            {
            DACOM->Release();
            }
         break;
      }

   return TRUE;
}

GENRESULT EVMGR::init (AGGDESC * info)
{
	if (info->description && strcmp(info->description, "EVMGR"))
		return GR_INTERFACE_UNSUPPORTED;
	return GR_OK;
}

//****************************************************************************
//*                                                                          *
//*  Dispatch event to all notification request handlers (private method)    *
//*                                                                          *
//****************************************************************************

void EVMGR::dispatch_event (EVENT *event, S32 current)
{
   NOTIFY_REQUEST request;

   if (current == -1)
      {
      return;
      }

   request = notify_requests.list[current];

   dispatch_event(event, request.next_request);

   //
   // This request is fulfilled if all request parameters not equal to -1
   // match their corresponding event parameters
   //

   if  (((request.parm0 == -1) || (request.parm0 == event->parm0)) &&
        ((request.parm1 == -1) || (request.parm1 == event->parm1)) &&
        ((request.parm2 == -1) || (request.parm2 == event->parm2)) &&
        ((request.parm3 == -1) || (request.parm3 == event->parm3)))
       {
       (*request.callback)(request.instance,
                           event->parm0,
                           event->parm1,
                           event->parm2,
                           event->parm3,
                           request.context,
                           message_types.list[event->message_type].user_tag);
       }
}

//****************************************************************************
//*                                                                          *
//*  Register event notification request with event manager                  *
//*                                                                          *
//****************************************************************************

HNOTIFY COMAPI EVMGR::notify         (const void *instance, //)
                           NOTIFY_CALLBACK        callback,
                                      const C8   *message,
                                      S32         parm0,    
                                      S32         parm1,    
                                      S32         parm2,    
                                      S32         parm3,
                                      S32         context)
{
   //
   // Get message type identifier
   //
   // Register message type if not yet in database
   //

   S32 id;

   if ((id = message_types.search(message)) == -1)
      {
      id = message_types.allocate(message);
      }

   //
   // Register notify request
   //

   S32 request = notify_requests.allocate();

   NOTIFY_REQUEST *req = &notify_requests.list[request];

   req->callback  = callback;

   req->instance  = instance;
   req->parm0     = parm0;
   req->parm1     = parm1;
   req->parm2     = parm2;
   req->parm3     = parm3;
   req->context   = context;
      
   req->message_type = id;

   //
   // Link request into event's notification chain at beginning
   //

   MESSAGE_TYPE *t = &message_types.list[id];

   req->next_request =  t->notify_list;
   req->prev_request = -1;

   if (t->notify_list != -1)
      {
      notify_requests.list[t->notify_list].prev_request = request;
      }

   t->notify_list = request;

   //
   // Return index of notification request in linear array (for convenient
   // cancellation)
   //

   return request;
}

//****************************************************************************
//*                                                                          *
//*  Cancel specific event-notification request                              *
//*                                                                          *
//****************************************************************************

void    COMAPI EVMGR::cancel             (HNOTIFY      handle)
{
   //
   // Get pointer to specified request structure
   //

   NOTIFY_REQUEST *req = &notify_requests.list[handle];

   //
   // Cancel this notification request by unlinking it from its 
   // message type's notification list
   //

   if (req->prev_request != -1)
      {
      NOTIFY_REQUEST *prev = &notify_requests.list[req->prev_request];

      prev->next_request = req->next_request;
      }
   else
      {
      message_types.list[req->message_type].notify_list = 
         req->next_request;
      }

   if (req->next_request != -1)
      {
      NOTIFY_REQUEST *next = &notify_requests.list[req->next_request];

      next->prev_request = req->prev_request;
      }

   //
   // Unlink request structure from pool's "allocated" list, 
   // making it available for subsequent reallocation
   //

   notify_requests.unlink(req);
}

//****************************************************************************
//*                                                                          *
//*  Cancel any/all event notification requests which match specified        *
//*  criteria                                                                *
//*                                                                          *
//****************************************************************************

void    COMAPI EVMGR::cancel             (const void  *instance, //)
                               NOTIFY_CALLBACK         callback,
                                          const C8    *message,
                                          S32          parm0,
                                          S32          parm1,
                                          S32          parm2,
                                          S32          parm3,
                                          S32          context)
{
   S32 id = -1;

   //
   // If message type specified, get its index
   // 

   if (message != NULL)
      {
      //
      // Get message type identifier
      //
      // Register message type if not yet in database
      //

      if ((id = message_types.search(message)) == -1)
         {
         id = message_types.allocate(message);
         }
      }

   S32 min,max;

   if (id == -1)
      {
      //
      // No specific message type provided (message=NULL) -- remove all
      // qualifying requests without regard to message type
      //

      min = 0;
      max = message_types.list_size - 1;
      }
   else
      {
      //
      // Specific message type provided -- remove qualifying requests for
      // this type only
      // 

      min = id;
      max = id;
      }

   //
   // For all qualifying message types...
   // 

   for (S32 i=min; i <= max; i++)
      {
      //
      // For all allocated notification requests associated with this 
      // event type...
      //

      if (message_types.list[i].index != i)
         {
         continue;
         }

      MESSAGE_TYPE *msg = &message_types.list[i];

      S32 request = msg->notify_list;

      while (request != -1)
         {
         NOTIFY_REQUEST *req  = &notify_requests.list[request];
         S32             next = req->next_request;

         //
         // If request matches cancellation criteria, free its entry in the
         // request pool and remove it from the message type's notification 
         // list
         //
         // Search criteria for request qualification:
         // 
         //    instance must match, if instance argument is !NULL
         //    parm0...3 must match each parm argument which is != -1
         //    context must match, if context argument is != -1
         //    callback must match, if callback argument is !NULL
         //

         if (instance != NULL)
            {
            if (instance != req->instance)
               {
               request = next;
               continue;
               }
            }

         if (((parm0 != -1) && (parm0 != req->parm0)) ||
             ((parm1 != -1) && (parm1 != req->parm1)) ||
             ((parm2 != -1) && (parm2 != req->parm2)) ||
             ((parm3 != -1) && (parm3 != req->parm3)))
             {
             request = next;
             continue;
             }

         if (context != -1)
            {
            if (context != req->context)
               {
               request = next;
               continue;
               }
            }

         if (callback != NULL)
            {
            if (callback != req->callback)
               {
               request = next;
               continue;
               }
            }

         //
         // Search criteria met -- cancel this notification request
         //

         cancel(request);
 
         //
         // Continue searching with next request in list, if any
         //

         request = next;
         }
      }
}

//****************************************************************************
//*                                                                          *
//*  Post event to event queue                                               *
//*                                                                          *
//****************************************************************************

void    COMAPI EVMGR::post_event  (const C8 *message, //)
                                   S32       parm0,
                                   S32       parm1,
                                   S32       parm2,
                                   S32       parm3)
                                   
{
   //
   // Get message type identifier
   //
   // Register message type if not yet in database
   //

   S32 id;

   if ((id = message_types.search(message)) == -1)
      {
      id = message_types.allocate(message);
      }

   //
   // Add event to queue 
   // 

   queue[head].message_type = id;
   queue[head].parm0        = parm0;
   queue[head].parm1        = parm1;
   queue[head].parm2        = parm2;
   queue[head].parm3        = parm3;

   //
   // Advance head pointer -- if head catches tail, advance tail
   //

   if (++head >= EV_QSIZE)
      {
      head = 0;
      }

   if (head == tail)
      {
      if (++tail >= EV_QSIZE)
         {
         tail = 0;
         }
      }
}

//****************************************************************************
//*                                                                          *
//*  Dispatch event immediately (without queueing it)                        *
//*                                                                          *
//****************************************************************************

void    COMAPI EVMGR::send_event  (const C8 *message, //)
                                   S32       parm0,
                                   S32       parm1,
                                   S32       parm2,
                                   S32       parm3)
{
   //
   // Get message type identifier
   //
   // Register message type if not yet in database
   //

   S32 id;

   if ((id = message_types.search(message)) == -1)
      {
      id = message_types.allocate(message);
      }
   
   //
   // Fill in temporary event structure
   //

   EVENT event;

   event.message_type = id;
   event.parm0        = parm0;
   event.parm1        = parm1;
   event.parm2        = parm2;
   event.parm3        = parm3;

   //
   // Dispatch event to all registered handlers
   //

   dispatch_event(&event, message_types.list[id].notify_list);
}

//****************************************************************************
//*                                                                          *
//*  Service event queue                                                     *
//*                                                                          *
//****************************************************************************

void    COMAPI EVMGR::serve              (void)
{
   //
   // Get pointers to current shadow buffer and its index
   //

   S32   *n = &shadow_events[current_shadow ^ 1];
   EVENT *s = &shadow       [current_shadow ^ 1][0];

   *n = 0;

   //
   // If any events are in the queue, remove and dispatch them
   //

   while (tail != head)
      {
      //
      // Copy event to shadow list, to make it visible to test_event()
      // during the next service interval
      //

      if (*n < EV_QSIZE)
         {
         s[(*n)++] = queue[tail];
         }

      //
      // Process this event
      //

      dispatch_event(&queue[tail], 
                     message_types.list[queue[tail].message_type].notify_list);

      //
      // Advance tail pointer to next event, if any, and continue
      //

      if (++tail >= EV_QSIZE)
         {
         tail = 0;
         }
      }

   //
   // Exchange "current shadow" buffer pointers, so that test_event() will see
   // events that were posted during the previous interval
   //

   current_shadow ^= 1;
}

//****************************************************************************
//*                                                                          *
//*  Return TRUE if event was posted during the previous service interval,   *
//*  else FALSE                                                              *
//*                                                                          *
//****************************************************************************

BOOL32  COMAPI EVMGR::test_event  (const C8 *message, //)
                                   S32       parm0,
                                   S32       parm1,
                                   S32       parm2,
                                   S32       parm3)
{
   //
   // Get message type identifier
   //
   // Register message type if not yet in database
   //

   S32 id;

   if ((id = message_types.search(message)) == -1)
      {
      id = message_types.allocate(message);
      }

   //
   // Get pointers to current shadow buffer and its index
   //

   S32    n     =  shadow_events[current_shadow];
   EVENT *event = &shadow       [current_shadow][0];

   //
   // Return TRUE if any event from the previous service interval 
   // qualifies as a match for the specified event, else FALSE
   //

   S32 i;

   for (i=0; i < n; i++,event++)
      {
      if (id == event->message_type)
         {
         if (((parm0 == -1) || (parm0 == event->parm0)) &&
             ((parm1 == -1) || (parm1 == event->parm1)) &&
             ((parm2 == -1) || (parm2 == event->parm2)) &&
             ((parm3 == -1) || (parm3 == event->parm3)))
            {
            return TRUE;
            }
         }
      }

   return FALSE;
}

//****************************************************************************
//*                                                                          *
//*  Associate a user-specified constant value with a message name           *
//*                                                                          *
//****************************************************************************

S32     COMAPI EVMGR::register_message_identifier (const C8    *message, //)
                                                   S32          message_identifier)
                                  
{
   //
   // Get message type identifier
   //
   // Register message type if not yet in database
   //

   S32 id;

   if ((id = message_types.search(message)) == -1)
      {
      id = message_types.allocate(message);
      }

   //
   // Set user message identifier, and return old value
   //

   S32 old = message_types.list[id].user_tag;

   message_types.list[id].user_tag = message_identifier;

   return old;
}

//****************************************************************************
//*                                                                          *
//*  Allow the application to inspect the message queue directly             *
//*                                                                          *
//*  Return TRUE if the requested entry number is out of range               *
//*                                                                          *
//****************************************************************************

BOOL32  COMAPI EVMGR::fetch_event (S32          entry_number,
                                   C8         **message_name,
                                   S32         *message_identifier,
                                   S32         *parm0,
                                   S32         *parm1,
                                   S32         *parm2,
                                   S32         *parm3)
{
   //
   // Get pointer to requested event entry
   //

   if (entry_number >= shadow_events[current_shadow])
      {
      return FALSE;
      }

   EVENT *event = &shadow[current_shadow][entry_number];

   //
   // Return requested information about event entry
   //

   if (message_name != NULL)
      {
      *message_name = message_types.list[event->message_type].name;
      }

   if (message_identifier != NULL)
      {
      *message_identifier = message_types.list[event->message_type].user_tag;
      }

   if (parm0 != NULL)
      {
      *parm0 = event->parm0;
      }

   if (parm1 != NULL)
      {
      *parm1 = event->parm1;
      }

   if (parm2 != NULL)
      {
      *parm2 = event->parm2;
      }

   if (parm3 != NULL)
      {
      *parm3 = event->parm3;
      }

   return TRUE;
}
//-----------------------------------------------------------------------------------------
//
GENRESULT EVMGR::Initialize (void)
{
	return GR_OK;	// nothing to initialize?
}
//-----------------------------------------------------------------------------------------
//
void EVMGR::Update (void)
{
	serve();
}

//-----------------------------------------------------------------------------------------
//--------------------------------End EventMgr.cpp-----------------------------------------
//-----------------------------------------------------------------------------------------
