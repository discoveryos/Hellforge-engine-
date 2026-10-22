/*
==========================================================================

Hellforge Power 1 Engine - hfscript Ruby-flavored front end

Like the Python front-end, this is purely a source tag: .rb files use the
same compiler/VM as .hfs.  The Ruby dialect supports:
  - # comments
  - def name( args ): body end
  - if / elsif / else / end
  - while / end
  - for i in range(n) / end
  - return, pass, break, continue
  - All standard hfscript operators

The tag affects error diagnostics (e.g., "ruby:5: syntax error").

==========================================================================
*/
#ifndef __HFSCRIPTRUBY_H__
#define __HFSCRIPTRUBY_H__

#include "HfScript.h"

bool HfScript_IsRubySource( const char * sourceName );

#endif /* !__HFSCRIPTRUBY_H__ */