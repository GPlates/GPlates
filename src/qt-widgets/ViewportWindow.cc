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
 
#include <iostream>
#include <iterator>
#include <boost/format.hpp>

#include <QFileDialog>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QHeaderView>
#include <QMessageBox>
#include <QProgressBar>

#include "ViewportWindow.h"
#include "InformationDialog.h"
#include "ShapefilePropertyMapper.h"
#include "TaskPanel.h"
#include "ActionButtonBox.h"
#include "CreateFeatureDialog.h"

#include "global/Exception.h"
#include "global/UnexpectedEmptyFeatureCollectionException.h"
#include "gui/CanvasToolAdapter.h"
#include "gui/CanvasToolChoice.h"
#include "gui/FeatureWeakRefSequence.h"
#include "maths/PointOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "maths/InvalidLatLonException.h"
#include "maths/Real.h"
#include "model/Model.h"
#include "model/types.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "model/DummyTransactionHandle.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/ErrorOpeningPipeFromGzipException.h"
#include "file-io/PlatesLineFormatReader.h"
#include "file-io/PlatesRotationFormatReader.h"
#include "file-io/FileInfo.h"
#include "file-io/Reader.h"
#include "file-io/RasterReader.h"
#include "file-io/ShapeFileReader.h"
#include "file-io/GpmlOnePointSixReader.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "gui/SvgExport.h"
#include "gui/PlatesColourTable.h"
#include "gui/GlobeCanvasPainter.h"

namespace
{
	bool
	file_name_ends_with(
			const GPlatesFileIO::FileInfo &file, 
			const QString &suffix)
	{
		return file.get_qfileinfo().completeSuffix().endsWith(QString(suffix), Qt::CaseInsensitive);
	}


	bool
	is_plates_line_format_file(
			const GPlatesFileIO::FileInfo &file)
	{
		return file_name_ends_with(file, "dat") || file_name_ends_with(file, "pla");
	}

	
	bool
	is_plates_rotation_format_file(
			const GPlatesFileIO::FileInfo &file)
	{
		return file_name_ends_with(file, "rot");
	}
	
	bool
	is_shapefile_format_file(
			const GPlatesFileIO::FileInfo &file)
	{
		return file_name_ends_with(file, "shp");
	}

	bool
	is_gpml_format_file(
			const GPlatesFileIO::FileInfo &file)
	{
		return file_name_ends_with(file, "gpml");
	}

	bool
	is_gpml_gz_format_file(
			const GPlatesFileIO::FileInfo &file)
	{
		return file_name_ends_with(file, "gpml.gz");
	}
}


void
GPlatesQtWidgets::ViewportWindow::save_file(
		const GPlatesFileIO::FileInfo &file_info)
{
	if ( ! file_info.is_writable()) {
		throw GPlatesFileIO::ErrorOpeningFileForWritingException(
				file_info.get_qfileinfo().filePath());
	}

	if ( ! file_info.get_feature_collection()) {
		throw GPlatesGlobal::UnexpectedEmptyFeatureCollectionException(
				"Attempted to write an empty feature collection.");
	}

	GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection =
		*file_info.get_feature_collection();
	boost::shared_ptr< GPlatesModel::ConstFeatureVisitor > writer = file_info.get_writer();
	GPlatesModel::FeatureCollectionHandle::features_const_iterator
		iter = feature_collection->features_begin(), 
		end = feature_collection->features_end();
	for ( ; iter != end; ++iter) {
		(*iter)->accept_visitor(*writer);
	}
}


void
GPlatesQtWidgets::ViewportWindow::save_file_as(
		const GPlatesFileIO::FileInfo &file_info,
		file_info_iterator features_to_save)
{
	GPlatesFileIO::FileInfo file_copy = save_file_copy(file_info, features_to_save);

	// Update iterator
	*features_to_save = file_copy;
}


GPlatesFileIO::FileInfo
GPlatesQtWidgets::ViewportWindow::save_file_copy(
		const GPlatesFileIO::FileInfo &file_info,
		file_info_iterator features_to_save)
{
	GPlatesFileIO::FileInfo file_copy(file_info.get_qfileinfo().filePath());
	if ( ! features_to_save->get_feature_collection()) {
		throw GPlatesGlobal::UnexpectedEmptyFeatureCollectionException(
				"Attempted to write an empty feature collection.");
	}
	file_copy.set_feature_collection(*(features_to_save->get_feature_collection()));
	save_file(file_copy);
	return file_copy;
}

void
GPlatesQtWidgets::ViewportWindow::load_files(
		const QStringList &file_names)
{
	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();	

	QStringList::const_iterator iter = file_names.begin();
	QStringList::const_iterator end = file_names.end();

	bool have_loaded_new_rotation_file = false;

	for ( ; iter != end; ++iter) {

		GPlatesFileIO::FileInfo file(*iter);

		try
		{
			if (is_plates_line_format_file(file))
			{
				GPlatesFileIO::PlatesLineFormatReader::read_file(file, *d_model_ptr, read_errors);

				if (file.get_feature_collection())
				{
					// All loaded files are added to the set of loaded files.
					GPlatesAppState::ApplicationState::file_info_iterator new_file =
						GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);

					// Line format files are made active by default.
					d_active_reconstructable_files.push_back(new_file);
				}
			}
			else if (is_plates_rotation_format_file(file))
			{
				GPlatesFileIO::PlatesRotationFormatReader::read_file(file, *d_model_ptr, read_errors);
	

				if (file.get_feature_collection())
				{
					// All loaded files are added to the set of loaded files.
					GPlatesAppState::ApplicationState::file_info_iterator new_file =
						GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);				
					
					// We only want to make the first rotation file active.
					if ( ! have_loaded_new_rotation_file) 
					{
						d_active_reconstruction_files.clear();
						d_active_reconstruction_files.push_back(new_file);
						have_loaded_new_rotation_file = true;
					}
				}
			} 
			else if (is_shapefile_format_file(file))
			{
				GPlatesFileIO::ShapeFileReader::set_property_mapper(
					boost::shared_ptr< ShapefilePropertyMapper >(new ShapefilePropertyMapper));
				GPlatesFileIO::ShapeFileReader::read_file(file,*d_model_ptr,read_errors);

				if (file.get_feature_collection())
				{
					GPlatesAppState::ApplicationState::file_info_iterator new_file =
						GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);
					d_active_reconstructable_files.push_back(new_file);
				}
			}
			else if (is_gpml_format_file(file) || is_gpml_gz_format_file(file))
			{
				GPlatesFileIO::GpmlOnePointSixReader::read_file(file, *d_model_ptr, read_errors);
				
				// All loaded files are added to the set of loaded files.
				GPlatesAppState::ApplicationState::file_info_iterator new_file =
					GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);

				// GPML format files are made active by default.
				// FIXME: However, GPML files are magical and can contain rotation trees too.
				// What do we do about those? Obviously this file must be made 'active' because
				// it contains new feature data - however we may want some clever code in here
				// to check if it also contains rotation data, and if so, deactivate any prior
				// rotation files.
				// FIXME: And THEN what happens when we load another rotation file on top? Can't
				// deactivate half of this GPML file, can we?
				d_active_reconstructable_files.push_back(new_file);

				// So for now, let's pretend we've got some rotation data in here.
				// We only want to make the first rotation file active.
				d_active_reconstruction_files.clear();
				d_active_reconstruction_files.push_back(new_file);
				have_loaded_new_rotation_file = true;
			}
			else
			{
				// FIXME: This should be added to the read errors!
				std::cerr << "Unrecognised file type for file: " 
					<< file.get_qfileinfo().absoluteFilePath().toStdString() << std::endl;
			}
		}
		catch (GPlatesFileIO::ErrorOpeningFileForReadingException &e)
		{
			// FIXME: A bit of a sucky conversion from ErrorOpeningFileForReadingException to
			// ReadErrorOccurrence, but hey, this whole function will be rewritten when we add
			// QFileDialog support.
			// FIXME: I suspect I'm Missing The Point with these shared_ptrs.
			boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
					new GPlatesFileIO::LocalFileDataSource(e.filename(), GPlatesFileIO::DataFormats::Unspecified));
			boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
					new GPlatesFileIO::LineNumberInFile(0));
			read_errors.d_failures_to_begin.push_back(GPlatesFileIO::ReadErrorOccurrence(
					e_source,
					e_location,
					GPlatesFileIO::ReadErrors::ErrorOpeningFileForReading,
					GPlatesFileIO::ReadErrors::FileNotLoaded));
		}
		catch (GPlatesFileIO::ErrorOpeningPipeFromGzipException &e)
		{
			QString message = tr("GPlates was unable to use the '%1' program to read the file '%2'."
					" Please check that gzip is installed and in your PATH. You will still be able to open"
					" files which are not compressed.")
					.arg(e.command())
					.arg(e.filename());
			QMessageBox::critical(this, tr("Error Opening File"), message,
					QMessageBox::Ok, QMessageBox::Ok);
		}
		catch (GPlatesGlobal::Exception &e)
		{
			std::cerr << "Caught exception: " << e << std::endl;
		}


	}

	// Internal state changed, make sure dialogs are up to date.
	d_read_errors_dialog.update();
	d_manage_feature_collections_dialog.update();

	// Pop up errors only if appropriate.
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors) {
		d_read_errors_dialog.show();
	}
}


namespace
{
	void
	get_features_collection_from_file_info_collection(
			GPlatesQtWidgets::ViewportWindow::active_files_collection_type &active_files,
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &features_collection) {

		GPlatesQtWidgets::ViewportWindow::active_files_collection_type::iterator
			iter = active_files.begin(),
			end = active_files.end();
		for ( ; iter != end; ++iter)
		{
			if ((*iter)->get_feature_collection())
			{
				features_collection.push_back(*((*iter)->get_feature_collection()));
			}
		}
	}

	GPlatesModel::Reconstruction::non_null_ptr_type 
	create_reconstruction(
			GPlatesQtWidgets::ViewportWindow::active_files_collection_type &active_reconstructable_files,
			GPlatesQtWidgets::ViewportWindow::active_files_collection_type &active_reconstruction_files,
			GPlatesModel::ModelInterface *model_ptr,
			double recon_time,
			GPlatesModel::integer_plate_id_type recon_root) {

		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
			reconstructable_features_collection,
			reconstruction_features_collection;

		get_features_collection_from_file_info_collection(
				active_reconstructable_files,
				reconstructable_features_collection);

		get_features_collection_from_file_info_collection(
				active_reconstruction_files,
				reconstruction_features_collection);

		return model_ptr->create_reconstruction(reconstructable_features_collection,
				reconstruction_features_collection, recon_time, recon_root);
	}


	void
	render_model(
			GPlatesQtWidgets::GlobeCanvas *canvas_ptr, 
			GPlatesModel::ModelInterface *model_ptr, 
			GPlatesModel::Reconstruction::non_null_ptr_type &reconstruction,
			GPlatesQtWidgets::ViewportWindow::active_files_collection_type &active_reconstructable_files,
			GPlatesQtWidgets::ViewportWindow::active_files_collection_type &active_reconstruction_files,
			double recon_time,
			GPlatesModel::integer_plate_id_type recon_root)
	{
		try {
			reconstruction = create_reconstruction(active_reconstructable_files, 
					active_reconstruction_files, model_ptr, recon_time, recon_root);

			GPlatesModel::Reconstruction::geometry_collection_type::iterator iter =
					reconstruction->geometries().begin();
			GPlatesModel::Reconstruction::geometry_collection_type::iterator end =
					reconstruction->geometries().end();
			for ( ; iter != end; ++iter) {
				GPlatesGui::PlatesColourTable::const_iterator colour = GPlatesGui::PlatesColourTable::Instance()->end();

				// We use a dynamic cast here (despite the fact that dynamic casts are generally
				// considered bad form) because we only care about one specific derivation.
				// There's no "if ... else if ..." chain, so I think it's not super-bad form.  (The
				// "if ... else if ..." chain would imply that we should be using polymorphism --
				// specifically, the double-dispatch of the Visitor pattern -- rather than updating
				// the "if ... else if ..." chain each time a new derivation is added.)
				GPlatesModel::ReconstructedFeatureGeometry *rfg =
						dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(iter->get());
				if (rfg) {
					// It's an RFG, so let's look at the feature it's referencing.
					if (rfg->reconstruction_plate_id()) {
						colour = GPlatesGui::PlatesColourTable::Instance()->lookup(*(rfg->reconstruction_plate_id()));
					}
				}

				if (colour == GPlatesGui::PlatesColourTable::Instance()->end()) {
					// Anything not in the table in gui/PlatesColourTable.cc uses the 'Olive' colour.
					colour = &GPlatesGui::Colour::OLIVE;
				}

				GPlatesGui::GlobeCanvasPainter painter(*canvas_ptr, colour);
				(*iter)->geometry()->accept_visitor(painter);
			}

			//render(reconstruction->point_geometries().begin(), reconstruction->point_geometries().end(), &GPlatesQtWidgets::GlobeCanvas::draw_point, canvas_ptr);
			//for_each(reconstruction->point_geometries().begin(), reconstruction->point_geometries().end(), render(canvas_ptr, &GlobeCanvas::draw_point, point_colour))
			// for_each(reconstruction->polyline_geometries().begin(), reconstruction->polyline_geometries().end(), polyline_point);
		} catch (GPlatesGlobal::Exception &e) {
			std::cerr << e << std::endl;
		}
	}


	void
	render_mouse_interaction_geometry_layer(
			GPlatesQtWidgets::GlobeCanvas *canvas_ptr, 
			const GPlatesQtWidgets::ViewportWindow::mouse_interaction_geometry_layer_type &
					geometry_layer)
	{
		typedef GPlatesQtWidgets::ViewportWindow::mouse_interaction_geometry_layer_type
				geometry_layer_type;

		GPlatesGui::PlatesColourTable::const_iterator colour = &GPlatesGui::Colour::WHITE;
		GPlatesGui::GlobeCanvasPainter painter(*canvas_ptr, colour);

		geometry_layer_type::const_iterator iter = geometry_layer.begin();
		geometry_layer_type::const_iterator end = geometry_layer.end();
		for ( ; iter != end; ++iter) {
			(*iter)->accept_visitor(painter);
		}
	}

} // namespace


GPlatesQtWidgets::ViewportWindow::ViewportWindow() :
	d_model_ptr(new GPlatesModel::Model()),
	d_reconstruction_ptr(d_model_ptr->create_empty_reconstruction(0.0, 0)),
	d_recon_time(0.0),
	d_recon_root(0),
	d_reconstruction_view_widget(*this, this),
	d_feature_focus(),
	d_specify_fixed_plate_dialog(d_recon_root, this),
	d_set_camera_viewpoint_dialog(*this, this),
	d_animate_dialog(*this, this),
	d_about_dialog(*this, this),
	d_license_dialog(&d_about_dialog),
	d_feature_properties_dialog(*this, d_feature_focus, this),
	d_read_errors_dialog(this),
	d_manage_feature_collections_dialog(*this, this),
	d_animate_dialog_has_been_shown(false),
	d_euler_pole_dialog(*this, this),
	d_task_panel_ptr(new TaskPanel(d_feature_focus, *d_model_ptr, *this, this)),
	d_feature_table_model_ptr(new GPlatesGui::FeatureTableModel(d_feature_focus)),
	d_open_file_path("")
{
	setupUi(this);

	d_canvas_ptr = &(d_reconstruction_view_widget.globe_canvas());

	// Connect all the Signal/Slot relationships of ViewportWindow's
	// toolbar buttons and menu items.
	connect_menu_actions();
	
	// Set up an emergency context menu to control QDockWidgets even if
	// they're no longer behaving properly.
	set_up_dock_context_menus();
	
	// FIXME: Set up the Task Panel in a more detailed fashion here.
	d_reconstruction_view_widget.insert_task_panel(d_task_panel_ptr);
	set_up_task_panel_actions();
	
	// Disable the feature-specific Actions as there is no currently focused feature to act on.
	enable_or_disable_feature_actions(d_feature_focus.focused_feature());
	QObject::connect(&d_feature_focus, SIGNAL(focus_changed(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			this, SLOT(enable_or_disable_feature_actions(GPlatesModel::FeatureHandle::weak_ref)));

	// Set up the Specify Fixed Plate dialog.
	QObject::connect(&d_specify_fixed_plate_dialog, SIGNAL(value_changed(unsigned long)),
			this, SLOT(reconstruct_with_root(unsigned long)));

	// Set up the Animate dialog.
	QObject::connect(&d_animate_dialog, SIGNAL(current_time_changed(double)),
			&d_reconstruction_view_widget, SLOT(set_reconstruction_time(double)));

	// Set up the Reconstruction View widget.
	setCentralWidget(&d_reconstruction_view_widget);

	QObject::connect(&d_reconstruction_view_widget, SIGNAL(reconstruction_time_changed(double)),
			this, SLOT(reconstruct_to_time(double)));
	QObject::connect(d_canvas_ptr, SIGNAL(mouse_pointer_position_changed(const GPlatesMaths::PointOnSphere &, bool)),
			&d_reconstruction_view_widget, SLOT(update_mouse_pointer_position(const GPlatesMaths::PointOnSphere &, bool)));

	// Render everything on the screen in present-day positions.
	d_canvas_ptr->clear_data();
	render_model(d_canvas_ptr, d_model_ptr, d_reconstruction_ptr, d_active_reconstructable_files, 
			d_active_reconstruction_files, 0.0, d_recon_root);
	d_canvas_ptr->update_canvas();

	// Set up the Clicked table.
	// FIXME: feature table model for this Qt widget and the Query Tool should be stored in ViewState.
	table_view_clicked_geometries->setModel(d_feature_table_model_ptr);
	table_view_clicked_geometries->verticalHeader()->hide();
	table_view_clicked_geometries->resizeColumnsToContents();
	GPlatesGui::FeatureTableModel::set_default_resize_modes(*table_view_clicked_geometries->horizontalHeader());
	table_view_clicked_geometries->horizontalHeader()->setMinimumSectionSize(60);
	table_view_clicked_geometries->horizontalHeader()->setMovable(true);
	table_view_clicked_geometries->horizontalHeader()->setHighlightSections(false);
	// When the user selects a row of the table, we should focus that feature.
	QObject::connect(table_view_clicked_geometries->selectionModel(),
			SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			d_feature_table_model_ptr,
			SLOT(handle_selection_change(const QItemSelection &, const QItemSelection &)));

	// If the focus is modified, we may need to reconstruct to update the view.
	QObject::connect(&d_feature_focus,
			SIGNAL(focused_feature_modified(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			this, SLOT(reconstruct()));

	// Set up the Canvas Tools.
	// FIXME:  This is, of course, very exception-unsafe.  This whole class needs to be nuked.
	d_canvas_tool_choice_ptr =
			new GPlatesGui::CanvasToolChoice(
					d_canvas_ptr->globe(),
					*d_canvas_ptr,
					*this,
					*d_feature_table_model_ptr,
					d_feature_properties_dialog,
					d_feature_focus,
					d_task_panel_ptr->digitisation_widget());

	// Set up the Canvas Tool Adapter for handling globe click and drag events.
	// FIXME:  This is, of course, very exception-unsafe.  This whole class needs to be nuked.
	d_canvas_tool_adapter_ptr = new GPlatesGui::CanvasToolAdapter(*d_canvas_tool_choice_ptr);

	QObject::connect(d_canvas_ptr, SIGNAL(mouse_clicked(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool, Qt::MouseButton,
					Qt::KeyboardModifiers)),
			d_canvas_tool_adapter_ptr, SLOT(handle_click(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool, Qt::MouseButton,
					Qt::KeyboardModifiers)));

	QObject::connect(d_canvas_ptr, SIGNAL(mouse_dragged(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &, bool,
					Qt::MouseButton, Qt::KeyboardModifiers)),
			d_canvas_tool_adapter_ptr, SLOT(handle_drag(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &, bool,
					Qt::MouseButton, Qt::KeyboardModifiers)));

	QObject::connect(d_canvas_ptr, SIGNAL(mouse_released_after_drag(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &, bool,
					Qt::MouseButton, Qt::KeyboardModifiers)),
			d_canvas_tool_adapter_ptr, SLOT(handle_release_after_drag(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &, bool,
					Qt::MouseButton, Qt::KeyboardModifiers)));
	
	// If the user creates a new feature with the DigitisationWidget, we need to reconstruct to
	// make sure everything is displayed properly.
	QObject::connect(&(d_task_panel_ptr->digitisation_widget().create_feature_dialog()),
			SIGNAL(feature_created(GPlatesModel::FeatureHandle::weak_ref)),
			this,
			SLOT(reconstruct()));

	// Add the DigitisationWidget's QUndoStack to the View State's QUndoGroup.
	d_undo_group.addStack(&(d_task_panel_ptr->digitisation_widget().undo_stack()));
	// Since we only have one stack right now, we may as well make it the active one.
	d_task_panel_ptr->digitisation_widget().undo_stack().setActive(true);
	
	// Add a progress bar to the status bar (Hidden until needed).
	QProgressBar *progress_bar = new QProgressBar(this);
	progress_bar->setMaximumWidth(100);
	progress_bar->hide();
	statusBar()->addPermanentWidget(progress_bar);
}


void	
GPlatesQtWidgets::ViewportWindow::connect_menu_actions()
{
	GlobeCanvas *canvas_ptr = &(d_reconstruction_view_widget.globe_canvas());
	// If you want to add a new menu action, the steps are:
	// 0. Open ViewportWindowUi.ui in the Designer.
	// 1. Create a QAction in the Designer's Action Editor, called action_Something.
	// 2. Assign icons, tooltips, and shortcuts as necessary.
	// 3. Drag this action to a menu.
	// 4. Add code for the triggered() signal your action generates here.
	//    Please keep this function sorted in the same order as menu items appear.

	// Main Tools:
	QObject::connect(action_Drag_Globe, SIGNAL(triggered()),
			this, SLOT(choose_drag_globe_tool()));
	QObject::connect(action_Zoom_Globe, SIGNAL(triggered()),
			this, SLOT(choose_zoom_globe_tool()));
	QObject::connect(action_Click_Geometry, SIGNAL(triggered()),
			this, SLOT(choose_click_geometry_tool()));
	QObject::connect(action_Digitise_New_Polyline, SIGNAL(triggered()),
			this, SLOT(choose_digitise_polyline_tool()));
	QObject::connect(action_Digitise_New_MultiPoint, SIGNAL(triggered()),
			this, SLOT(choose_digitise_multipoint_tool()));
	QObject::connect(action_Digitise_New_Polygon, SIGNAL(triggered()),
			this, SLOT(choose_digitise_polygon_tool()));

	// File Menu:
	QObject::connect(action_Open_Feature_Collection, SIGNAL(triggered()),
			&d_manage_feature_collections_dialog, SLOT(open_file()));
	QObject::connect(action_Open_Global_Raster, SIGNAL(triggered()),
			this, SLOT(open_global_raster()));
	QObject::connect(action_Open_Time_Dependent_Global_Raster_Set, SIGNAL(triggered()),
			this, SLOT(open_time_dependent_global_raster_set()));
	QObject::connect(action_File_Errors, SIGNAL(triggered()),
			this, SLOT(pop_up_read_errors_dialog()));
	QObject::connect(action_Manage_Feature_Collections, SIGNAL(triggered()),
			this, SLOT(pop_up_manage_feature_collections_dialog()));
	// ----
	QObject::connect(action_Quit, SIGNAL(triggered()),
			this, SLOT(close()));
	
	// Edit Menu:
	QObject::connect(action_Query_Feature, SIGNAL(triggered()),
			&d_feature_properties_dialog, SLOT(choose_query_widget_and_open()));
	QObject::connect(action_Edit_Feature, SIGNAL(triggered()),
			&d_feature_properties_dialog, SLOT(choose_edit_widget_and_open()));
	// ----
	// Unfortunately, the Undo and Redo actions cannot be added in the Designer,
	// or at least, not nicely. We need to ask the QUndoGroup to create some
	// QActions for us, and add them programmatically. To follow the principle
	// of least surprise, placeholder actions are set up in the designer, which
	// this code can use to insert the actions in the correct place with the
	// correct shortcut.
	// The new actions will be linked to the QUndoGroup appropriately.
	QAction *undo_action_ptr = d_undo_group.createUndoAction(this, tr("&Undo"));
	QAction *redo_action_ptr = d_undo_group.createRedoAction(this, tr("&Redo"));
	undo_action_ptr->setShortcut(action_Undo_Placeholder->shortcut());
	redo_action_ptr->setShortcut(action_Redo_Placeholder->shortcut());
	menu_Edit->insertAction(action_Undo_Placeholder, undo_action_ptr);
	menu_Edit->insertAction(action_Redo_Placeholder, redo_action_ptr);
	menu_Edit->removeAction(action_Undo_Placeholder);
	menu_Edit->removeAction(action_Redo_Placeholder);
	// ----
	// FIXME: The following Edit Menu items are all unimplemented:
	action_Delete_Feature->setDisabled(true);

	// Reconstruction Menu:
	QObject::connect(action_Reconstruct_to_Time, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(activate_time_spinbox()));
	QObject::connect(action_Specify_Fixed_Plate, SIGNAL(triggered()),
			this, SLOT(pop_up_specify_fixed_plate_dialog()));
	// ----
	QObject::connect(action_Increment_Reconstruction_Time, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(increment_reconstruction_time()));
	QObject::connect(action_Decrement_Reconstruction_Time, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(decrement_reconstruction_time()));
	// ----
	QObject::connect(action_Animate, SIGNAL(triggered()),
			this, SLOT(pop_up_animate_dialog()));
	
	// View Menu:
	QObject::connect(action_Show_Rasters, SIGNAL(triggered()),
			this, SLOT(enable_raster_display()));
	// ----
	QObject::connect(action_Set_Camera_Viewpoint, SIGNAL(triggered()),
			this, SLOT(pop_up_set_camera_viewpoint_dialog()));
	QObject::connect(action_Move_Camera_Up, SIGNAL(triggered()),
			&(canvas_ptr->globe().orientation()), SLOT(move_camera_up()));
	QObject::connect(action_Move_Camera_Down, SIGNAL(triggered()),
			&(canvas_ptr->globe().orientation()), SLOT(move_camera_down()));
	QObject::connect(action_Move_Camera_Left, SIGNAL(triggered()),
			&(canvas_ptr->globe().orientation()), SLOT(move_camera_left()));
	QObject::connect(action_Move_Camera_Right, SIGNAL(triggered()),
			&(canvas_ptr->globe().orientation()), SLOT(move_camera_right()));
	// ----
	QObject::connect(action_Rotate_Camera_Clockwise, SIGNAL(triggered()),
			&(canvas_ptr->globe().orientation()), SLOT(rotate_camera_clockwise()));
	QObject::connect(action_Rotate_Camera_Anticlockwise, SIGNAL(triggered()),
			&(canvas_ptr->globe().orientation()), SLOT(rotate_camera_anticlockwise()));
	QObject::connect(action_Reset_Camera_Orientation, SIGNAL(triggered()),
			&(canvas_ptr->globe().orientation()), SLOT(orient_poles_vertically()));
	// ----
	QObject::connect(action_Set_Zoom, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(activate_zoom_spinbox()));
	QObject::connect(action_Zoom_In, SIGNAL(triggered()),
			&(canvas_ptr->viewport_zoom()), SLOT(zoom_in()));
	QObject::connect(action_Zoom_Out, SIGNAL(triggered()),
			&(canvas_ptr->viewport_zoom()), SLOT(zoom_out()));
	QObject::connect(action_Reset_Zoom_Level, SIGNAL(triggered()),
			&(canvas_ptr->viewport_zoom()), SLOT(reset_zoom()));
	// ----
	QObject::connect(action_Export_Geometry_Snapshot, SIGNAL(triggered()),
			this, SLOT(pop_up_export_geometry_snapshot_dialog()));
	
	// Tools Menu:
	QObject::connect(action_Reconstruction_Tree_and_Poles, SIGNAL(triggered()),
			this, SLOT(pop_up_euler_pole_dialog()));
	
	// Help Menu:
	QObject::connect(action_About, SIGNAL(triggered()),
			this, SLOT(pop_up_about_dialog()));
}


void	
GPlatesQtWidgets::ViewportWindow::set_up_task_panel_actions()
{
	ActionButtonBox &feature_actions = d_task_panel_ptr->feature_action_button_box();

	// If you want to add a new action button, the steps are:
	// 0. Open ViewportWindowUi.ui in the Designer.
	// 1. Create a QAction in the Designer's Action Editor, called action_Something.
	// 2. Assign icons, tooltips, and shortcuts as necessary.
	// 3. Drag this action to a menu (optional).
	// 4. Add code for the triggered() signal your action generates,
	//    see ViewportWindow::connect_menu_actions().
	// 5. Add a new line of code here adding the QAction to the ActionButtonBox.

	feature_actions.add_action(action_Query_Feature);
	feature_actions.add_action(action_Edit_Feature);
	// FIXME: The following are just to eat up space and see how things will look.
	// Remove for a release or a merge.
	feature_actions.add_action(action_Delete_Feature);
	feature_actions.add_action(action_Delete_Feature);
	feature_actions.add_action(action_Delete_Feature);
	feature_actions.add_action(action_Delete_Feature);
	feature_actions.add_action(action_Delete_Feature);
	feature_actions.add_action(action_Delete_Feature);
}


void
GPlatesQtWidgets::ViewportWindow::set_up_dock_context_menus()
{
	// Search Results Dock:
	dock_search_results->addAction(action_Information_Dock_At_Top);
	dock_search_results->addAction(action_Information_Dock_At_Bottom);
	QObject::connect(action_Information_Dock_At_Top, SIGNAL(triggered()),
			this, SLOT(dock_search_results_at_top()));
	QObject::connect(action_Information_Dock_At_Bottom, SIGNAL(triggered()),
			this, SLOT(dock_search_results_at_bottom()));
}


void
GPlatesQtWidgets::ViewportWindow::dock_search_results_at_top()
{
	dock_search_results->setFloating(false);
	addDockWidget(Qt::TopDockWidgetArea, dock_search_results);
}

void
GPlatesQtWidgets::ViewportWindow::dock_search_results_at_bottom()
{
	dock_search_results->setFloating(false);
	addDockWidget(Qt::BottomDockWidgetArea, dock_search_results);
}


void
GPlatesQtWidgets::ViewportWindow::highlight_first_clicked_feature_table_row() const
{
	QModelIndex idx = d_feature_table_model_ptr->index(0, 0);
	
	if (idx.isValid()) {
		table_view_clicked_geometries->selectionModel()->clear();
		table_view_clicked_geometries->selectionModel()->select(idx,
				QItemSelectionModel::Select |
				QItemSelectionModel::Current |
				QItemSelectionModel::Rows);
	}
	table_view_clicked_geometries->scrollToTop();
}


void
GPlatesQtWidgets::ViewportWindow::reconstruct_to_time(
		double recon_time)
{
	GPlatesMaths::Real original_recon_time(d_recon_time);
	
	d_recon_time = recon_time;
	reconstruct();
	
	// != does not work with doubles, so we must wrap them in Real.
	if (original_recon_time != GPlatesMaths::Real(d_recon_time)) {
		emit reconstruction_time_changed(d_recon_time);
	}
}


void
GPlatesQtWidgets::ViewportWindow::reconstruct_with_root(
		unsigned long recon_root)
{
	d_recon_root = recon_root;
	reconstruct();
}


void
GPlatesQtWidgets::ViewportWindow::reconstruct()
{
	d_canvas_ptr->clear_data();
	render_model(d_canvas_ptr, d_model_ptr, d_reconstruction_ptr, d_active_reconstructable_files, 
			d_active_reconstruction_files, d_recon_time, d_recon_root);
	render_mouse_interaction_geometry_layer(d_canvas_ptr, mouse_interaction_geometry_layer());


	if (d_euler_pole_dialog.isVisible()){
		d_euler_pole_dialog.update();
	}

	if (action_Show_Rasters->isChecked() &&
		!d_time_dependent_raster_map.isEmpty())
	{	
		update_time_dependent_raster();
	}
	d_canvas_ptr->update_canvas();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_specify_fixed_plate_dialog()
{
	d_specify_fixed_plate_dialog.show();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_set_camera_viewpoint_dialog()
{
	static const GPlatesMaths::PointOnSphere centre_of_canvas =
			GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(0, 0));

	GPlatesMaths::PointOnSphere oriented_centre = d_canvas_ptr->globe().Orient(centre_of_canvas);
	GPlatesMaths::LatLonPoint cur_llp = GPlatesMaths::make_lat_lon_point(oriented_centre);

	d_set_camera_viewpoint_dialog.set_lat_lon(cur_llp.latitude(), cur_llp.longitude());
	if (d_set_camera_viewpoint_dialog.exec())
	{
		try {
			GPlatesMaths::LatLonPoint desired_centre(
					d_set_camera_viewpoint_dialog.latitude(),
					d_set_camera_viewpoint_dialog.longitude());
			
			GPlatesMaths::PointOnSphere oriented_desired_centre = 
					d_canvas_ptr->globe().orientation().orient_point(
							GPlatesMaths::make_point_on_sphere(desired_centre));
			d_canvas_ptr->globe().SetNewHandlePos(oriented_desired_centre);
			d_canvas_ptr->globe().UpdateHandlePos(centre_of_canvas);
			
			d_canvas_ptr->globe().orientation().orient_poles_vertically();
			d_canvas_ptr->update_canvas();
		} catch (GPlatesMaths::InvalidLatLonException &) {
			// The argument name in the above expression was removed to
			// prevent "unreferenced local variable" compiler warnings under MSVC.

			// User somehow managed to specify an invalid lat,lon. Pretend it didn't happen.
		}
	}
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_euler_pole_dialog()
{
	d_euler_pole_dialog.update();

	d_euler_pole_dialog.show();
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_animate_dialog()
{
	if ( ! d_animate_dialog_has_been_shown) {
		d_animate_dialog.set_start_time_value_to_view_time();
		d_animate_dialog.set_current_time_value_to_view_time();
		d_animate_dialog_has_been_shown = true;
	}
	d_animate_dialog.show();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_about_dialog()
{
	d_about_dialog.show();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_license_dialog()
{
	d_license_dialog.show();
}


void
GPlatesQtWidgets::ViewportWindow::choose_drag_globe_tool()
{
	uncheck_all_tools();
	action_Drag_Globe->setChecked(true);
	d_canvas_tool_choice_ptr->choose_reorient_globe_tool();
}


void
GPlatesQtWidgets::ViewportWindow::choose_zoom_globe_tool()
{
	uncheck_all_tools();
	action_Zoom_Globe->setChecked(true);
	d_canvas_tool_choice_ptr->choose_zoom_globe_tool();
}


void
GPlatesQtWidgets::ViewportWindow::choose_click_geometry_tool()
{
	uncheck_all_tools();
	action_Click_Geometry->setChecked(true);
	d_canvas_tool_choice_ptr->choose_click_geometry_tool();
	d_task_panel_ptr->choose_feature_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_digitise_polyline_tool()
{
	uncheck_all_tools();
	action_Digitise_New_Polyline->setChecked(true);
	d_canvas_tool_choice_ptr->choose_digitise_polyline_tool();
	d_task_panel_ptr->choose_digitisation_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_digitise_multipoint_tool()
{
	uncheck_all_tools();
	action_Digitise_New_MultiPoint->setChecked(true);
	d_canvas_tool_choice_ptr->choose_digitise_multipoint_tool();
	d_task_panel_ptr->choose_digitisation_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_digitise_polygon_tool()
{
	uncheck_all_tools();
	action_Digitise_New_Polygon->setChecked(true);
	d_canvas_tool_choice_ptr->choose_digitise_polygon_tool();
	d_task_panel_ptr->choose_digitisation_tab();
}


void
GPlatesQtWidgets::ViewportWindow::uncheck_all_tools()
{
	action_Drag_Globe->setChecked(false);
	action_Zoom_Globe->setChecked(false);
	action_Click_Geometry->setChecked(false);
	action_Digitise_New_Polyline->setChecked(false);
	action_Digitise_New_MultiPoint->setChecked(false);
	action_Digitise_New_Polygon->setChecked(false);
}


void
GPlatesQtWidgets::ViewportWindow::enable_or_disable_feature_actions(
		GPlatesModel::FeatureHandle::weak_ref focused_feature)
{
	bool enable = focused_feature.is_valid();
	action_Query_Feature->setEnabled(enable);
	action_Edit_Feature->setEnabled(enable);
#if 0
	// FIXME: Delete Feature is unimplemented and should stay disabled for now.
	action_Delete_Feature->setEnabled(enable);
#endif
#if 0
	// FIXME: Add to Selection is unimplemented and should stay disabled for now.
	// FIXME: To handle the "Remove from Selection", "Clear Selection" actions,
	// we may want to modify this method to also test for a nonempty selection of features.
	action_Add_Feature_To_Selection->setEnabled(enable);
#endif
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_read_errors_dialog()
{
	d_read_errors_dialog.show();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_manage_feature_collections_dialog()
{
	d_manage_feature_collections_dialog.show();
}


void
GPlatesQtWidgets::ViewportWindow::deactivate_loaded_file(
		file_info_iterator loaded_file)
{
	// Don't bother checking whether 'loaded_file' is actually an element of
	// 'd_active_reconstructable_files' and/or 'd_active_reconstruction_files' -- just tell the
	// lists to remove the value if it *is* an element.

	// list<T>::remove(const T &val) -- remove all elements with value 'val'.
	// Will not throw (unless element comparisons can throw).
	// See Josuttis section 6.10.7 "Inserting and Removing Elements".
	d_active_reconstructable_files.remove(loaded_file);
	d_active_reconstruction_files.remove(loaded_file);

	// FIXME:  This should not happen here -- in fact, it should be removal of the loaded file
	// (using 'remove_loaded_file' in ApplicationState) which triggers *this*! -- but until we
	// have multiple view windows, it doesn't matter.
	GPlatesAppState::ApplicationState::instance()->remove_loaded_file(loaded_file);
}


bool
GPlatesQtWidgets::ViewportWindow::is_file_active(
		file_info_iterator loaded_file)
{
	active_files_iterator reconstructable_it = d_active_reconstructable_files.begin();
	active_files_iterator reconstructable_end = d_active_reconstructable_files.end();
	for (; reconstructable_it != reconstructable_end; ++reconstructable_it) {
		if (*reconstructable_it == loaded_file) {
			return true;
		}
	}

	active_files_iterator reconstruction_it = d_active_reconstruction_files.begin();
	active_files_iterator reconstruction_end = d_active_reconstruction_files.end();
	for (; reconstruction_it != reconstruction_end; ++reconstruction_it) {
		if (*reconstruction_it == loaded_file) {
			return true;
		}
	}

	// loaded_file not found in any active files lists, must not be active.
	return false;
}


void
GPlatesQtWidgets::ViewportWindow::create_svg_file()
{
	QString filename = QFileDialog::getSaveFileName(this,
			tr("Save As"), "", tr("SVG file (*.svg)"));

	if (filename.isEmpty()){
		return;
	}

	bool result = GPlatesGui::SvgExport::create_svg_output(filename,d_canvas_ptr);
	if (!result){
		std::cerr << "Error creating SVG output.." << std::endl;
	}

}


void
GPlatesQtWidgets::ViewportWindow::close_all_dialogs()
{
	d_specify_fixed_plate_dialog.reject();
	d_set_camera_viewpoint_dialog.reject();
	d_animate_dialog.reject();
	d_about_dialog.reject();
	d_license_dialog.reject();
	d_feature_properties_dialog.reject();
	d_read_errors_dialog.reject();
	d_manage_feature_collections_dialog.reject();
}

void
GPlatesQtWidgets::ViewportWindow::closeEvent(QCloseEvent *close_event)
{
	// For now, always accept the close event.
	// In the future, ->reject() can be used to postpone closure in the event of
	// unsaved files, etc.
	close_event->accept();
	// If we decide to accept the close event, we should also tidy up after ourselves.
	close_all_dialogs();
}

void
GPlatesQtWidgets::ViewportWindow::remap_shapefile_attributes(
	GPlatesFileIO::FileInfo &file_info)
{

	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();	

	GPlatesFileIO::ShapeFileReader::remap_shapefile_attributes(file_info,*d_model_ptr,read_errors);

	d_read_errors_dialog.update();

	// Pop up errors only if appropriate.
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors) {
		d_read_errors_dialog.show();
	}


	// plate-ids may have changed, so update the reconstruction. 
	reconstruct();
}

void
GPlatesQtWidgets::ViewportWindow::enable_raster_display()
{
	if (action_Show_Rasters->isChecked())
	{
		d_canvas_ptr->enable_raster_display();
	}
	else
	{
		d_canvas_ptr->disable_raster_display();
	}
}

void
GPlatesQtWidgets::ViewportWindow::open_global_raster()
{

	QString filename = QFileDialog::getOpenFileName(0,
		QObject::tr("Open File"), d_open_file_path, QObject::tr("Image files (*.jpg *.jpeg)") );

	if ( filename.isEmpty()){
		return;
	}

	if (load_global_raster(filename))
	{
		// If we've successfully loaded a single raster, clear the raster_map.
		d_time_dependent_raster_map.clear();
	}
	QFileInfo last_opened_file(filename);
	d_open_file_path = last_opened_file.path();
}

bool
GPlatesQtWidgets::ViewportWindow::load_global_raster(
	QString filename)
{
	bool result = false;
	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();	
	GPlatesFileIO::FileInfo file_info(filename);

	try{

		GPlatesFileIO::RasterReader::read_file(file_info, d_canvas_ptr->globe().texture(), read_errors);
		action_Show_Rasters->setChecked(true);
		result = true;
	}
	catch (GPlatesFileIO::ErrorOpeningFileForReadingException &e)
	{
		// FIXME: A bit of a sucky conversion from ErrorOpeningFileForReadingException to
		// ReadErrorOccurrence, but hey, this whole function will be rewritten when we add
		// QFileDialog support.
		// FIXME: I suspect I'm Missing The Point with these shared_ptrs.
		boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
			new GPlatesFileIO::LocalFileDataSource(e.filename(), GPlatesFileIO::DataFormats::Unspecified));
		boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
			new GPlatesFileIO::LineNumberInFile(0));
		read_errors.d_failures_to_begin.push_back(GPlatesFileIO::ReadErrorOccurrence(
			e_source,
			e_location,
			GPlatesFileIO::ReadErrors::ErrorOpeningFileForReading,
			GPlatesFileIO::ReadErrors::FileNotLoaded));
	}
	catch (GPlatesGlobal::Exception &e)
	{
		std::cerr << "Caught GPlates exception: " << e << std::endl;
	}
	catch (...)
	{
		std::cerr << "Caught exception loading raster file." << std::endl;
	}

	d_canvas_ptr->update_canvas();
	d_read_errors_dialog.update();

	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors) {
		d_read_errors_dialog.show();
	}
	return result;
}

void
GPlatesQtWidgets::ViewportWindow::open_time_dependent_global_raster_set()
{
	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();	

	QFileDialog file_dialog(0,QObject::tr("Select folder containing time-dependent file set"),d_open_file_path,NULL);
	file_dialog.setFileMode(QFileDialog::DirectoryOnly);

	if (file_dialog.exec())
	{
		QStringList directory_list = file_dialog.selectedFiles();
		QString directory = directory_list.at(0);

		GPlatesFileIO::RasterReader::populate_time_dependent_raster_map(d_time_dependent_raster_map,directory,read_errors);
		
		QFileInfo last_opened_file(file_dialog.directory().absoluteFilePath(directory_list.last()));
		d_open_file_path = last_opened_file.path();

		d_read_errors_dialog.update();
	
		// Pop up errors only if appropriate.
		GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
		if (num_initial_errors != num_final_errors) {
			d_read_errors_dialog.show();
		}
	}

	if (!d_time_dependent_raster_map.isEmpty())
	{
		action_Show_Rasters->setChecked(true);
		update_time_dependent_raster();
	}



}

void
GPlatesQtWidgets::ViewportWindow::update_time_dependent_raster()
{
	QString filename = GPlatesFileIO::RasterReader::get_nearest_raster_filename(d_time_dependent_raster_map,d_recon_time);
	load_global_raster(filename);
}

