/* Splits what arrives on the Toy Engine's input into command lines: one
 * line per '\n', a '\r' before it dropped, a line longer than the buffer
 * cut at the buffer's end. Header only, for the host tests.
 * Copyright (c) 2026 AmiCradle Team. MIT licence, like Tarrasch. */
#ifndef TOY_LINES_H
#define TOY_LINES_H

#include <stddef.h>

#define TOY_LINE_MAX 8190

typedef struct toy_lines
{
    size_t len;
    char line[TOY_LINE_MAX + 2];
} toy_lines;

typedef int (*toy_line_fn)( const char *line, void *user );

/* Feeds n bytes; calls fn for each whole line. Returns 0 at once when fn
 * returns 0 (the engine was told to quit), else 1. */
static inline int toy_lines_feed( toy_lines *l, const char *p, size_t n, toy_line_fn fn, void *user )
{
    size_t i;
    for( i = 0; i < n; i++ )
    {
        char c = p[i];
        if( c == '\n' )
        {
            if( l->len && l->line[l->len - 1] == '\r' )
                l->len--;
            l->line[l->len] = '\0';
            l->len = 0;
            if( !fn( l->line, user ) )
                return 0;
        }
        else if( l->len < TOY_LINE_MAX )
            l->line[l->len++] = c;
    }
    return 1;
}

/* Case-blind: is line the word w, alone ("quit", "stop")? */
static inline int toy_is_word( const char *line, const char *w )
{
    while( *line == ' ' || *line == '\t' )
        line++;
    while( *w )
    {
        char a = *line++, b = *w++;
        if( a >= 'A' && a <= 'Z' ) a = (char)( a - 'A' + 'a' );
        if( a != b )
            return 0;
    }
    while( *line == ' ' || *line == '\t' )
        line++;
    return *line == '\0';
}

#endif
