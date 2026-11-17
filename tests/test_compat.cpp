// Host tests for compat/: the engine link's ring, pipe names, command line,
// clock arithmetic, and the Toy Engine's line splitter.
// Copyright (c) 2026 AmiCradle Team. MIT licence, like Tarrasch.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "tarrasch_engine_util.h"
#include "toy_lines.h"
#include "toy_amiga.h"

static int fails = 0;
#define CHECK(c) do { if( !(c) ) { std::printf( "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c ); fails++; } } while( 0 )

static te_ring ring;

static void test_ring()
{
    char out[64];
    te_ring_init( &ring );
    CHECK( te_ring_used( &ring ) == 0 );
    CHECK( te_ring_get( &ring, out, sizeof out ) == 0 );
    CHECK( te_ring_put( &ring, "uciok\n", 6 ) == 6 );
    CHECK( te_ring_get( &ring, out, 3 ) == 3 && std::memcmp( out, "uci", 3 ) == 0 );
    CHECK( te_ring_get( &ring, out, sizeof out ) == 3 && std::memcmp( out, "ok\n", 3 ) == 0 );

    // full: only what fits goes in, the rest is left for the reader to retry
    std::vector<char> big( TE_RING_SIZE + 100, 'x' );
    CHECK( te_ring_put( &ring, big.data(), big.size() ) == TE_RING_SIZE );
    CHECK( te_ring_free( &ring ) == 0 );
    CHECK( te_ring_put( &ring, "y", 1 ) == 0 );

    // wrap round the end with the order kept
    std::vector<char> got( TE_RING_SIZE );
    CHECK( te_ring_get( &ring, got.data(), TE_RING_SIZE - 10 ) == TE_RING_SIZE - 10 );
    CHECK( te_ring_put( &ring, "bestmove e2e4\n", 14 ) == 14 );
    CHECK( te_ring_get( &ring, got.data(), got.size() ) == 24 );
    CHECK( std::memcmp( got.data(), "xxxxxxxxxxbestmove e2e4\n", 24 ) == 0 );
    CHECK( te_ring_used( &ring ) == 0 );
}

static void test_pipe_names()
{
    char n[64];
    CHECK( te_pipe_name( n, sizeof n, 0x0801a2b0UL, 1, "in" ) );
    CHECK( std::strcmp( n, "PIPE:tarrasch_0801a2b0_1_in" ) == 0 );
    CHECK( te_pipe_name( n, sizeof n, 0x0801a2b0UL, 12, "out" ) );
    CHECK( std::strcmp( n, "PIPE:tarrasch_0801a2b0_12_out" ) == 0 );
    char small[16];
    CHECK( !te_pipe_name( small, sizeof small, 1, 1, "in" ) );
}

static void test_engine_line()
{
    char l[64];
    CHECK( te_engine_line( l, sizeof l, "SYS:Utilities/Tarrasch/Engines/ToyEngine" ) );
    CHECK( std::strcmp( l, "\"SYS:Utilities/Tarrasch/Engines/ToyEngine\"" ) == 0 );
    CHECK( te_engine_line( l, sizeof l, "Work:Chess Engines/Fruit" ) );
    CHECK( std::strcmp( l, "\"Work:Chess Engines/Fruit\"" ) == 0 );
    CHECK( te_engine_line( l, sizeof l, "Work:a\"b*c" ) );
    CHECK( std::strcmp( l, "\"Work:a*\"b**c\"" ) == 0 );
    CHECK( !te_engine_line( l, sizeof l, "" ) );
    CHECK( !te_engine_line( l, sizeof l, nullptr ) );
    CHECK( !te_engine_line( l, sizeof l, "Work:x\nEcho bad" ) );
    char tight[6];
    CHECK( te_engine_line( tight, sizeof tight, "a:b" ) && std::strcmp( tight, "\"a:b\"" ) == 0 );
    CHECK( !te_engine_line( tight, sizeof tight, "a:bc" ) );
}

static void test_clock()
{
    CHECK( te_stamp_ms( 100, 0, 0, 100 ) == 0 );
    CHECK( te_stamp_ms( 100, 1, 50, 100 ) == 61000 );
    CHECK( te_stamp_ms( 101, 0, 1, 100 ) == 86400020UL );
    CHECK( te_delay_ticks( 0 ) == 1 );
    CHECK( te_delay_ticks( -5 ) == 1 );
    CHECK( te_delay_ticks( 1 ) == 1 );
    CHECK( te_delay_ticks( 20 ) == 1 );
    CHECK( te_delay_ticks( 21 ) == 2 );
    CHECK( te_delay_ticks( 1000 ) == 50 );
}

static std::vector<std::string> lines;
static int take( const char *line, void * )
{
    lines.push_back( line );
    return !toy_is_word( line, "quit" );
}

static void test_lines()
{
    static toy_lines l;
    l.len = 0;
    lines.clear();
    const char *a = "uci\r\nisre";
    CHECK( toy_lines_feed( &l, a, std::strlen( a ), take, nullptr ) == 1 );
    CHECK( lines.size() == 1 && lines[0] == "uci" );
    const char *b = "ady\nposition startpos moves e2e4\n";
    CHECK( toy_lines_feed( &l, b, std::strlen( b ), take, nullptr ) == 1 );
    CHECK( lines.size() == 3 && lines[1] == "isready" && lines[2] == "position startpos moves e2e4" );
    const char *c = "go infinite\n QUIT \nstop\n";
    CHECK( toy_lines_feed( &l, c, std::strlen( c ), take, nullptr ) == 0 );   // stops at quit
    CHECK( lines.size() == 5 && lines[4] == " QUIT " );

    // a line longer than the buffer is cut, and the next one is whole
    l.len = 0;
    lines.clear();
    std::string longl( TOY_LINE_MAX + 50, 'a' );
    longl += "\nstop\n";
    CHECK( toy_lines_feed( &l, longl.data(), longl.size(), take, nullptr ) == 1 );
    CHECK( lines.size() == 2 && lines[0].size() == TOY_LINE_MAX && lines[1] == "stop" );

    CHECK( toy_is_word( "stop", "stop" ) );
    CHECK( toy_is_word( "  Stop\t", "stop" ) );
    CHECK( !toy_is_word( "stopx", "stop" ) );
    CHECK( !toy_is_word( "sto", "stop" ) );
    CHECK( !toy_is_word( "go stop", "stop" ) );
}

static void test_toy_shims()
{
    char sq[8];
    CHECK( strcpy_s( sq, sizeof sq, "rnbqkbn" ) == 0 && std::strcmp( sq, "rnbqkbn" ) == 0 );
    CHECK( strcpy_s( sq, sizeof sq, "rnbqkbnr" ) != 0 && sq[0] == '\0' );   // no room for the end
    CHECK( strcpy_s( sq, sizeof sq, nullptr ) != 0 );
    CHECK( _strcmpi( "QUIT", "quit" ) == 0 );
    CHECK( _strcmpi( "stop", "quit" ) != 0 );
    CHECK( _strcmpi( "go", "goo" ) < 0 );
    FILE *f = nullptr;
    CHECK( fopen_s( &f, "/nonexistent/dir/toy.log", "r" ) != 0 && f == nullptr );
    time_t t = 0;
    struct tm tmv;
    char buf[64];
    CHECK( localtime_s( &tmv, &t ) == 0 );
    CHECK( asctime_s( buf, sizeof buf, &tmv ) == 0 && std::strchr( buf, '\n' ) );
    char tiny[4];
    CHECK( asctime_s( tiny, sizeof tiny, &tmv ) != 0 );
}

int main()
{
    test_ring();
    test_pipe_names();
    test_engine_line();
    test_clock();
    test_lines();
    test_toy_shims();
    if( fails )
        std::printf( "%d failed\n", fails );
    else
        std::printf( "compat: all checks passed\n" );
    return fails != 0;
}
