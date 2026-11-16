/* Tarrasch's UCI engine link on AmigaOS: the engine runs as its own
 * AmigaDOS process, joined to Tarrasch by two PIPE: channels; a small
 * reader process drains the engine's replies so the GUI never blocks.
 * Stands in for pipe(), fork(), execlp(), select() and read() in
 * UnixUciInterface.cpp (patches/0002).
 * Copyright (c) 2026 AmiCradle Team. MIT licence, like Tarrasch. */
#ifndef TARRASCH_AMIGA_ENGINE_H
#define TARRASCH_AMIGA_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Starts the engine at path (an AmigaDOS path), lower_pri (0 to 3) below
 * Tarrasch's own priority: Tarrasch asks 1 for normal, 2 for low and 3 for
 * idle. Returns 1 when it started. One engine at a time: an earlier one is
 * let go first. */
int  tarrasch_engine_start( const char *path, int lower_pri );

/* Up to max bytes of what the engine wrote since the last call; 0 when
 * there is nothing (never waits). */
int  tarrasch_engine_read( char *buf, int max );

/* Sends s to the engine. */
void tarrasch_engine_write( const char *s );

/* Lets the engine go: closes its input (it sees end of file), waits for
 * the reader to finish. Also run at exit. */
void tarrasch_engine_stop( void );

/* GetTickCount() and Sleep() for Tarrasch on AmigaOS. */
unsigned long tarrasch_ticks_ms( void );
void tarrasch_sleep_ms( int ms );

#ifdef __cplusplus
}
#endif

#endif
