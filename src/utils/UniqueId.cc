/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include <sstream>

#include <ctime>
#include <cstdlib>

#include <QtCore/QUuid>
#include <QtNetwork/QHostInfo>

#include "UniqueId.h"

const UnicodeString
GPlatesUtils::generate_unique_id()
{
	QUuid uuid = QUuid::createUuid();

	char buffer[45];

	sprintf(buffer, "GPlates-%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
			uuid.data1, uuid.data2, uuid.data3, uuid.data4[0], uuid.data4[1],
			uuid.data4[2], uuid.data4[3], uuid.data4[4], uuid.data4[5], uuid.data4[6],
			uuid.data4[7]);

	return UnicodeString(buffer);
}
