/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_FILEIO_ARBITRARYXMLREADER_H
#define GPLATES_FILEIO_ARBITRARYXMLREADER_H

#include <boost/shared_ptr.hpp>

#include "ArbitraryXmlProfile.h"
#include "File.h"
#include "FileInfo.h"
#include "ReadErrorAccumulation.h"


namespace GPlatesFileIO
{
	class ArbitraryXmlReader
	{
	public:

		class AccessedOutsideXmlProfileMethodException: 
			public GPlatesGlobal::Exception
		{
		public:
			AccessedOutsideXmlProfileMethodException():
				GPlatesGlobal::Exception(GPLATES_EXCEPTION_SOURCE)
			{ }
			~AccessedOutsideXmlProfileMethodException() throw()
			{ }
		protected:
			void
			write_message(
					std::ostream &os) const
			{
				write_string_message(
					os,
					"An ArbitraryXmlReader method using a XML profile has been accessed while the profile is not active.");
			}

			const char *
			exception_name() const 
			{
				return ACCESSED_OUTSIDE_XML_PROFILE_METHOD_EXCEPTION_NAME;
			}

		private:
			static const char *ACCESSED_OUTSIDE_XML_PROFILE_METHOD_EXCEPTION_NAME;
		};

		/*
		* TODO: This is not thread-safe.
		* TODO: Create an instance for each thread.
		*/
		static 
		ArbitraryXmlReader*
		instance()
		{
			static ArbitraryXmlReader* p = new ArbitraryXmlReader();
			return p;
		}

		void
		read_file(
				File::Reference &file,
				boost::shared_ptr<ArbitraryXmlProfile> profile,
				ReadErrorAccumulation &read_errors);

		void
		read_xml_data(
				File::Reference &file,
				boost::shared_ptr<ArbitraryXmlProfile> profile,
				QByteArray& data,
				ReadErrorAccumulation &read_errors);
		
		int
		count_features(
				boost::shared_ptr<ArbitraryXmlProfile> profile,
				QByteArray& data,
				ReadErrorAccumulation &read_errors);
		
		/**
		 * Throws exception if called while not inside @a read_file, @a read_xml_data or @a count_features.
		 */
		ReadErrorAccumulation*
		get_read_error_accumulation() const
		{
			if(!d_read_errors)
			{
				throw AccessedOutsideXmlProfileMethodException();
			}
			return d_read_errors;
		}

	private:

		ArbitraryXmlReader() :
			d_read_errors(NULL)
		{  }

		ArbitraryXmlReader(
				const ArbitraryXmlReader&);


		class SetXmlProfileAccess
		{
		public:
			SetXmlProfileAccess(
					ReadErrorAccumulation* error_accumulation,
					ArbitraryXmlReader* parent):
				d_parent(parent)
			{
				d_parent->d_read_errors = error_accumulation;
			}
			~SetXmlProfileAccess()
			{
				d_parent->d_read_errors = NULL;
			}
		private:
			ArbitraryXmlReader* d_parent;
		};


		ReadErrorAccumulation* d_read_errors;
	};
}

#endif  // GPLATES_FILEIO_ARBITRARYXMLREADER_H
