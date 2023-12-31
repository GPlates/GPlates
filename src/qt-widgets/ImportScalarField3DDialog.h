/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_QT_WIDGETS_IMPORTSCALARFIELD3DDIALOG_H
#define GPLATES_QT_WIDGETS_IMPORTSCALARFIELD3DDIALOG_H

#include <vector>
#include <boost/optional.hpp>
#include <QFileInfo>
#include <QString>
#include <QWizard>

#include "OpenFileDialog.h"

#include "global/PointerTraits.h"

#include "model/PropertyValue.h"

#include "property-values/CoordinateTransformation.h"
#include "property-values/Georeferencing.h"


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

namespace GPlatesOpenGL
{
	class GLRenderer;
	class GLViewport;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class ScalarField3DDepthLayersSequence
	{
	public:

		//! Radius of Earth in Kms.
		static const double DEFAULT_RADIUS_OF_EARTH;

		struct FileInfo
		{
			FileInfo(
					const boost::optional<double> &depth_,
					const QString &absolute_file_path_,
					const QString &file_name_,
					unsigned width_,
					unsigned height_,
					bool remove_cache_files_) :
				depth(depth_),
				absolute_file_path(absolute_file_path_),
				file_name(file_name_),
				width(width_),
				height(height_),
				remove_cache_files(remove_cache_files_)
			{  }

			/**
			 * Remove cache files (if they didn't already exist prior to import).
			 *
			 * Once we've generated the 3D scalar field file we don't need the depth layer rasters anymore.
			 * So cleaning up the cache files saves disk space - but it's only done if we created the
			 * cache files during import (otherwise user might be using them to view the depth rasters in GPlates).
			 */
			void
			clear_cache_files();

			//! Depth in Kms.
			boost::optional<double> depth;
			QString absolute_file_path;
			QString file_name;
			unsigned int width;
			unsigned int height;
			bool remove_cache_files;
		};

		typedef FileInfo element_type;
		typedef std::vector<element_type> sequence_type;

		const sequence_type &
		get_sequence() const
		{
			return d_sequence;
		}

		sequence_type &
		get_sequence()
		{
			return d_sequence;
		}

		bool
		empty() const;

		void
		push_back(
				boost::optional<double> depth,
				const QString &absolute_file_path,
				const QString &file_name,
				unsigned int width,
				unsigned int height,
				bool remove_cache_files);

		void
		add_all(
				const ScalarField3DDepthLayersSequence &other);

		void
		clear();

		/**
		 * Remove cache files of the depth layer rasters (if they didn't already exist prior to import).
		 *
		 * Once we've generated the 3D scalar field file we don't need the depth layer rasters anymore.
		 * So cleaning up the cache files saves disk space - but it's only done if we created the
		 * cache files during import (otherwise user might be using them to view the depth rasters in GPlates).
		 */
		void
		clear_cache_files();

		void
		erase(
				unsigned int begin_index,
				unsigned int end_index);

		void
		set_depth(
				unsigned int index,
				const boost::optional<double> &depth);

		void
		sort_by_depth();

		void
		sort_by_file_name();

	private:

		sequence_type d_sequence;
	};


	class ImportScalarField3DDialog :
			public QWizard
	{
		Q_OBJECT

	public:

		explicit
		ImportScalarField3DDialog(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow &viewport_window,
				GPlatesGui::UnsavedChangesTracker *unsaved_changes_tracker,
				GPlatesGui::FileIOFeedback *file_io_feedback,
				QWidget *parent_ = NULL);

		/**
		 * Call this to open the import scalar field wizard, instead of @a show().
		 */
		void
		display(
				GPlatesFileIO::ReadErrorAccumulation *read_errors = NULL);

	private:

		/**
		 * Wizard page ids.
		 */
		enum PageId
		{
			DEPTH_LAYERS_PAGE_ID,
			GEOREFERENCING_PAGE_ID,
			SCALAR_FIELD_COLLECTION_PAGE_ID
		};


		/**
		 * Override the next page id so we can skip georeferencing page if raster has inbuilt georeferencing.
		 */
		virtual
		int
		nextId() const;

		GPlatesGlobal::PointerTraits<GPlatesOpenGL::GLRenderer>::non_null_ptr_type
		create_gl_renderer() const;

		bool
		is_scalar_field_import_supported() const;

		/**
		 * Import georeferencing and spatial reference system (if any) from first depth layer raster.
		 *
		 * Returns true if imported (if found in first depth layer raster).
		 */
		bool
		import_georeferencing_and_spatial_reference_system() const;

		void
		import_scalar_field_3d(
				GPlatesFileIO::ReadErrorAccumulation *read_errors);

		bool
		generate_scalar_field(
				const QString &gpsf_file_path,
				GPlatesFileIO::ReadErrorAccumulation *read_errors);

		GPlatesModel::PropertyValue::non_null_ptr_type
		create_scalar_field_3d_file_property_value(
				const QString &gpsf_file_path);

		QString
		create_gpml_file_path() const;

		QString
		create_gpsf_file_path() const;

		QString
		create_file_basename_with_path() const;

		static const QString GPML_EXT;
		static const QString GPSF_EXT;

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow &d_viewport_window;
		GPlatesGui::UnsavedChangesTracker *d_unsaved_changes_tracker;
		GPlatesGui::FileIOFeedback *d_file_io_feedback;
		OpenFileDialog d_open_file_dialog;

		// For communication between pages.
		unsigned int d_raster_width;
		unsigned int d_raster_height;
		ScalarField3DDepthLayersSequence d_depth_layers_sequence;
		mutable GPlatesPropertyValues::Georeferencing::non_null_ptr_type d_georeferencing;
		mutable GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_type d_coordinate_transformation;
		bool d_save_after_finish;
	};
}

#endif // GPLATES_QT_WIDGETS_IMPORTSCALARFIELD3DDIALOG_H
