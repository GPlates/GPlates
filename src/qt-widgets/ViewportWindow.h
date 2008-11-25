/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_VIEWPORTWINDOW_H
#define GPLATES_QTWIDGETS_VIEWPORTWINDOW_H

#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include <vector>
#include <string>
#include <list>
#include <QtCore>
#include <QtCore/QTimer>
#include <QCloseEvent>
#include <QStringList>
#include <QUndoGroup>
#include <QSplitter>

#include "ApplicationState.h"
#include "ViewportWindowUi.h"
#include "GlobeCanvas.h"
#include "SpecifyFixedPlateDialog.h"
#include "SetCameraViewpointDialog.h"
#include "AnimateDialog.h"
#include "AboutDialog.h"
#include "LicenseDialog.h"
#include "FeaturePropertiesDialog.h"
#include "ReadErrorAccumulationDialog.h"
#include "ManageFeatureCollectionsDialog.h"
#include "EulerPoleDialog.h"

#include "ReconstructionViewWidget.h"
#include "TaskPanel.h"

#include "file-io/FeatureCollectionFileFormat.h"

#include "gui/FeatureFocus.h"
#include "gui/FeatureTableModel.h"
#include "gui/ColourTable.h"

#include "model/ModelInterface.h"

#include "maths/GeometryOnSphere.h"


namespace GPlatesGui
{
	class CanvasToolAdapter;
	class CanvasToolChoice;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow:
			public QMainWindow, 
			protected Ui_ViewportWindow
	{
		Q_OBJECT
		
	public:
		ViewportWindow();
		
		GPlatesModel::Reconstruction &
		reconstruction() const
		{
			return *d_reconstruction_ptr;
		}

		const double &
		reconstruction_time() const
		{
			return d_recon_time;
		}

		unsigned long
		reconstruction_root() const
		{
			return d_recon_root;
		}
		
		const GPlatesQtWidgets::ReconstructionViewWidget &
		reconstruction_view_widget() const
		{
			return d_reconstruction_view_widget;
		}

		GlobeCanvas &
		globe_canvas() const
		{
			return *d_canvas_ptr;
		}

		GPlatesGui::FeatureTableModel &
		feature_table_model() 
		{
			return *d_feature_table_model_ptr;
		}


		GPlatesGui::FeatureTableModel &
		segments_feature_table_model() 
		{
			return *d_segments_feature_table_model_ptr;
		}

		void
		create_svg_file();


		void	
		change_tab(int i) {
			tabWidget->setCurrentIndex( i );
		}

		int
		get_tab() {
			return tabWidget->currentIndex();
		}

	public slots:
		
		void
		status_message(
				const QString &message,
				int timeout = 20000) const
		{
			statusBar()->showMessage(message, timeout);
		}

		/**
		 * Highlights the first row in the "Clicked" feature table.
		 */
		void
		highlight_first_clicked_feature_table_row() const;

		/**
		 * Clears the "Segments" feature table selection
		 */
		void
		highlight_segments_table_clear() const;

		/**
		 * Highlights a row in the "Segments" feature table.
		 */
		void
		highlight_segments_table_row(int i, bool state) const;


		void
		reconstruct_to_time(
				double recon_time);

		void
		reconstruct_with_root(
				unsigned long recon_root);

		void
		reconstruct_to_time_with_root(
				double recon_time,
				unsigned long recon_root);

		void
		reconstruct();

		void
		pop_up_license_dialog();

		void
		choose_drag_globe_tool();

		void
		choose_zoom_globe_tool();

		void
		choose_click_geometry_tool();

		void
		choose_digitise_polyline_tool();

		void
		choose_digitise_multipoint_tool();

		void
		choose_digitise_polygon_tool();

		void
		choose_plate_closure_platepolygon_tool();

		void
		choose_move_geometry_tool();

		void
		choose_move_vertex_tool();

		void
		choose_manipulate_pole_tool();

		void
		enable_or_disable_feature_actions(
				GPlatesModel::FeatureHandle::weak_ref focused_feature);

		void		
		choose_colour_by_plate_id();

		void
		choose_colour_by_single_colour();
		
		void	
		choose_colour_by_feature_type();

		void
		choose_colour_by_age();

		void
		pop_up_read_errors_dialog();

		void
		pop_up_manage_feature_collections_dialog();

		void
		pop_up_export_geometry_snapshot_dialog()
		{
			create_svg_file();
		}

		void
		pop_up_euler_pole_dialog();
	
		void
		open_global_raster();

		void
		open_time_dependent_global_raster_set();
		
		// FIXME: Should be a ViewState operation, or /somewhere/ better than this.
		void
		delete_focused_feature();

	signals:
		
		/**
		 * Emitted when the current reconstruction time has changed and after the reconstruction
		 * has been performed and the canvas updated.
		 *
		 * Note that this signal is emitted ONLY when the new reconstruction time is different
		 * to the old one - if the ViewportWindow is asked to reconstruct to the current time
		 * (e.g. after a feature gets a plateid changed), this signal will not be emitted.
		 */
		void
		reconstruction_time_changed(double time);

	public:
		typedef GPlatesAppState::ApplicationState::file_info_iterator file_info_iterator;
		typedef std::list<file_info_iterator> active_files_collection_type;
		typedef active_files_collection_type::iterator active_files_iterator;
		
		/**
		 * Returns the current colour table in use.
		 */
		GPlatesGui::ColourTable *
		get_colour_table();

		void
		load_files(
				const QStringList &file_names);

		/**
		 * Given a file_info_iterator, reloads the data for that file from disk,
		 * replacing the FeatureCollection associated with that file_info_iterator.
		 */
		void
		reload_file(
				file_info_iterator file_it);

		/**
		 * Creates a fresh, empty, FeatureCollection. Associates a 'dummy'
		 * FileInfo for it, and registers it with ApplicationState so that
		 * the FeatureCollection can be reconstructed and saved via
		 * the ManageFeatureCollectionsDialog.
		 */
		GPlatesAppState::ApplicationState::file_info_iterator
		create_empty_reconstructable_file();


		/**
		 * Write the feature collection associated to @a file_info to the file
		 * from which the features were read.
		 */
		void
		save_file(
				const GPlatesFileIO::FileInfo &file_info,
				GPlatesFileIO::FeatureCollectionWriteFormat::Format =
					GPlatesFileIO::FeatureCollectionWriteFormat::USE_FILE_EXTENSION);


		/**
		 * Write the feature collection associated to @a features_to_save to the file
		 * specified by @file_info.
		 *
		 * The list of loaded files is updated so as to refer to the recently written file.
		 */
		void
		save_file_as(
				const GPlatesFileIO::FileInfo &file_info,
				file_info_iterator features_to_save,
				GPlatesFileIO::FeatureCollectionWriteFormat::Format =
					GPlatesFileIO::FeatureCollectionWriteFormat::USE_FILE_EXTENSION);


		/**
		 * Write the feature collection associated to @a features_to_save to the file
		 * specified by @file_info.  Returns a FileInfo object corresponding to the file
		 * that was written (this will usually be ignored).
		 *
		 * The list of loaded files is @b not updated so as to refer to the recently written file.
		 */
		GPlatesFileIO::FileInfo
		save_file_copy(
				const GPlatesFileIO::FileInfo &file_info,
				file_info_iterator features_to_save,
				GPlatesFileIO::FeatureCollectionWriteFormat::Format =
					GPlatesFileIO::FeatureCollectionWriteFormat::USE_FILE_EXTENSION);


		/**
		 * Ensure the @a loaded_file is deactivated.
		 *
		 * It is assumed that @a loaded_file is an iterator which references the FileInfo
		 * instance of a loaded file.  The file is going to be unloaded, so it will be
		 * removed from the list of files in the application state.  Let's also ensure that
		 * it is not an "active" file in this class (ie, an element of the collections of
		 * active reconstructable or reconstruction files).
		 *
		 * This function should be invoked when a feature collection is unloaded by the
		 * user.
		 */
		void
		deactivate_loaded_file(
				file_info_iterator loaded_file);

		/**
		 * Tests if the @a loaded_file is active, either as reconstructable features
		 * or an active reconstruction tree.
		 */
		bool
		is_file_active(
				file_info_iterator loaded_file);

		/**
		 * Tests if the @a loaded_file is active and being used for reconstructable
		 * feature data.
		 */
		bool
		is_file_active_reconstructable(
				file_info_iterator loaded_file);

		/**
		 * Tests if the @a loaded_file is active and being used for reconstruction
		 * tree data.
		 */
		bool
		is_file_active_reconstruction(
				file_info_iterator loaded_file);
		
		/**
		 * Temporarily shows or hides a feature collection by adding or removing
		 * it from the list of active reconstructable files. This does not
		 * un-load the file.
		 */
		void
		set_file_active_reconstructable(
				file_info_iterator file_it,
				bool activate);

		/**
		 * Temporarily enables or disables a reconstruction tree by adding or
		 * removing it from the list of active reconstruction files. This does not
		 * un-load the file.
		 */
		void
		set_file_active_reconstruction(
				file_info_iterator file_it,
				bool activate);
			
		/**
		 * Temporary method for initiating shapefile attribute remapping. 
		 */
		void
		remap_shapefile_attributes(
			GPlatesFileIO::FileInfo &file_info);

	private:
	
		/**
		 * Connects all the Signal/Slot relationships for ViewportWindow toolbar
		 * buttons and menu items.
		 */
		void
		connect_menu_actions();

		/**
		 * Configures the ActionButtonBox inside the Feature tab of the Task Panel
		 * with some of the QActions that ViewportWindow has on the menu bar.
		 */
		void
		set_up_task_panel_actions();

		/**
		 * Creates a context menu to allow the user to dock DockWidgets even if
		 * they are misbehaving and not docking when dragged over the main window.
		 */
		void
		set_up_dock_context_menus();


		GPlatesModel::ModelInterface *d_model_ptr;
		GPlatesModel::Reconstruction::non_null_ptr_type d_reconstruction_ptr;

		//@{
		// ViewState 

		active_files_collection_type d_active_reconstructable_files;
		active_files_collection_type d_active_reconstruction_files;
		
		QUndoGroup d_undo_group;

		//@}

		double d_recon_time;
		GPlatesModel::integer_plate_id_type d_recon_root;

		QSplitter *d_central_splitter;

		ReconstructionViewWidget d_reconstruction_view_widget;

		GPlatesGui::FeatureFocus d_feature_focus;	// Might be in ViewState.
		SpecifyFixedPlateDialog d_specify_fixed_plate_dialog;
		SetCameraViewpointDialog d_set_camera_viewpoint_dialog;
		AnimateDialog d_animate_dialog;
		AboutDialog d_about_dialog;
		LicenseDialog d_license_dialog;
		FeaturePropertiesDialog d_feature_properties_dialog;	// Depends on FeatureFocus.
		ReadErrorAccumulationDialog d_read_errors_dialog;
		ManageFeatureCollectionsDialog d_manage_feature_collections_dialog;
		bool d_animate_dialog_has_been_shown;
		GlobeCanvas *d_canvas_ptr;
		GPlatesGui::CanvasToolAdapter *d_canvas_tool_adapter_ptr;
		// Depends on FeatureFocus, because QueryFeature does. Also depends on DigitisationWidget.
		GPlatesGui::CanvasToolChoice *d_canvas_tool_choice_ptr;		
		EulerPoleDialog d_euler_pole_dialog;

		// Depends on FeatureFocus and the Model d_model_ptr.  Held in ReconstructionViewWidget
		TaskPanel *d_task_panel_ptr;	

		// The 'Clicked' table. Should be in ViewState. Depends on FeatureFocus.
		GPlatesGui::FeatureTableModel *d_feature_table_model_ptr;	

		// The 'Segments' table. Should be in ViewState. Depends on FeatureFocus.
		GPlatesGui::FeatureTableModel *d_segments_feature_table_model_ptr;	

		//  map a time value to a raster filename
		QMap<int,QString> d_time_dependent_raster_map;

		// The last path used for opening raster files.
		QString d_open_file_path; 

		// The current colour table in use. Do not access this directly, use
		// get_colour_table() instead.
		GPlatesGui::ColourTable *d_colour_table_ptr;

		void
		uncheck_all_tools();

		void 
		uncheck_all_colouring_tools();

		bool
		load_global_raster(
			QString filename);

		void
		update_time_dependent_raster();


	private slots:
		void
		pop_up_specify_fixed_plate_dialog();

		void
		pop_up_set_camera_viewpoint_dialog();
		
		void
		pop_up_animate_dialog();

		void
		pop_up_about_dialog();

		void
		close_all_dialogs();
		
		void
		dock_search_results_at_top();
		
		void
		dock_search_results_at_bottom();

		void
		enable_raster_display();

		void
		enable_point_display();

		void
		enable_line_display();

		void
		enable_polygon_display();

		void
		enable_topology_display();

		void
		enable_multipoint_display();

	protected:
	
		/**
		 * A reimplementation of QWidget::closeEvent() to allow closure to be postponed.
		 * To request program termination in the same manner as using the window manager's
		 * 'close' button, you should call ViewportWindow::close().
		 */
		void
		closeEvent(QCloseEvent *close_event);
	};
}

#endif  // GPLATES_QTWIDGETS_VIEWPORTWINDOW_H
