# Copyright (C) 2026 The University of Sydney, Australia
#
# This file is part of GPlates.
#
# GPlates is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License, version 2, as published by
# the Free Software Foundation.
#
# GPlates is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


#
# Clearer messages when no overload of a pyGPlates function accepts the arguments of a call.
#
# Boost.Python then raises 'Boost.Python.ArgumentError' (a 'TypeError'), whose message lists the
# C++ signatures of the function's overloads:
#
#   Python argument types in
#       PolylineOnSphere.get_arc_length(PolylineOnSphere, str)
#   did not match C++ signature:
#       get_arc_length(class GPlatesMaths::PolylineOnSphere {lvalue})
#
# This replaces them with the signatures in the function's docstring, each followed by the types
# its ':type:' fields give the parameters:
#
#   Python argument types in
#       PolylineOnSphere.get_arc_length(PolylineOnSphere, str)
#   did not match this signature:
#       PolylineOnSphere.get_arc_length()
#
# A function whose docstring has no signature (a pickle-only constructor, say) gets the docstring's
# first paragraph instead, and one with no docstring (most special methods) keeps its C++
# signatures.
#
# The first line stays Boost.Python's, since code recognises the error by it: GPlately's
# 'rasterise()' passes its argument to 'FeaturesFunctionArgument' and, if the message starts with
# "Python argument types in", treats it as geometries instead. 1.1.0rc1 started it "Positional
# argument types in" (Boost.Python lists no keyword arguments), and that broke GPlately's
# seafloor gridding.
#
# It replaces the '__str__' of the exception class, so it costs nothing until a message is shown
# ('args[0]' keeps Boost.Python's message). The class belongs to the Boost.Python library, which
# other modules may share (in a conda environment; a wheel bundles its own copy), so a message is
# rewritten only if it names a pyGPlates function and its C++ signatures name the parameters that
# the function documents. Otherwise it is passed on unchanged.
#
# The alternatives were worse: a catch-all overload for every function (defined first, so that
# Boost.Python tries it last, when nothing else matches) touches every '.def' and adds a signature
# that the docstrings and the stub would have to hide; and a Python wrapper around every function
# adds a call to every call, including the hot ones. Boost.Python itself comes from conda-forge and
# the wheel images, so it can't be patched.
#
def _install_argument_error_messages():
    import re

    # Boost.Python creates the class when it raises its first argument error.
    try:
        Vector3D.get_magnitude(None)
    except TypeError as error:
        argument_error_class = type(error)
    else:
        argument_error_class = TypeError

    # Name the class 'pygplates.ArgumentError', so that it can be caught by name ('Boost.Python' is
    # not a module, so it can't be imported from there). It is the same class, not a subclass, since
    # Boost.Python raises its own class. Its '__module__' stays 'Boost.Python': other modules can
    # share the class (see above), and the name would be wrong for theirs. It is named before the
    # checks below, so that the documented name exists whatever Boost.Python raised.
    pygplates_dict = globals()
    pygplates_dict['ArgumentError'] = argument_error_class

    # Change the class only if it is Boost.Python's. Anything else could be Python's own 'TypeError'
    # (a built-in type, which can't be changed, and changing it would change every 'TypeError'), or
    # another library's class, whose messages the rewriting below doesn't know. (The tests would
    # notice this happening: they check the docstring and the rewritten messages.)
    if (argument_error_class.__module__ != 'Boost.Python' or
            argument_error_class.__name__ != 'ArgumentError'):
        return

    # Like '__str__' and '__reduce__' below, the docstring is set on the shared class, so another
    # module sharing it could replace any of them with its own (and we would replace its).
    argument_error_class.__doc__ = (
        "No form of a pyGPlates function or method accepts the arguments of a call (for example, an argument\n"
        "  of the wrong type, or too many arguments).\n"
        "\n"
        "  Its message lists the documented signatures of the function, each with the types of its parameters\n"
        "  (or, for a function that documents none, the first paragraph of its documentation).\n"
        "\n"
        "  It inherits from Python's ``TypeError`` (so an ``except TypeError`` handler also catches it), not from\n"
        "  :class:`GPlatesError`.\n"
        "\n"
        "  ::\n"
        "\n"
        "    try:\n"
        "        arc_length = polyline.get_arc_length('degrees')\n"
        "    except pygplates.ArgumentError as error:\n"
        "        print(error)\n"
        "\n"
        "  .. note:: Write ``pygplates.ArgumentError``, rather than ``from pygplates import ArgumentError``.\n"
        "     The class itself can't be pickled (its instances can), since its module, ``Boost.Python``, can't\n"
        "     be imported. So it can't be passed to another process (eg, with ``multiprocessing``), and joblib or\n"
        "     dask (which use cloudpickle) can't send a function defined in a script or notebook that refers to\n"
        "     ``ArgumentError`` by itself. A function that refers to ``pygplates.ArgumentError`` can be sent,\n"
        "     since only the ``pygplates`` module is sent then (by name).\n"
        "\n"
        "  .. note:: The class belongs to Boost.Python (the library that exposes pyGPlates' C++ code to Python),\n"
        "     so its type is shown as ``Boost.Python.ArgumentError``. Another extension module built with\n"
        "     Boost.Python raises the same class if it uses the same copy of the Boost.Python library as\n"
        "     pyGPlates (as can happen in a conda environment), so ``except pygplates.ArgumentError`` also\n"
        "     catches its argument errors.\n"
        "\n"
        "  .. versionadded:: 1.1\n")

    boost_python_function_type = type(Vector3D.get_magnitude)
    original_str = argument_error_class.__str__

    message_re = re.compile(
        r'Python argument types in\n'
        r'    (?P<qualified_name>[\w.]+)\((?P<argument_types>[^\n]*)\)\n'
        r'did not match C\+\+ signature:\n'
        r'(?P<cpp_signatures>.*)\Z',
        re.DOTALL)
    # The docstring forms that 'pygplates/stub/generate_stub.py' also reads ('_SIG_LINE_RE',
    # '_TYPE_FIELD_RE' and '_BULLET_TYPE_FIELD_RE' there; 'docs/pygplates/README.md' has them):
    # a column-0 signature line, 'name(arguments)'; and a ':type name: text' field, or a nested
    # class's '* **name** (text): ...' bullet.
    signature_line_re = re.compile(r'(\w+)\((.*)\)\s*\Z')
    type_field_res = (
        re.compile(r'\s*:type\s+([\w*]+):\s*(.*?)\s*\Z'),
        re.compile(r'\s*\*\s+\*\*(\w+)\*\*\s+\((.*?)\):\s'))
    # A cross-reference, eg ':class:`PointOnSphere`' or ':meth:`its name<Feature.get_name>`'.
    role_re = re.compile(r':\w+:`([^`]*)`')
    # Emphasis and literals, eg '*name*' and '``None``'.
    inline_markup_re = re.compile(r'\*\*|\*|``')
    # A C++ parameter that 'UnnamedArgumentPatcher' (in 'PyGPlatesModule.cc') named 'None'.
    unnamed_parameter_re = re.compile(r'(?<=\S) None(?=[,)])')

    def role_text(match):
        """The text Sphinx shows for a cross-reference."""
        content = match.group(1)
        title, _, target = content.partition('<')
        if target:
            return title.strip()
        if content.startswith('~'):
            return content.rsplit('.', 1)[-1]
        return content[len('pygplates.'):] if content.startswith('pygplates.') else content

    def find_function(qualified_name):
        """The pyGPlates function that Boost.Python's 'qualified_name' names, or None."""
        namespace_name, _, function_name = qualified_name.rpartition('.')
        namespace = pygplates_dict
        # A module function's is 'None' (Boost.Python couldn't get the module's name).
        if namespace_name != 'None':
            # A class, or a class nested in one ('NetworkTriangulation.Triangle').
            for class_name in namespace_name.split('.'):
                cls = namespace.get(class_name)
                if not (isinstance(cls, type) and cls.__module__.startswith('pygplates')):
                    return None
                namespace = cls.__dict__
        function = namespace.get(function_name)
        if isinstance(function, (staticmethod, classmethod)):
            function = function.__func__
        if type(function) is not boost_python_function_type or function.__name__ != function_name:
            return None
        return function

    def documented_signatures(function):
        """[(arguments, {parameter name: type})], for each signature that 'function' documents."""
        signatures = []
        for line in (function.__doc__ or '').split('\n'):
            match = signature_line_re.match(line)
            if match and match.group(1) == function.__name__:
                signatures.append((match.group(2), {}))
            elif signatures:
                for type_field_re in type_field_res:
                    match = type_field_re.match(line)
                    if match:
                        parameter_types = signatures[-1][1]
                        parameter_types.setdefault(
                                match.group(1).lstrip('*'), role_re.sub(role_text, match.group(2)))
                        break
        # A '__init__(...)' line introduces text about all the signatures.
        return [signature for signature in signatures if signature[0].strip() != '...']

    def parameter_names(arguments):
        """The parameter names in a signature's arguments, eg 'x' and 'y' in 'x, [y=(1, 2)]'."""
        names = []
        depth = 0
        quote = None
        argument = ''
        # A comma in a default value is inside parentheses or quotes.
        for char in arguments + ',':
            if quote:
                if char == quote:
                    quote = None
            elif char in '\'"':
                quote = char
            elif char == '(':
                depth += 1
            elif char == ')':
                depth -= 1
            elif char == ',' and depth == 0:
                name = argument.split('=', 1)[0].strip(' []*')
                if name:
                    names.append(name)
                argument = ''
                continue
            argument += char
        return names

    def first_paragraph(function):
        """The first paragraph of the docstring of 'function', skipping a 'name(...)' line."""
        lines = []
        for line in (function.__doc__ or '').split('\n'):
            line = line.strip()
            if line and not signature_line_re.match(line):
                lines.append(line)
            elif lines:
                break
        return inline_markup_re.sub('', role_re.sub(role_text, ' '.join(lines)))

    def rewrite(message):
        match = message_re.match(message)
        if not match:
            return None
        qualified_name = match.group('qualified_name')
        function = find_function(qualified_name)
        if function is None:
            return None
        signatures = documented_signatures(function)
        cpp_signatures = match.group('cpp_signatures')
        # Another module's function can have the same name, but its C++ signatures are unlikely to
        # also name our parameters. (Ours do, since a keyword argument works only by its C++ name.)
        for arguments, _ in signatures:
            for name in parameter_names(arguments):
                if not re.search(r'(?<=[\s&*])%s(?=[=,)])' % re.escape(name), cpp_signatures):
                    return None

        namespace_name, _, function_name = qualified_name.rpartition('.')
        if namespace_name == 'None':
            qualified_name = display_name = 'pygplates.' + function_name
        elif function_name == '__init__':
            # How a constructor is called, eg 'RotationModel(...)'.
            display_name = namespace_name
        else:
            display_name = qualified_name
        lines = [
            'Python argument types in',
            '    %s(%s)' % (qualified_name, match.group('argument_types'))]

        if signatures:
            lines.append('did not match %s:' % (
                    'this signature' if len(signatures) == 1 else 'any of these signatures'))
            for arguments, parameter_types in signatures:
                lines.append('    %s(%s)' % (display_name, arguments))
                for name in parameter_names(arguments):
                    if name in parameter_types:
                        lines.append('        %s: %s' % (name, parameter_types[name]))
        else:
            paragraph = first_paragraph(function)
            if paragraph:
                lines.append('did not match any signature:')
                lines.append('    ' + paragraph)
            else:
                lines.append('did not match C++ signature:')
                lines.append(unnamed_parameter_re.sub('', cpp_signatures))
        return '\n'.join(lines)

    def __str__(self):
        message = original_str(self)
        try:
            return rewrite(message) or message
        except Exception:
            # A failure here must not hide the error being reported.
            return message

    argument_error_class.__str__ = __str__

    def __reduce__(self):
        return _restore_argument_error, self.args, self.__dict__ or None

    argument_error_class.__reduce__ = __reduce__


#
# Pickling an 'ArgumentError' (as 'multiprocessing' does to return one from a worker process).
#
# By default pickle refers to the class by its module, 'Boost.Python', which can't be imported, so
# the worker reported only "Can't pickle <class 'Boost.Python.ArgumentError'>", and not the error.
# Instead '__reduce__' (above) refers to this function, which pickle finds as a module attribute
# (hence not nested in '_install_argument_error_messages').
#
def _restore_argument_error(*args):
    return ArgumentError(*args)

_install_argument_error_messages()
del _install_argument_error_messages
