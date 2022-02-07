/* $Id$ */

/**
 * \file 
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

#ifndef GPLATES_UTILS_QTSTREAMABLE_H
#define GPLATES_UTILS_QTSTREAMABLE_H

#include <sstream>
#include <QDebug>
#include <QString>
#include <QTextStream>


namespace GPlatesUtils
{
	/**
	 * If you provide the following operator for a class 'Derived':
	 *
	 *    std::ostream &
	 *    operator <<(
	 *          std::ostream &os,
	 *          const Derived &derived_object);
	 *
	 * ...then if you inherit from 'QtStreamble' you can also do the following:
	 *
	 *    qDebug() << derived_object;
	 *    qWarning() << derived_object;
	 *    qCritical() << derived_object;
	 *    qFatal() << derived_object;
	 *
	 * ...and similary for any QTextStream.
	 *    
	 * The template parameter 'Derived' should be the type of class that is deriving from 'QtStreamable'.
	 * For example,
	 *    class UnitVector3D :
	 *       public GPlatesUtils::QtStreamable<UnitVector3D>
	 *    ...
	 *
	 * See the Barton–Nackman trick (http://en.wikipedia.org/wiki/Barton-Nackman_trick)
	 * for details on how this works.
	 */
	template <class Derived>
	class QtStreamable
	{
	public:

		/**
		 * Gives us:
		 *    qDebug() << derived_object;
		 *    qWarning() << derived_object;
		 *    qCritical() << derived_object;
		 *    qFatal() << derived_object;
		 */
		friend
		QDebug
		operator <<(
				QDebug dbg,
				const Derived &derived_object)
		{
			std::ostringstream output_string_stream;
			output_string_stream << derived_object;

			dbg.nospace() << QString::fromStdString(output_string_stream.str());

			return dbg.space();
		}


		/**
		 * Gives us:
		 *    QTextStream text_stream(device);
		 *    text_stream << derived_object;
		 */
		friend
		QTextStream &
		operator <<(
				QTextStream &stream,
				const Derived &derived_object)
		{
			std::ostringstream output_string_stream;
			output_string_stream << derived_object;

			stream << QString::fromStdString(output_string_stream.str());

			return stream;
		}
	};
}

#endif // GPLATES_UTILS_QTSTREAMABLE_H
