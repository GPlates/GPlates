/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_SCRIBEEXPORTEXTERNAL_H
#define GPLATES_SCRIBE_SCRIBEEXPORTEXTERNAL_H

#include <string>
#include <QByteArray>
#include <QString>
#include <QStringList>

#include "TranscribeUtils.h"


/**
 * Scribe export registered classes/types for *external* libraries.
 *
 * Also includes are the fundamental types (integral/floating-point).
 *
 * The list of libraries include:
 *   (1) C++ standard library,
 *   (2) Qt.
 *
 *
 * Note: Apparently 'char', 'signed char' and 'unsigned char' are three distinct types (unlike integer types).
 *
 * Note: The types 'short', 'int' and 'long' also pick up 'signed short', 'signed int' and 'signed long'.
 *
 * Also including scribe utility classes in this 'scribe' source sub-directory.
 *
 *******************************************************************************
 * WARNING: Changing the string ids will break backward/forward compatibility. *
 *******************************************************************************
 */
#define SCRIBE_EXPORT_EXTERNAL \
		\
		((char, "char")) \
		((signed char, "signed char")) \
		((unsigned char, "unsigned char")) \
		\
		((short, "short")) \
		((unsigned short, "unsigned short")) \
		\
		((int, "int")) \
		((unsigned int, "unsigned int")) \
		\
		((long, "long")) \
		((unsigned long, "unsigned long")) \
		\
		((float, "float")) \
		((double, "double")) \
		((long double, "long double")) \
		\
		\
		\
		((std::string, "std::string")) \
		\
		\
		\
		((QByteArray, "QByteArray")) \
		((QString, "QString")) \
		((QStringList, "QStringList")) \
		\
		\
		\
		((GPlatesScribe::TranscribeUtils::FilePath, \
			"GPlatesScribe::TranscribeUtils::FilePath")) \
		\


#endif // GPLATES_SCRIBE_SCRIBEEXPORTEXTERNAL_H
