/* The Tarrasch Toy Engine's input loop on AmigaOS, in place of its Windows
 * reader thread and semaphore (engine-patches/0001 takes this in at the end
 * of toyengine.cpp). A reader process reads the engine's Input() and queues
 * whole lines, setting gbl_stop at once for "stop" and "quit" so a search
 * in progress ends; the main process takes the lines in order and answers
 * them through process(). End of input counts as "quit".
 * Copyright (c) 2026 AmiCradle Team. MIT licence, like Tarrasch. */
#include <exec/types.h>
#include <exec/semaphores.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "toy_lines.h"
#include "tarrasch_engine_util.h"

/* the search recurses deeply: libnix's startup sets this much stack up */
unsigned long __stack = 256 * 1024;
static const char toy_version[] __attribute__((used)) = "$VER: ToyEngine 0.908 Amiga (16.11.2026)";

static struct SignalSemaphore toy_lock;
static struct Task *toy_main;
static BYTE toy_cmd_sig = -1, toy_done_sig = -1;
static volatile int toy_queued;
static volatile int toy_reader_running;
static BPTR toy_in;
static toy_lines toy_split;

extern "C" unsigned long GetTickCount( void )
{
    static long base_days = -1;
    struct DateStamp ds;
    DateStamp( &ds );
    if( base_days < 0 )
        base_days = ds.ds_Days;
    return te_stamp_ms( ds.ds_Days, ds.ds_Minute, ds.ds_Tick, base_days );
}

/* one whole line: into the queue of 8 (waits while it is full) */
static int toy_queue_line( const char *line, void *user )
{
    (void)user;
    int quit = toy_is_word( line, "quit" );
    if( quit || toy_is_word( line, "stop" ) )
        gbl_stop = true;
    while( toy_queued >= 8 )
        Delay( 1 );
    strncpy( &cmdline[cmd_put][0], line, BIGBUF );
    cmdline[cmd_put][BIGBUF] = '\0';
    cprintf( "cmd[%s]>%s\n", log_time(), &cmdline[cmd_put][0] );
    cmd_put = ( cmd_put + 1 ) & 7;
    ObtainSemaphore( &toy_lock );
    toy_queued++;
    ReleaseSemaphore( &toy_lock );
    Signal( toy_main, 1UL << toy_cmd_sig );
    return !quit;
}

static void toy_reader( void )
{
    char buf[256];
    int more = 1;
    while( more )
    {
        LONG n = Read( toy_in, buf, sizeof buf );
        if( n <= 0 )
            break;
        more = toy_lines_feed( &toy_split, buf, (size_t)n, toy_queue_line, NULL );
    }
    if( more )
        toy_queue_line( "quit", NULL );     /* end of input */
    /* in Forbid() to the end: this code stays loaded until the process is gone */
    Forbid();
    toy_reader_running = 0;
    Signal( toy_main, 1UL << toy_done_sig );
}

int main( int argc, char *argv[] )
{
    (void)argc;
    (void)argv;
    InitSemaphore( &toy_lock );
    toy_main = FindTask( NULL );
    toy_in = Input();
    toy_cmd_sig = AllocSignal( -1 );
    toy_done_sig = AllocSignal( -1 );
    if( toy_cmd_sig < 0 || toy_done_sig < 0 || !toy_in )
        return 20;
    setvbuf( stdout, NULL, _IOLBF, 1024 );

    toy_reader_running = 1;
    struct Process *reader = CreateNewProcTags( NP_Entry, (ULONG)toy_reader,
                                                NP_Name, (ULONG)"ToyEngine input",
                                                NP_StackSize, 16384,
                                                NP_Priority, (LONG)toy_main->tc_Node.ln_Pri + 1,
                                                NP_Input, 0, NP_CloseInput, FALSE,
                                                NP_Output, 0, NP_CloseOutput, FALSE,
                                                TAG_DONE );
    if( !reader )
        return 20;

    bool quit = false;
    while( !quit )
    {
        Wait( 1UL << toy_cmd_sig );
        while( !quit && toy_queued > 0 )
        {
            quit = process( &cmdline[cmd_get][0] );
            cmd_get = ( cmd_get + 1 ) & 7;
            ObtainSemaphore( &toy_lock );
            toy_queued--;
            ReleaseSemaphore( &toy_lock );
        }
    }
    while( toy_reader_running )
        Wait( 1UL << toy_done_sig );
    if( file_log )
        fclose( file_log );
    return 0;
}
