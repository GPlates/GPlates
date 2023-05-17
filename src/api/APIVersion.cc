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
#include <boost/shared_ptr.hpp>
#include <QDebug>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "APIVersion.h"

#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"
#include "PythonPickle.h"

#include "global/GPlatesAssert.h"
#include "global/Version.h"

#include "scribe/Scribe.h"


namespace bp = boost::python;


GPlatesApi::Version
GPlatesApi::Version::get_imported_version()
{
	return Version(
			GPlatesGlobal::Version::get_pyGPlates_version_major(),
			GPlatesGlobal::Version::get_pyGPlates_version_minor(),
			GPlatesGlobal::Version::get_pyGPlates_version_patch(),
			GPlatesGlobal::Version::get_pyGPlates_version_prerelease_suffix());
}


GPlatesApi::Version::Version(
		unsigned int major,
		unsigned int minor,
		unsigned int patch,
		boost::optional<QString> prerelease_suffix_string) :
	d_major(major),
	d_minor(minor),
	d_patch(patch)
{
	bool valid_version = true;

	if (prerelease_suffix_string)
	{
		d_prerelease_suffix = extract_prerelease_suffix(prerelease_suffix_string.get());
		if (!d_prerelease_suffix)
		{
			valid_version = false;
		}
	}

	if (!valid_version)
	{
		PyErr_SetString(
			PyExc_ValueError,
			"Version string is not in expected format (major.minor[.patch][prerelease_suffix] and "
			"using only 'aN', 'bN', 'rcN' and '.devN' suffixes of PEP440 version spec)");
		bp::throw_error_already_set();
	}
}


GPlatesApi::Version::Version(
		QString version_string) :
	d_major(0),
	d_minor(0),
	d_patch(0)
{
	bool valid_version = true;

	// Version string should match "N.N[.N][prerelease_suffix]".
	QRegExp version_regex("^([0-9]+)\\.([0-9]+)(\\.[0-9]+)?(.+)?$");

	if (version_regex.indexIn(version_string) == -1)
	{
		valid_version = false;
	}
	else
	{
		// Extract from regex.
		d_major = version_regex.cap(1).toUInt();
		d_minor = version_regex.cap(2).toUInt();

		// Extract optional patch (defaults to 0).
		d_patch = 0;
		const QString patch_match = version_regex.cap(3);
		if (!patch_match.isEmpty())
		{
			// Remove the initial '.' and then convert patch to integer.
			d_patch = patch_match.right(patch_match.size() - 1).toUInt();
		}

		// Extract optional pre-release suffix (defaults to none).
		const QString prerelease_suffix_match = version_regex.cap(4);
		if (!prerelease_suffix_match.isEmpty())
		{
			d_prerelease_suffix = extract_prerelease_suffix(prerelease_suffix_match);
			if (!d_prerelease_suffix)
			{
				valid_version = false;
			}
		}
	}

	// Either our regular expression or the prerelease suffix regular expression didn't match.
	if (!valid_version)
	{
		PyErr_SetString(
				PyExc_ValueError,
				"Version string is not in expected format (major.minor[.patch][prerelease_suffix] and "
				"using only 'aN', 'bN', 'rcN' and '.devN' suffixes of PEP440 version spec)");
		bp::throw_error_already_set();
	}
}


boost::optional<GPlatesApi::Version::PrereleaseSuffix>
GPlatesApi::Version::extract_prerelease_suffix(
		QString prerelease_suffix_string)
{
	// Prerelease suffix should match ".devN", "aN", "bN" or "rcN".
	QRegExp prerelease_suffix_regex("^(\\.dev|a|b|rc)([0-9]+)$");

	if (prerelease_suffix_regex.indexIn(prerelease_suffix_string) == -1)
	{
		// Regular expression didn't match so the pre-release suffix string is not valid.
		return boost::none;
	}

	// Extract from regex.
	const QString prerelease_suffix_type_string = prerelease_suffix_regex.cap(1);
	const unsigned int prerelease_suffix_number = prerelease_suffix_regex.cap(2).toUInt();

	// Convert type string to type enum.
	PrereleaseSuffix::Type prerelease_suffix_type;
	if (prerelease_suffix_type_string == ".dev")
	{
		prerelease_suffix_type = PrereleaseSuffix::DEVELOPMENT;
	}
	else if (prerelease_suffix_type_string == "a")
	{
		prerelease_suffix_type = PrereleaseSuffix::ALPHA;
	}
	else if (prerelease_suffix_type_string == "b")
	{
		prerelease_suffix_type = PrereleaseSuffix::BETA;
	}
	else if (prerelease_suffix_type_string == "rc")
	{
		prerelease_suffix_type = PrereleaseSuffix::RELEASE_CANDIDATE;
	}
	else
	{
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	}

	if (prerelease_suffix_number < 0)
	{
		// Integer should be non-negative, so pre-release suffix string is not valid.
		return boost::none;
	}

	const PrereleaseSuffix prerelease_suffix = { prerelease_suffix_type, prerelease_suffix_number };

	return prerelease_suffix;
}


boost::optional<QString>
GPlatesApi::Version::get_prerelease_suffix_string() const
{
	if (!d_prerelease_suffix)
	{
		return boost::none;
	}

	switch (d_prerelease_suffix->type)
	{
	case PrereleaseSuffix::DEVELOPMENT:
		return QString(".dev") + QString::number(d_prerelease_suffix->number);

	case PrereleaseSuffix::ALPHA:
		return QString("a") + QString::number(d_prerelease_suffix->number);

	case PrereleaseSuffix::BETA:
		return QString("b") + QString::number(d_prerelease_suffix->number);

	case PrereleaseSuffix::RELEASE_CANDIDATE:
		return QString("rc") + QString::number(d_prerelease_suffix->number);
	}

	// Shouldn't get here.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
}


QString
GPlatesApi::Version::get_version_string() const
{
	QString version =
			QString::number(d_major) + '.' +
			QString::number(d_minor) + '.' +
			QString::number(d_patch);

	boost::optional<QString> prerelease_suffix_string = get_prerelease_suffix_string();
	if (prerelease_suffix_string)
	{
		version += prerelease_suffix_string.get();
	}

	return version;
}


bool
GPlatesApi::Version::operator==(
		const Version &rhs) const
{
	if (d_major != rhs.d_major)
	{
		return false;
	}

	if (d_minor != rhs.d_minor)
	{
		return false;
	}

	if (d_patch != rhs.d_patch)
	{
		return false;
	}

	if (d_prerelease_suffix && rhs.d_prerelease_suffix)
	{
		return d_prerelease_suffix->type == rhs.d_prerelease_suffix->type &&
				d_prerelease_suffix->number == rhs.d_prerelease_suffix->number;
	}
	else if (!d_prerelease_suffix && !rhs.d_prerelease_suffix)
	{
		return true;
	}
	else
	{
		return false;
	}
}


bool
GPlatesApi::Version::operator<(
		const Version &rhs) const
{
	// Major.
	if (d_major < rhs.d_major)
	{
		return true;
	}
	else if (d_major > rhs.d_major)
	{
		return false;
	}

	// Minor.
	if (d_minor < rhs.d_minor)
	{
		return true;
	}
	else if (d_minor > rhs.d_minor)
	{
		return false;
	}

	// Patch.
	if (d_patch < rhs.d_patch)
	{
		return true;
	}
	else if (d_patch > rhs.d_patch)
	{
		return false;
	}

	// Pre-release suffix.
	if (d_prerelease_suffix && rhs.d_prerelease_suffix)
	{
		// The 'type' enum values are ordered by version precedence (ie, .dev < a < b < rc).
		if (d_prerelease_suffix->type < rhs.d_prerelease_suffix->type)
		{
			return true;
		}
		else if (d_prerelease_suffix->type > rhs.d_prerelease_suffix->type)
		{
			return false;
		}

		return d_prerelease_suffix->number < rhs.d_prerelease_suffix->number;
	}
	else if (d_prerelease_suffix)
	{
		return true;
	}
	else // either (1) only rhs has pre-release suffix or (2) neither side has ...
	{
		return false;
	}
}


GPlatesScribe::TranscribeResult
GPlatesApi::Version::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<Version> &version)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, version->d_major, "major");
		scribe.save(TRANSCRIBE_SOURCE, version->d_minor, "minor");
		scribe.save(TRANSCRIBE_SOURCE, version->d_patch, "patch");
	}
	else // loading
	{
		unsigned int major, minor, patch;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, major, "major") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, minor, "minor") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, patch, "patch"))
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		version.construct_object(major, minor, patch);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesApi::Version::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_major, "major") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_minor, "minor") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_patch, "patch"))
		{
			return scribe.get_transcribe_result();
		}
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_prerelease_suffix, "prerelease_suffix"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesApi::Version::PrereleaseSuffix::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, type, "type") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, number, "number"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


std::ostream &
GPlatesApi::operator<<(
		std::ostream &os,
		const Version &version)
{
	os << version.get_version_string().toStdString();
	return os;
}


namespace GPlatesApi
{
	bp::object
	version_hash(
			const Version &version)
	{
		// Use the Python built-in 'hash()' function on the version string.
		return bp::object(PyObject_Hash(bp::object(version.get_version_string()).ptr()));
	}

	/**
	 * DEPRECATED - Creating Version using a revision number is no longer supported.
	 *              Versions are now major.minor[.patch][prerelease_suffix].
	 *              However we accept it as equivalent to 0.revision (for major.minor) since
	 *              that's essentially what it was up to (and including) revision 33.
	 */
	boost::shared_ptr<Version>
	deprecated_version_create(
			unsigned int revision_number)
	{
		// Support if revision number represents a version prior to the new versioning scheme.
		// This is so old Python source code using old pyGPlates versions still works.
		if (revision_number <= 33)
		{
			return boost::shared_ptr<Version>(new Version(0, revision_number));
		}

		PyErr_SetString(
				PyExc_RuntimeError,
				"pygplates.Version(revision) deprecated - "
				"and only supported for versions <= 0.33 - "
				"version format is now major.minor[.patch][prerelease_suffix]");
		bp::throw_error_already_set();

		// Shouldn't be able to get here.
		return 0;
	}

	/**
	 * DEPRECATED - Revision numbers are no longer supported.
	 *              Versions are now major.minor[.patch][prerelease_suffix].
	 *              However we return the minor version (as the revision number) if version is
	 *              currently 0.revision (for major.minor) and "revision" is 33 or less
	 *              (since that's essentially what versions were at the time).
	 */
	unsigned int
	deprecated_version_get_revision(
			const Version &version)
	{
		// Support if version is prior to the new versioning scheme.
		// This is so old Python source code using old pyGPlates versions still works.
		if (version.get_major() == 0 &&
			version.get_minor() <= 33 &&
			version.get_patch() == 0 &&
			!version.get_prerelease_suffix_string())
		{
			return version.get_minor();
		}

		PyErr_SetString(
				PyExc_RuntimeError,
				"pygplates.Version.get_revision() deprecated - "
				"and only supported for versions <= 0.33 (where it now returns minor version) - "
				"version format is now major.minor[.patch][prerelease_suffix]");
		bp::throw_error_already_set();

		// Shouldn't be able to get here.
		return 0;
	}
}


void
export_version()
{
	std::stringstream version_class_docstring_stream;
	version_class_docstring_stream <<
			"A version of pyGPlates (GPlates Python API).\n"
			"\n"
			"Versions are defined by the `PEP440 versioning scheme <https://www.python.org/dev/peps/pep-0440/>`_ as "
			"``N.N.N[(.dev|a|b|rc)N]`` where ``N.N.N`` is the major.minor.patch version and "
			"``(.dev|a|b|rc)N`` is an optional pre-release suffix. Examples include ``1.0.0`` for an official release, "
			"``1.0.0.dev1`` for a first development pre-release and ``1.0.0rc1`` for a first release candidate.\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported and Version is "
			"hashable (can be used as a key in a ``dict``).\n"
			"\n"
			"| During the lifespan of pyGPlates, the :meth:`imported pyGPlates version<get_imported_version>` "
			"has been updated for each API change. So it can be used to ensure new API additions are "
			"present in the imported pyGPlates library.\n"
			"| For example, if we are using an API function that was added in version ``0.28`` "
			"(the official beta public release of pyGPlates in 2020, known at the time as revision 28) "
			"then we can ensure we are using a sufficient API version by checking this at the beginning of our script:\n"
			"\n"
			"::\n"
			"\n"
			"  if pygplates.Version.get_imported_version() < pygplates.Version(0, 28):\n"
			"      raise RuntimeError('Using pygplates version {0} but version {1} or greater is required'.format(\n"
			"          pygplates.Version.get_imported_version(), pygplates.Version(0, 28)))\n"
			"\n"
			"To print the version string of the imported pyGPlates library:\n"
			"::\n"
			"\n"
			"  print('imported pyGPlates version: {}'.format(pygplates.Version.get_imported_version()))\n"
			"\n"
			"...which, for this version of pyGPlates, will print ``imported pyGPlates version: "
			<< GPlatesApi::Version::get_imported_version().get_version_string().toStdString() <<
			"``.\n"
			"\n"
			"There is also a ``pygplates.__version__`` string which will also print ``"
			<< GPlatesApi::Version::get_imported_version().get_version_string().toStdString() <<
			"``.\n";

	//
	// Version - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::Version,
			// A pointer holder is required by 'bp::make_constructor'...
			boost::shared_ptr<GPlatesApi::Version>
			// Since it's immutable it can be copied without worrying that a modification from the
			// C++ side will not be visible on the python side, or vice versa. It needs to be
			// copyable anyway so that boost-python can copy it into a shared holder pointer...
#if 0
			boost::noncopyable
#endif
			>(
					"Version",
					version_class_docstring_stream.str().c_str(),
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def(bp::init<unsigned int, unsigned int, unsigned int, boost::optional<QString>>(
				(bp::arg("major"),
						bp::arg("minor"),
						bp::arg("patch")=0,
						bp::arg("prerelease_suffix")=boost::optional<QString>()),
				// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
				"__init__(...)\n"
				"A *Version* object can be constructed in more than one way...\n"
				"\n"
				// Specific overload signature...
				"__init__(major, minor, [patch=0], [prerelease_suffix])\n"
				"  Create from major, minor, patch numbers and optional pre-release suffix string.\n"
				"\n"
				"  :param major: the major version number\n"
				"  :type major: int\n"
				"  :param minor: the minor version number\n"
				"  :type minor: int\n"
				"  :param patch: the patch version number (defaults to zero)\n"
				"  :type patch: int\n"
				"  :param prerelease_suffix: the optional pre-release PEP440 suffix ``(.dev|a|b|rc)N`` (defaults to ``None``)\n"
				"  :type prerelease_suffix: string or None\n"
				"  :raises: ValueError if *prerelease_suffix* is specified but doesn't match pattern ``(.dev|a|b|rc)N``\n"
				"\n"
				"  To create version ``0.28``:\n"
				"  ::\n"
				"\n"
				"    version = pygplates.Version(0, 28)\n"))
		.def(bp::init<QString>(
				(bp::arg("version")),
				// Specific overload signature...
				"__init__(version)\n"
				"  Create from a version string.\n"
				"\n"
				"  :param version: the version string in PEP440 format matching ``N.N[.N][(.dev|a|b|rc)N]``\n"
				"  :type version: string\n"
				"  :raises: ValueError if version string doesn't match pattern ``N.N[.N][(.dev|a|b|rc)N]``\n"
				"\n"
				"  To create the first development pre-release of version ``0.34``:\n"
				"  ::\n"
				"\n"
				"    version = pygplates.Version('0.34.dev1')\n"))
		// Deprecated '__init__'...
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::deprecated_version_create,
						bp::default_call_policies(),
						(bp::arg("revision"))),
				// Specific overload signature...
				"__init__(revision)\n"
				"\n"
				"  Only supported for versions <= 0.33 (where created version is 0.revision).\n"
				"\n"
				"  :param revision: the revision number\n"
				"  :type revision: int\n"
				"  :raises: RuntimeError if *revision* is greater than 33\n"
				"\n"
				"  .. deprecated:: 0.34\n")
		.def("get_imported_version",
				&GPlatesApi::Version::get_imported_version,
				"get_imported_version()\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Return the version of the imported pyGPlates library.\n"
				"\n"
				"  :returns: a Version instance representing the version of the imported pyGPlates library\n"
				"  :rtype: :class:`Version`\n"
				"\n"
				"  To get the imported version:\n"
				"  ::\n"
				"\n"
				"    imported_version = pygplates.Version.get_imported_version()\n")
		.staticmethod("get_imported_version")
		.def("get_major",
				&GPlatesApi::Version::get_major,
				"get_major()\n"
				"\n"
				"  Return the major version number.\n"
				"\n"
				"  :rtype: int\n")
		.def("get_minor",
				&GPlatesApi::Version::get_minor,
				"get_minor()\n"
				"\n"
				"  Return the minor version number.\n"
				"\n"
				"  :rtype: int\n")
		.def("get_patch",
				&GPlatesApi::Version::get_patch,
				"get_patch()\n"
				"\n"
				"  Return the patch version number.\n"
				"\n"
				"  :rtype: int\n")
		.def("get_prerelease_suffix",
				&GPlatesApi::Version::get_prerelease_suffix_string,
				"get_prerelease_suffix()\n"
				"\n"
				"  Return the pre-release PEP440 suffix (matching pattern ``(.dev|a|b|rc)N``), "
				"or ``None`` if not a pre-release.\n"
				"\n"
				"  :rtype: str or None\n")
		// Deprecated method...
		.def("get_revision",
				&GPlatesApi::deprecated_version_get_revision,
				"get_revision()\n"
				"\n"
				"  Only supported for versions <= 0.33 (with zero patch number and no pre-release suffix).\n"
				"\n"
				"  :returns: the minor version number\n"
				"  :rtype: int\n"
				"  :raises: RuntimeError if internal version is not <= 0.33 (with zero patch number and no pre-release)\n"
				"\n"
				"  .. deprecated:: 0.34\n")
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__' is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.def("__hash__", &GPlatesApi::version_hash)
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		.def(bp::self < bp::self)
		.def(bp::self <= bp::self)
		.def(bp::self > bp::self)
		.def(bp::self >= bp::self)
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<boost::shared_ptr<GPlatesApi::Version>>())
	;

	// Enable boost::optional<Version> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::Version>();

	// Supply a module '__version__' string in PEP440 format.
	bp::scope().attr("__version__") =
			bp::object(GPlatesApi::Version::get_imported_version().get_version_string());
}
