/* $Id$ */

/**
 * @file 
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
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_FILEIO_ERRORHANDLER_H_
#define _GPLATES_FILEIO_ERRORHANDLER_H_

#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax/SAXParseException.hpp>

namespace GPlatesFileIO
{
	class ErrorHandler : public xercesc::HandlerBase
	{
		public:
			/**
			 * Error handler interface.
			 */

			void
			warning(const xercesc::SAXParseException& ex);

			void
			error(const xercesc::SAXParseException& ex);

			void
			fatalError(const xercesc::SAXParseException& ex);
	};
}

#endif  // _GPLATES_FILEIO_ERRORHANDLER_H_
