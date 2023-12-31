/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2008 The University of Sydney, Australia
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

#ifndef _GPLATES_MATHS_HIGHPRECISION_H_
#define _GPLATES_MATHS_HIGHPRECISION_H_

#include <ostream>
#include <sstream>
#include <QDebug>
#include <QTextStream>


namespace GPlatesMaths
{
	/**
	 * This class is used to enable high-precision output of scalar values.
	 *
	 * Since it is a template class, it can be used to cause a temporary "burst" of
	 * high-precision output of any variable.  Simply place 'HighPrecision( )' around the
	 * variable which is being inserted into a stream.
	 * 
	 * For example:
	 * - std::cout << val << std::endl;
	 * would become:
	 * - std::cout << HighPrecision(val) << std::endl;
	 */
	template< typename T >
	class HighPrecision
	{
		public:
			typedef T output_type;

			explicit
			HighPrecision(
					const T &val):
				d_val(val)
			{  }

			void
			write_to(
					std::ostream &os) const;

		private:
			static const unsigned int HIGH_PRECISION = 18;

			output_type d_val;
	};


	template< typename T >
	inline
	void
	HighPrecision< T >::write_to(
			std::ostream &os) const
	{
		std::streamsize high_prec = HIGH_PRECISION;
		std::streamsize orig_prec = os.precision(high_prec);
		os << d_val;
		os.precision(orig_prec);
	}


	/**
	 * Write to standard stream.
	 */
	template< typename T >
	inline
	std::ostream &
	operator<<(
			std::ostream &os,
			const HighPrecision< T > &hp)
	{
		hp.write_to(os);
		return os;
	}


	/**
	 * Write using qDebug(), qWarning(), qCritical() or qFatal().
	 */
	template< typename T >
	inline
	QDebug
	operator <<(
			QDebug dbg,
			const HighPrecision< T > &hp)
	{
		std::ostringstream output_string_stream;
		output_string_stream << hp;

		dbg.nospace() << QString::fromStdString(output_string_stream.str());

		return dbg.space();
	}


	/**
	 * Write to a QTextStream.
	 */
	template< typename T >
	inline
	QTextStream &
	operator <<(
			QTextStream &stream,
			const HighPrecision< T > &hp)
	{
		std::ostringstream output_string_stream;
		output_string_stream << hp;

		stream << QString::fromStdString(output_string_stream.str());

		return stream;
	}
}

#endif  // _GPLATES_MATHS_HIGHPRECISION_H_
