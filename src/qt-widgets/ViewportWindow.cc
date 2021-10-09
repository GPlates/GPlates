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

#include "ViewportWindow.h"
#include "InformationDialog.h"

#include "global/Exception.h"
#include "global/UnexpectedEmptyFeatureCollectionException.h"
#include "gui/CanvasToolAdapter.h"
#include "gui/CanvasToolChoice.h"
#include "gui/FeatureWeakRefSequence.h"
#include "maths/PointOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "maths/InvalidLatLonException.h"
#include "model/Model.h"
#include "model/types.h"
#include "model/DummyTransactionHandle.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/PlatesLineFormatReader.h"
#include "file-io/PlatesRotationFormatReader.h"
#include "file-io/FileInfo.h"
#include "file-io/Reader.h"
#include "file-io/ShapeFileReader.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "gui/SvgExport.h"
#include "gui/PlatesColourTable.h"


namespace
{
	bool
	file_name_ends_with(
			const GPlatesFileIO::FileInfo &file, 
			const QString &suffix)
	{
		return file.get_qfileinfo().suffix().endsWith(QString(suffix), Qt::CaseInsensitive);
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

				// All loaded files are added to the set of loaded files.
				GPlatesAppState::ApplicationState::file_info_iterator new_file =
					GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);

				// Line format files are made active by default.
				d_active_reconstructable_files.push_back(new_file);
			}
			else if (is_plates_rotation_format_file(file))
			{
				GPlatesFileIO::PlatesRotationFormatReader::read_file(file, *d_model_ptr, read_errors);
				// All loaded files are added to the set of loaded files.
				GPlatesAppState::ApplicationState::file_info_iterator new_file =
					GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);

				if ( ! have_loaded_new_rotation_file) 
				{
					// We only want to make the first rotation file active.
					d_active_reconstruction_files.clear();
					d_active_reconstruction_files.push_back(new_file);
					have_loaded_new_rotation_file = true;
				}
			} 
			else if (file.get_qfileinfo().suffix().endsWith(QString("shp"),Qt::CaseInsensitive))
			{
				GPlatesFileIO::ShapeFileReader::read_file(file,*d_model_ptr,read_errors);
				GPlatesAppState::ApplicationState::file_info_iterator new_file =
					GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);
				d_active_reconstructable_files.push_back(new_file);
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

			std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> >::iterator iter =
					reconstruction->point_geometries().begin();
			std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> >::iterator finish =
					reconstruction->point_geometries().end();
			while (iter != finish) {
				GPlatesGui::PlatesColourTable::const_iterator colour = GPlatesGui::PlatesColourTable::Instance()->end();
				if (iter->reconstruction_plate_id()) {
					colour = GPlatesGui::PlatesColourTable::Instance()->lookup(*(iter->reconstruction_plate_id()));
				}

				if (colour == GPlatesGui::PlatesColourTable::Instance()->end()) {
					// Anything not in the table in gui/PlatesColourTable.cc uses the 'Olive' colour.
					colour = &GPlatesGui::Colour::OLIVE;
				}

				canvas_ptr->draw_point(iter->geometry(), colour);
				++iter;
			}
			std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> >::iterator iter2 =
					reconstruction->polyline_geometries().begin();
			std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> >::iterator finish2 =
					reconstruction->polyline_geometries().end();
			while (iter2 != finish2) {
				GPlatesGui::PlatesColourTable::const_iterator colour = GPlatesGui::PlatesColourTable::Instance()->end();
				if (iter2->reconstruction_plate_id()) {
					colour = GPlatesGui::PlatesColourTable::Instance()->lookup(*(iter2->reconstruction_plate_id()));
				}

				if (colour == GPlatesGui::PlatesColourTable::Instance()->end()) {
					// Anything not in the table in gui/PlatesColourTable.cc uses the 'Olive' colour.
					colour = &GPlatesGui::Colour::OLIVE;
				}

				canvas_ptr->draw_polyline(iter2->geometry(), colour);
				++iter2;
			}

			//render(reconstruction->point_geometries().begin(), reconstruction->point_geometries().end(), &GPlatesQtWidgets::GlobeCanvas::draw_point, canvas_ptr);
			//for_each(reconstruction->point_geometries().begin(), reconstruction->point_geometries().end(), render(canvas_ptr, &GlobeCanvas::draw_point, point_colour))
			// for_each(reconstruction->polyline_geometries().begin(), reconstruction->polyline_geometries().end(), polyline_point);
		} catch (GPlatesGlobal::Exception &e) {
			std::cerr << e << std::endl;
		}
	}

} // namespace


GPlatesQtWidgets::ViewportWindow::ViewportWindow() :
	d_model_ptr(new GPlatesModel::Model()),
	d_reconstruction_ptr(d_model_ptr->create_empty_reconstruction(0.0, 0)),
	d_recon_time(0.0),
	d_recon_root(0),
	d_reconstruction_view_widget(*this, this),
	d_specify_fixed_plate_dialog(d_recon_root, this),
	d_set_camera_viewpoint_dialog(*this, this),
	d_animate_dialog(*this, this),
	d_about_dialog(*this, this),
	d_license_dialog(&d_about_dialog),
	d_query_feature_properties_dialog(*this, this),
	d_read_errors_dialog(this),
	d_manage_feature_collections_dialog(*this, this),
	d_animate_dialog_has_been_shown(false),
	d_feature_table_model_ptr(new GPlatesGui::FeatureTableModel())
{
	setupUi(this);

	d_canvas_ptr = &(d_reconstruction_view_widget.globe_canvas());
	
#if 0
	QObject::connect(d_canvas_ptr, SIGNAL(items_selected()), this, SLOT(selection_handler()));
	QObject::connect(d_canvas_ptr, SIGNAL(left_mouse_button_clicked()), this, SLOT(mouse_click_handler()));
#endif
	QObject::connect(action_Reconstruct_to_Time, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(activate_time_spinbox()));
	QObject::connect(&d_reconstruction_view_widget, SIGNAL(reconstruction_time_changed(double)),
			this, SLOT(reconstruct_to_time(double)));

	QObject::connect(action_Specify_Fixed_Plate, SIGNAL(triggered()),
			this, SLOT(pop_up_specify_fixed_plate_dialog()));
	QObject::connect(&d_specify_fixed_plate_dialog, SIGNAL(value_changed(unsigned long)),
			this, SLOT(reconstruct_with_root(unsigned long)));

	QObject::connect(action_Set_Camera_Viewpoint, SIGNAL(triggered()),
			this, SLOT(pop_up_set_camera_viewpoint_dialog()));
	QObject::connect(action_Move_Camera_Up, SIGNAL(triggered()),
			&(d_canvas_ptr->globe().orientation()), SLOT(move_camera_up()));
	QObject::connect(action_Move_Camera_Down, SIGNAL(triggered()),
			&(d_canvas_ptr->globe().orientation()), SLOT(move_camera_down()));
	QObject::connect(action_Move_Camera_Left, SIGNAL(triggered()),
			&(d_canvas_ptr->globe().orientation()), SLOT(move_camera_left()));
	QObject::connect(action_Move_Camera_Right, SIGNAL(triggered()),
			&(d_canvas_ptr->globe().orientation()), SLOT(move_camera_right()));
	QObject::connect(action_Rotate_Camera_Clockwise, SIGNAL(triggered()),
			&(d_canvas_ptr->globe().orientation()), SLOT(rotate_camera_clockwise()));
	QObject::connect(action_Rotate_Camera_Anticlockwise, SIGNAL(triggered()),
			&(d_canvas_ptr->globe().orientation()), SLOT(rotate_camera_anticlockwise()));
	QObject::connect(action_Reset_Camera_Orientation, SIGNAL(triggered()),
			&(d_canvas_ptr->globe().orientation()), SLOT(orient_poles_vertically()));

	QObject::connect(action_Animate, SIGNAL(triggered()),
			this, SLOT(pop_up_animate_dialog()));
	QObject::connect(&d_animate_dialog, SIGNAL(current_time_changed(double)),
			&d_reconstruction_view_widget, SLOT(set_reconstruction_time(double)));

	QObject::connect(action_Increment_Reconstruction_Time, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(increment_reconstruction_time()));
	QObject::connect(action_Decrement_Reconstruction_Time, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(decrement_reconstruction_time()));
	
	QObject::connect(action_Set_Zoom, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(activate_zoom_spinbox()));

	QObject::connect(action_Zoom_In, SIGNAL(triggered()),
			&(d_canvas_ptr->viewport_zoom()), SLOT(zoom_in()));
	QObject::connect(action_Zoom_Out, SIGNAL(triggered()),
			&(d_canvas_ptr->viewport_zoom()), SLOT(zoom_out()));
	QObject::connect(action_Reset_Zoom_Level, SIGNAL(triggered()),
			&(d_canvas_ptr->viewport_zoom()), SLOT(reset_zoom()));
	
	QObject::connect(action_Export_Geometry_Snapshot, SIGNAL(triggered()),
			this, SLOT(pop_up_export_geometry_snapshot_dialog()));

	QObject::connect(action_About, SIGNAL(triggered()),
			this, SLOT(pop_up_about_dialog()));

	QObject::connect(d_canvas_ptr, SIGNAL(mouse_pointer_position_changed(const GPlatesMaths::PointOnSphere &, bool)),
			&d_reconstruction_view_widget, SLOT(update_mouse_pointer_position(const GPlatesMaths::PointOnSphere &, bool)));

	QObject::connect(action_Drag_Globe, SIGNAL(triggered()),
			this, SLOT(choose_drag_globe_tool()));
	QObject::connect(action_Zoom_Globe, SIGNAL(triggered()),
			this, SLOT(choose_zoom_globe_tool()));
	QObject::connect(action_Query_Feature, SIGNAL(triggered()),
			this, SLOT(choose_query_feature_tool()));

	QObject::connect(action_Open_Feature_Collection, SIGNAL(triggered()),
			&d_manage_feature_collections_dialog, SLOT(open_file()));
	QObject::connect(action_Manage_Feature_Collections, SIGNAL(triggered()),
			this, SLOT(pop_up_manage_feature_collections_dialog()));
	QObject::connect(action_File_Errors, SIGNAL(triggered()),
			this, SLOT(pop_up_read_errors_dialog()));

	setCentralWidget(&d_reconstruction_view_widget);

	// Render everything on the screen in present-day positions.
	d_canvas_ptr->clear_data();
	render_model(d_canvas_ptr, d_model_ptr, d_reconstruction_ptr, d_active_reconstructable_files, 
			d_active_reconstruction_files, 0.0, d_recon_root);
	d_canvas_ptr->update_canvas();

	// FIXME: feature table model for this Qt widget and the Query Tool should be stored in ViewState.
	tab_list_clicked->setModel(d_feature_table_model_ptr);
	tab_list_clicked->verticalHeader()->hide();
	tab_list_clicked->resizeColumnsToContents();
	GPlatesGui::FeatureTableModel::set_default_resize_modes(*tab_list_clicked->horizontalHeader());
	tab_list_clicked->horizontalHeader()->setMinimumSectionSize(60);
	tab_list_clicked->horizontalHeader()->setMovable(true);
	tab_list_clicked->horizontalHeader()->setHighlightSections(false);
	QObject::connect(tab_list_clicked->selectionModel(),
			SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			d_feature_table_model_ptr,
			SLOT(handle_selection_change(const QItemSelection &, const QItemSelection &)));
	QObject::connect(d_feature_table_model_ptr,
			SIGNAL(selected_feature_changed(GPlatesModel::FeatureHandle::weak_ref)),
			&d_query_feature_properties_dialog,
			SLOT(display_feature(GPlatesModel::FeatureHandle::weak_ref)));

	// FIXME:  This is, of course, very exception-unsafe.  This whole class needs to be nuked.
	d_canvas_tool_choice_ptr =
			new GPlatesGui::CanvasToolChoice(
					d_canvas_ptr->globe(),
					*d_canvas_ptr,
					*this,
					*d_feature_table_model_ptr,
					d_query_feature_properties_dialog);

	QObject::connect(action_Drag_Globe, SIGNAL(triggered()),
			d_canvas_tool_choice_ptr, SLOT(choose_reorient_globe_tool()));
	QObject::connect(action_Zoom_Globe, SIGNAL(triggered()),
			d_canvas_tool_choice_ptr, SLOT(choose_zoom_globe_tool()));
	QObject::connect(action_Query_Feature, SIGNAL(triggered()),
			d_canvas_tool_choice_ptr, SLOT(choose_query_feature_tool()));

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

	QObject::connect(action_Quit, SIGNAL(triggered()), this, SLOT(close()));
}


void
GPlatesQtWidgets::ViewportWindow::reconstruct_to_time(
		double recon_time)
{
	d_recon_time = recon_time;
	reconstruct();
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
}


void
GPlatesQtWidgets::ViewportWindow::choose_zoom_globe_tool()
{
	uncheck_all_tools();
	action_Zoom_Globe->setChecked(true);
}


void
GPlatesQtWidgets::ViewportWindow::choose_query_feature_tool()
{
	uncheck_all_tools();
	action_Query_Feature->setChecked(true);
}


void
GPlatesQtWidgets::ViewportWindow::uncheck_all_tools()
{
	action_Drag_Globe->setChecked(false);
	action_Zoom_Globe->setChecked(false);
	action_Query_Feature->setChecked(false);
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
	d_query_feature_properties_dialog.reject();
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

