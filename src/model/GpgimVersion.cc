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
#include <QDebug>
#include <QStringList>
#include <QTextStream>

#include "GpgimVersion.h"

#include "global/LogException.h"


boost::optional<GPlatesModel::GpgimVersion>
GPlatesModel::GpgimVersion::create(
		const QString &version)
{
	const QStringList version_fields = version.split('.', QString::KeepEmptyParts);
	// The number of fields should be 3 (or can be 2 if "major.minor" is "1.6").
	if (version_fields.count() < 2 ||
		version_fields.count() > 3)
	{
		qWarning() << "GpgimVersion: incorrect number of fields in version string.";
		return boost::none;
	}

	bool ok;

	// Parse the major version.
	const unsigned int major_ = version_fields[0].toUInt(&ok);
	if (!ok)
	{
		qWarning() << "GpgimVersion: unable to parse major version in version string.";
		return boost::none;
	}
	if (major_ == 0 || major_ > 9)
	{
		qWarning() << "GpgimVersion: major version should be a non-zero single digit integer.";
		return boost::none;
	}

	// Parse the minor version.
	const unsigned int minor_ = version_fields[1].toUInt(&ok);
	if (!ok)
	{
		qWarning() << "GpgimVersion: unable to parse minor version in version string.";
		return boost::none;
	}
	if (minor_ == 0 || minor_ > 9)
	{
		qWarning() << "GpgimVersion: minor version should be a non-zero single digit integer.";
		return boost::none;
	}
	if (major_ == 1 && minor_ < 6)
	{
		qWarning() << "GpgimVersion: cannot have a '<major>.<minor>' version less than '1.6'.";
		return boost::none;
	}

	// Determine the revision number.
	unsigned int revision_ = DEFAULT_ONE_POINT_SIX_REVISION;
	if (version_fields.count() == 2)
	{
		// Only "major.minor" == "1.6" can have an optional revision number.
		if (major_ != 1 || minor_ != 6)
		{
			qWarning() << "GpgimVersion: only version '1.6' can have an optional third revision field.";
			return boost::none;
		}
	}
	else // parse the revision number...
	{
		revision_ = version_fields[2].toUInt(&ok);
		if (!ok)
		{
			qWarning() << "GpgimVersion: only version '1.6' can have an optional third revision field.";
			return boost::none;
		}
		if (revision_ == 0 || revision_ > 9999)
		{
			qWarning() << "GpgimVersion: revision number should be a non-zero four digit integer.";
			return boost::none;
		}
	}

	return GpgimVersion(major_, minor_, revision_);
}


GPlatesModel::GpgimVersion::GpgimVersion(
		unsigned int major_,
		unsigned int minor_,
		unsigned int revision_) :
	d_major(major_),
	d_minor(minor_),
	d_revision(revision_)
{
	if (major_ == 0 || major_ > 9)
	{
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				"GpgimVersion: major version should be a non-zero single digit integer.");
	}

	if (minor_ == 0 || minor_ > 9)
	{
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				"GpgimVersion: minor version should be a non-zero single digit integer.");
	}

	if (major_ == 1 && minor_ < 6)
	{
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				"GpgimVersion: cannot have a '<major>.<minor>' version less than '1.6'.");
	}

	if (revision_ == 0 || revision_ > 9999)
	{
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				"GpgimVersion: revision number should be a non-zero four digit integer.");
	}
}


QString
GPlatesModel::GpgimVersion::version_string() const
{
	QString version_string_;
	QTextStream version_string_stream(&version_string_);

	version_string_stream << d_major << '.' << d_minor << '.';

	// The revision number occupies four characters with the first character a zero.
	version_string_stream.setFieldWidth(4);
	version_string_stream.setFieldAlignment(QTextStream::AlignRight);
	version_string_stream.setPadChar('0');
	version_string_stream << d_revision;

	return version_string_;
}


bool
GPlatesModel::GpgimVersion::operator<(
		const GpgimVersion &rhs) const
{
	if (d_major < rhs.d_major)
	{
		return true;
	}

	if (d_major == rhs.d_major)
	{
		if (d_minor < rhs.d_minor)
		{
			return true;
		}

		if (d_minor == rhs.d_minor)
		{
			return d_revision < rhs.d_revision;
		}
	}

	return false;
}


std::ostream &
GPlatesModel::operator<<(
		std::ostream &o,
		const GpgimVersion &gpml_version)
{
	o << gpml_version.version_string().toLatin1().data();

	return o;
}
