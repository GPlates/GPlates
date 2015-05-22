/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <algorithm>
#include <iterator>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "PythonConverterUtils.h"

#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/slice.hpp>
#include <boost/python/stl_iterator.hpp>

#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * Enables a sequence, such as tuple or list, of (x,y,z) to be passed from python (to a Vector3D).
	 *
	 * For more information on boost python to/from conversions, see:
	 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
	 */
	struct python_Vector3DFromXYZSequence :
			private boost::noncopyable
	{
		explicit
		python_Vector3DFromXYZSequence()
		{
			namespace bp = boost::python;

			// From python conversion.
			bp::converter::registry::push_back(
					&convertible,
					&construct,
					bp::type_id<GPlatesMaths::Vector3D>());
		}

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			bp::object py_obj = bp::object(bp::handle<>(bp::borrowed(obj)));

			// If the check fails then we'll catch a python exception and return NULL.
			try
			{
				// A sequence containing floats.
				bp::stl_input_iterator<double>
						float_seq_begin(py_obj),
						float_seq_end;

				// Copy into a vector.
				std::vector<double> float_vector;
				std::copy(float_seq_begin, float_seq_end, std::back_inserter(float_vector));
				if (float_vector.size() != 3)   // (x,y,z)
				{
					return NULL;
				}
			}
			catch (const boost::python::error_already_set &)
			{
				PyErr_Clear();
				return NULL;
			}

			return obj;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			bp::object py_obj = bp::object(bp::handle<>(bp::borrowed(obj)));

			// A sequence containing floats.
			bp::stl_input_iterator<double>
					float_seq_begin(py_obj),
					float_seq_end;

			// Copy into a vector.
			std::vector<double> float_vector;
			std::copy(float_seq_begin, float_seq_end, std::back_inserter(float_vector));

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<GPlatesMaths::Vector3D> *>(
							data)->storage.bytes;

			// Note that 'convertible' has checked for the correct size of the vector (size 3).
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					float_vector.size() == 3,
					GPLATES_ASSERTION_SOURCE);

			// (x,y,z) sequence.
			new (storage) GPlatesMaths::Vector3D(float_vector[0], float_vector[1], float_vector[2]);

			data->convertible = storage;
		}
	};


	// Zero vector.
	const GPlatesMaths::Vector3D vector_zero(0, 0, 0);

	// Axis vectors (x, y, z).
	const GPlatesMaths::Vector3D vector_x_axis(1, 0, 0);
	const GPlatesMaths::Vector3D vector_y_axis(0, 1, 0);
	const GPlatesMaths::Vector3D vector_z_axis(0, 0, 1);


	namespace Implementation
	{
		GPlatesMaths::Vector3D
		vector_extract_vector(
				bp::object vector_object)
		{
			// There is a from-python converter from the sequence(x,y,z) that will get matched by this...
			bp::extract<GPlatesMaths::Vector3D> extract_vector(vector_object);
			if (extract_vector.check())
			{
				return extract_vector();
			}

			PyErr_SetString(PyExc_TypeError, "Expected sequence (x,y,z) or Vector3D");
			bp::throw_error_already_set();

			// Shouldn't be able to get here.
			return GPlatesMaths::Vector3D();
		}
	}

	boost::shared_ptr<GPlatesMaths::Vector3D>
	vector_create(
			bp::object vector_object)
	{
		return boost::shared_ptr<GPlatesMaths::Vector3D>(
				new GPlatesMaths::Vector3D(
						Implementation::vector_extract_vector(vector_object)));
	}

	GPlatesMaths::Vector3D
	vector_get_normalised(
			bp::object vector_object)
	{
		return GPlatesMaths::Vector3D(
				Implementation::vector_extract_vector(vector_object).get_normalisation());
	}

	GPlatesMaths::Vector3D
	vector_get_normalised_from_xyz(
			const GPlatesMaths::Real &x,
			const GPlatesMaths::Real &y,
			const GPlatesMaths::Real &z)
	{
		return GPlatesMaths::Vector3D(
				GPlatesMaths::Vector3D(x, y, z).get_normalisation());
	}

	GPlatesMaths::Real
	vector_angle_between(
			bp::object vector1_object,
			bp::object vector2_object)
	{
		// Get normalised versions of both vectors.
		const GPlatesMaths::Vector3D vector1 = vector_get_normalised(vector1_object);
		const GPlatesMaths::Vector3D vector2 = vector_get_normalised(vector2_object);

		return acos(dot(vector1, vector2));
	}

	GPlatesMaths::Real
	vector_dot(
			bp::object vector1_object,
			bp::object vector2_object)
	{
		return dot(
				Implementation::vector_extract_vector(vector1_object),
				Implementation::vector_extract_vector(vector2_object));
	}

	GPlatesMaths::Vector3D
	vector_cross(
			bp::object vector1_object,
			bp::object vector2_object)
	{
		return cross(
				Implementation::vector_extract_vector(vector1_object),
				Implementation::vector_extract_vector(vector2_object));
	}

	bp::tuple
	vector_to_xyz(
			const GPlatesMaths::Vector3D &vec)
	{
		return bp::make_tuple(vec.x(), vec.y(), vec.z());
	}
}


void
export_vector_3d()
{
	//
	// Vector3D - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesMaths::Vector3D,
			// A pointer holder is required by 'bp::make_constructor'...
			boost::shared_ptr<GPlatesMaths::Vector3D>
			// Since it's immutable it can be copied without worrying that a modification from the
			// C++ side will not be visible on the python side, or vice versa. It needs to be
			// copyable anyway so that boost-python can copy it into a shared holder pointer...
#if 0
			boost::noncopyable
#endif
			>(
					"Vector3D",
					"Represents a vector in 3D catesian coordinates. Vectors are equality (``==``, ``!=``) comparable.\n"
					"\n"
					"The following operations can be used:\n"
					"\n"
					"=========================== =======================================================================\n"
					"Operation                    Result\n"
					"=========================== =======================================================================\n"
					"``-vector``                  Creates a new *Vector3D* that points in the opposite direction to *vector*\n"
					"``scalar * vector``          Creates a new *Vector3D* from *vector* with each component of (x,y,z) multiplied by *scalar*\n"
					"``vector * scalar``          Creates a new *Vector3D* from *vector* with each component of (x,y,z) multiplied by *scalar*\n"
					"``vector1 + vector2``        Creates a new *Vector3D* that is the sum of *vector1* and *vector2*\n"
					"``vector1 - vector2``        Creates a new *Vector3D* that is *vector2* subtracted from *vector1*\n"
					"=========================== =======================================================================\n"
					"\n"
					"For example, to interpolate between two vectors:\n"
					"::\n"
					"\n"
					"  vector1 = pygplates.Vector3D(...)\n"
					"  vector2 = pygplates.Vector3D(...)\n"
					"  vector_interp = t * vector1 + (1-t) * vector2\n"
					"\n"
					"Convenience class static data are available for the zero vector (all zero components) and "
					"the x, y and z axes (unit vectors in the respective directions):\n"
					"\n"
					"* ``pygplates.Vector3D.zero``\n"
					"* ``pygplates.Vector3D.x_axis``\n"
					"* ``pygplates.Vector3D.y_axis``\n"
					"* ``pygplates.Vector3D.z_axis``\n"
					"\n"
					"For example, to create a vector from a triplet of axis basis weights (triplet of scalars):\n"
					"::\n"
					"\n"
					"  vector = (\n"
					"      x_weight * pygplates.Vector3D.x_axis +\n"
					"      y_weight * pygplates.Vector3D.y_axis +\n"
					"      z_weight * pygplates.Vector3D.z_axis)\n",
					bp::init<GPlatesMaths::Real, GPlatesMaths::Real, GPlatesMaths::Real>(
							(bp::arg("x"), bp::arg("y"), bp::arg("z")),
							"__init__(x, y, z)\n"
							"  Construct a *Vector3D* instance from 3D cartesian coordinates consisting of the "
							"floating-point numbers *x*, *y* and *z*.\n"
							"\n"
							"  :param x: the *x* component of the 3D vector\n"
							"  :type x: float\n"
							"  :param y: the *y* component of the 3D vector\n"
							"  :type y: float\n"
							"  :param z: the *z* component of the 3D vector\n"
							"  :type z: float\n"
							"\n"
							"  ::\n"
							"\n"
							"    vector = pygplates.Vector3D(x, y, z)\n"))
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::vector_create,
						bp::default_call_policies(),
						(bp::arg("vector"))),
				"__init__(vector)\n"
				"  Create a *Vector3D* instance from an (x,y,z) sequence (or *Vector3D*).\n"
				"\n"
				"  :param point: (x,y,z) vector\n"
				"  :type point: sequence, such as list or tuple, of (float,float,float), or :class:`Vector3D`\n"
				"\n"
				"  The following example shows a few different ways to use this method:\n"
				"  ::\n"
				"\n"
				"    vector = pygplates.Vector3D((x,y,z))\n"
				"    vector = pygplates.Vector3D([x,y,z])\n"
				"    vector = pygplates.Vector3D(numpy.array([x,y,z]))\n"
				"    vector = pygplates.Vector3D(pygplates.Vector3D(x,y,z))\n")
		// Static property 'pygplates.Vector3D.zero'...
		.def_readonly("zero", GPlatesApi::vector_zero)
		// Static property 'pygplates.Vector3D.x_axis'...
		.def_readonly("x_axis", GPlatesApi::vector_x_axis)
		// Static property 'pygplates.Vector3D.y_axis'...
		.def_readonly("y_axis", GPlatesApi::vector_y_axis)
		// Static property 'pygplates.Vector3D.z_axis'...
		.def_readonly("z_axis", GPlatesApi::vector_z_axis)

		.def("angle_between",
				&GPlatesApi::vector_angle_between,
				(bp::arg("vector1"), bp::arg("vector2")),
				"angle_between(vector1, vector2) -> float\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Returns the angle between two vectors (in radians).\n"
				"\n"
				"  :param vector1: the first vector\n"
				"  :type vector1: :class:`Vector3D`, or sequence (such as list or tuple) of (float,float,float)\n"
				"  :param vector2: the second vector\n"
				"  :type vector2: :class:`Vector3D`, or sequence (such as list or tuple) of (float,float,float)\n"
				"  :rtype: float\n"
				"  :raises: UnableToNormaliseZeroVectorError if either *vector1* or *vector2* is (0,0,0) "
				"(ie, :meth:`has zero magnitude<is_zero_magnitude>`)\n"
				"\n"
				"  Note that the angle between a vector (``vec``) and its opposite (``-vec``) is ``math.pi`` "
				"(and not zero) even though both vectors are parallel. This is because they point in "
				"opposite directions.\n"
				"\n"
				"  The following example shows a few different ways to use this function:\n"
				"  ::\n"
				"\n"
				"    vec1 = pygplates.Vector3D(1.1, 2.2, 3.3)\n"
				"    vec2 = pygplates.Vector3D(-1.1, -2.2, -3.3)\n"
				"    angle = pygplates.Vector3D.angle_between(vec1, vec2)\n"
				"    \n"
				"    angle = pygplates.Vector3D.angle_between((1.1, 2.2, 3.3), (-1.1, -2.2, -3.3))\n"
				"    \n"
				"    angle = pygplates.Vector3D.angle_between(vec1, (-1.1, -2.2, -3.3))\n"
				"    \n"
				"    angle = pygplates.Vector3D.angle_between((1.1, 2.2, 3.3), vec2)\n"
				"\n"
				"  This function is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    if not vector1.is_zero_magnitude() and not vector2.is_zero_magnitude():\n"
				"        angle_between = math.acos(\n"
				"            pygplates.Vector3D.dot(vector1.to_normalised(), vector2.to_normalised()))\n"
				"    else:\n"
				"        raise pygplates.UnableToNormaliseZeroVectorError\n")
		.staticmethod("angle_between")

		.def("dot",
				&GPlatesApi::vector_dot,
				(bp::arg("vector1"), bp::arg("vector2")),
				"dot(vector1, vector2) -> float\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Returns the dot product of two vectors.\n"
				"\n"
				"  :param vector1: the first vector\n"
				"  :type vector1: :class:`Vector3D`, or sequence (such as list or tuple) of (float,float,float)\n"
				"  :param vector2: the second vector\n"
				"  :type vector2: :class:`Vector3D`, or sequence (such as list or tuple) of (float,float,float)\n"
				"  :rtype: float\n"
				"\n"
				"  The following example shows a few different ways to use this function:\n"
				"  ::\n"
				"\n"
				"    vec1 = pygplates.Vector3D(1.1, 2.2, 3.3)\n"
				"    vec2 = pygplates.Vector3D(-1.1, -2.2, -3.3)\n"
				"    dot_product = pygplates.Vector3D.dot(vec1, vec2)\n"
				"    \n"
				"    dot_product = pygplates.Vector3D.dot((1.1, 2.2, 3.3), (-1.1, -2.2, -3.3))\n"
				"    \n"
				"    dot_product = pygplates.Vector3D.dot(vec1, (-1.1, -2.2, -3.3))\n"
				"    \n"
				"    dot_product = pygplates.Vector3D.dot((1.1, 2.2, 3.3), vec2)\n"
				"\n"
				"  The dot product is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    dot_product = (\n"
				"        vector1.get_x() * vector2.get_x() +\n"
				"        vector1.get_y() * vector2.get_y() +\n"
				"        vector1.get_z() * vector2.get_z())\n")
		.staticmethod("dot")

		.def("cross",
				&GPlatesApi::vector_cross,
				(bp::arg("vector1"), bp::arg("vector2")),
				"cross(vector1, vector2) -> Vector3D\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Returns the cross product of two vectors.\n"
				"\n"
				"  :param vector1: the first vector\n"
				"  :type vector1: :class:`Vector3D`, or sequence (such as list or tuple) of (float,float,float)\n"
				"  :param vector2: the second vector\n"
				"  :type vector2: :class:`Vector3D`, or sequence (such as list or tuple) of (float,float,float)\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  The following example shows a few different ways to use this function:\n"
				"  ::\n"
				"\n"
				"    vec1 = pygplates.Vector3D(1.1, 2.2, 3.3)\n"
				"    vec2 = pygplates.Vector3D(-1.1, -2.2, -3.3)\n"
				"    cross_product = pygplates.Vector3D.cross(vec1, vec2)\n"
				"    \n"
				"    cross_product = pygplates.Vector3D.cross((1.1, 2.2, 3.3), (-1.1, -2.2, -3.3))\n"
				"    \n"
				"    cross_product = pygplates.Vector3D.cross(vec1, (-1.1, -2.2, -3.3))\n"
				"    \n"
				"    cross_product = pygplates.Vector3D.cross((1.1, 2.2, 3.3), vec2)\n"
				"\n"
				"  The cross product is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    cross_product = pygplates.Vector3D(\n"
				"        vector1.get_y() * vector2.get_z() - vector1.get_z() * vector2.get_y(),\n"
				"        vector1.get_z() * vector2.get_x() - vector1.get_x() * vector2.get_z(),\n"
				"        vector1.get_x() * vector2.get_y() - vector1.get_y() * vector2.get_x())\n")
		.staticmethod("cross")

		.def("create_normalised",
				&GPlatesApi::vector_get_normalised,
				(bp::arg("xyz")),
				"create_normalised(xyz) -> Vector3D\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Returns a new vector that is a normalised (unit length) version of *vector*.\n"
				"\n"
				"  :param xyz: the vector (x,y,z) components\n"
				"  :type xyz: sequence (such as list or tuple) of (float,float,float), or :class:`Vector3D`\n"
				"  :rtype: :class:`Vector3D`\n"
				"  :raises: UnableToNormaliseZeroVectorError if *xyz* is (0,0,0) "
				"(ie, :meth:`has zero magnitude<is_zero_magnitude>`)\n"
				"\n"
				"  ::\n"
				"\n"
				"    normalised_vector = pygplates.Vector3D.create_normalised((2, 1, 0))\n"
				"\n"
				"  This function is similar to :meth:`to_normalised` but is typically used when you don't "
				"have a :class:`Vector3D` object to call :meth:`to_normalised` on. Such as "
				"``pygplates.Vector3D.create_normalised((2, 1, 0))``.\n")
		.def("create_normalised",
				&GPlatesApi::vector_get_normalised_from_xyz,
				(bp::arg("x"), bp::arg("y"), bp::arg("z")),
				"create_normalised(x, y, z) -> Vector3D\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Returns a new vector that is a normalised (unit length) version of vector (x, y, z).\n"
				"\n"
				"  :param x: the *x* component of the 3D vector\n"
				"  :type x: float\n"
				"  :param y: the *y* component of the 3D vector\n"
				"  :type y: float\n"
				"  :param z: the *z* component of the 3D vector\n"
				"  :type z: float\n"
				"  :raises: UnableToNormaliseZeroVectorError if (x,y,z) is (0,0,0) "
				"(ie, :meth:`has zero magnitude<is_zero_magnitude>`)\n"
				"\n"
				"  ::\n"
				"\n"
				"    normalised_vector = pygplates.Vector3D.create_normalised(2, 1, 0)\n"
				"\n"
				"  This function is similar to the *create_normalised* function above but takes three "
				"arguments *x*, *y* and *z* instead of a single argument (such as a tuple or list).\n")
		.staticmethod("create_normalised")
		// Allow for American spelling (but we don't document it)...
		.def("create_normalized",
				&GPlatesApi::vector_get_normalised,
				(bp::arg("vector")))
		.def("create_normalized",
				&GPlatesApi::vector_get_normalised_from_xyz,
				(bp::arg("x"), bp::arg("y"), bp::arg("z")))
		.staticmethod("create_normalized")

		.def("to_normalised",
				&GPlatesApi::vector_get_normalised,
				"to_normalised() -> Vector3D\n"
				"  Returns a new vector that is a normalised (unit length) version of this vector.\n"
				"\n"
				"  :raises: UnableToNormaliseZeroVectorError if this vector is (0,0,0) "
				"(ie, :meth:`has zero magnitude<is_zero_magnitude>`)\n"
				"\n"
				"  If a vector is not :meth:`zero magnitude<is_zero_magnitude>` then it can return "
				"a normalised version of itself:\n"
				"  ::\n"
				"\n"
				"    if not vector.is_zero_magnitude():\n"
				"        normalised_vector = vector.to_normalised()\n"
				"\n"
				"  **NOTE:** This does not normalise this vector. Instead it returns a new vector object "
				"that is the equivalent of this vector but has a magnitude of 1.0.\n"
				"\n"
				"  This function is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    if not vector.is_zero_magnitude():\n"
				"        scale = 1.0 / vector.get_magnitude()\n"
				"        normalised_vector = pygplates.Vector3D(\n"
				"            scale * vector.get_x(),\n"
				"            scale * vector.get_y(),\n"
				"            scale * vector.get_z())\n"
				"    else:\n"
				"        raise pygplates.UnableToNormaliseZeroVectorError\n")
		// Allow for American spelling (but we don't document it)...
		.def("to_normalized",
				&GPlatesApi::vector_get_normalised)

		.def("get_x",
				&GPlatesMaths::Vector3D::x,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_x() -> float\n"
				"  Returns the *x* coordinate.\n"
				"\n"
				"  :rtype: float\n")
		.def("get_y",
				&GPlatesMaths::Vector3D::y,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_y() -> float\n"
				"  Returns the *y* coordinate.\n"
				"\n"
				"  :rtype: float\n")
		.def("get_z",
				&GPlatesMaths::Vector3D::z,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_z() -> float\n"
				"  Returns the *z* coordinate.\n"
				"\n"
				"  :rtype: float\n")
		.def("to_xyz",
				&GPlatesApi::vector_to_xyz,
				"to_xyz() -> x, y, z\n"
				"  Returns the cartesian coordinates as the tuple (x,y,z).\n"
				"\n"
				"  :rtype: the tuple (float,float,float)\n"
				"\n"
				"  ::\n"
				"\n"
				"    x, y, z = vector.to_xyz()\n")
		.def("is_zero_magnitude",
				&GPlatesMaths::Vector3D::is_zero_magnitude,
				"is_zero_magnitude() -> bool\n"
				"  Returns ``True`` if the magnitude of this vector is zero.\n"
				"\n"
				"  :rtype: bool\n"
				"\n"
				"  This method will also return ``True`` for tiny, non-zero magnitudes that "
				"would cause :meth:`to_normalised` to raise *UnableToNormaliseZeroVectorError*.\n")
		.def("get_magnitude",
				&GPlatesMaths::Vector3D::magnitude,
				"get_magnitude() -> float\n"
				"  Returns the magnitude, or length, of the vector.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  ::\n"
				"\n"
				"    magnitude = vector.get_magnitude()\n"
				"\n"
				"  The magnitude is the equivalent of:\n"
				"  ::\n"
				"\n"
				"    magnitude = math.sqrt(\n"
				"        vector.get_x() * vector.get_x() +\n"
				"        vector.get_y() * vector.get_y() +\n"
				"        vector.get_z() * vector.get_z())\n")
		.def(-bp::self) // Negation
		.def(bp::self - bp::self) // Vector subtraction
		.def(bp::self + bp::self) // Vector addition
		.def(bp::other<GPlatesMaths::Real>() * bp::self) // Scalar multiplication
		.def(bp::self * bp::other<GPlatesMaths::Real>()) // Scalar multiplication
		// Comparisons...
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Enable boost::optional<Vector3D> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesMaths::Vector3D>();

	// Registers the from-python converter from an (x,y,z) sequence.
	GPlatesApi::python_Vector3DFromXYZSequence();
}

#endif // GPLATES_NO_PYTHON
