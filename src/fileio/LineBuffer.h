/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_FILEIO_LINEBUFFER_H_
#define _GPLATES_FILEIO_LINEBUFFER_H_

#include <ios>
#include <iostream>
#include <string>

namespace GPlatesFileIO
{
	/**
	 * LineBuffer is a wrapper around a standard istream which allows
	 * you to associate a filename with that istream, and read from
	 * the istream in a line-oriented fashion.  It also keeps track of
	 * the line-number of the line you just read.
	 */
	class LineBuffer {

		public:

			LineBuffer(std::istream &istr, const char *fname)
			 : _istr(istr), _fname(fname), _line_num(0) {  }


			~LineBuffer() {  }


			/**
			 * Get a line from the istream.
			 * This member function is designed to behave just like
			 * the member function of the same name in istream.
			 */
			void *
			getline(char *str, std::streamsize count) {

				if ( ! _istr) {
					/*
					 * Input stream _istr is not
					 * in a usable state, hence
					 * every stream operation
					 * is a no-op until the state
					 * of _istr is reset
					 * (Josuttis, 13.4.2).
					 */
					return _istr;
				}
				_line_num++;
				return (_istr.getline(str, count));
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
			eof() const { return _istr.eof(); }


			/**
			 * Reset the failbit in the istream, if it is set.
			 */
			void
			resetFailbit() {

				// based upon code in Josuttis, 13.4.2
				if (_istr.rdstate() & std::ios::failbit) {
					/*
					 * Input stream _istr has its
					 * failbit set.  We only want
					 * to clear the failbit.
					 */
				  _istr.clear(_istr.rdstate()
					 & ~std::ios::failbit);
				}
			}


			/**
			 * Return the line number of the line you just read.
			 */
			unsigned
			lineNum() const { return _line_num; }


			void
			writeTo(std::ostream &ostr) const {

				ostr << "file \"" << _fname << "\" [line "
				     << _line_num << "]";
			}

		protected:

			// protected cctor, to help prevent accidental copying
			LineBuffer(const LineBuffer &other)
			 : _istr(other._istr),
			   _fname(other._fname),
			   _line_num(other._line_num) {  }

		private:

			std::istream &_istr;
			std::string _fname;
			unsigned _line_num;
	};


	inline std::ostream &
	operator<<(std::ostream &ostr, const LineBuffer &lbuf) {

		lbuf.writeTo(ostr);
		return ostr;
	}
}

#endif  // _GPLATES_FILEIO_LINEBUFFER_H_
