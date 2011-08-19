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

#include "model/ModelInterface.h"

namespace GPlatesFileIO
{
	static const char * ReadErrorAccumulationExceptionName = 
			"Read Error Accumulation Exception";
	class ArbitraryXmlReader
	{
	public:
		class ReadErrorAccumulationException: 
			public GPlatesGlobal::Exception
		{
		public:
			ReadErrorAccumulationException():
				GPlatesGlobal::Exception(GPLATES_EXCEPTION_SOURCE)
			{ }
			~ReadErrorAccumulationException() throw()
			{ }
		protected:
			void
			write_message(
					std::ostream &os) const
			{
				write_string_message(
					os,
					"Invalid ReadErrorAccumulation." 
					"ArbitraryXmlReader has not been configurated properly.");
			}

			const char *
			exception_name() const 
			{
				return ReadErrorAccumulationExceptionName;
			}
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
				GPlatesModel::ModelInterface &model,
				ReadErrorAccumulation &read_errors);

		void
		read_xml_data(
				File::Reference &file,
				boost::shared_ptr<ArbitraryXmlProfile> profile,
				GPlatesModel::ModelInterface &model,
				QByteArray& data,
				ReadErrorAccumulation &read_errors);
		
		int
		count_features(
				boost::shared_ptr<ArbitraryXmlProfile> profile,
				QByteArray& data,
				ReadErrorAccumulation &read_errors);
		
		ReadErrorAccumulation*
		get_read_error_accumulation() const
		{
			if(!d_read_errors)
			{
				throw ReadErrorAccumulationException();
			}
			return d_read_errors;
		}
		
		void
		set_read_error_accumulation(
				ReadErrorAccumulation* new_read_errors)
		{
			d_read_errors = new_read_errors;
		}
	private:
		ArbitraryXmlReader():d_read_errors(NULL) {}
		ArbitraryXmlReader(const ArbitraryXmlReader&);
		ReadErrorAccumulation* d_read_errors;

		class SetErrorAccumulation
		{
		public:
			SetErrorAccumulation(
					ReadErrorAccumulation* error_accumulation,
					ArbitraryXmlReader* parent):
				d_parent(parent)
			{
				d_parent->set_read_error_accumulation(error_accumulation);
			}
			~SetErrorAccumulation()
			{
				d_parent->set_read_error_accumulation(NULL);
			}
		private:
			ArbitraryXmlReader* d_parent;
		};
	};
}

#endif  // GPLATES_FILEIO_ARBITRARYXMLREADER_H
