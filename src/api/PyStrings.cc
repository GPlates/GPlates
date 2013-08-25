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

#include <boost/noncopyable.hpp>
#include <QString>

#include "PythonConverterUtils.h"

#include "global/CompilerWarnings.h"
#include "global/python.h"

#include "utils/UnicodeString.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
// For PyString_Check below.
DISABLE_GCC_WARNING("-Wold-style-cast")

	/**
	 * Enables QString to be passed to and from python.
	 *
	 * For more information on boost python to/from conversions, see:
	 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
	 */
	struct python_QString :
			private boost::noncopyable
	{
		explicit
		python_QString()
		{
			namespace bp = boost::python;

			// To python conversion.
			bp::to_python_converter<QString, Conversion>();

			// From python conversion.
			bp::converter::registry::push_back(
					&convertible,
					&construct,
					bp::type_id<QString>());
		}

		struct Conversion
		{
			static
			PyObject *
			convert(
					const QString &qstring)
			{
				namespace bp = boost::python;

				// Encode as UTF8.
				// FIXME: Not sure how this interacts with python unicode strings.
				return bp::incref(bp::str(qstring.toUtf8().constData()).ptr());
			};
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			// FIXME: Not sure how this interacts with python unicode strings.
			//return bp::extract<const char*>(obj).check() ? obj : NULL;
			return PyString_Check(obj) ? obj : NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<QString> *>(
							data)->storage.bytes;

			// Decode as UTF8.
			// FIXME: Not sure how this interacts with python unicode strings.
			new (storage) QString(QString::fromUtf8(bp::extract<const char*>(obj)));

			data->convertible = storage;
		}
	};

ENABLE_GCC_WARNING("-Wold-style-cast")


	/**
	 * Enables GPlatesUtils::UnicodeString to be passed to and from python.
	 *
	 * For more information on boost python to/from conversions, see:
	 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
	 */
	struct python_UnicodeString :
			private boost::noncopyable
	{
		explicit
		python_UnicodeString()
		{
			namespace bp = boost::python;

			// To python conversion.
			bp::to_python_converter<GPlatesUtils::UnicodeString, Conversion>();

			// From python conversion.
			bp::converter::registry::push_back(
					&convertible,
					&construct,
					bp::type_id<GPlatesUtils::UnicodeString>());
		}

		struct Conversion
		{
			static
			PyObject *
			convert(
					const GPlatesUtils::UnicodeString &unicode_string)
			{
				namespace bp = boost::python;

				// Use the converter registered for QString (UnicodeString contains a QString).
				return bp::incref(bp::object(unicode_string.qstring()).ptr());
			};
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			// UnicodeString is constructed from a QString.
			return bp::extract<QString>(obj).check() ? obj : NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<GPlatesUtils::UnicodeString> *>(
							data)->storage.bytes;

			// UnicodeString is constructed from a QString.
			new (storage) GPlatesUtils::UnicodeString(bp::extract<QString>(obj));

			data->convertible = storage;
		}
	};
}


void
export_qstring()
{
	// Registers the python to/from converters for QString.
	GPlatesApi::python_QString();

	// Enable boost::optional<QString> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<QString>();
}


void
export_unicode_string()
{
	// Registers the python to/from converters for GPlatesUtils::UnicodeString.
	GPlatesApi::python_UnicodeString();

	// Enable boost::optional<GPlatesUtils::UnicodeString> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesUtils::UnicodeString>();
}


void
export_strings()
{
	export_qstring();
	export_unicode_string();
}

#endif // GPLATES_NO_PYTHON
