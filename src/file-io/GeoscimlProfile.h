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

#ifndef GPLATES_FILEIO_GEOSCIMLPROFILE_H
#define GPLATES_FILEIO_GEOSCIMLPROFILE_H

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "ArbitraryXmlProfile.h"
#include "model/FeatureCollectionHandle.h"

namespace GPlatesFileIO
{
	class GeoscimlProfile :
			public ArbitraryXmlProfile
	{
	public:

		/**
		 * Reports the progress of @a populate to the user and lets them cancel it.
		 *
		 * The only implementation (@a GPlatesQtWidgets::GeoscimlProgressDialog) is a Qt Widgets
		 * progress dialog, which the pygplates module does not link, so - like
		 * @a RasterReader::set_rgba_reader_factory - it is injected by GPlates
		 * (see @a GPlatesPresentation::Application) rather than referenced from here.
		 * When no factory is registered @a populate runs silently and cannot be cancelled.
		 */
		class ProgressReporter
		{
		public:

			virtual
			~ProgressReporter()
			{ }

			/**
			 * The number of features about to be translated.
			 */
			virtual
			void
			set_count(
					int count) = 0;

			/**
			 * Called before translating the feature at @a index (1-based).
			 *
			 * Returns false if the user has cancelled, in which case @a populate stops.
			 */
			virtual
			bool
			update(
					int index) = 0;
		};

		typedef boost::function<boost::shared_ptr<ProgressReporter> ()>
				progress_reporter_factory_type;

		/**
		 * Registers the factory that @a populate uses to create its @a ProgressReporter.
		 */
		static
		void
		set_progress_reporter_factory(
				const progress_reporter_factory_type &progress_reporter_factory);


		GeoscimlProfile()
		{ }

		explicit
		GeoscimlProfile(
				const QString& profile_name)
		{ }
		
		void
		populate(
				File::Reference& xml_file);

		void
		populate(
				QByteArray& xml_data,
				GPlatesModel::FeatureCollectionHandle::weak_ref fch);
				
		// check for features, return the number 
		int 
		count_features(
				QByteArray& xml_data);

	protected:
		GeoscimlProfile(
					const GeoscimlProfile&);

	private:
		static progress_reporter_factory_type s_progress_reporter_factory;

	};
}

#endif  // GPLATES_FILEIO_GEOSCIMLPROFILE_H
