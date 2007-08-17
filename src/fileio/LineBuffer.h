/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_LINEBUFFER_H
#define GPLATES_FILEIO_LINEBUFFER_H

#include <iostream>
#include <string>

namespace GPlatesFileIO
{
	/**
	 * LineBuffer is a wrapper around a standard istream which allows you to associate a
	 * filename with that istream, and read from the istream in a line-oriented fashion.
	 *
	 * It also keeps track of the line-number of the line you just read.
	 */
	class LineBuffer
	{
		public:
			/**
			 * This is the type used for line-number counting.
			 */
			typedef unsigned line_num_type;

			// Use std::string parameters for consistency: overhead is minimal.
			LineBuffer(
					std::istream &istr,
					const std::string& fname):
				d_istr_ptr(&istr),
				d_fname(fname),
				d_line_num(0)
			{  }


			~LineBuffer()
			{  }


			/**
			 * Get a line from the istream.
			 *
			 * This member function is designed to behave just like the member function
			 * of the same name in istream.
			 */
			void *
			getline(
					std::string &str) {

				if ( ! *d_istr_ptr) {
					/*
					 * Input stream *d_istr_ptr is not in a usable state, hence
					 * every stream operation is a no-op until the state of
					 * *d_istr_ptr is reset (Josuttis, 13.4.2).
					 */
					return *d_istr_ptr;
				}
				d_line_num++;
				// Use global getline instead of istream member.
				return (std::getline(*d_istr_ptr, str));
			}


			/**
			 * Return bool of whether EOF has been encountered.
			 *
			 * Note that in C++ I/O, the state of the stream is
			 * not set to 'eof' until a read has been attempted,
			 * and failed due to reaching EOF.  Thus, generally
			 * the 'failbit' will be set at the same time as
			 * the 'eofbit'.
			 */
			bool
			eof() const
			{
				return d_istr_ptr->eof();
			}


			/**
			 * Reset the failbit in the istream, if it is set.
			 */
			void
			resetFailbit()
			{
				// Based upon code in Josuttis, 13.4.2
				if (d_istr_ptr->rdstate() & std::ios::failbit) {
					/*
					 * Input stream *d_istr_ptr has its failbit set.  We only
					 * want to clear the failbit.
					 */
					d_istr_ptr->clear(d_istr_ptr->rdstate() & ~std::ios::failbit);
				}
			}


			/**
			 * Return the line number of the line you just read.
			 */
			line_num_type
			lineNum() const
			{
				return d_line_num;
			}


			void
			writeTo(
					std::ostream &ostr) const
			{
				ostr << "\"" << d_fname << "\" [line " << d_line_num << "]";
			}

		protected:

			// A protected cctor, to help prevent accidental copying.
			LineBuffer(
					const LineBuffer &other):
				d_istr_ptr(other.d_istr_ptr),
				d_fname(other.d_fname),
				d_line_num(other.d_line_num)
			{  }

		private:

			std::istream *d_istr_ptr;
			const std::string d_fname;  //  This shouldn't change.
			line_num_type d_line_num;
	};


	inline
	std::ostream &
	operator<<(
			std::ostream &ostr,
			const LineBuffer &lbuf)
	{
		lbuf.writeTo(ostr);
		return ostr;
	}
}

#endif  // GPLATES_FILEIO_LINEBUFFER_H
