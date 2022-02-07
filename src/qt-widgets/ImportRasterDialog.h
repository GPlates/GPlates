/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_IMPORTRASTERDIALOG_H
#define GPLATES_QTWIDGETS_IMPORTRASTERDIALOG_H

#include <vector>
#include <boost/optional.hpp>
#include <QWizard>
#include <QString>

#include "OpenFileDialog.h"

#include "model/PropertyValue.h"

#include "property-values/Georeferencing.h"
#include "property-values/RasterType.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesFileIO
{
	struct ReadErrorAccumulation;
}

namespace GPlatesGui
{
	class FileIOFeedback;
	class UnsavedChangesTracker;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class TimeDependentRasterSequence
	{
	public:

		struct FileInfo
		{
			FileInfo(
					const boost::optional<double> &time_,
					const QString &absolute_file_path_,
					const QString &file_name_,
					const std::vector<GPlatesPropertyValues::RasterType::Type> &band_types_,
					unsigned width_,
					unsigned height_) :
				time(time_),
				absolute_file_path(absolute_file_path_),
				file_name(file_name_),
				band_types(band_types_),
				width(width_),
				height(height_)
			{  }

			boost::optional<double> time;
			QString absolute_file_path;
			QString file_name;
			std::vector<GPlatesPropertyValues::RasterType::Type> band_types;
			unsigned int width;
			unsigned int height;
		};

		typedef FileInfo element_type;
		typedef std::vector<element_type> sequence_type;

		const sequence_type &
		get_sequence() const
		{
			return d_sequence;
		}

		bool
		empty() const;

		void
		push_back(
				boost::optional<double> time,
				const QString &absolute_file_path,
				const QString &file_name,
				const std::vector<GPlatesPropertyValues::RasterType::Type> &band_types_,
				unsigned int width,
				unsigned int height);

		void
		add_all(
				const TimeDependentRasterSequence &other);

		void
		clear();

		void
		erase(
				unsigned int begin_index,
				unsigned int end_index);

		void
		set_time(
				unsigned int index,
				const boost::optional<double> &time);

		void
		sort_by_time();

		void
		sort_by_file_name();

	private:

		sequence_type d_sequence;
	};

	class ImportRasterDialog :
			public QWizard
	{
		Q_OBJECT

	public:

		explicit
		ImportRasterDialog(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				GPlatesGui::UnsavedChangesTracker *unsaved_changes_tracker,
				GPlatesGui::FileIOFeedback *file_io_feedback,
				QWidget *parent_ = NULL);

		/**
		 * Call this to open the import raster wizard, instead of @a show().
		 */
		void
		display(
				bool time_dependent_raster,
				GPlatesFileIO::ReadErrorAccumulation *read_errors = NULL);

	private:

		/**
		 * Wizard page ids.
		 */
		enum PageId
		{
			TIME_DEPENDENT_RASTER_PAGE_ID,
			RASTER_BAND_PAGE_ID,
			GEOREFERENCING_PAGE_ID,
			RASTER_FEATURE_COLLECTION_PAGE_ID
		};


		/**
		 * Override the next page id so we can skip georeferencing page if raster has inbuilt georeferencing.
		 */
		virtual
		int
		nextId() const;

		/**
		 * Returns (first) raster's inbuilt georeferencing (if any).
		 */
		boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type>
		get_raster_georeferencing() const;

		void
		set_number_of_bands(
				unsigned int number_of_bands);

		// Note: this sorts d_raster_sequence, in place.
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_range_set(
				bool time_dependent_raster);

		GPlatesModel::PropertyValue::non_null_ptr_type
		create_band_names() const;

		GPlatesModel::PropertyValue::non_null_ptr_type
		create_domain_set() const;

		QString
		create_gpml_file_path(
				bool time_dependent_raster) const;

		static const QString GPML_EXT;

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		GPlatesGui::UnsavedChangesTracker *d_unsaved_changes_tracker;
		GPlatesGui::FileIOFeedback *d_file_io_feedback;
		OpenFileDialog d_open_file_dialog;

		// For communication between pages.
		unsigned int d_raster_width;
		unsigned int d_raster_height;
		TimeDependentRasterSequence d_raster_sequence;
		std::vector<QString> d_band_names;
		GPlatesPropertyValues::Georeferencing::non_null_ptr_type d_georeferencing;
		bool d_save_after_finish;
	};
}

#endif  // GPLATES_QTWIDGETS_IMPORTRASTERDIALOG_H
