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
			GPlatesGlobal::Version::get_pyGPlates_version_release_suffix());
}


GPlatesApi::Version::Version(
		unsigned int major,
		unsigned int minor,
		unsigned int patch,
		boost::optional<QString> deprecated_prerelease_suffix_string,
		boost::optional<QString> release_suffix_string) :
	d_major(major),
	d_minor(minor),
	d_patch(patch)
{
	// Renamed 'prerelease_suffix' keyword argument to 'release_suffix' since version 1.0.
	// Prior to 1.0 was 'prerelease_suffix', and since 1.0 it is 'release_suffix'.
	// So only one should be specified by the user as a keyword argument.
	// And if user instead specifies as a positional argument then it'll come via
	// 'deprecated_prerelease_suffix_string' (and 'release_suffix_string' will be none).
	if (deprecated_prerelease_suffix_string && release_suffix_string)
	{
		PyErr_SetString(
			PyExc_ValueError,
			"Cannot specify both new 'release_suffix_string' and deprecated 'prerelease_suffix_string' arguments");
		bp::throw_error_already_set();
	}

	// If release suffix comes via 'deprecated_prerelease_suffix_string' then transfer it to 'release_suffix_string'.
	if (deprecated_prerelease_suffix_string)
	{
		release_suffix_string = deprecated_prerelease_suffix_string;
		deprecated_prerelease_suffix_string = boost::none;
	}

	bool valid_version = true;

	if (release_suffix_string)
	{
		if (!extract_release_suffix(release_suffix_string.get()))
		{
			valid_version = false;
		}
	}

	if (!valid_version)
	{
		PyErr_SetString(
			PyExc_ValueError,
			"Version string is not in expected PEP440 format 'major.minor[.patch][{a|b|rc}N][.postN][.devN]'");
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

	// Version string should match "N.N[.N][<release_suffix>]".
	const QRegExp version_regex("^(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)(?:\\.(0|[1-9][0-9]*))?(.+)?$");

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
			// Convert patch string to integer.
			d_patch = patch_match.toUInt();
		}

		// Extract optional release suffix (defaults to none).
		const QString release_suffix_match = version_regex.cap(4);
		if (!release_suffix_match.isEmpty())
		{
			if (!extract_release_suffix(release_suffix_match))
			{
				valid_version = false;
			}
		}
	}

	// Either our regular expression or the release suffix regular expression didn't match.
	if (!valid_version)
	{
		PyErr_SetString(
				PyExc_ValueError,
				"Version string is not in expected PEP440 format 'major.minor[.patch][{a|b|rc}N][.postN][.devN]'");
		bp::throw_error_already_set();
	}
}


bool
GPlatesApi::Version::extract_release_suffix(
		QString release_suffix_string)
{
	// Release suffix should match "[{a|b|rc}N][.postN][.devN]".
	const QRegExp release_suffix_regex(
			"^(?:(a|b|rc)(0|[1-9][0-9]*))?(?:\\.post(0|[1-9][0-9]*))?(?:\\.dev(0|[1-9][0-9]*))?$");

	if (release_suffix_regex.indexIn(release_suffix_string) == -1)
	{
		// Regular expression didn't match so the release suffix string is not valid.
		return false;
	}

	// Extract from pre-release suffix (if any) from regex.
	const QString pre_release_suffix_type_string = release_suffix_regex.cap(1);
	if (!pre_release_suffix_type_string.isEmpty())
	{
		// Convert type string to type enum.
		PreReleaseSuffix::Type pre_release_suffix_type;

		if (pre_release_suffix_type_string == "a")
		{
			pre_release_suffix_type = PreReleaseSuffix::ALPHA;
		}
		else if (pre_release_suffix_type_string == "b")
		{
			pre_release_suffix_type = PreReleaseSuffix::BETA;
		}
		else if (pre_release_suffix_type_string == "rc")
		{
			pre_release_suffix_type = PreReleaseSuffix::RELEASE_CANDIDATE;
		}
		else
		{
			// Shouldn't get here.
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		}

		const unsigned int pre_release_suffix_number = release_suffix_regex.cap(2).toUInt();

		d_pre_release_suffix = PreReleaseSuffix{ pre_release_suffix_type, pre_release_suffix_number };
	}

	// Extract from post-release suffix (if any) from regex.
	const QString post_release_suffix_number_string = release_suffix_regex.cap(3);
	if (!post_release_suffix_number_string.isEmpty())
	{
		const unsigned int post_release_suffix_number = post_release_suffix_number_string.toUInt();

		d_post_release_suffix = PostReleaseSuffix{ post_release_suffix_number };
	}

	// Extract from development-release suffix (if any) from regex.
	const QString development_release_suffix_number_string = release_suffix_regex.cap(4);
	if (!development_release_suffix_number_string.isEmpty())
	{
		const unsigned int development_release_suffix_number = development_release_suffix_number_string.toUInt();

		d_development_release_suffix = DevelopmentReleaseSuffix{ development_release_suffix_number };
	}

	return true;
}


boost::optional<QString>
GPlatesApi::Version::get_release_suffix_string() const
{
	QString release_suffix_string;

	if (d_pre_release_suffix)
	{
		switch (d_pre_release_suffix->type)
		{
		case PreReleaseSuffix::ALPHA:
			release_suffix_string += QString("a") + QString::number(d_pre_release_suffix->number);
			break;

		case PreReleaseSuffix::BETA:
			release_suffix_string += QString("b") + QString::number(d_pre_release_suffix->number);
			break;

		case PreReleaseSuffix::RELEASE_CANDIDATE:
			release_suffix_string += QString("rc") + QString::number(d_pre_release_suffix->number);
			break;
		}
	}

	if (d_post_release_suffix)
	{
		release_suffix_string += QString(".post") + QString::number(d_post_release_suffix->number);
	}

	if (d_development_release_suffix)
	{
		release_suffix_string += QString(".dev") + QString::number(d_development_release_suffix->number);
	}

	if (!release_suffix_string.isEmpty())
	{
		return release_suffix_string;
	}

	return boost::none;
}


QString
GPlatesApi::Version::get_version_string() const
{
	QString version =
			QString::number(d_major) + '.' +
			QString::number(d_minor) + '.' +
			QString::number(d_patch);

	boost::optional<QString> release_suffix_string = get_release_suffix_string();
	if (release_suffix_string)
	{
		version += release_suffix_string.get();
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

	if (d_pre_release_suffix && rhs.d_pre_release_suffix)
	{
		if (d_pre_release_suffix->type != rhs.d_pre_release_suffix->type ||
			d_pre_release_suffix->number != rhs.d_pre_release_suffix->number)
		{
			return false;
		}
	}
	else if (d_pre_release_suffix || rhs.d_pre_release_suffix)
	{
		return false;
	}

	if (d_post_release_suffix && rhs.d_post_release_suffix)
	{
		if (d_post_release_suffix->number != rhs.d_post_release_suffix->number)
		{
			return false;
		}
	}
	else if (d_post_release_suffix || rhs.d_post_release_suffix)
	{
		return false;
	}

	if (d_development_release_suffix && rhs.d_development_release_suffix)
	{
		if (d_development_release_suffix->number != rhs.d_development_release_suffix->number)
		{
			return false;
		}
	}
	else if (d_development_release_suffix || rhs.d_development_release_suffix)
	{
		return false;
	}

	return true;
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

	//
	// Major.Minor.Patch is equal for both sides.
	//

	// Release suffix.
	if (d_pre_release_suffix && rhs.d_pre_release_suffix)
	{
		// The 'type' enum values are ordered by version precedence (ie, a < b < rc).
		if (d_pre_release_suffix->type < rhs.d_pre_release_suffix->type)
		{
			return true;
		}
		else if (d_pre_release_suffix->type > rhs.d_pre_release_suffix->type)
		{
			return false;
		}

		if (d_pre_release_suffix->number < rhs.d_pre_release_suffix->number)
		{
			return true;
		}
		else if (d_pre_release_suffix->number > rhs.d_pre_release_suffix->number)
		{
			return false;
		}
	}
	else if (d_pre_release_suffix)
	{
		// Pre-releases have lower precedence (unless other other side is purely a development-release).
		if (!rhs.d_post_release_suffix && rhs.d_development_release_suffix)
		{
			return false;
		}

		return true;
	}
	else if (rhs.d_pre_release_suffix)
	{
		// Pre-releases have lower precedence (unless other other side is purely a development-release).
		if (!d_post_release_suffix && d_development_release_suffix)
		{
			return true;
		}

		// Pre-releases have lower precedence.
		return false;
	}
	// ...else neither side has a pre-release suffix.

	if (d_post_release_suffix && rhs.d_post_release_suffix)
	{
		if (d_post_release_suffix->number < rhs.d_post_release_suffix->number)
		{
			return true;
		}
		else if (d_post_release_suffix->number > rhs.d_post_release_suffix->number)
		{
			return false;
		}
	}
	else if (d_post_release_suffix)
	{
		// Post-releases have higher precedence.
		return false;
	}
	else if (rhs.d_post_release_suffix)
	{
		// Post-releases have higher precedence.
		return true;
	}
	// ...else neither side has a post-release suffix.

	if (d_development_release_suffix && rhs.d_development_release_suffix)
	{
		if (d_development_release_suffix->number < rhs.d_development_release_suffix->number)
		{
			return true;
		}
		else if (d_development_release_suffix->number > rhs.d_development_release_suffix->number)
		{
			return false;
		}
	}
	else if (d_development_release_suffix)
	{
		// Development-releases have lower precedence.
		return true;
	}
	else if (rhs.d_development_release_suffix)
	{
		// Development-releases have lower precedence.
		return false;
	}
	// ...else neither side has a development-release suffix.

	// Both versions are equal.
	return false;
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

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_pre_release_suffix, "pre_release_suffix") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, d_post_release_suffix, "post_release_suffix") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, d_development_release_suffix, "development_release_suffix"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesApi::Version::PreReleaseSuffix::transcribe(
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


GPlatesScribe::TranscribeResult
GPlatesApi::Version::PostReleaseSuffix::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, number, "number"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesApi::Version::DevelopmentReleaseSuffix::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, number, "number"))
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
	 *              Versions are now major.minor[.patch][release_suffix].
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
				"version format is now major.minor[.patch][release_suffix]");
		bp::throw_error_already_set();

		// Shouldn't be able to get here.
		return 0;
	}

	/**
	 * DEPRECATED - Revision numbers are no longer supported.
	 *              Versions are now major.minor[.patch][release_suffix].
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
			!version.get_release_suffix_string())
		{
			return version.get_minor();
		}

		PyErr_SetString(
				PyExc_RuntimeError,
				"pygplates.Version.get_revision() deprecated - "
				"and only supported for versions <= 0.33 (where it now returns minor version) - "
				"version format is now major.minor[.patch][release_suffix]");
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
			"``N.N[.N][{a|b|rc}N][.postN][.devN]`` where ``N.N[.N]`` is the major.minor[.patch] version and "
			"``[{a|b|rc}N][.postN][.devN]`` is an optional release suffix. Examples include ``1.0.0`` for an official release, "
			"``1.0.0.dev1`` for a first development release and ``1.0.0rc1`` for a first release candidate.\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported and ``Version`` is "
			"hashable (can be used as a key in a ``dict``). "
			"And ``Version`` can be `pickled <https://docs.python.org/3/library/pickle.html>`_.\n"
			"\n"
			"| During the lifespan of pyGPlates, the :meth:`imported pyGPlates version<get_imported_version>` "
			"has been updated for each API change. So it can be used to ensure new API additions are "
			"present in the imported pyGPlates library.\n"
			"| For example, if we are using an API function that was added in version ``1.0`` "
			"(the official public release of pyGPlates in 2025) then we can ensure we are using a "
			"sufficient API version by checking this at the beginning of our script:\n"
			"\n"
			"::\n"
			"\n"
			"  if pygplates.Version.get_imported_version() < pygplates.Version(1, 0):\n"
			"      raise RuntimeError('Using pygplates version {0} but version {1} or greater is required'.format(\n"
			"          pygplates.Version.get_imported_version(), pygplates.Version(1, 0)))\n"
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
			"``.\n"
			"\n"
			".. versionchanged:: 0.42\n"
			"   Added pickle support.\n";

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
		.def(bp::init<unsigned int, unsigned int, unsigned int, boost::optional<QString>, boost::optional<QString>>(
				(bp::arg("major"),
						bp::arg("minor"),
						bp::arg("patch")=0,
						bp::arg("prerelease_suffix")=boost::optional<QString>(),  // 'prerelease_suffix' keyword deprecated since 1.0
						bp::arg("release_suffix")=boost::optional<QString>()),
				// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
				"__init__(...)\n"
				"A *Version* object can be constructed in more than one way...\n"
				"\n"
				// Specific overload signature...
				"__init__(major, minor, [patch=0], [release_suffix])\n"
				"  Create from major, minor, patch numbers and optional release suffix string.\n"
				"\n"
				"  :param major: the major version number\n"
				"  :type major: int\n"
				"  :param minor: the minor version number\n"
				"  :type minor: int\n"
				"  :param patch: the patch version number (defaults to zero)\n"
				"  :type patch: int\n"
				"  :param release_suffix: the optional release PEP440 suffix ``[{a|b|rc}N][.postN][.devN]`` (defaults to ``None``)\n"
				"  :type release_suffix: str or None\n"
				"  :raises: ValueError if *release_suffix* is specified but doesn't match pattern ``[{a|b|rc}N][.postN][.devN]``\n"
				"\n"
				"  To create version ``1.0``:\n"
				"  ::\n"
				"\n"
				"    version = pygplates.Version(1, 0)\n"
				"\n"
				"  .. versionadded:: 0.34\n"
				"\n"
				"  .. versionchanged:: 1.0\n"
				"     Deprecated *prerelease_suffix* argument and renamed to *release_suffix*.\n"))
		.def(bp::init<QString>(
				(bp::arg("version")),
				// Specific overload signature...
				"__init__(version)\n"
				"  Create from a version string.\n"
				"\n"
				"  :param version: the version string in PEP440 format matching ``N.N[.N][{a|b|rc}N][.postN][.devN]``\n"
				"  :type version: str\n"
				"  :raises: ValueError if version string doesn't match pattern ``N.N[.N][{a|b|rc}N][.postN][.devN]``\n"
				"\n"
				"  To create the first development release of version ``1.0``:\n"
				"  ::\n"
				"\n"
				"    version = pygplates.Version('1.0.dev1')\n"
				"\n"
				"  .. versionadded:: 0.34\n"))
		// Deprecated '__init__'...
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::deprecated_version_create,
						bp::default_call_policies(),
						(bp::arg("revision"))),
				// Specific overload signature...
				"__init__(revision)\n"
				"\n"
				"  Only supported when *revision* <= 33 (where creates version *0.revision*).\n"
				"\n"
				"  :param revision: the revision number\n"
				"  :type revision: int\n"
				"  :raises: RuntimeError if *revision* is greater than 33\n"
				"\n"
				"  .. deprecated:: 0.34\n")
		// Pickle support...
		//
		// Note: This adds an __init__ method accepting a single argument (of type 'bytes') that supports pickling.
		//       So we define this *after* (higher priority) the other __init__ methods in case one of them accepts a single argument
		//       of type bp::object (which, being more general, would otherwise obscure the __init__ that supports pickling).
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<boost::shared_ptr<GPlatesApi::Version>>())
		.def("get_imported_version",
				&GPlatesApi::Version::get_imported_version,
				"get_imported_version()\n"
				"  Return the version of the imported pyGPlates library.\n"
				"\n"
				"  :returns: a Version instance representing the version of the imported pyGPlates library\n"
				"  :rtype: Version\n"
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
				"  :rtype: int\n"
				"\n"
				"  .. versionadded:: 0.34\n")
		.def("get_minor",
				&GPlatesApi::Version::get_minor,
				"get_minor()\n"
				"\n"
				"  Return the minor version number.\n"
				"\n"
				"  :rtype: int\n"
				"\n"
				"  .. versionadded:: 0.34\n")
		.def("get_patch",
				&GPlatesApi::Version::get_patch,
				"get_patch()\n"
				"\n"
				"  Return the patch version number.\n"
				"\n"
				"  :rtype: int\n"
				"\n"
				"  .. versionadded:: 0.34\n")
		.def("get_prerelease_suffix",
				&GPlatesApi::Version::get_release_suffix_string,
				"get_prerelease_suffix()\n"
				"\n"
				"  Same as :meth:`get_release_suffix`.\n"
				"\n"
				"  .. versionadded:: 0.34\n"
				"\n"
				"  .. deprecated:: 1.0\n")
		.def("get_release_suffix",
				&GPlatesApi::Version::get_release_suffix_string,
				"get_release_suffix()\n"
				"\n"
				"  Return the PEP440 release suffix (matching pattern ``[{a|b|rc}N][.postN][.devN]``), "
				"or ``None`` if a *final* release.\n"
				"\n"
				"  :rtype: str or None\n"
				"\n"
				"  .. versionadded:: 1.0\n")
		// Deprecated method...
		.def("get_revision",
				&GPlatesApi::deprecated_version_get_revision,
				"get_revision()\n"
				"\n"
				"  Only supported for versions <= 0.33 (with zero patch number and no release suffix).\n"
				"\n"
				"  :returns: the minor version number\n"
				"  :rtype: int\n"
				"  :raises: RuntimeError if internal version is not <= 0.33 (with zero patch number and no release)\n"
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
	;

	// Enable boost::optional<Version> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::Version>();

	// Supply a module '__version__' string in PEP440 format.
	bp::scope().attr("__version__") =
			bp::object(GPlatesApi::Version::get_imported_version().get_version_string());
}
