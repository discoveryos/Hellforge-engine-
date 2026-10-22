/*
==========================================================================

Hellforge Power 1 Engine - CPython bridge

Links hfscript to a real embedded CPython interpreter so game scripts can
escape the tiny built-in dialect and use the full Python standard library
(strings, math, json, os, random, game-specific modules, ...).

The bridge is entirely optional.  To enable it define HF_PYTHON when
building (and link against -lpython3.x).  Without the define every
function here is a no-op stub and hfscript runs exactly as before.

Exposed to scripts as host functions (registered with HfScript_RegisterHost):

	python_eval( "expr" )                -> evaluates an expression, returns its value
	python_exec( "stmt" )                -> runs statements, returns nil
	python_import( "module" )            -> makes a module importable, returns the module
	python_get( "module", "attr" )       -> returns a module attribute
	python_call( "module", "func", a...) -> calls module.func(args...), returns result

Value conversion:
	HST_NIL    <-> None
	HST_BOOL   <-> bool
	HST_INT    <-> int
	HST_FLOAT  <-> float
	HST_STRING <-> str
Anything else returns None / nil.

Threading: CPython's GIL is held for the duration of each host call; the
interpreter uses the main thread, so no extra PyGILState juggling is done.
Call HfScript_PyBridge_Shutdown() during engine shutdown to finalize Python.

==========================================================================
*/
#ifndef __HFSCRIPTPYBRIDGE_H__
#define __HFSCRIPTPYBRIDGE_H__

#include "HfScript.h"

#if defined( HF_PYTHON )

// Initializes the embedded interpreter (idempotent).  Returns false if
// Python cannot be initialized.
bool			HfScript_PyBridge_Init();
void			HfScript_PyBridge_Shutdown();

// Registers the host functions (safe to call more than once).
void			HfScript_PyBridge_RegisterHosts();

#else

// Stubs when HF_PYTHON is not defined.
inline bool		HfScript_PyBridge_Init() { return false; }
inline void		HfScript_PyBridge_Shutdown() {}
inline void		HfScript_PyBridge_RegisterHosts() {}

#endif /* HF_PYTHON */

#endif /* !__HFSCRIPTPYBRIDGE_H__ */