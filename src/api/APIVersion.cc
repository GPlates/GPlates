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
#include <sstream>
#include <boost/optional.hpp>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "APIVersion.h"

#include "PyGPlatesModule.h"
#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "global/Version.h"


namespace bp = boost::python;


unsigned int
GPlatesApi::Version::get_imported_major()
{
	return GPlatesGlobal::Version::get_GPlates_version_major();
}


unsigned int
GPlatesApi::Version::get_imported_minor()
{
	return GPlatesGlobal::Version::get_GPlates_version_minor();
}


unsigned int
GPlatesApi::Version::get_imported_patch()
{
	return GPlatesGlobal::Version::get_GPlates_version_patch();
}


unsigned int
GPlatesApi::Version::get_imported_revision()
{
	return GPlatesGlobal::Version::get_pyGPlates_revision();
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
	std::stringstream version_class_docstring_stream;
	version_class_docstring_stream <<
			"A version of pyGPlates (GPlates Python API).\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported and Version is "
			"hashable (can be used as a key in a ``dict``).\n"
			"\n"
			"| During the lifespan of pyGPlates, the :meth:`imported pyGPlates version<get_imported_version>` "
			"has been incremented for each API change. So it can be used to ensure new API additions are "
			"present in the imported pyGPlates library.\n"
			"| For example, if we are using a new API function that was added in this revision (which is "
			<< GPlatesGlobal::Version::get_pyGPlates_revision() <<
			") then we can ensure we are using a sufficient API version by checking "
			"this at the beginning of our script:\n"
			"\n"
			"::\n"
			"\n"
			"  if pygplates.Version.get_imported_version() < pygplates.Version("
			<< GPlatesGlobal::Version::get_pyGPlates_revision() <<
			"):\n"
			"      print 'PyGPlates version %s is not supported' % "
			"pygplates.Version.get_imported_version()\n"
			"\n"
			"To print the version string of the imported pyGPlates library:\n"
			"::\n"
			"\n"
			"  print 'imported pyGPlates version: %s' % pygplates.Version.get_imported_version()\n"
			"\n"
			"...which for pyGPlates revision "
			<< GPlatesGlobal::Version::get_pyGPlates_revision() <<
			" (associated with GPlates version "
			<< GPlatesGlobal::Version::get_GPlates_version().toStdString() <<
			") will print ``"
			<< GPlatesGlobal::Version::get_pyGPlates_revision() << " (GPlates "
			<< GPlatesGlobal::Version::get_GPlates_version().toStdString() <<
			")``. However, note that the associated GPlates version is only printed "
			"when using :meth:`get_imported_version`. So the following example only prints the "
			"revision number ``"
			<< GPlatesGlobal::Version::get_pyGPlates_revision() <<
			"``:\n"
			"::\n"
			"\n"
			"  print 'PyGPlates version: %s' % pygplates.Version("
			<< GPlatesGlobal::Version::get_pyGPlates_revision() <<
			")\n"
			"\n"
			"There is also a ``pygplates.__version__`` string equal to the concatenation of the GPlates "
			"version and the pyGPlates revision of the imported pyGPlates library. For pyGPlates revision "
			<< GPlatesGlobal::Version::get_pyGPlates_revision() <<
			" (associated with GPlates version "
			<< GPlatesGlobal::Version::get_GPlates_version().toStdString() <<
			") this would be ``'"
			<< GPlatesGlobal::Version::get_GPlates_version().toStdString() << "."
			<< GPlatesGlobal::Version::get_pyGPlates_revision() <<
			"'``.\n";

	std::stringstream version_init_docstring_stream;
	version_init_docstring_stream <<
			"__init__(revision)\n"
			"  Create a *Version* instance from a *revision* number.\n"
			"\n"
			"  :param revision: the revision number\n"
			"  :type revision: int\n"
			"\n"
			"  To check if the imported pyGPlates library is revision "
			<< GPlatesGlobal::Version::get_pyGPlates_revision() <<
			" or greater:\n"
			"  ::\n"
			"\n"
			"    if pygplates.Version.get_imported_version() < pygplates.Version("
			<< GPlatesGlobal::Version::get_pyGPlates_revision() <<
			"):\n"
			"        print 'PyGPlates version %s is not supported' % "
			"pygplates.Version.get_imported_version()\n";

	std::stringstream version_get_imported_version_docstring_stream;
	version_get_imported_version_docstring_stream <<
			"get_imported_version()\n"
			// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
			// (like it can a pure python function) and we cannot document it in first (signature) line
			// because it messes up Sphinx's signature recognition...
			"  [*staticmethod*] Return the version of the imported pyGPlates library.\n"
			"\n"
			"  :returns: a Version instance representing the :meth:`revision number<get_revision>` "
			"of the imported pyGPlates library\n"
			"  :rtype: :class:`Version`\n"
			"\n"
			"  To check if the imported pyGPlates library is revision "
			<< GPlatesGlobal::Version::get_pyGPlates_revision() <<
			" or greater:\n"
			"  ::\n"
			"\n"
			"    if pygplates.Version.get_imported_version() < pygplates.Version("
			<< GPlatesGlobal::Version::get_pyGPlates_revision() <<
			"):\n"
			"        print 'PyGPlates version %s is not supported' % "
			"pygplates.Version.get_imported_version()\n";

	//
	// Version - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesApi::Version>(
			"Version",
			version_class_docstring_stream.str().c_str(),
			bp::init<unsigned int>(
					(bp::arg("revision")),
					version_init_docstring_stream.str().c_str()))
		.def("get_imported_version",
				&GPlatesApi::Version::get_imported_version,
				version_get_imported_version_docstring_stream.str().c_str())
		.staticmethod("get_imported_version")
		.def("get_revision",
				&GPlatesApi::Version::get_revision,
				"get_revision()\n"
				"  Returns the integer revision number.\n"
				"\n"
				"  :rtype: int\n")
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__' is based on 'id()' which is not compatible and
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
	bp::scope().attr("__version__") =
			bp::object(
					QString::number(GPlatesApi::Version::get_imported_major()) + '.' +
							QString::number(GPlatesApi::Version::get_imported_minor()) + '.' +
							QString::number(GPlatesApi::Version::get_imported_patch()) + '.' +
							QString::number(GPlatesApi::Version::get_imported_revision()));

}
