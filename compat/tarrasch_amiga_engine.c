/* Tarrasch's UCI engine link on AmigaOS (see tarrasch_amiga_engine.h).
 *
 *   Tarrasch --write--> PIPE:tarrasch_<me>_<n>_in  --> engine Input()
 *   engine Output() --> PIPE:tarrasch_<me>_<n>_out --> reader --> ring --> Tarrasch
 *
 * The reader is a process of its own because a Read() on a PIPE: waits
 * until there is something to read; the GUI takes what the reader has put
 * in the ring and never waits. When the engine ends, its output closes
 * and the reader sees end of file. To let a reader go while an engine still
 * holds its output, Tarrasch writes one byte into that channel itself, so
 * the waiting Read() returns and the reader sees it is asked to stop.
 * Copyright (c) 2026 AmiCradle Team. MIT licence, like Tarrasch. */
#include <exec/types.h>
#include <exec/semaphores.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdlib.h>
#include <string.h>
#include "tarrasch_amiga_engine.h"
#include "tarrasch_engine_util.h"

static struct SignalSemaphore te_lock;
static te_ring te_buf;
static BPTR te_to;                  /* our end of the engine's input */
static BPTR te_from;                /* the reader's end of the engine's output */
static char te_from_name[64];
static struct Task *te_owner;
static BYTE te_done_sig = -1;
static volatile int te_reader_running;
static volatile int te_stopping;
static unsigned te_count;
static int te_ready;

static void te_setup( void )
{
    if( te_ready )
        return;
    InitSemaphore( &te_lock );
    te_ring_init( &te_buf );
    te_owner = FindTask( NULL );
    te_done_sig = AllocSignal( -1 );
    atexit( tarrasch_engine_stop );
    te_ready = 1;
}

/* The reader process: everything the engine writes goes into the ring. */
static void te_reader( void )
{
    char buf[512];
    for( ;; )
    {
        LONG n = Read( te_from, buf, sizeof buf );
        if( n <= 0 || te_stopping )
            break;
        LONG put = 0;
        while( put < n && !te_stopping )
        {
            ObtainSemaphore( &te_lock );
            put += (LONG)te_ring_put( &te_buf, buf + put, (size_t)( n - put ) );
            ReleaseSemaphore( &te_lock );
            if( put < n )
                Delay( 1 );     /* the GUI has not taken it yet */
        }
    }
    Close( te_from );
    te_from = 0;
    /* in Forbid() to the end: this code stays loaded until the process is gone */
    Forbid();
    te_reader_running = 0;
    if( te_done_sig >= 0 )
        Signal( te_owner, 1UL << te_done_sig );
}

void tarrasch_engine_stop( void )
{
    if( !te_ready )
        return;
    if( te_to )
    {
        Close( te_to );         /* the engine sees end of file */
        te_to = 0;
    }
    if( !te_reader_running )
        return;
    /* up to 5 s for the engine to finish and close its output */
    for( int i = 0; i < 250 && te_reader_running; i++ )
        Delay( 1 );
    if( te_reader_running )
    {
        /* still there: wake the reader's Read() ourselves */
        te_stopping = 1;
        BPTR poke = Open( (CONST_STRPTR)te_from_name, MODE_NEWFILE );
        if( poke )
        {
            Write( poke, (APTR)"\n", 1 );
            Close( poke );
        }
        while( te_reader_running )
            Wait( 1UL << te_done_sig );
    }
    SetSignal( 0, 1UL << te_done_sig );
    te_stopping = 0;
}

int tarrasch_engine_start( const char *path, int lower_pri )
{
    char line[600], in_name[64];
    BPTR lock, engine_in = 0, engine_out = 0;
    struct Process *reader;
    LONG rc;

    te_setup();
    if( te_done_sig < 0 )
        return 0;
    tarrasch_engine_stop();
    ObtainSemaphore( &te_lock );
    te_ring_init( &te_buf );
    ReleaseSemaphore( &te_lock );

    /* no engine there: say so now, not after Start()'s 20 s wait */
    lock = Lock( (CONST_STRPTR)path, ACCESS_READ );
    if( !lock )
        return 0;
    UnLock( lock );
    if( !te_engine_line( line, sizeof line, path ) )
        return 0;

    te_count++;
    if( !te_pipe_name( in_name, sizeof in_name, (unsigned long)te_owner, te_count, "in" )
     || !te_pipe_name( te_from_name, sizeof te_from_name, (unsigned long)te_owner, te_count, "out" ) )
        return 0;

    engine_in  = Open( (CONST_STRPTR)in_name, MODE_OLDFILE );
    te_to      = Open( (CONST_STRPTR)in_name, MODE_NEWFILE );
    engine_out = Open( (CONST_STRPTR)te_from_name, MODE_NEWFILE );
    te_from    = Open( (CONST_STRPTR)te_from_name, MODE_OLDFILE );
    if( !engine_in || !te_to || !engine_out || !te_from )
        goto fail;

    if( lower_pri < 0 ) lower_pri = 0;
    if( lower_pri > 3 ) lower_pri = 3;
    rc = SystemTags( (CONST_STRPTR)line,
                     SYS_Input, engine_in,
                     SYS_Output, engine_out,
                     SYS_Asynch, TRUE,
                     NP_StackSize, 262144,
                     NP_Priority, (LONG)te_owner->tc_Node.ln_Pri - lower_pri,
                     NP_Name, (ULONG)"Tarrasch engine",
                     TAG_DONE );
    if( rc == -1 )
        goto fail;
    engine_in = engine_out = 0;     /* the engine's now; closed when it ends */

    te_reader_running = 1;
    reader = CreateNewProcTags( NP_Entry, (ULONG)te_reader,
                                NP_Name, (ULONG)"Tarrasch engine reader",
                                NP_StackSize, 8192,
                                NP_Priority, (LONG)te_owner->tc_Node.ln_Pri + 1,
                                NP_Input, 0, NP_CloseInput, FALSE,
                                NP_Output, 0, NP_CloseOutput, FALSE,
                                TAG_DONE );
    if( !reader )
    {
        te_reader_running = 0;
        Close( te_to );         /* the engine sees end of file and ends */
        te_to = 0;
        Close( te_from );
        te_from = 0;
        return 0;
    }
    return 1;

fail:
    if( engine_in )  Close( engine_in );
    if( engine_out ) Close( engine_out );
    if( te_to )      { Close( te_to ); te_to = 0; }
    if( te_from )    { Close( te_from ); te_from = 0; }
    return 0;
}

int tarrasch_engine_read( char *buf, int max )
{
    size_t n;
    if( !te_ready || max <= 0 )
        return 0;
    ObtainSemaphore( &te_lock );
    n = te_ring_get( &te_buf, buf, (size_t)max );
    ReleaseSemaphore( &te_lock );
    return (int)n;
}

void tarrasch_engine_write( const char *s )
{
    if( te_to && s && *s )
        Write( te_to, (APTR)s, (LONG)strlen( s ) );
}

unsigned long tarrasch_ticks_ms( void )
{
    static long base_days = -1;
    struct DateStamp ds;
    DateStamp( &ds );
    if( base_days < 0 )
        base_days = ds.ds_Days;
    return te_stamp_ms( ds.ds_Days, ds.ds_Minute, ds.ds_Tick, base_days );
}

void tarrasch_sleep_ms( int ms )
{
    Delay( te_delay_ticks( ms ) );
}

/* --- one Tarrasch at a time (tarrasch_amiga_snglinst.h) ---------------------
 * The first Tarrasch puts up a public port of this name; a second finds it.
 * The port only marks the name: nothing is ever sent to it. */
static struct MsgPort *te_instance;
static char te_instance_name[40];

int tarrasch_instance_claim( const char *name )
{
    int another;
    if( te_instance )
        return 0;
    Forbid();
    another = FindPort( (CONST_STRPTR)name ) != NULL;
    if( !another )
    {
        te_instance = (struct MsgPort *)AllocVec( sizeof *te_instance, MEMF_PUBLIC | MEMF_CLEAR );
        if( te_instance )
        {
            strncpy( te_instance_name, name, sizeof te_instance_name - 1 );
            te_instance->mp_Node.ln_Type = NT_MSGPORT;
            te_instance->mp_Node.ln_Name = te_instance_name;
            te_instance->mp_Flags = PA_IGNORE;
            te_instance->mp_MsgList.lh_Head = (struct Node *)&te_instance->mp_MsgList.lh_Tail;
            te_instance->mp_MsgList.lh_Tail = NULL;
            te_instance->mp_MsgList.lh_TailPred = (struct Node *)&te_instance->mp_MsgList.lh_Head;
            AddPort( te_instance );
        }
    }
    Permit();
    if( !another && te_instance )
        atexit( tarrasch_instance_release );
    return another;
}

void tarrasch_instance_release( void )
{
    if( !te_instance )
        return;
    RemPort( te_instance );
    FreeVec( te_instance );
    te_instance = NULL;
}
