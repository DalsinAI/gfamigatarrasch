/* What the Tarrasch Toy Engine takes from Windows, on AmigaOS: forced in
 * with -include for engine/ (engine-patches/0001).
 * Copyright (c) 2026 AmiCradle Team. MIT licence, like Tarrasch. */
#ifndef TOY_AMIGA_H
#define TOY_AMIGA_H

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif
unsigned long GetTickCount( void );     /* toy_amiga_io.inc */
#ifdef __cplusplus
}
#endif

typedef int errno_t;

static inline errno_t fopen_s( FILE **f, const char *name, const char *mode )
{
    *f = fopen( name, mode );
    return *f ? 0 : ( errno ? errno : ENOENT );
}

static inline errno_t localtime_s( struct tm *out, const time_t *t )
{
    struct tm *p = localtime( t );
    if( !p )
        return EINVAL;
    *out = *p;
    return 0;
}

static inline errno_t asctime_s( char *buf, size_t size, const struct tm *t )
{
    const char *s = asctime( t );
    if( !s || strlen( s ) >= size )
        return EINVAL;
    strcpy( buf, s );
    return 0;
}

static inline int toy_strcmpi( const char *a, const char *b )
{
    for( ;; )
    {
        int x = (unsigned char)*a++, y = (unsigned char)*b++;
        if( x >= 'A' && x <= 'Z' ) x += 'a' - 'A';
        if( y >= 'A' && y <= 'Z' ) y += 'a' - 'A';
        if( x != y || !x )
            return x - y;
    }
}
#define _strcmpi toy_strcmpi

#endif
