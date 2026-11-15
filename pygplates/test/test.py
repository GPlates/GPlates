#!/usr/bin/env python

"""
Unit tests for the pygplates native library.
"""

import os
import re
import sys
import pickle
import platform
import subprocess
import types
import unittest

# If on Windows and Python >= 3.8 and not in a conda environment then we need to specify the paths to all dependency DLLs of the
# pygplates '.pyd' since Python 3.8 no longer searches for these using the PATH environment variable.
#
# Note: Conda already ensures dependencies are loaded correctly.
if platform.system() == 'Windows' and sys.version_info >= (3, 8) and not os.environ.get('CONDA_PREFIX'):
    # Just add all the paths in the PATH environment variable.
    env_path = os.environ.get('PATH')
    if env_path:
        # Note that we reverse the order of PATH since 'os.add_dll_directory()'
        # appears to insert to the front of the DLL search order.
        for dll_path in reversed(env_path.split(os.pathsep)):
            if dll_path and os.path.exists(dll_path):
                os.add_dll_directory(os.path.abspath(dll_path))

import pygplates

import test_app_logic.test
import test_maths.test
import test_model.test

# Fixture path
FIXTURES = os.path.join(os.path.dirname(__file__), 'fixtures')


class HashableCase(unittest.TestCase):
    def test_hash_value(self):
        # We create clones in order that 'id()' returns a different value (even though they compare equal).
        feature_id1 = pygplates.FeatureId.create_unique_id()
        feature_id2 = pygplates.FeatureId.create_unique_id()
        feature_type = pygplates.FeatureType.create_gpml('FeatureType')
        feature_type_clone = pygplates.FeatureType.create_from_qualified_string(feature_type.to_qualified_string())
        enumeration_type = pygplates.EnumerationType.create_gpml('EnumerationType')
        enumeration_type_clone = pygplates.EnumerationType.create_from_qualified_string(enumeration_type.to_qualified_string())
        property_name = pygplates.PropertyName.create_gpml('PropertyName')
        property_name_clone = pygplates.PropertyName.create_from_qualified_string(property_name.to_qualified_string())

        # Should be able to insert hashable types into dictionary without raising TypeError.
        # And also retrieve them properly (defining __eq__ without __hash__ makes a type
        # unhashable).
        d = {}
        
        # Insert originals.
        d[feature_id1] = 'test1'
        d[feature_id2] = 'test2'
        d[feature_type] = 'FeatureType value'
        d[enumeration_type] = 'EnumerationType value'
        d[property_name] = 'PropertyName value'
        
        # Test using clones where possible (since clone gives different value for 'id()').
        self.assertTrue(d[feature_id1] == 'test1')
        self.assertTrue(d[feature_id2] == 'test2')
        self.assertTrue(d[feature_type_clone] == 'FeatureType value')
        self.assertTrue(d[enumeration_type_clone] == 'EnumerationType value')
        self.assertTrue(d[property_name_clone] == 'PropertyName value')

    def test_hash_identity(self):
        # Test some classes that hash using object identity (address).
        # Pretty much the majority of pygplates classes will fall in this category
        # (except for the large number of property value types).
        d = {}
        
        feature = pygplates.Feature()
        feature_ref = pygplates.FeatureCollection([feature]).get(feature.get_feature_id())
        d[feature] = 'Feature'
        self.assertTrue(d[feature_ref] == d[feature])

    def test_unhashable(self):
        d = {}
        
        # Make sure the following types are unhashable (ie, __hash__ has been set to None).
        self.assertRaises(TypeError, d.get, pygplates.GeoTimeInstant(0))
        self.assertRaises(TypeError, d.get, pygplates.FiniteRotation((0,0), 0))
        # All property value types are unhashable (due to base PropertyValue defining __hash__ to None)
        # So just test one derived property value type (since the unhashable-ness is done in the base PropertyValue class anyway).
        self.assertRaises(TypeError, d.get, pygplates.XsInteger(0))
        self.assertRaises(TypeError, d.get, pygplates.Property(pygplates.PropertyName.create_gpml('name'), pygplates.XsInteger(0)))
        self.assertRaises(TypeError, d.get, pygplates.GpmlTimeSample(pygplates.XsInteger(0), 10))
        self.assertRaises(TypeError, d.get, pygplates.GpmlTimeWindow(pygplates.XsInteger(0), 20, 10))
        self.assertRaises(TypeError, d.get, pygplates.PointOnSphere((0,0)))
        self.assertRaises(TypeError, d.get, pygplates.MultiPointOnSphere([(0,0), (0,1)]))
        self.assertRaises(TypeError, d.get, pygplates.PolylineOnSphere([(0,0), (0,1)]))
        self.assertRaises(TypeError, d.get, pygplates.PolygonOnSphere([(0,0), (0,1), (1,0)]))
        self.assertRaises(TypeError, d.get, pygplates.GreatCircleArc((0,0), (0,1)))
        self.assertRaises(TypeError, d.get, pygplates.LatLonPoint(0, 0))


class VersionCase(unittest.TestCase):
    def test_version(self):
        self.assertTrue(pygplates.Version(0, 1) < pygplates.Version.get_imported_version())
        self.assertTrue(pygplates.Version(0, 21) > pygplates.Version(0, 20))
        self.assertTrue(pygplates.Version(0, 34, release_suffix='.dev1') < pygplates.Version(0, 34))
        self.assertTrue(pygplates.Version(0, 34, release_suffix='.dev1') > pygplates.Version(0, 33))
        self.assertTrue(pygplates.Version(0, 34, release_suffix='.dev1') == pygplates.Version('0.34.dev1'))
        self.assertTrue(pygplates.Version(0, 34, 0, '.dev1') == pygplates.Version('0.34.dev1'))
        self.assertTrue(pygplates.Version('0.34.dev1') == pygplates.Version(0, 34, 0, '.dev1'))
        self.assertTrue(pygplates.Version('0.34.dev1') < pygplates.Version(0, 34))
        self.assertTrue(pygplates.Version('0.34.1.dev1') > pygplates.Version(0, 34))
        self.assertTrue(pygplates.Version('0.34.1.dev1') < pygplates.Version(0, 34, 1))
        self.assertTrue(pygplates.Version('0.34.dev1') < pygplates.Version('0.34a1'))
        self.assertTrue(pygplates.Version('0.34a1') < pygplates.Version('0.34a2'))
        self.assertTrue(pygplates.Version('0.34a2') < pygplates.Version('0.34b1'))
        self.assertTrue(pygplates.Version('0.34b1') < pygplates.Version('0.34rc1'))
        self.assertTrue(pygplates.Version('0.34rc1') < pygplates.Version('0.34'))
        self.assertTrue(pygplates.Version('0.34.post0') < pygplates.Version('0.34.post1'))
        self.assertTrue(pygplates.Version('0.34rc0') > pygplates.Version('0.34.dev0'))
        self.assertTrue(pygplates.Version('0.34.post0') > pygplates.Version(0, 34))
        self.assertTrue(pygplates.Version('1.34.post0.dev1') < pygplates.Version(1, 34, release_suffix='.post0'))
        self.assertTrue(pygplates.Version('1.34rc1.post0.dev1') > pygplates.Version(1, 34, release_suffix='rc1'))
        self.assertTrue(pygplates.Version('1.34rc1.post1.dev1') > pygplates.Version(1, 34, release_suffix='rc1.post0'))
        self.assertTrue(pygplates.Version('1.34rc1.dev1') < pygplates.Version(1, 34, release_suffix='rc1.dev2'))
        self.assertTrue(pygplates.Version('1.34rc1.dev2') < pygplates.Version(1, 34, release_suffix='rc1'))
        
        self.assertTrue(pygplates.Version('0.33').get_major() == 0)
        self.assertTrue(pygplates.Version('0.33').get_minor() == 33)
        self.assertTrue(pygplates.Version('1.33').get_major() == 1)
        self.assertTrue(pygplates.Version('0.33.1').get_patch() == 1)
        self.assertTrue(pygplates.Version('0.33.1').get_release_suffix() is None)
        self.assertTrue(pygplates.Version('1.33.2.dev1').get_patch() == 2)
        self.assertTrue(pygplates.Version('1.33.2.dev1').get_release_suffix() == ".dev1")
        self.assertTrue(pygplates.Version('1.33.dev1').get_patch() == 0)
        self.assertTrue(pygplates.Version('1.33.dev1').get_release_suffix() == ".dev1")
        with self.assertRaises(ValueError):
            pygplates.Version('1.33.2dev1')  # Should be "1.33.2.dev1"
        self.assertTrue(pygplates.Version('1.33.2a1').get_patch() == 2)
        self.assertTrue(pygplates.Version('1.33.2a1').get_release_suffix() == "a1")
        self.assertTrue(pygplates.Version('1.33a1').get_patch() == 0)
        self.assertTrue(pygplates.Version('1.33a1').get_release_suffix() == "a1")
        with self.assertRaises(ValueError):
            pygplates.Version('1.33.2.a1')  # Should be "1.33.2a1"
        self.assertTrue(pygplates.Version('1.33.2b2').get_patch() == 2)
        self.assertTrue(pygplates.Version('1.33.2b2').get_release_suffix() == "b2")
        self.assertTrue(pygplates.Version('1.33b2').get_patch() == 0)
        self.assertTrue(pygplates.Version('1.33b2').get_release_suffix() == "b2")
        with self.assertRaises(ValueError):
            pygplates.Version('1.33.2.b2')  # Should be "1.33.2b2"
        self.assertTrue(pygplates.Version('1.33.2rc3').get_patch() == 2)
        self.assertTrue(pygplates.Version('1.33.2rc3').get_release_suffix() == "rc3")
        self.assertTrue(pygplates.Version('1.33rc3').get_patch() == 0)
        self.assertTrue(pygplates.Version('1.33rc3').get_release_suffix() == "rc3")
        with self.assertRaises(ValueError):
            pygplates.Version('1.33.2.rc3')  # Should be "1.33.2rc3"
        with self.assertRaises(ValueError):
            pygplates.Version('1.33.rc3')  # Should be "1.33rc3"
        version = pygplates.Version('1.33.2.post1')
        self.assertTrue(version.get_major() == 1 and version.get_minor() == 33 and version.get_patch() == 2 and version.get_release_suffix() == '.post1')
        version = pygplates.Version('1.33.2.post1.dev1')
        self.assertTrue(version.get_major() == 1 and version.get_minor() == 33 and version.get_patch() == 2 and version.get_release_suffix() == '.post1.dev1')
        version = pygplates.Version('1.33.2a2.dev0')
        self.assertTrue(version.get_major() == 1 and version.get_minor() == 33 and version.get_patch() == 2 and version.get_release_suffix() == 'a2.dev0')
        version = pygplates.Version('1.33.2rc1.post1.dev0')
        self.assertTrue(version.get_major() == 1 and version.get_minor() == 33 and version.get_patch() == 2 and version.get_release_suffix() == 'rc1.post1.dev0')
        with self.assertRaises(ValueError):
            pygplates.Version('1.33.2rc1.dev0.post1')  # Should be "1.33.2rc1.post1.dev0"
            pygplates.Version('1.33.2.post1rc1.dev0')  # Should be "1.33.2rc1.post1.dev0"
        
        # Test deprecated functions
        self.assertTrue(pygplates.Version(20).get_revision() == 20)  # 'get_revision()' is deprecated
        self.assertTrue(pygplates.Version(20).get_major() == 0)  # 'Version(revision)' is deprecated
        self.assertTrue(pygplates.Version(20).get_minor() == 20)  # 'Version(revision)' is deprecated
        self.assertTrue(pygplates.Version(20).get_patch() == 0)  # 'Version(revision)' is deprecated
        self.assertTrue(pygplates.Version(20).get_prerelease_suffix() is None)  # 'Version(revision)' and 'get_prerelease_suffix()' are deprecated
        self.assertTrue(pygplates.Version(20) < pygplates.Version(21))  # 'Version(revision)' is deprecated
        self.assertTrue(pygplates.Version(20) < pygplates.Version(0, 21))  # 'Version(revision)' is deprecated
        self.assertTrue(pygplates.Version(1, 0, prerelease_suffix='.dev1') == pygplates.Version('1.0.dev1'))  # 'prerelease_suffix' keyword deprecated
        self.assertTrue(pygplates.Version(1, 0, 0, '.dev1', None) == pygplates.Version('1.0.dev1'))  # actually two keywords deprecated 'prerelease_suffix' and new 'release_suffix'
        self.assertTrue(pygplates.Version(1, 0, 0, None, '.dev1') == pygplates.Version('1.0.dev1'))  # actually two keywords deprecated 'prerelease_suffix' and new 'release_suffix'
        with self.assertRaises(ValueError):
            pygplates.Version(1, 0, 0, '.dev1', '.dev1')  # can't specify both deprecated 'prerelease_suffix' and new 'release_suffix'

    def test_pickle(self):
        for version in (pygplates.Version(0, 1), pygplates.Version('0.34.dev1'), pygplates.Version('0.34a2'), pygplates.Version('0.34b1'), pygplates.Version('0.34rc1'),
                        pygplates.Version('1.0.1.post0'), pygplates.Version('1.0.1b1.post0'), pygplates.Version('1.0.1a0.dev0'), pygplates.Version('1.0.1rc0.post0.dev0')):
            self.assertTrue(version == pickle.loads(pickle.dumps(version)))


class ArgumentErrorCase(unittest.TestCase):
    """
    When no overload accepts the arguments of a call, 'ArgumentError' lists the documented
    signatures, not Boost.Python's C++ ones (see 'src/qt-resources/python/api/ArgumentErrors.py').
    """

    def _message(self, function, *arguments):
        with self.assertRaises(TypeError) as context:
            function(*arguments)
        self.assertEqual(type(context.exception).__name__, 'ArgumentError')
        return str(context.exception)

    def test_method(self):
        polyline = pygplates.PolylineOnSphere([(0, 0), (0, 10)])
        self.assertEqual(
                self._message(polyline.get_arc_length, 'x'),
                'Positional argument types in\n'
                '    PolylineOnSphere.get_arc_length(PolylineOnSphere, str)\n'
                'did not match this signature:\n'
                '    PolylineOnSphere.get_arc_length()')

    def test_overloaded_constructor(self):
        message = self._message(pygplates.FiniteRotation, (0, 0), 'x')
        self.assertTrue(message.startswith(
                'Positional argument types in\n'
                '    FiniteRotation.__init__(FiniteRotation, tuple, str)\n'
                'did not match any of these signatures:\n'
                '    FiniteRotation(pole, angle_radians)\n'
                '        pole: '), message)
        self.assertIn(
                '\n        angle_radians: float\n    FiniteRotation(from_point, to_point)\n', message)
        self.assertTrue(message.endswith('\n    FiniteRotation()'), message)
        # Not the pickle-only constructor, which isn't documented.
        self.assertNotIn('Bytes', message)

    def test_functions(self):
        # A module function (which Boost.Python names 'None.calculate_velocities').
        message = self._message(pygplates.calculate_velocities, 'x', 1, 2)
        self.assertTrue(message.startswith(
                'Positional argument types in\n'
                '    pygplates.calculate_velocities(str, int, int)\n'
                'did not match this signature:\n'
                '    pygplates.calculate_velocities(domain_points, finite_rotation, '), message)
        # A static method.
        self.assertTrue(self._message(pygplates.FeatureType.create_gpml, 1).endswith(
                '\n    FeatureType.create_gpml(name)\n        name: str'))
        # A method of a nested class, whose parameters are documented in a list.
        message = self._message(pygplates.NetworkTriangulation.Triangle.get_vertex, 1, 2)
        self.assertTrue(message.endswith(
                '\n    NetworkTriangulation.Triangle.get_vertex(index)\n        index: int'), message)

    def test_without_documented_signature(self):
        # The docstring's first paragraph.
        self.assertTrue(self._message(pygplates.FeatureType, 'gpml:Coastline').endswith(
                '\ndid not match any signature:\n'
                '    You cannot directly instantiate this class from Python.'))
        # Its markup removed (it's 'See :meth:`create_normalised`.').
        self.assertTrue(self._message(pygplates.Vector3D.create_normalized).endswith(
                '\n    See create_normalised.'))
        # No docstring: the C++ signatures.
        self.assertIn(
                '\ndid not match C++ signature:\n', self._message(pygplates.PolylineOnSphere.__len__, 1))
        # Without the 'None' that 'UnnamedArgumentPatcher' (in 'PyGPlatesModule.cc') names the
        # receiver (which 'args[0]', Boost.Python's message, still has).
        with self.assertRaises(TypeError) as context:
            pygplates.ReconstructionTreeBuilder.build_reconstruction_tree(1, 2, 3, 4)
        self.assertIn('\ndid not match C++ signature:\n', str(context.exception))
        self.assertNotIn(' None,', str(context.exception))
        self.assertIn(' None,', context.exception.args[0])

    def test_other_messages_unchanged(self):
        # Other Boost.Python modules can share the class (in a conda environment).
        try:
            pygplates.Vector3D.get_magnitude(None)
        except TypeError as error:
            argument_error_class = type(error)
        for message in (
                'Python argument types in\n    Molecule.get_atoms(Molecule, int)\n'
                'did not match C++ signature:\n    get_atoms(class Molecule {lvalue} self)',
                'Python argument types in\n    None.parse_formula(int)\n'
                'did not match C++ signature:\n    parse_formula(std::string formula)',
                'Python argument types in\n    PolylineOnSphere.get_atoms(PolylineOnSphere, int)\n'
                'did not match C++ signature:\n    get_atoms(class Molecule {lvalue} self)',
                # The name of a pyGPlates function, but not the names of its parameters.
                'Python argument types in\n    None.calculate_velocities(int)\n'
                'did not match C++ signature:\n    calculate_velocities(int x)',
                'not a Boost.Python message'):
            self.assertEqual(str(argument_error_class(message)), message)

    def test_every_function(self):
        # Boost.Python checks the number of arguments before converting any, so too many raises
        # 'ArgumentError' without running the function. ('RAW_FUNCTIONS' accept any number.)
        import keyword_argument_probe
        functions = keyword_argument_probe.boost_python_functions(pygplates.pygplates)
        unexpected = []
        num_rewritten = 0
        for name, function in functions:
            try:
                function(*[None] * 30)
            except TypeError as error:
                message = str(error)
            else:
                unexpected.append('%s returned' % name)
                continue
            cpp_signatures = message.partition('\ndid not match C++ signature:\n')[2]
            if not cpp_signatures:
                num_rewritten += 1
            elif (function.__doc__ or '').strip():
                # A function with a docstring has its message rewritten. (Only if each parameter
                # it documents is named in its C++ signatures, which is also what a keyword
                # argument needs: see 'rewrite()' in 'ArgumentErrors.py'.)
                unexpected.append('%s: %s' % (name, message))
            elif re.search(r'\S None[,)]', cpp_signatures):
                # A receiver named 'None' by 'UnnamedArgumentPatcher' (in 'PyGPlatesModule.cc').
                unexpected.append('%s: %s' % (name, message))
        self.assertEqual(unexpected, [])
        # Every documented function (645 of the 1739 functions and accessors when this was written;
        # the rest are mostly special methods).
        self.assertGreater(num_rewritten, 600)


class KeywordArgumentCase(unittest.TestCase):
    """
    Documented argument names work as keyword arguments (they once raised 'ArgumentError'), and
    keywords that no overload accepts raise 'ArgumentError' (they could crash).
    """

    def test_geometry(self):
        arc = pygplates.GreatCircleArc((0, 0), (0, 10))
        self.assertEqual(arc.get_arc_point(normalised_distance_from_start_point=0.5), arc.get_arc_point(0.5))
        self.assertEqual(arc.get_arc_direction(normalised_distance_from_start_point=0.5), arc.get_arc_direction(0.5))
        self.assertEqual(
                pygplates.FiniteRotation.create_great_circle_point_rotation(from_point=(0, 0), to_point=(0, 10)),
                pygplates.FiniteRotation.create_great_circle_point_rotation((0, 0), (0, 10)))
        self.assertEqual(
                pygplates.FiniteRotation.create_small_circle_point_rotation(rotation_pole=(90, 0), from_point=(0, 0), to_point=(0, 10)),
                pygplates.FiniteRotation.create_small_circle_point_rotation((90, 0), (0, 0), (0, 10)))
        self.assertEqual(
                pygplates.FiniteRotation.create_segment_rotation(
                        from_segment_start=(0, 0), from_segment_end=(0, 10), to_segment_start=(10, 0), to_segment_end=(10, 10)),
                pygplates.FiniteRotation.create_segment_rotation((0, 0), (0, 10), (10, 0), (10, 10)))

    def test_qualified_names(self):
        self.assertEqual(pygplates.PropertyName.create_gpml(name='x'), pygplates.PropertyName.create_gpml('x'))
        self.assertEqual(pygplates.PropertyName.create_gml(name='x'), pygplates.PropertyName.create_gml('x'))
        self.assertEqual(pygplates.FeatureType.create_gpml(name='Isochron'), pygplates.FeatureType.gpml_isochron)
        self.assertEqual(pygplates.ScalarType.create_xsi(name='double'), pygplates.ScalarType.create_xsi('double'))
        self.assertEqual(pygplates.PropertyName.create_from_qualified_string(name='gml:name'), pygplates.PropertyName.gml_name)

    def test_property_value_visitor(self):
        class Visitor(pygplates.PropertyValueVisitor):
            def __init__(self):
                super(Visitor, self).__init__()
                self.visited = []
            def visit_xs_double(self, xs_double):
                self.visited.append(xs_double)
        visitor = Visitor()
        # The overridden method and the default implementation both accept the keyword.
        visitor.visit_xs_double(xs_double=pygplates.XsDouble(1))
        visitor.visit_gpml_finite_rotation(gpml_finite_rotation=pygplates.GpmlFiniteRotation(pygplates.FiniteRotation()))
        pygplates.PropertyValueVisitor().visit_xs_double(xs_double=pygplates.XsDouble(1))
        self.assertEqual(len(visitor.visited), 1)

    def test_no_crash_without_positional_receiver(self):
        # Every Boost.Python function, called with keywords that no overload accepts, raises
        # 'ArgumentError'. A method called through its class without its receiver passed
        # positionally used to crash instead (see "keyword_argument_probe.py").
        probe = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'keyword_argument_probe.py')
        environment = dict(os.environ, PYTHONPATH=os.pathsep.join(path for path in sys.path if path))
        result = subprocess.run(
                [sys.executable, '-X', 'faulthandler', probe], capture_output=True, text=True, env=environment)
        lines = result.stdout.splitlines()
        self.assertEqual(
                result.returncode, 0,
                'Exit code %d, after "%s":\n%s' % (result.returncode, lines[-1] if lines else '', result.stderr))
        self.assertEqual([line for line in lines if line.startswith('UNEXPECTED')], [])
        # The probe must have found the functions (1739 when this was written).
        num_probed = int(next(line for line in lines if line.startswith('PROBED')).split()[1])
        self.assertGreater(num_probed, 1000)
        # One of the methods that crashed (before the fix) on Python 3.14, Windows.
        self.assertIn('CALL LocalCartesian.from_geocentric_to_north_east_down', lines)


class PickleCase(unittest.TestCase):
    def test_pickle_bytes(self):
        # Only a 'bytes' object is accepted (previously a 'str' left an error pending, raising 'SystemError').
        self.assertRaises(TypeError, pygplates.PickleBytes, 'abc')


class IterableArgumentCase(unittest.TestCase):
    """
    Tests of how arguments taking an iterable, or a sequence, are iterated
    (shared by many functions, so tested here through a few of them).
    """

    class _RaisingIterable(object):
        # An iterable (not an iterator) whose iteration raises part-way.
        def __init__(self, first_item):
            self.first_item = first_item
        def __iter__(self):
            yield self.first_item
            raise ValueError('raised by the iteration')

    def test_iterable(self):
        # An iterable argument is iterated once, so a generator works.
        polyline = pygplates.PolylineOnSphere(pygplates.PointOnSphere(0, lon) for lon in range(3))
        self.assertEqual(len(polyline), 3)
        # An item that doesn't convert raises the function's own TypeError.
        self.assertRaises(TypeError, pygplates.PolylineOnSphere, [(0, 0), 'not a point'])
        self.assertRaises(TypeError, pygplates.PolylineOnSphere, 1)
        # Anything the iteration itself raises propagates unchanged
        # (rather than being replaced by the function's TypeError).
        self.assertRaises(ValueError, pygplates.PolylineOnSphere,
                          self._RaisingIterable(pygplates.PointOnSphere(0, 0)))

    def test_sequence(self):
        # A sequence argument is iterated more than once, so an iterator raises TypeError.
        self.assertRaises(TypeError, pygplates.FeaturesFunctionArgument, iter([pygplates.Feature()]))
        self.assertRaises(TypeError, pygplates.FeatureCollection, iter([pygplates.Feature()]))
        # Anything the iteration itself raises propagates (rather than being left pending).
        self.assertRaises(ValueError, pygplates.FeaturesFunctionArgument,
                          self._RaisingIterable(pygplates.Feature()))
        self.assertRaises(ValueError, pygplates.FeaturesFunctionArgument.contains_features,
                          self._RaisingIterable(pygplates.Feature()))

    def test_contains_features(self):
        contains_features = pygplates.FeaturesFunctionArgument.contains_features
        self.assertTrue(contains_features([pygplates.Feature()]))
        self.assertTrue(contains_features([[pygplates.Feature()]]))
        # An iterator, top-level or nested, gives False (rather than raising).
        self.assertFalse(contains_features(iter([pygplates.Feature()])))
        self.assertFalse(contains_features([iter([pygplates.Feature()])]))
        self.assertFalse(contains_features(1))

    def test_point_and_vector_from_iterator(self):
        # An iterator isn't converted to a point or vector (it's a sequence of 2 or 3 floats),
        # and the error says what is expected (rather than "not an iterator").
        with self.assertRaisesRegex(TypeError, 'Expected PointOnSphere'):
            pygplates.PointOnSphere(x for x in (0.0, 0.0, 1.0))
        with self.assertRaisesRegex(TypeError, 'Expected sequence'):
            pygplates.Vector3D(x for x in (0.0, 0.0, 1.0))
        self.assertEqual(pygplates.PointOnSphere((0.0, 0.0, 1.0)), pygplates.PointOnSphere.north_pole)


class PackageCase(unittest.TestCase):
    """
    Tests of the 'pygplates' Python package itself (as opposed to the API it exposes).

    Note that these tests require 'pygplates' to be imported as a package (which is how it is both built and
    installed) - that is, the directory on 'sys.path' should be the one *containing* the 'pygplates/' package,
    not the package directory itself.
    """

    def test_private_module(self):
        # The pygplates shared library (C++) is a module *inside* the 'pygplates' package, and so the classes
        # it defines report a '__module__' of 'pygplates.pygplates'. Since Boost.Python pickles classes by
        # reference, that name is embedded in every pickle we write, and hence must not change (it's a file
        # format). For the full rationale see 'cmake/modules/Install.cmake'.
        module_name = type(pygplates.FiniteRotation((0,0), 0)).__module__
        self.assertEqual(module_name, 'pygplates.pygplates',
                         "pygplates should be imported as a package (see this test case's docstring)")

        # And the private module must remain reachable as an *attribute* of the 'pygplates' package, since that
        # is how 'dill' resolves a dotted module name (unlike the standard library 'pickle', which goes through
        # 'sys.modules' and so wouldn't notice the attribute disappearing).
        #
        # Note: this mimics 'dill/_dill.py::_import_module' rather than using 'dill' itself, so that we don't
        #       add a test dependency on dill.
        parent_module_name, _, child_module_name = module_name.rpartition('.')
        module = getattr(__import__(parent_module_name, None, None, [child_module_name]), child_module_name)
        self.assertIs(module, sys.modules[module_name])
        self.assertIs(module.FiniteRotation, pygplates.FiniteRotation)

    def test_post_import(self):
        # The pygplates package '__init__.py' calls this to tell the shared library (C++) where it was imported
        # from, and the type stub ('pygplates/stub/__init__.pyi') declares it - so it must be importable from
        # the package (note that "import *" skips names with a leading underscore).
        self.assertTrue(callable(pygplates._post_import))


def suite():
    suite = unittest.TestSuite()
    
    # Add test cases from this module.
    test_cases = [
            ArgumentErrorCase,
            HashableCase,
            IterableArgumentCase,
            KeywordArgumentCase,
            PackageCase,
            PickleCase,
            VersionCase
        ]

    for test_case in test_cases:
        suite.addTest(unittest.defaultTestLoader.loadTestsFromTestCase(test_case))
    
    # Add test suites from sibling modules.
    test_modules = [
            test_app_logic.test,
            test_maths.test,
            test_model.test
        ]

    for test_module in test_modules:
        suite.addTest(test_module.suite())
    
    return suite

# The lower bound on the number of tests the run must contain.
#
# The suite is assembled from hand-maintained lists (the 'test_cases' and 'test_modules' lists in
# 'suite()' above, and the equivalent lists in the sibling test modules), so a test case - or an
# entire module - can drop out of the run without anything failing. An empty suite is the extreme
# case: 'unittest' prints "NO TESTS RAN", but 'wasSuccessful()' is still True and the exit status
# is still 0, so the run passes having tested nothing.
#
# This is a floor, *not* an exact count, and is deliberately not meant to be updated whenever a
# test is added. At the time of writing the suite holds 441 tests (223 from 'test_model', 134 from
# 'test_maths', 77 from 'test_app_logic' and 7 from this module), so this catches an empty run and
# the loss of any one of the three sibling modules. It cannot catch a single unregistered test
# case; only raise it if it stops being a useful lower bound.
MINIMUM_EXPECTED_TESTS = 400

if __name__ == "__main__":
    test_result = unittest.TextTestRunner().run(suite())
    if test_result.testsRun < MINIMUM_EXPECTED_TESTS:
        # Note: this is reported separately from the 'unittest' verdict because it is not a failing
        # test - it means tests that should have run were never registered in the first place.
        sys.stderr.write(
                "Only %d tests ran, expected at least %d. A test case or module is probably missing "
                "from the lists in 'suite()'.\n" % (test_result.testsRun, MINIMUM_EXPECTED_TESTS))
        sys.exit(1)
    if test_result.wasSuccessful():
        sys.exit(0)
    else:
        sys.exit(1)
