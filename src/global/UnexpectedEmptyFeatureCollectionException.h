/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_GLOBAL_UNEXPECTEDEMPTYFEATURECOLLECTIONEXCEPTION_H
#define GPLATES_GLOBAL_UNEXPECTEDEMPTYFEATURECOLLECTIONEXCEPTION_H

#include "Exception.h"

namespace GPlatesGlobal
{
	/**
	 * Should be thrown when a function which expects a non-empty 
	 * FeatureCollectionHandle is given an empty one.
	 */
	class UnexpectedEmptyFeatureCollectionException : public Exception
	{
		public:
			/**
			 * @param msg is a message describing the situation.
			 */
			explicit
			UnexpectedEmptyFeatureCollectionException(const char *msg)
				: _msg(msg) {  }

			virtual
			~UnexpectedEmptyFeatureCollectionException() {  }

		protected:
			virtual const char *
			ExceptionName() const {

				return "UnexpectedEmptyFeatureCollectionException";
			}

			virtual std::string
			Message() const { return _msg; }

		private:
			std::string _msg;
	};
}

#endif  // GPLATES_GLOBAL_UNEXPECTEDEMPTYFEATURECOLLECTIONEXCEPTION_H
