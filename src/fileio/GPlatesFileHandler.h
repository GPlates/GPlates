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

#ifndef _GPLATES_FILEIO_GPLATESFILEHANDLER_H_
#define _GPLATES_FILEIO_GPLATESFILEHANDLER_H_

#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax/AttributeList.hpp>

namespace GPlatesFileIO
{
	class GPlatesFileHandler : public xercesc::HandlerBase
	{
		public:
			GPlatesFileHandler()
				: _locator(NULL) {  }

			/** 
			 * SAX DocumentHandler interface.
			 */

			void
			startDocument() {  }

			/**
			 * Check whether everything that should have been specified
			 * has been specified.
			 */
			void
			endDocument();

			void
			startElement(const XMLCh* const name, 
						 xercesc::AttributeList& attrs);

			void
			endElement(const XMLCh* const name);

			void
			characters(const XMLCh* const chars, 
					   const unsigned int len) {  }

			void
			ignorableWhitespace(const XMLCh* chars, 
								const unsigned int len) {  }

			/**
			 * Register a locator.
			 */
			void
			setLocator(const xercesc::Locator* const locator) {

				_locator = locator;
			}

		private:
			const xercesc::Locator* _locator;
	};
}

#endif  // _GPLATES_FILEIO_GPLATESFILEHANDLER_H_
