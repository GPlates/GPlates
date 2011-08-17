/* $Id$ */

/**
 * \file A wrapper around std::getenv() that returns QStrings.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "Environment.h"

#include <cstdlib>	// getenv()


QString
GPlatesUtils::getenv(
		const char *variable_name)
{
	// Protect against user error.
	if (variable_name == NULL) {
		return QString();
	}
	
	return QString(std::getenv(variable_name));
}


bool
GPlatesUtils::getenv_as_bool(
		const char *variable_name,
		bool default_value)
{
	QString value = GPlatesUtils::getenv(variable_name);
	if (value.isNull()) {
		return default_value;
	}
	
	// *ANY* value (as long as the variable is defined) is considered "true",
	// *UNLESS* the lowercased value is "0", "false", "off", "disabled", or "no".
	value = value.simplified().toLower().normalized(QString::NormalizationForm_KC);
	if (value == "0" || value == "false" || value == "off" || value == "disabled" || value == "no") {
		return false;
	} else {
		return true;
	}
}


