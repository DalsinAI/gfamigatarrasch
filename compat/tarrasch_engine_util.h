/* The parts of the AmigaOS engine link that need no AmigaOS: the byte ring
 * between the reader process and the GUI, the PIPE: channel names and the
 * command line that starts an engine. Header only, so the host tests take it
 * as it is (tests/test_compat.cpp).
 * Copyright (c) 2026 AmiCradle Team. MIT licence, like Tarrasch. */
#ifndef TARRASCH_ENGINE_UTIL_H
#define TARRASCH_ENGINE_UTIL_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A single reader, single writer byte ring. The caller holds a lock around
 * each call when the two ends run in different processes. */
#define TE_RING_SIZE 16384
typedef struct te_ring
{
    unsigned long head;     /* bytes put, ever */
    unsigned long tail;     /* bytes taken, ever */
    char buf[TE_RING_SIZE];
} te_ring;

static inline void te_ring_init( te_ring *r ) { r->head = r->tail = 0; }
static inline size_t te_ring_used( const te_ring *r ) { return (size_t)( r->head - r->tail ); }
static inline size_t te_ring_free( const te_ring *r ) { return TE_RING_SIZE - te_ring_used( r ); }

/* Puts up to n bytes; returns how many went in (fewer when the ring is full). */
static inline size_t te_ring_put( te_ring *r, const char *p, size_t n )
{
    size_t room = te_ring_free( r ), i;
    if( n > room )
        n = room;
    for( i = 0; i < n; i++ )
        r->buf[ ( r->head + i ) % TE_RING_SIZE ] = p[i];
    r->head += n;
    return n;
}

/* Takes up to max bytes; returns how many came out (0 when empty). */
static inline size_t te_ring_get( te_ring *r, char *p, size_t max )
{
    size_t n = te_ring_used( r ), i;
    if( n > max )
        n = max;
    for( i = 0; i < n; i++ )
        p[i] = r->buf[ ( r->tail + i ) % TE_RING_SIZE ];
    r->tail += n;
    return n;
}

/* "PIPE:tarrasch_<owner>_<n>_<dir>": one name per engine start, never
 * shared with another Tarrasch or an earlier engine of this one. Returns 0
 * when it does not fit. */
static inline int te_pipe_name( char *out, size_t size, unsigned long owner, unsigned n, const char *dir )
{
    int w = snprintf( out, size, "PIPE:tarrasch_%08lx_%u_%s", owner, n, dir );
    return w > 0 && (size_t)w < size;
}

/* The AmigaDOS command line for an engine: the program in quotes, so a
 * path with spaces works; a quote inside becomes *" and a star ** (the
 * AmigaDOS escapes inside quotes). Returns 0 when it does not fit or the
 * path is empty or has a line break. */
static inline int te_engine_line( char *out, size_t size, const char *path )
{
    size_t o = 0;
    const char *s;
    if( !path || !*path || size < 3 )
        return 0;
    out[o++] = '"';
    for( s = path; *s; s++ )
    {
        if( *s == '\n' || *s == '\r' )
            return 0;
        if( *s == '"' || *s == '*' )
        {
            if( o + 1 >= size )
                return 0;
            out[o++] = '*';
        }
        if( o + 1 >= size )
            return 0;
        out[o++] = *s;
    }
    if( o + 2 > size )
        return 0;
    out[o++] = '"';
    out[o] = '\0';
    return 1;
}

/* DateStamp fields to milliseconds since the first call's day: days,
 * minutes and ticks (50 a second). */
static inline unsigned long te_stamp_ms( long days, long minute, long tick, long base_days )
{
    return (unsigned long)( days - base_days ) * 86400000UL
         + (unsigned long)minute * 60000UL
         + (unsigned long)tick * 20UL;
}

/* Sleep(ms) as dos.library Delay() ticks: at least one, so a Sleep(0) in a
 * busy wait still lets an engine at a lower priority run. */
static inline long te_delay_ticks( int ms )
{
    long t = ms > 0 ? ( ms + 19 ) / 20 : 1;
    return t < 1 ? 1 : t;
}

#ifdef __cplusplus
}
#endif

#endif
