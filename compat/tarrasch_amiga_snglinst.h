/* wxSingleInstanceChecker for Tarrasch while the GFOpenWX Toolkit has none
 * (wxUSE_SNGLINST_CHECKER 0): the first Tarrasch puts up a public message
 * port, and a second one finds it (tarrasch_amiga_engine.c). Only what
 * main.cpp uses: IsAnotherRunning(), which claims the name on first use as
 * wx's own does. With a toolkit that has the real one, this adds nothing.
 * Copyright (c) 2026 AmiCradle Team. MIT licence, like Tarrasch. */
#ifndef TARRASCH_AMIGA_SNGLINST_H
#define TARRASCH_AMIGA_SNGLINST_H

#include <wx/defs.h>

#if !wxUSE_SNGLINST_CHECKER
extern "C" {
int  tarrasch_instance_claim( const char *name );
void tarrasch_instance_release( void );
}

class wxSingleInstanceChecker
{
public:
    wxSingleInstanceChecker() {}
    ~wxSingleInstanceChecker() { if( m_claimed ) tarrasch_instance_release(); }
    bool Create( const wxString & ) { Claim(); return true; }
    bool CreateDefault() { Claim(); return true; }
    bool IsAnotherRunning() const
    {
        const_cast<wxSingleInstanceChecker *>( this )->Claim();
        return m_another;
    }

private:
    void Claim()
    {
        if( m_claimed )
            return;
        m_claimed = true;
        m_another = tarrasch_instance_claim( "Tarrasch.instance" ) != 0;
    }
    bool m_claimed = false;
    bool m_another = false;
};
#endif

#endif
