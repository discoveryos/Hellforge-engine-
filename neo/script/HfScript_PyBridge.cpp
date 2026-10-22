/*
==========================================================================

Hellforge Power 1 Engine - CPython bridge implementation

Only compiled when HF_PYTHON is defined.  Embedds the real CPython
interpreter and exposes it to hfscript through host functions:

	python_eval( "expr" ) | python_exec( "stmt" )
	python_import( "module" ) | python_get( "module", "attr" )
	python_call( "module", "func", a, b, ... )

See HfScript_PyBridge.h for the type mapping.

==========================================================================
*/
#include "HfScript_PyBridge.h"

#if defined( HF_PYTHON )

#include <Python.h>
#include <stdarg.h>

static bool g_pyInitialized = false;
static bool g_pyHostsRegistered = false;
static PyObject * g_pyGlobals = NULL;			// __main__ module dict (persistent)

/*
=========================
Value conversion
=========================
*/

// hfscript -> PyObject
static PyObject * PyBridge_ToPy( const HfScriptValue & v ) {
	switch ( v.type ) {
		case HST_NIL:
			Py_RETURN_NONE;
		case HST_INT:
			return PyLong_FromLongLong( (long long)v.i );
		case HST_FLOAT:
			return PyFloat_FromDouble( (double)v.f );
		case HST_STRING:
			return PyUnicode_FromString( v.s != NULL ? v.s : "" );
		default:
			Py_RETURN_NONE;
	}
}

// PyObject -> hfscript; strings are copied into the VM string pool so they
// stay valid for the lifetime of the currently running program.
static HfScriptValue PyBridge_FromPy( HfScriptVM * vm, PyObject * obj ) {
	if ( obj == NULL || obj == Py_None ) {
		return HfScriptValue::Nil();
	}
	if ( PyBool_Check( obj ) ) {
		return HfScriptValue::Bool( obj == Py_True );
	}
	if ( PyLong_Check( obj ) ) {
		return HfScriptValue::Int( (int64)PyLong_AsLongLong( obj ) );
	}
	if ( PyFloat_Check( obj ) ) {
		return HfScriptValue::Float( (float)PyFloat_AsDouble( obj ) );
	}
	if ( PyUnicode_Check( obj ) ) {
		const char * text = PyUnicode_AsUTF8( obj );
		return HfScriptValue::Str( vm->NewString( text != NULL ? text : "" ) );
	}
	return HfScriptValue::Nil();
}

// Fetch the log output buffer if any was written, as a pooled string.
static HfScriptValue PyBridge_Result( HfScriptVM * vm, PyObject * obj ) {
	HfScriptValue r = PyBridge_FromPy( vm, obj );
	Py_XDECREF( obj );
	return r;
}

static PyObject * PyBridge_GetGlobals() {
	if ( g_pyGlobals == NULL ) {
		PyObject * mainMod = PyImport_AddModule( "__main__" );	// borrows
		g_pyGlobals = PyModule_GetDict( mainMod );				// borrows
	}
	return g_pyGlobals;
}

static void PyBridge_PrintPendingError() {
	if ( PyErr_Occurred() ) {
		PyErr_Print();			// prints traceback to stderr, clears the error
	}
}

/*
=========================
Host functions
=========================
*/

// python_eval( "expr" ) -> value
static void Host_python_eval( HfScriptVM * vm, int argc, HfScriptValue * outResult ) {
	*outResult = HfScriptValue::Nil();
	if ( argc < 1 || !g_pyInitialized ) {
		return;
	}
	const char * expr = "";
	if ( vm->GetArg( 0 ).type == HST_STRING ) {
		expr = vm->GetArg( 0 ).s != NULL ? vm->GetArg( 0 ).s : "";
	}
	PyObject * r = PyRun_String( expr, Py_eval_input, PyBridge_GetGlobals(), PyBridge_GetGlobals() );
	if ( r == NULL ) {
		PyBridge_PrintPendingError();
		return;
	}
	*outResult = PyBridge_Result( vm, r );
}

// python_exec( "stmt" ) -> nil
static void Host_python_exec( HfScriptVM * vm, int argc, HfScriptValue * outResult ) {
	*outResult = HfScriptValue::Nil();
	if ( argc < 1 || !g_pyInitialized ) {
		return;
	}
	const char * stmt = "";
	if ( vm->GetArg( 0 ).type == HST_STRING ) {
		stmt = vm->GetArg( 0 ).s != NULL ? vm->GetArg( 0 ).s : "";
	}
	PyObject * r = PyRun_String( stmt, Py_file_input, PyBridge_GetGlobals(), PyBridge_GetGlobals() );
	Py_XDECREF( r );
	if ( PyErr_Occurred() ) {
		PyBridge_PrintPendingError();
		return;
	}
}

// python_import( "module" ) -> module
static void Host_python_import( HfScriptVM * vm, int argc, HfScriptValue * outResult ) {
	*outResult = HfScriptValue::Nil();
	if ( argc < 1 || !g_pyInitialized ) {
		return;
	}
	const char * name = vm->GetArg( 0 ).s;
	PyObject * mod = PyImport_ImportModule( name );
	if ( mod == NULL ) {
		PyBridge_PrintPendingError();
		return;
	}
	// stash under module name in globals so subsequent eval() etc. can see it
	if ( PyDict_SetItemString( PyBridge_GetGlobals(), name, mod ) != 0 ) {
		PyBridge_PrintPendingError();
	}
	*outResult = PyBridge_Result( vm, mod );
}

// python_get( "module", "attr" ) -> value
static void Host_python_get( HfScriptVM * vm, int argc, HfScriptValue * outResult ) {
	*outResult = HfScriptValue::Nil();
	if ( argc < 2 || !g_pyInitialized ) {
		return;
	}
	const char * modName = vm->GetArg( 0 ).s;
	const char * attrName = vm->GetArg( 1 ).s;
	PyObject * mod = PyImport_ImportModule( modName );
	if ( mod == NULL ) {
		PyBridge_PrintPendingError();
		return;
	}
	PyObject * attr = PyObject_GetAttrString( mod, attrName );
	Py_DECREF( mod );
	if ( attr == NULL ) {
		PyBridge_PrintPendingError();
		return;
	}
	*outResult = PyBridge_Result( vm, attr );
}

// python_call( "module", "func", a, b, ... ) -> value
static void Host_python_call( HfScriptVM * vm, int argc, HfScriptValue * outResult ) {
	*outResult = HfScriptValue::Nil();
	if ( argc < 2 || !g_pyInitialized ) {
		return;
	}
	const char * modName = vm->GetArg( 0 ).s;
	const char * funcName = vm->GetArg( 1 ).s;
	PyObject * mod = PyImport_ImportModule( modName );
	if ( mod == NULL ) {
		PyBridge_PrintPendingError();
		return;
	}
	PyObject * fn = PyObject_GetAttrString( mod, funcName );
	Py_DECREF( mod );
	if ( fn == NULL || !PyCallable_Check( fn ) ) {
		Py_XDECREF( fn );
		PyBridge_PrintPendingError();
		return;
	}
	PyObject * args = PyTuple_New( argc - 2 );
	if ( args == NULL ) {
		Py_DECREF( fn );
		PyBridge_PrintPendingError();
		return;
	}
	int ok = 0;
	for ( int i = 2; i < argc; i++ ) {
		PyObject * a = PyBridge_ToPy( vm->GetArg( i ) );
		if ( PyTuple_SetItem( args, i - 2, a ) != 0 ) {
			ok = -1;
			PyBridge_PrintPendingError();
			break;
		}
	}
	if ( ok == 0 ) {
		PyObject * r = PyObject_CallObject( fn, args );
		if ( r != NULL ) {
			*outResult = PyBridge_Result( vm, r );
		} else {
			PyBridge_PrintPendingError();
		}
	}
	Py_DECREF( args );
	Py_DECREF( fn );
}

/*
=========================
Public API
=========================
*/
bool HfScript_PyBridge_Init() {
	if ( g_pyInitialized ) {
		return true;
	}
	Py_Initialize();
	g_pyInitialized = true;
	PyBridge_GetGlobals();
	return true;
}

void HfScript_PyBridge_Shutdown() {
	if ( !g_pyInitialized ) {
		return;
	}
	Py_Finalize();
	g_pyInitialized = false;
	g_pyGlobals = NULL;
}

void HfScript_PyBridge_RegisterHosts() {
	if ( !g_pyHostsRegistered ) {
		HfScript_RegisterHost( "python_eval", Host_python_eval );
		HfScript_RegisterHost( "python_exec", Host_python_exec );
		HfScript_RegisterHost( "python_import", Host_python_import );
		HfScript_RegisterHost( "python_get", Host_python_get );
		HfScript_RegisterHost( "python_call", Host_python_call );
		g_pyHostsRegistered = true;
	}
}

#endif /* HF_PYTHON */