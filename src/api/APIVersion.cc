/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <ostream>
#include <boost/optional.hpp>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "APIVersion.h"

#include "PyGPlatesModule.h"
#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "global/Constants.h"
#include "global/LogException.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	namespace
	{
		unsigned int
		convert_version_string_to_unsigned_int(
				const char *version_string,
				const char *error_message)
		{
			bool ok;
			const int version = QString(version_string).toInt(&ok);
			if (!ok ||
				version < 0)
			{
				throw GPlatesGlobal::LogException(GPLATES_EXCEPTION_SOURCE, error_message);
			}

			return version;
		}
	}
}


unsigned int
GPlatesApi::Version::get_imported_major()
{
	static boost::optional<unsigned int> major;
	if (!major)
	{
		major = convert_version_string_to_unsigned_int(
				GPlatesGlobal::GPlatesVersionMajor,
				"Api::Version: major version should be a non-negative integer.");
	}

	return major.get();
}


unsigned int
GPlatesApi::Version::get_imported_minor()
{
	static boost::optional<unsigned int> minor;
	if (!minor)
	{
		minor = convert_version_string_to_unsigned_int(
				GPlatesGlobal::GPlatesVersionMinor,
				"Api::Version: minor version should be a non-negative integer.");
	}

	return minor.get();
}


unsigned int
GPlatesApi::Version::get_imported_patch()
{
	static boost::optional<unsigned int> patch;
	if (!patch)
	{
		patch = convert_version_string_to_unsigned_int(
				GPlatesGlobal::GPlatesVersionPatch,
				"Api::Version: patch version should be a non-negative integer.");
	}

	return patch.get();
}


unsigned int
GPlatesApi::Version::get_imported_revision()
{
	static boost::optional<unsigned int> revision;
	if (!revision)
	{
		revision = convert_version_string_to_unsigned_int(
				GPlatesGlobal::PygplatesRevision,
				"Api::Version: revision should be a non-negative integer.");
	}

	return revision.get();
}


std::ostream &
GPlatesApi::operator<<(
		std::ostream &os,
		const Version &version)
{
	if (version.d_revision)
	{
		os << version.d_revision.get();
	}
	else
	{
		os
			<< Version::get_imported_revision()
			<< " (GPlates "
			<< Version::get_imported_major() << '.'
			<< Version::get_imported_minor() << '.'
			<< Version::get_imported_patch()
			<< ')';
	}

	return os;
}


void
export_version()
{
	//
	// Version - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesApi::Version>(
			"Version",
			"A version of pygplates (GPlates Python API).\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported and Version is "
			"hashable (can be used as a key in a ``dict``).\n"
			"\n"
			"During the lifespan of pygplates, the :meth:`imported pygplates version<get_imported_version>` "
			"has been incremented for each API change. So it can be used to ensure new API additions are "
			"present in the imported pygplates library. For example, if we are using a new API function that "
			"was added in revision 10 then we can ensure we are using a sufficient API version by checking "
			"this at the beginning of our script:\n"
			"::\n"
			"\n"
			"  if pygplates.Version.get_imported_version() < pygplates.Version(10):\n"
			"      print 'pygplates version %s is not supported' % "
			"pygplates.Version.get_imported_version()\n"
			"\n"
			"To print the version string of the imported pygplates library:\n"
			"::\n"
			"\n"
			"  print 'imported pygplates version: %s' % pygplates.Version.get_imported_version()\n"
			"\n"
			"...which for pygplates revision 10 (associated with GPlates version 1.4.0) will print "
			"``10 (GPlates 1.4.0)``. However, note that the associated GPlates version is only printed "
			"when using :meth:`get_imported_version`. So the following example only prints the "
			"revision number ``10``:\n"
			"::\n"
			"\n"
			"  print 'pygplates version: %s' % pygplates.Version(10)\n"
			"\n"
			"There is also a ``pygplates.__version__`` string equal to the concatenation of the GPlates "
			"version and the pygplates revision of the imported pygplates library. For pygplates revision "
			"10 (associated with GPlates version 1.4.0) this would be ``'1.4.0.10'``.\n",
			bp::init<unsigned int>(
					(bp::arg("revision")),
					"__init__(revision)\n"
					"  Create a *Version* instance from a *revision* number.\n"
					"\n"
					"  :param revision: the revision number\n"
					"  :type revision: int\n"
					"\n"
					"  To check if the imported pygplates library is revision 10 or greater:\n"
					"  ::\n"
					"\n"
					"    if pygplates.Version.get_imported_version() < pygplates.Version(10):\n"
					"        print 'pygplates version %s is not supported' % "
					"pygplates.Version.get_imported_version()\n"))
		.def("get_imported_version",
				&GPlatesApi::Version::get_imported_version,
				"get_imported_version() -> Version\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Return the version of the imported pygplates library.\n"
				"\n"
				"  :returns: a Version instance representing the :meth:`revision number<get_revision>` "
				"of the imported pygplates library\n"
				"  :rtype: :class:`Version`\n"
				"\n"
				"  To check if the imported pygplates library is revision 10 or greater:\n"
				"  ::\n"
				"\n"
				"    if pygplates.Version.get_imported_version() < pygplates.Version(10):\n"
				"        print 'pygplates version %s is not supported' % "
				"pygplates.Version.get_imported_version()\n")
		.staticmethod("get_imported_version")
		.def("get_revision",
				&GPlatesApi::Version::get_revision,
				"get_revision() -> int\n"
				"\n"
				"  :rtype: int\n"
				"\n"
				"  Returns the integer revision number.\n")
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__'is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.def("__hash__", &GPlatesApi::Version::get_revision)
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		.def(bp::self < bp::self)
		.def(bp::self <= bp::self)
		.def(bp::self > bp::self)
		.def(bp::self >= bp::self)
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Enable boost::optional<Version> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::Version>();

	// Supply a module '__version__' string - see http://legacy.python.org/dev/peps/pep-0396/#specification.
	GPlatesApi::get_pygplates_module().attr("__version__") =
			bp::object(
					QString::number(GPlatesApi::Version::get_imported_major()) + '.' +
							QString::number(GPlatesApi::Version::get_imported_minor()) + '.' +
							QString::number(GPlatesApi::Version::get_imported_patch()) + '.' +
							QString::number(GPlatesApi::Version::get_imported_revision()));

}

#endif // GPLATES_NO_PYTHON
