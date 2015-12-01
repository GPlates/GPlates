/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"

#include "global/python.h"

#include "maths/Real.h"

#include "property-values/GeoTimeInstant.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * Handle conversion from +/- infinity (in a 'float') to distant-past and distant future.
	 */
	GPlatesPropertyValues::GeoTimeInstant
	convert_float_to_geo_time_instant(
			const double &time_value)
	{
		if (GPlatesMaths::is_positive_infinity(time_value))
		{
			return GPlatesPropertyValues::GeoTimeInstant::create_distant_past();
		}
		else if (GPlatesMaths::is_negative_infinity(time_value))
		{
			return GPlatesPropertyValues::GeoTimeInstant::create_distant_future();
		}

		return GPlatesPropertyValues::GeoTimeInstant(time_value);
	}

	/**
	 * Handle conversion from distant-past and distant future to +/- infinity (in a 'float').
	 */
	double
	convert_geo_time_instant_to_float(
			const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant)
	{
		if (geo_time_instant.is_distant_past())
		{
			return GPlatesMaths::positive_infinity<double>();
		}
		else if (geo_time_instant.is_distant_future())
		{
			return GPlatesMaths::negative_infinity<double>();
		}

		return geo_time_instant.value();
	}


	/**
	 * This is the python wrapped 'GeoTimeInstant' (the 'GeoTimeInstant' seen on the python side).
	 *
	 * We create a wrapper around GPlatesPropertyValues::GeoTimeInstant because we're not storing
	 * GPlatesPropertyValues::GeoTimeInstant in the python object.
	 */
	class GeoTimeInstant
	{
	public:

		static
		GeoTimeInstant
		create_distant_past()
		{
			return GeoTimeInstant(GPlatesPropertyValues::GeoTimeInstant::create_distant_past());
		}

		static
		GeoTimeInstant
		create_distant_future()
		{
			return GeoTimeInstant(GPlatesPropertyValues::GeoTimeInstant::create_distant_future());
		}

		explicit
		GeoTimeInstant(
				const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant) :
			d_geo_time_instant(geo_time_instant)
		{  }

		double
		get_value() const
		{
			// Handle conversion from distant-past and distant future to +/- infinity (in a 'float').
			return convert_geo_time_instant_to_float(d_geo_time_instant);
		}

		bool
		is_distant_past() const
		{
			return d_geo_time_instant.is_distant_past();
		}

		bool
		is_distant_future() const
		{
			return d_geo_time_instant.is_distant_future();
		}

		bool
		is_real() const
		{
			return d_geo_time_instant.is_real();
		}

		//! Returns internal GPlatesPropertyValues::GeoTimeInstant.
		const GPlatesPropertyValues::GeoTimeInstant &
		get_impl() const
		{
			return d_geo_time_instant;
		}

	private:

		GPlatesPropertyValues::GeoTimeInstant d_geo_time_instant;
	};

	boost::shared_ptr<GeoTimeInstant>
	geo_time_instant_create(
			// NOTE: Using the 'GPlatesPropertyValues' version of GeoTimeInstant enables conversions
			// both from python 'float' and python 'GeoTimeInstant' such as...
			//
			// >>> x = pygplates.GeoTimeInstant(10)
			// >>> y = pygplates.GeoTimeInstant(20)
			// >>> def is_equal(i,j): return pygplates.GeoTimeInstant(i) == pygplates.GeoTimeInstant(j)
			// >>> print is_equal(x,y)
			// >>> print is_equal(10,y)
			// >>> print is_equal(10,20)
			//
			// ...where function 'is_equal()' can accept either 'float' or 'GeoTimeInstant' but
			// always does an epison equality test (ie, uses GeoTimeInstant equality).
			const GPlatesPropertyValues::GeoTimeInstant &time_value)
	{
		return boost::shared_ptr<GeoTimeInstant>(new GeoTimeInstant(time_value));
	}

	bp::object
	geo_time_instant_eq(
			const GeoTimeInstant &geo_time_instant,
			bp::object other)
	{
		bp::extract<GeoTimeInstant> extract_other_geo_time_instant(other);
		if (extract_other_geo_time_instant.check())
		{
			return bp::object(geo_time_instant.get_impl() == extract_other_geo_time_instant().get_impl());
		}

		bp::extract<double> extract_other_float(other);
		if (extract_other_float.check())
		{
			return bp::object(geo_time_instant.get_impl() ==
					// We want to use the epsilon comparison of GeoTimeInstant...
					// Handle conversion from +/- infinity in a python 'float' to
					// distant-past and distant future...
					convert_float_to_geo_time_instant(extract_other_float()));
		}

#if 1
		// Return NotImplemented so python can continue looking for a match
		// (eg, in case 'other' is a class that implements relational operators with GeoTimeInstant).
		return bp::object(bp::handle<>(bp::borrowed(Py_NotImplemented)));
#else
		PyErr_SetString(PyExc_TypeError, "Can only '==' compare GeoTimeInstant with another "
				"GeoTimeInstant or float");
		bp::throw_error_already_set();

		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		// Keep compiler happy.
		return false;
#endif
	}

	bp::object
	geo_time_instant_ne(
			const GeoTimeInstant &geo_time_instant,
			bp::object other)
	{
		bp::object eq_result = geo_time_instant_eq(geo_time_instant, other);
		if (eq_result.ptr() == Py_NotImplemented)
		{
			// Return NotImplemented.
			return eq_result;
		}

		// Invert the result.
		return bp::object(!bp::extract<bool>(eq_result));
	}

	bp::object
	geo_time_instant_lt(
			const GeoTimeInstant &geo_time_instant,
			bp::object other)
	{
		//
		// NOTE: We invert the comparison because we want python's GeoTimeInstant to have larger
		// time values further back in time (which is the opposite of C++'s GeoTimeInstant).
		// This is to avoid potential confusion with python users if they're unsure whether their
		// python object is a 'float' or a 'GeoTimeInstant' (due to the dynamic nature of python).
		//

		bp::extract<GeoTimeInstant> extract_other_geo_time_instant(other);
		if (extract_other_geo_time_instant.check())
		{
			return bp::object(geo_time_instant.get_impl() > extract_other_geo_time_instant().get_impl());
		}

		bp::extract<double> extract_other_float(other);
		if (extract_other_float.check())
		{
			return bp::object(geo_time_instant.get_impl() >
					// We want to use the epsilon comparison of GeoTimeInstant...
					// Handle conversion from +/- infinity in a python 'float' to
					// distant-past and distant future...
					convert_float_to_geo_time_instant(extract_other_float()));
		}

#if 1
		// Return NotImplemented so python can continue looking for a match
		// (eg, in case 'other' is a class that implements relational operators with GeoTimeInstant).
		return bp::object(bp::handle<>(bp::borrowed(Py_NotImplemented)));
#else
		PyErr_SetString(PyExc_TypeError, "Can only '<' compare GeoTimeInstant with another "
				"GeoTimeInstant or float");
		bp::throw_error_already_set();

		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		// Keep compiler happy.
		return false;
#endif
	}

	bp::object
	geo_time_instant_le(
			const GeoTimeInstant &geo_time_instant,
			bp::object other)
	{
		//
		// NOTE: We invert the comparison because we want python's GeoTimeInstant to have larger
		// time values further back in time (which is the opposite of C++'s GeoTimeInstant).
		// This is to avoid potential confusion with python users if they're unsure whether their
		// python object is a 'float' or a 'GeoTimeInstant' (due to the dynamic nature of python).
		//

		bp::extract<GeoTimeInstant> extract_other_geo_time_instant(other);
		if (extract_other_geo_time_instant.check())
		{
			return bp::object(geo_time_instant.get_impl() >= extract_other_geo_time_instant().get_impl());
		}

		bp::extract<double> extract_other_float(other);
		if (extract_other_float.check())
		{
			return bp::object(geo_time_instant.get_impl() >=
					// We want to use the epsilon comparison of GeoTimeInstant...
					// Handle conversion from +/- infinity in a python 'float' to
					// distant-past and distant future...
					convert_float_to_geo_time_instant(extract_other_float()));
		}

#if 1
		// Return NotImplemented so python can continue looking for a match
		// (eg, in case 'other' is a class that implements relational operators with GeoTimeInstant).
		return bp::object(bp::handle<>(bp::borrowed(Py_NotImplemented)));
#else
		PyErr_SetString(PyExc_TypeError, "Can only '<=' compare GeoTimeInstant with another "
				"GeoTimeInstant or float");
		bp::throw_error_already_set();

		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		// Keep compiler happy.
		return false;
#endif
	}

	bp::object
	geo_time_instant_gt(
			const GeoTimeInstant &geo_time_instant,
			bp::object other)
	{
		bp::object le_result = geo_time_instant_le(geo_time_instant, other);
		if (le_result.ptr() == Py_NotImplemented)
		{
			// Return NotImplemented.
			return le_result;
		}

		// Invert the result.
		return bp::object(!bp::extract<bool>(le_result));
	}

	bp::object
	geo_time_instant_ge(
			const GeoTimeInstant &geo_time_instant,
			bp::object other)
	{
		bp::object lt_result = geo_time_instant_lt(geo_time_instant, other);
		if (lt_result.ptr() == Py_NotImplemented)
		{
			// Return NotImplemented.
			return lt_result;
		}

		// Invert the result.
		return bp::object(!bp::extract<bool>(lt_result));
	}

	// For "__str__".
	std::ostream &
	operator<<(
			std::ostream &o,
			const GeoTimeInstant &g)
	{
		o << g.get_impl();

		return o;
	}


	/**
	 * Enables passing GPlatesApi::GeoTimeInstant object (the python 'GeoTimeInstant') to
	 * GPlatesPropertyValues::GeoTimeInstant (the C++ 'GeoTimeInstant').
	 *
	 * NOTE: We don't enable passing the other direction (from C++ to python) because only
	 * one to-python converter is allowed per type and we always pass
	 * GPlatesPropertyValues::GeoTimeInstant (the C++ 'GeoTimeInstant') to python as a
	 * python 'float' (using 'ConversionGPlatesPropertyValuesGeoTimeInstant').
	 *
	 * For more information on boost python to/from conversions, see:
	 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
	 */
	struct ConversionGPlatesApiGeoTimeInstant :
			private boost::noncopyable
	{
		explicit
		ConversionGPlatesApiGeoTimeInstant()
		{
			namespace bp = boost::python;

			// NOTE: We do not register a to_python converter.

			// From python conversion.
			bp::converter::registry::push_back(
					&convertible,
					&construct,
					bp::type_id<GPlatesPropertyValues::GeoTimeInstant>());
		}

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			return bp::extract<GeoTimeInstant>(obj).check() ? obj : NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<GPlatesPropertyValues::GeoTimeInstant> *>(
							data)->storage.bytes;

			new (storage) GPlatesPropertyValues::GeoTimeInstant(bp::extract<GeoTimeInstant>(obj)().get_impl());

			data->convertible = storage;
		}
	};


	/**
	 * Enables GPlatesPropertyValues::GeoTimeInstant to be passed to and from python as a
	 * python 'float' object.
	 *
	 * For more information on boost python to/from conversions, see:
	 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
	 */
	struct ConversionGPlatesPropertyValuesGeoTimeInstant :
			private boost::noncopyable
	{
		explicit
		ConversionGPlatesPropertyValuesGeoTimeInstant()
		{
			namespace bp = boost::python;

			// To python conversion.
			bp::to_python_converter<GPlatesPropertyValues::GeoTimeInstant, Conversion>();

			// From python conversion.
			bp::converter::registry::push_back(
					&convertible,
					&construct,
					bp::type_id<GPlatesPropertyValues::GeoTimeInstant>());
		}

		struct Conversion
		{
			static
			PyObject *
			convert(
					const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant)
			{
				namespace bp = boost::python;

				return bp::incref(bp::object(
						// Handle conversion from distant-past and distant future to +/- infinity (in a 'float')...
						convert_geo_time_instant_to_float(geo_time_instant)).ptr());
			};
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			// Note: We now use 'bp::extract<double>', instead of explicitly checking for a
			// python 'float', because this allows conversion of integers to 'GeoTimeInstant'.
#if 0
			return PyFloat_Check(obj) ? obj : NULL;
#endif
			return bp::extract<double>(obj).check() ? obj : NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<GPlatesPropertyValues::GeoTimeInstant> *>(
							data)->storage.bytes;

			new (storage) GPlatesPropertyValues::GeoTimeInstant(
					// Handle conversion from +/- infinity in a python 'float' to distant-past and distant future...
					convert_float_to_geo_time_instant(
							bp::extract<double>(obj)));

			data->convertible = storage;
		}
	};
}

void
export_geo_time_instant()
{
	//
	// GeoTimeInstant - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	// NOTE: We wrap GPlatesApi::GeoTimeInstant instead of GPlatesPropertyValues::GeoTimeInstant
	// since we already have a converter from the latter to python 'float' (and vice versa).
	// So GPlatesPropertyValues::GeoTimeInstant converts to/from python 'float' and
	// GPlatesApi::GeoTimeInstant only converts *from* python 'GeoTimeInstant'.
	// In other words a C++ GeoTimeInstant is only passed *to* python as python 'float' whereas both
	// python GeoTimeInstant and python 'float' can be passed *from* python to C++ GeoTimeInstant.
	// The python 'GeoTimeInstant' is mainly provided as a convenience class for python users so they
	// can test for distant past/future and perform epsilon equality comparison tests - to do this
	// they simply create a python 'GeoTimeInstant' (from their python 'float') and then do tests on that.
	//
	// GeoTimeInstant is immutable (contains no mutable methods) hence we can copy it into python
	// wrapper objects without worrying that modifications from the C++ will not be visible to the
	// python side and vice versa.
	//
	bp::class_<
			GPlatesApi::GeoTimeInstant/*NOTE: This is not GPlatesPropertyValues::GeoTimeInstant*/,
			// A pointer holder is required by 'bp::make_constructor'...
			boost::shared_ptr<GPlatesApi::GeoTimeInstant>
			// Since it's immutable it can be copied without worrying that a modification from the
			// C++ side will not be visible on the python side, or vice versa. It needs to be
			// copyable anyway so that boost-python can copy it into a shared holder pointer...
#if 0
			boost::noncopyable
#endif
			>(
					"GeoTimeInstant",
					"Represents an instant in geological time. This class is able to represent:\n"
					"\n"
					"* time-instants with a *specific* time-position relative to the present-day\n"
					"* time-instants in the *distant past* (time-position of +infinity)\n"
					"* time-instants in the *distant future* (time-position of -infinity)\n"
					"\n"
					"Note that *positive* values represent times in the *past* and *negative* values represent "
					"times in the *future*. This can be confusing at first, but the reason for this is "
					"geological times are represented by how far in the *past* to go back compared to present day.\n"
					"\n"
					"All comparison operators (==, !=, <, <=, >, >=) are supported (but GeoTimeInstant is "
					"not hashable - cannot be used as a key in a ``dict``). The comparisons are such that "
					"times further in the past are *greater than* more recent times. Note that this is the opposite "
					"how we normally think of time (where future time values are greater than past values).\n"
					"\n"
					"So far this is the same as the native ``float`` type which can represent *distant past* "
					"as ``float('inf')`` and *distant future* as ``float('-inf')`` (and support all comparisons with "
					"+/- infinity). "
					"The advantage with :class:`GeoTimeInstant` is comparisons use a numerical tolerance such "
					"that they compare equal when close enough to each other, and there are explicit methods "
					"to create and test *distant past* and *distant future*. Note that due to the numerical "
					"tolerance in comparisons, a GeoTimeInstant is not hashable and hence cannot be used "
					"as a key in a ``dict`` - however the ``float`` returned by :meth:`get_value` can be.\n"
					"\n"
					"Comparisons can also be made between a GeoTimeInstant and a ``float`` (or ``int``, etc).\n"
					"::\n"
					"\n"
					"  print 'Time instant is distant past: %s' % pygplates.GeoTimeInstant(float_time).is_distant_past()\n"
					"  time10Ma = pygplates.GeoTimeInstant(10)\n"
					"  time20Ma = pygplates.GeoTimeInstant(20)\n"
					"  assert(time20Ma > time10Ma)\n"
					"  assert(20 > time10Ma)\n"
					"  assert(20 > time10Ma.get_value()\n"
					"  assert(time20Ma > 10)\n"
					"  assert(time20Ma.get_value() > 10)\n"
					"  assert(time20Ma.get_value() > time10Ma.get_value())\n"
					"  assert(time20Ma > time10Ma.get_value())\n"
					"  assert(time20Ma.get_value() > time10Ma)\n"
					"  assert(time20Ma < pygplates.GeoTimeInstant.create_distant_past())\n"
					"  assert(time20Ma.get_value() < pygplates.GeoTimeInstant.create_distant_past())\n"
					"  assert(20 < pygplates.GeoTimeInstant.create_distant_past())\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::geo_time_instant_create,
						bp::default_call_policies(),
						(bp::arg("time_value"))),
				"__init__(time_value)\n"
				"  Create a GeoTimeInstant instance from *time_value*.\n"
				"\n"
				"  :param time_value: the time position - positive values represent times in the *past*\n"
				"  :type time_value: float or :class:`GeoTimeInstant`\n"
				"\n"
				"  Note that if *time_value* is +infinity then :meth:`is_distant_past` will subsequently return true. "
				"And if *time_value* is -infinity then :meth:`is_distant_future` will subsequently return true.\n"
				"  ::\n"
				"\n"
				"    time_instant = pygplates.GeoTimeInstant(time_value)\n")
		.def("create_distant_past",
				&GPlatesApi::GeoTimeInstant::create_distant_past,
				"create_distant_past() -> GeoTimeInstant\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a GeoTimeInstant instance for the distant past.\n"
				"  ::\n"
				"\n"
				"    distant_past = pygplates.GeoTimeInstant.create_distant_past()\n"
				"\n"
				"  This is basically creating a time-instant which is infinitely far in the past.\n"
				"\n"
				"  Subsequent calls to :meth:`get_value` will return a value of +infinity.\n"
				"\n"
				"  All distant-past time-instants will compare greater than all non-distant-past time-instants.\n")
		.staticmethod("create_distant_past")
		.def("create_distant_future",
				&GPlatesApi::GeoTimeInstant::create_distant_future,
				"create_distant_future() -> GeoTimeInstant\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a GeoTimeInstant instance for the distant future.\n"
				"  ::\n"
				"\n"
				"    distant_future = pygplates.GeoTimeInstant.create_distant_future()\n"
				"\n"
				"  This is basically creating a time-instant which is infinitely far in the future.\n"
				"\n"
				"  Subsequent calls to :meth:`get_value` will return a value of -infinity.\n"
				"\n"
				"  All distant-future time-instants will compare less than all non-distant-future time-instants.\n")
		.staticmethod("create_distant_future")
		.def("get_value",
				&GPlatesApi::GeoTimeInstant::get_value,
				"get_value() -> float\n"
				"  Access the floating-point representation of the time-position of this instance. "
				"Units are in Ma (millions of year ago).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  If :meth:`is_distant_past` is ``True`` then *get_value* returns ``float('inf')`` and if "
				":meth:`is_distant_future` is ``True`` then *get_value* returns ``float('-inf')``.\n"
				"\n"
				"  Note that positive values represent times in the past and negative values represent "
				"times in the future.\n")
		.def("is_distant_past",
				&GPlatesApi::GeoTimeInstant::is_distant_past,
				"is_distant_past() -> bool\n"
				"  Returns ``True`` if this instance is a time-instant in the distant past.\n"
				"\n"
				"  :rtype: bool\n")
		.def("is_distant_future",
				&GPlatesApi::GeoTimeInstant::is_distant_future,
				"is_distant_future() -> bool\n"
				"  Returns ``True`` if this instance is a time-instant in the distant future.\n"
				"\n"
				"  :rtype: bool\n")
		.def("is_real",
				&GPlatesApi::GeoTimeInstant::is_real,
				"is_real() -> bool\n"
				"  Returns ``True`` if this instance is a time-instant whose time-position may be "
				"expressed as a *real* floating-point number.\n"
				"\n"
				"  :rtype: bool\n"
				"\n"
				"  If :meth:`is_real` is ``True`` then both :meth:`is_distant_past` and "
				":meth:`is_distant_future` will be ``False``.\n")
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, false))
		.def("__eq__", &GPlatesApi::geo_time_instant_eq)
		.def("__ne__", &GPlatesApi::geo_time_instant_ne)
		.def("__lt__", &GPlatesApi::geo_time_instant_lt)
		.def("__le__", &GPlatesApi::geo_time_instant_le)
		.def("__gt__", &GPlatesApi::geo_time_instant_gt)
		.def("__ge__", &GPlatesApi::geo_time_instant_ge)
	;

	// Enable boost::optional<GPlatesApi::GeoTimeInstant> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::GeoTimeInstant>();

	// Enable boost::optional<GPlatesPropertyValues::GeoTimeInstant> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesPropertyValues::GeoTimeInstant>();

	// Registers the python converters to/from GPlatesPropertyValues::GeoTimeInstant (C++)
	// from/to GPlatesApi::GeoTimeInstant (python) and float (python).
	GPlatesApi::ConversionGPlatesApiGeoTimeInstant();
	GPlatesApi::ConversionGPlatesPropertyValuesGeoTimeInstant();
}

#endif // GPLATES_NO_PYTHON
