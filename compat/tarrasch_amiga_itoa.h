/* _itoa(), the MSVC call CompressMoves.cpp writes move numbers with:
 * value in radix 2 to 36 into buf, a '-' first for a negative radix 10
 * value, as MSVC does. Header only, for the host tests.
 * Copyright (c) 2026 AmiCradle Team. MIT licence, like Tarrasch. */
#ifndef TARRASCH_AMIGA_ITOA_H
#define TARRASCH_AMIGA_ITOA_H

static inline char *_itoa( int value, char *buf, int radix )
{
    char tmp[34];
    int n = 0, i = 0;
    unsigned int v;
    if( radix < 2 || radix > 36 )
    {
        buf[0] = '\0';
        return buf;
    }
    if( radix == 10 && value < 0 )
    {
        buf[i++] = '-';
        v = 0u - (unsigned int)value;
    }
    else
        v = (unsigned int)value;
    do
    {
        unsigned int d = v % (unsigned int)radix;
        tmp[n++] = (char)( d < 10 ? '0' + d : 'a' + d - 10 );
        v /= (unsigned int)radix;
    } while( v );
    while( n )
        buf[i++] = tmp[--n];
    buf[i] = '\0';
    return buf;
}

#endif
