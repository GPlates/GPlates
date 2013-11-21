/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYTHONCONVERTERUTILS_H
#define GPLATES_API_PYTHONCONVERTERUTILS_H

#include <boost/mpl/bool.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>

#include "global/python.h"

#include "maths/GeometryOnSphere.h"

#include "model/PropertyValue.h"

#include "utils/non_null_intrusive_ptr.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	namespace PythonConverterUtils
	{
		/**
		 * Returns the actual *derived* property value (converted to boost::python::object).
		 *
		 * The derived type is needed otherwise python is unable to access the attributes of the
		 * derived property value type. In other words 'PropertyValue::non_null_ptr_type' is
		 * never returned to python without first going through this function.
		 */
		boost::python::object/*derived property value non_null_intrusive_ptr*/
		get_property_value_as_derived_type(
				GPlatesModel::PropertyValue::non_null_ptr_type property_value);


		/**
		 * Returns the actual *derived* GeometryOnSphere (converted to boost::python::object).
		 *
		 * The derived type is needed otherwise python is unable to access the attributes of the
		 * derived geometry-on-sphere type. In other words 'GeometryOnSphere::non_null_ptr_to_const_type'
		 * is never returned to python without first going through this function.
		 */
		boost::python::object/*derived geometry-on-sphere non_null_ptr_to_const_type*/
		get_geometry_on_sphere_as_derived_type(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_on_sphere);


		/**
		 * Register implicit conversions of non_null_intrusive_ptr for the specified derived/base classes.
		 *
		 * The conversions include 'non-const' to 'const' conversions where possible (ie, when
		 * 'SourceType' and/or 'TargetType' are non-const types, eg, 'GpmlPlateId' versus 'const GpmlPlateId').
		 *
		 * For example...
		 *
		 *  SourceType=GpmlPlateId and TargetType=PropertyValue
		 *
		 * ...provides the implicit conversions for GPlatesUtils::non_null_intrusive_ptr<>...
		 *
		 *  'GpmlPlateId::non_null_ptr_type'          -> 'PropertyValue::non_null_ptr_type'
		 *  'GpmlPlateId::non_null_ptr_type'          -> 'PropertyValue::non_null_ptr_to_const_type'
		 *  'GpmlPlateId::non_null_ptr_to_const_type' -> 'PropertyValue::non_null_ptr_to_const_type'
		 *
		 * If @a include_non_const_source_to_const_source is true (the default) then also provided is...
		 *
		 *  'GpmlPlateId::non_null_ptr_type'          -> 'GpmlPlateId::non_null_ptr_to_const_type'
		 *
		 * Note that these registrations need to be done explicitly for non_null_intrusive_ptr, and
		 * boost::intrusive_ptr, however boost python does treat boost::shared_ptr as a special case
		 * (where base/derived conversions are registered/handled automatically).
		 */
		template <class SourceType, class TargetType>
		void
		implicitly_convertible_non_null_intrusive_ptr(
				bool include_non_const_source_to_const_source = true);

		/**
		 * Register implicit conversions of boost::optional<non_null_intrusive_ptr> for the
		 * specified derived/base classes.
		 *
		 * The conversions include 'non-const' to 'const' conversions where possible (ie, when
		 * 'SourceType' and/or 'TargetType' are non-const types, eg, 'GpmlPlateId' versus 'const GpmlPlateId').
		 *
		 * For example...
		 *
		 *  SourceType=GpmlPlateId and TargetType=PropertyValue
		 *
		 * ...provides the implicit conversions for boost::optional<GPlatesUtils::non_null_intrusive_ptr<> >...
		 *
		 *  'boost::optional<GpmlPlateId::non_null_ptr_type>'
		 *     -> 'boost::optional<PropertyValue::non_null_ptr_type>'
		 *
		 *  'boost::optional<GpmlPlateId::non_null_ptr_type>'
		 *     -> 'boost::optional<PropertyValue::non_null_ptr_to_const_type>'
		 *
		 *  'boost::optional<GpmlPlateId::non_null_ptr_to_const_type>'
		 *     -> 'boost::optional<PropertyValue::non_null_ptr_to_const_type>'
		 *
		 * If @a include_non_const_source_to_const_source is true (the default) then also provided is...
		 *
		 *  'boost::optional<GpmlPlateId::non_null_ptr_type>'
		 *     -> 'boost::optional<GpmlPlateId::non_null_ptr_to_const_type>'
		 *
		 * NOTE: You'll also need to register a conversion for boost::optional<SourceType::non_null_ptr_type>
		 * using @a register_optional_conversion.
		 *
		 * Note that these registrations need to be done explicitly, however boost python does treat
		 * boost::shared_ptr as a special case (where base/derived conversions are registered/handled automatically).
		 */
		template <class SourceType, class TargetType>
		void
		implicitly_convertible_optional_non_null_intrusive_ptr(
				bool include_non_const_source_to_const_source = true);


		/**
		 * Enables boost::optional<T> to be passed to and from python.
		 *
		 * A python object that is None will become boost::none and vice versa.
		 *
		 * To register the to/from converters, for boost::optional<T>, simply call:
		 *
		 *   register_optional_conversion<T>();
		 *
		 * ...in the module initialisation code for the desired type 'T'.
		 *
		 * For more information on boost python to/from conversions, see:
		 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
		 *   http://stackoverflow.com/questions/6274822/boost-python-no-to-python-converter-found-for-stdstring
		 *   http://mail.python.org/pipermail/cplusplus-sig/2007-May/012003.html
		 */
		template <typename T>
		void
		register_optional_conversion();


		/**
		 * Enables boost::optional<SourceType> to be passed to and from python.
		 *
		 * Also enables a python-wrapped boost::optional<SourceType::non_null_ptr_type> to be used when a
		 * boost::optional<TargetType::non_null_ptr_type> is requested.
		 *
		 * Also enables a python-wrapped SourceType::non_null_ptr_type to be used when a
		 * TargetType::non_null_ptr_type is requested.
		 *
		 * The conversions include 'non-const' to 'const' conversions where possible (ie, when
		 * 'SourceType' and/or 'TargetType' are non-const types, eg, 'GpmlPlateId' versus 'const GpmlPlateId').
		 */
		template <class SourceType, class TargetType>
		void
		register_optional_non_null_intrusive_ptr_and_implicit_conversions();


		/**
		 * Enables a GPlatesUtils::non_null_intrusive_ptr<const T> to be passed to python when a
		 * GPlatesUtils::non_null_intrusive_ptr<T> is expected (eg, because the HeldType of the
		 * 'bp::class_' wrapper of the type 'T' is the non-const version).
		 *
		 * Many types are considered immutable and hence a lot of general functions return *const*
		 * objects. This is fine except that boost-python currently does not compile when wrapping
		 * *const* objects (eg, 'GeometryOnSphere::non_null_ptr_to_const_type') - see:
		 *   https://svn.boost.org/trac/boost/ticket/857
		 *   https://mail.python.org/pipermail/cplusplus-sig/2006-November/011354.html
		 *
		 * ...so the current solution is to wrap *non-const* objects (to keep boost-python happy)
		 * but provide a python to/from converter that converts:
		 *   to-python: T::non_null_ptr_to_const_type -> T::non_null_ptr_type (using this method)
		 *   from-python: T::non_null_ptr_type -> T::non_null_ptr_to_const_type (using bp::implicitly_convertible)
		 * ...which enables us to keep using 'non_null_ptr_to_const_type' everywhere (ie, we don't have
		 * to change our C++ source code) but have it converted to/from 'non_null_ptr_type' when
		 * interacting with python. And since our python-wrapped objects are 'non_null_ptr_type' then
		 * boost-python takes care of the rest of the conversion/wrapping.
		 *
		 * WARNING: This is dangerous since it involves casting away const so it should be used with care.
		 * The danger is if a const object is passed to python (as non-const) which is then passed
		 * back to C++ (as non-const) which then modifies it (thus modifying the original const object).
		 * Or even just modifying it via python.
		 *
		 * NOTE: You will also need to specify the reverse from-python conversion from
		 * non-const back to const if haven't already.
		 *
		 * For more information on boost python to/from conversions, see:
		 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
		 */
		template <typename T>
		void
		register_to_python_const_to_non_const_non_null_intrusive_ptr_conversion();
	}
}

////////////////////
// Implementation //
////////////////////

namespace GPlatesApi
{
	namespace PythonConverterUtils
	{
		template <class SourceType, class TargetType>
		void
		implicitly_convertible_non_null_intrusive_ptr(
				bool include_non_const_source_to_const_source)
		{
			typedef SourceType source_type;
			typedef typename boost::add_const<source_type>::type const_source_type;
			typedef TargetType target_type;
			typedef typename boost::add_const<target_type>::type const_target_type;

			// Enable a python-wrapped non_null_intrusive_ptr<SourceType> to be used when a
			// non_null_intrusive_ptr<TargetType> is requested.
			if (boost::is_convertible<
					GPlatesUtils::non_null_intrusive_ptr<source_type>,
					GPlatesUtils::non_null_intrusive_ptr<target_type> >::value &&
				!boost::is_same<source_type, target_type>::value)
			{
				boost::python::implicitly_convertible<
						GPlatesUtils::non_null_intrusive_ptr<source_type>,
						GPlatesUtils::non_null_intrusive_ptr<target_type> >();
			}
			if (boost::is_convertible<
					GPlatesUtils::non_null_intrusive_ptr<source_type>,
					GPlatesUtils::non_null_intrusive_ptr<const_target_type> >::value &&
				!boost::is_same<source_type, const_target_type>::value)
			{
				boost::python::implicitly_convertible<
						GPlatesUtils::non_null_intrusive_ptr<source_type>,
						GPlatesUtils::non_null_intrusive_ptr<const_target_type> >();
			}
			if (boost::is_convertible<
					GPlatesUtils::non_null_intrusive_ptr<const_source_type>,
					GPlatesUtils::non_null_intrusive_ptr<const_target_type> >::value &&
				!boost::is_same<const_source_type, const_target_type>::value)
			{
				boost::python::implicitly_convertible<
						GPlatesUtils::non_null_intrusive_ptr<const_source_type>,
						GPlatesUtils::non_null_intrusive_ptr<const_target_type> >();
			}

			if (include_non_const_source_to_const_source)
			{
				if (boost::is_convertible<
						GPlatesUtils::non_null_intrusive_ptr<source_type>,
						GPlatesUtils::non_null_intrusive_ptr<const_source_type> >::value &&
					!boost::is_same<source_type, const_source_type>::value)
				{
					boost::python::implicitly_convertible<
							GPlatesUtils::non_null_intrusive_ptr<source_type>,
							GPlatesUtils::non_null_intrusive_ptr<const_source_type> >();
				}
			}
		}


		template <class SourceType, class TargetType>
		void
		implicitly_convertible_optional_non_null_intrusive_ptr(
				bool include_non_const_source_to_const_source)
		{
			typedef SourceType source_type;
			typedef typename boost::add_const<source_type>::type const_source_type;
			typedef TargetType target_type;
			typedef typename boost::add_const<target_type>::type const_target_type;

			// Enable a python-wrapped boost::optional<non_null_intrusive_ptr<SourceType> > to be used when a
			// boost::optional<non_null_intrusive_ptr<TargetType> > is requested.
			if (boost::is_convertible<
					GPlatesUtils::non_null_intrusive_ptr<source_type>,
					GPlatesUtils::non_null_intrusive_ptr<target_type> >::value &&
				!boost::is_same<source_type, target_type>::value)
			{
				boost::python::implicitly_convertible<
						boost::optional<GPlatesUtils::non_null_intrusive_ptr<source_type> >,
						boost::optional<GPlatesUtils::non_null_intrusive_ptr<target_type> > >();
			}
			if (boost::is_convertible<
					GPlatesUtils::non_null_intrusive_ptr<source_type>,
					GPlatesUtils::non_null_intrusive_ptr<const_target_type> >::value &&
				!boost::is_same<source_type, const_target_type>::value)
			{
				boost::python::implicitly_convertible<
						boost::optional<GPlatesUtils::non_null_intrusive_ptr<source_type> >,
						boost::optional<GPlatesUtils::non_null_intrusive_ptr<const_target_type> > >();
			}
			if (boost::is_convertible<
					GPlatesUtils::non_null_intrusive_ptr<const_source_type>,
					GPlatesUtils::non_null_intrusive_ptr<const_target_type> >::value &&
				!boost::is_same<const_source_type, const_target_type>::value)
			{
				boost::python::implicitly_convertible<
						boost::optional<GPlatesUtils::non_null_intrusive_ptr<const_source_type> >,
						boost::optional<GPlatesUtils::non_null_intrusive_ptr<const_target_type> > >();
			}

			if (include_non_const_source_to_const_source)
			{
				if (boost::is_convertible<
						GPlatesUtils::non_null_intrusive_ptr<source_type>,
						GPlatesUtils::non_null_intrusive_ptr<const_source_type> >::value &&
					!boost::is_same<source_type, const_source_type>::value)
				{
					boost::python::implicitly_convertible<
							boost::optional<GPlatesUtils::non_null_intrusive_ptr<source_type> >,
							boost::optional<GPlatesUtils::non_null_intrusive_ptr<const_source_type> > >();
				}
			}
		}


		namespace Implementation
		{
			template <typename T>
			struct python_optional :
					private boost::noncopyable
			{
				struct Conversion
				{
					static
					PyObject *
					convert(
							const boost::optional<T> &value)
					{
						namespace bp = boost::python;

						return bp::incref((value ? bp::object(value.get()) : bp::object()).ptr());
					}
				};

				static
				void *
				convertible(
						PyObject *obj)
				{
					namespace bp = boost::python;

					if (obj == Py_None ||
						bp::extract<T>(obj).check())
					{
						return obj;
					}

					return NULL;
				}

				static
				void
				construct(
						PyObject *obj,
						boost::python::converter::rvalue_from_python_stage1_data *data)
				{
					namespace bp = boost::python;

					void *const storage = reinterpret_cast<
							bp::converter::rvalue_from_python_storage<boost::optional<T> > *>(
									data)->storage.bytes;

					if (obj == Py_None)
					{
						new (storage) boost::optional<T>();
					}
					else
					{
						new (storage) boost::optional<T>(bp::extract<T>(obj)());
					}

					data->convertible = storage;
				}
			};
		}


		template <typename T>
		void
		register_optional_conversion()
		{
			namespace bp = boost::python;

			if (!bp::extract<boost::optional<T> >(bp::object()).check())
			{
				// To python conversion.
				bp::to_python_converter<
						boost::optional<T>,
						typename Implementation::python_optional<T>::Conversion>();

				// From python conversion.
				bp::converter::registry::push_back(
						&Implementation::python_optional<T>::convertible,
						&Implementation::python_optional<T>::construct,
						bp::type_id<boost::optional<T> >());
			}
		}


		template <class SourceType, class TargetType>
		void
		register_optional_non_null_intrusive_ptr_and_implicit_conversions()
		{
			// Enable boost::optional<SourceType::non_null_ptr_type> to be passed to and from python.
			register_optional_conversion< GPlatesUtils::non_null_intrusive_ptr<SourceType> >();

			// Enable a python-wrapped boost::optional<SourceType::non_null_ptr_type> to be used when a
			// boost::optional<TargetType::non_null_ptr_type> is requested (and various 'const' conversions).
			implicitly_convertible_optional_non_null_intrusive_ptr<SourceType, TargetType>();

			// Enable a python-wrapped SourceType::non_null_ptr_type to be used when a
			// TargetType::non_null_ptr_type is requested (and various 'const' conversions).
			implicitly_convertible_non_null_intrusive_ptr<SourceType, TargetType>();
		}


		namespace Implementation
		{
			template <class T>
			struct to_python_ConstToNonConst :
					private boost::noncopyable
			{
				struct Conversion
				{
					static
					PyObject *
					convert(
							const GPlatesUtils::non_null_intrusive_ptr<const T> &value)
					{
						namespace bp = boost::python;

						// 'GPlatesUtils::non_null_intrusive_ptr<T>' is the HeldType of the
						// 'bp::class_' wrapper of the T type so it will be used to complete
						// the conversion to a python-wrapped object. See note above about casting away const.
						return bp::incref(bp::object(GPlatesUtils::const_pointer_cast<T>(value)).ptr());
					};
				};
			};
		}


		template <typename T>
		void
		register_to_python_const_to_non_const_non_null_intrusive_ptr_conversion()
		{
			namespace bp = boost::python;

			// To python conversion.
			bp::to_python_converter<
					GPlatesUtils::non_null_intrusive_ptr<const T>,
					typename Implementation::to_python_ConstToNonConst<T>::Conversion>();
		}
	}
}

#endif   //GPLATES_NO_PYTHON)

#endif // GPLATES_API_PYTHONCONVERTERUTILS_H
