#!/usr/bin/env python

"""
Call every Boost.Python function in pygplates with keyword arguments that no overload accepts.

Each call must raise 'ArgumentError'. A method called through its class without its receiver
passed positionally (eg, 'pygplates.Vector3D.dot(vector1=..., vector2=...)') used to crash instead,
in Boost.Python's matching of keywords to an overload that leaves its receiver unnamed: see
'UnnamedArgumentPatcher' in 'src/api/PyGPlatesModule.cc'. Whether it crashed
depended on the interpreter build, so the probe runs on every wheel's interpreter too.

Run by 'KeywordArgumentCase' in 'test.py', in a process of its own so that a crash fails that test
rather than ending the run. Each function's name is printed before it is called, so after a crash
the last line names the function.

The fix patches the functions it finds by walking the module, so the probe finds them by walking
too (Boost.Python functions aren't tracked by the garbage collector, so there's no list of them
all). To check more than the fix's own walk, it enters every class it reaches, not only
pyGPlates' own.
"""

import inspect

import pygplates

# Raw functions ('bp::raw_function') accept any keywords, so a bogus one would run them.
RAW_FUNCTIONS = {'reconstruct', 'resolve_topologies'}

# More arguments than any pyGPlates function requires (the most that crashed was 7). An overload
# can only reach the crash with at least as many arguments as it has without a default.
MAX_KEYWORDS = 12


def boost_python_functions(module):
    """(dotted name, function) for every Boost.Python function reachable from 'module'."""
    functions = []
    visited_classes = set()

    def is_boost_python_function(obj):
        return type(obj).__module__ == 'Boost.Python' and type(obj).__name__ == 'function'

    def visit(name, obj):
        if isinstance(obj, (staticmethod, classmethod)):
            obj = obj.__func__
        if is_boost_python_function(obj):
            functions.append((name, obj))
        elif isinstance(obj, property):
            for accessor in ('fget', 'fset', 'fdel'):
                visit(name + '.' + accessor, getattr(obj, accessor))
        elif inspect.isclass(obj) and obj not in visited_classes:
            visited_classes.add(obj)
            for attribute_name, attribute in sorted(vars(obj).items()):
                visit(name + '.' + attribute_name, attribute)

    for name, obj in sorted(vars(module).items()):
        if name not in RAW_FUNCTIONS:
            visit(name, obj)
    return functions


def probe(function):
    """The first call of 'function' that doesn't raise 'ArgumentError', described, or None."""
    # No positional argument (the receiver, for a method), then one.
    for positional_arguments in ((), (object(),)):
        for num_keywords in range(1, MAX_KEYWORDS + 1):
            keyword_arguments = {'_unknown_%d' % index: 0 for index in range(num_keywords)}
            try:
                function(*positional_arguments, **keyword_arguments)
            except Exception as exc:
                if not isinstance(exc, pygplates.ArgumentError):
                    return 'raised %s: %s' % (type(exc).__name__, exc)
            else:
                return 'returned'
    return None


def main():
    # The extension module itself (the package's namespace leaves out its private names).
    functions = boost_python_functions(pygplates.pygplates)
    unexpected = []
    for name, function in functions:
        print('CALL %s' % name, flush=True)
        outcome = probe(function)
        if outcome is not None:
            unexpected.append('%s %s' % (name, outcome))
    print('PROBED %d' % len(functions))
    for line in unexpected:
        print('UNEXPECTED %s' % line)


if __name__ == '__main__':
    main()
