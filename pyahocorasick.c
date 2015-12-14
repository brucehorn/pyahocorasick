/*
	This is part of pyahocorasick Python module.
	
	Python module.

	This file include all code from *.c files.

	Author    : Wojciech Muła, wojciech_mula@poczta.onet.pl
	License   : 3-clauses BSD (see LICENSE)
*/

#include "common.h"
#include "slist.h"
#include "trienode.h"
#include "trie.h"
#include "Automaton.h"
#include "AutomatonSearchIter.h"
#include "AutomatonItemsIter.h"

/* code */
#include "utils.c"
#include "trienode.c"
#include "trie.c"
#include "slist.c"
#include "Automaton.c"
#include "AutomatonItemsIter.c"
#include "AutomatonSearchIter.c"


static
PyMethodDef
ahocorasick_module_methods[] = {
	{NULL, NULL, 0, NULL}
};


#ifdef PY3K
static
PyModuleDef ahocorasick_module = {
	PyModuleDef_HEAD_INIT,
	"ahocorasick",
	"ahocorasick module",
	-1,
	ahocorasick_module_methods
};
#endif

#ifdef PY3K
#define init_function PyInit_ahocorasick
#else
#define init_function initahocorasick
#endif

PyMODINIT_FUNC
init_function(void) {
	PyObject* module;

	automaton_as_sequence.sq_length   = automaton_len;
	automaton_as_sequence.sq_contains = automaton_contains;

	automaton_type.tp_as_sequence = &automaton_as_sequence;
	
#ifdef PY3K
	module = PyModule_Create(&ahocorasick_module);
#else
    module = Py_InitModule("ahocorasick", ahocorasick_module_methods);
#endif
	if (module == NULL)
		return NULL;

	if (PyType_Ready(&automaton_type) < 0) {
		Py_DECREF(module);
		return NULL;
	}
	else
		PyModule_AddObject(module, "Automaton", (PyObject*)&automaton_type);

#define add_enum_const(name) PyModule_AddIntConstant(module, #name, name)
	add_enum_const(TRIE);
	add_enum_const(AHOCORASICK);
	add_enum_const(EMPTY);

	add_enum_const(STORE_LENGTH);
	add_enum_const(STORE_INTS);
	add_enum_const(STORE_ANY);

	add_enum_const(MATCH_EXACT_LENGTH);
	add_enum_const(MATCH_AT_MOST_PREFIX);
	add_enum_const(MATCH_AT_LEAST_PREFIX);
#undef add_enum_const

#ifdef AHOCORASICK_UNICODE
	PyModule_AddIntConstant(module, "unicode", 1);
#else
	PyModule_AddIntConstant(module, "unicode", 0);
#endif

	return module;
}
