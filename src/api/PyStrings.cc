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

#include "model/XmlAttributeValue.h"

#include "property-values/EnumerationContent.h"
#include "property-values/TextContent.h"

#include "utils/StringUtils.h"
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
	struct ConversionQString :
			private boost::noncopyable
	{
		explicit
		ConversionQString()
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

#if PY_MAJOR_VERSION >= 3
				// Python 3 supports 'str' and 'bytes' on the Python side where
				// 'str' is now unicode (unlike Python 2) and 'bytes' is a sequence of bytes
				// (similar to 'str' in Python 2).
				//
				// For Python 3 we return don't need to encode our C++ unicode QString into
				// a sequence of bytes (since Python 3 'str' is unicode).
				// So we just convert our QString into std::wstring and then let boost python
				// do its implicit conversion from std::wstring to Python 'str'.
				//
				bp::object unicode_string(GPlatesUtils::make_wstring_from_qstring(qstring));
				return bp::incref(unicode_string.ptr());
#else
				// Python 2 supports 'str' and 'unicode' (on the Python side) where 'str' is a
				// sequence of bytes with an implicit encoding (eg, 'latin1' or 'utf8') and
				// 'unicode' contains the actual characters (code points) with no implicit encoding.
				//
				// For Python 2 we return a Python 'str' encoded as UTF8.
				// We don't return a Python 'unicode' object (there's no boost::python::unicode object
				// in earlier versions of boost python but there is an implicit conversion from
				// std::wstring to 'unicode').
				// Note we should always be consistent with the return type (ie, always return the
				// same type rather than sometimes 'str' and sometimes 'unicode').
				// We choose to always return 'str' (instead of 'unicode'), even though it has an implicit encoding.
				//
				// Maybe we should return 'unicode' though ? It's hard to decide since a user's program
				// (if they don't use 'unicode') might then be mixing their 'str' objects with our
				// 'unicode' types which would cause problems if their 'str' objects contained non-ascii
				// encoded data (because then an implicit conversion from 'str' to 'unicode', perhaps due to
				// concatenating their 'str' with our 'unicode', would trigger a unicode decoding exception
				// assuming their default 'ascii' decoding hasn't been changed). However if we return 'str'
				// (encoded as UTF8) then an exception would not be raised (since no need to decode to 'unicode')
				// but their 'str' objects might use a different encoding than us (ie, not UTF8) and mixing
				// encodings would cause problems if not taken care of.
				// However UTF8 is the most common encoding, and it includes Ascii (7-bit), so the user
				// just needs to know that 'str' objects returned by pyGPlates are UTF8-encoded.
				// They can decode and re-encode to their own 'str' encoding if they use a different encoding.
				//
				// Also this means we're returning 'str' in both Python 2 and Python 3 which is consistent
				// even if they represent different types internally (encoded byte stream vs unicode).
				//
				// So, for the above reasons, I think it's probably safer to return 'str' (as UTF8)
				// instead of 'unicode' (for Python 2).
				//
				QByteArray byte_array = qstring.toUtf8();
				bp::str byte_string(byte_array.constData());
				return bp::incref(byte_string.ptr());
#endif
			};
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

#if PY_MAJOR_VERSION >= 3
			// Handle Python 'str' and  'bytes' since we can handle more than one type
			// when converting *from* Python.
			//
			// Note that we use 'PyUnicode_Check' instead of 'PyString_Check' since, in Python 3,
			// 'str' objects are unicode.
			return (PyUnicode_Check(obj) || PyBytes_Check(obj))
					? obj
					: NULL;
#else
			// Handle Python 'str' and  'unicode' since we can handle more than one type
			// when converting *from* Python.
			return (PyString_Check(obj) || PyUnicode_Check(obj))
					? obj
					: NULL;
#endif
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

#if PY_MAJOR_VERSION >= 3
			// Handle Python 'str' and  'bytes' since we can handle more than one type
			// when converting *from* Python.
			// Check if a Python 'str', otherwise it must be a Python 'bytes' since those are
			// the only two types that can get here (due to 'convertible()').
			//
			// Note that we use 'PyUnicode_Check' instead of 'PyString_Check' since, in Python 3,
			// 'str' objects are unicode.
			if (PyUnicode_Check(obj))
			{
				// Extracting Python 'str' object (which is unicode in Python 3) directly
				// as std::wstring is more direct than encoding and decoding as UTF8.
				// For this we take advantage of the implicit conversion from 'unicode' to
				// std::wstring in boost python.
				new (storage) QString(
						GPlatesUtils::make_qstring_from_wstring(bp::extract<std::wstring>(obj)));
			}
			else // PyBytes_Check(obj)
			{
				// Decode as UTF8.
				new (storage) QString(QString::fromUtf8(bp::extract<const char*>(obj)));
			}
#else
			// Handle Python 'str' and  'unicode' since we can handle more than one type
			// when converting *from* Python.
			// Check if a Python 'str', otherwise it must be a Python 'unicode' since those are
			// the only two types that can get here (due to 'convertible()').
			if (PyString_Check(obj))
			{
				// Decode as UTF8.
				new (storage) QString(QString::fromUtf8(bp::extract<const char*>(obj)));
			}
			else // PyUnicode_Check(obj)
			{
#if 1
				// Extracting Python 'unicode' object directly as std::wstring is more direct
				// than encoding and decoding as UTF8. For this we take advantage of the implicit
				// conversion from 'unicode' to std::wstring in boost python.
				new (storage) QString(
						GPlatesUtils::make_qstring_from_wstring(bp::extract<std::wstring>(obj)));
#else
				// First encode as UTF8 into a Python 'str' object.
				bp::object utf8_string = bp::object(bp::handle<>(PyUnicode_AsUTF8String(obj)));

				// Then decode from UTF8 into our unicode QString.
				new (storage) QString(QString::fromUtf8(bp::extract<const char*>(utf8_string)));
#endif
			}
#endif

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
	struct ConversionUnicodeString :
			private boost::noncopyable
	{
		explicit
		ConversionUnicodeString()
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


	/**
	 * Enables GPlatesModel::StringContentTypeGenerator<T> to be passed to and from python.
	 *
	 * For more information on boost python to/from conversions, see:
	 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
	 */
	template <class StringContentTypeGeneratorType>
	struct ConversionStringContentTypeGenerator :
			private boost::noncopyable
	{
		explicit
		ConversionStringContentTypeGenerator()
		{
			namespace bp = boost::python;

			// To python conversion.
			bp::to_python_converter<StringContentTypeGeneratorType, Conversion>();

			// From python conversion.
			bp::converter::registry::push_back(
					&convertible,
					&construct,
					bp::type_id<StringContentTypeGeneratorType>());
		}

		struct Conversion
		{
			static
			PyObject *
			convert(
					const StringContentTypeGeneratorType &string_content_type_generator)
			{
				namespace bp = boost::python;

				// Use the converter registered for UnicodeString
				// (StringContentTypeGeneratorType contains a UnicodeString).
				return bp::incref(bp::object(string_content_type_generator.get()).ptr());
			};
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			// StringContentTypeGeneratorType is constructed from a UnicodeString.
			return bp::extract<GPlatesUtils::UnicodeString>(obj).check() ? obj : NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<StringContentTypeGeneratorType> *>(
							data)->storage.bytes;

			// StringContentTypeGeneratorType is constructed from a UnicodeString.
			new (storage) StringContentTypeGeneratorType(bp::extract<GPlatesUtils::UnicodeString>(obj));

			data->convertible = storage;
		}
	};
}


void
export_qstring()
{
	// Registers the python to/from converters for QString.
	GPlatesApi::ConversionQString();

	// Enable boost::optional<QString> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<QString>();
}


void
export_unicode_string()
{
	// Registers the python to/from converters for GPlatesUtils::UnicodeString.
	GPlatesApi::ConversionUnicodeString();

	// Enable boost::optional<GPlatesUtils::UnicodeString> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesUtils::UnicodeString>();
}


void
export_xml_attribute_value()
{
	// Registers the python to/from converters for GPlatesModel::XmlAttributeValue.
	GPlatesApi::ConversionStringContentTypeGenerator<GPlatesModel::XmlAttributeValue>();

	// Enable boost::optional<GPlatesModel::XmlAttributeValue> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesModel::XmlAttributeValue>();
}


void
export_enumeration_content()
{
	// Registers the python to/from converters for GPlatesPropertyValues::EnumerationContent.
	GPlatesApi::ConversionStringContentTypeGenerator<GPlatesPropertyValues::EnumerationContent>();

	// Enable boost::optional<GPlatesPropertyValues::EnumerationContent> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesPropertyValues::EnumerationContent>();
}


void
export_text_content()
{
	// Registers the python to/from converters for GPlatesPropertyValues::TextContent.
	GPlatesApi::ConversionStringContentTypeGenerator<GPlatesPropertyValues::TextContent>();

	// Enable boost::optional<GPlatesPropertyValues::TextContent> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesPropertyValues::TextContent>();
}


void
export_strings()
{
	export_qstring();
	export_unicode_string();

	// Export the StringContentTypeGenerator template instantiations.
	export_xml_attribute_value();
	export_enumeration_content();
	export_text_content();
}

#endif // GPLATES_NO_PYTHON
