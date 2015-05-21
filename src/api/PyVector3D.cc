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

	boost::shared_ptr<GPlatesMaths::Vector3D>
	vector_create(
			bp::object vector_object)
	{
		// There is a from-python converter from the sequence(x,y,z) that will get matched by this...
		bp::extract<GPlatesMaths::Vector3D> extract_vector(vector_object);
		if (extract_vector.check())
		{
			return boost::shared_ptr<GPlatesMaths::Vector3D>(
					new GPlatesMaths::Vector3D(extract_vector()));
		}

		PyErr_SetString(PyExc_TypeError, "Expected sequence (x,y,z) or Vector3D");
		bp::throw_error_already_set();

		// Shouldn't be able to get here.
		return boost::shared_ptr<GPlatesMaths::Vector3D>();
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
					"Represents a vector in 3D catesian coordinates. Vectors are equality (``==``, ``!=``) comparable.\n",
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
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__' is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.setattr("__hash__", bp::object()/*None*/) // make unhashable
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
