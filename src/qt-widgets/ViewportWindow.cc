/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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
#include <QLocale>
#include <QString>
#include <QStringList>

#include "ViewportWindow.h"
#include "InformationDialog.h"

#include "global/Exception.h"
#include "gui/CanvasToolAdapter.h"
#include "gui/CanvasToolChoice.h"
#include "gui/FeatureWeakRefSequence.h"
#include "model/Model.h"
#include "model/types.h"
#include "model/DummyTransactionHandle.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/PlatesLineFormatReader.h"
#include "file-io/PlatesRotationFormatReader.h"
#include "file-io/FileInfo.h"
#include "file-io/Reader.h"
#include "gui/PlatesColourTable.h"


void
GPlatesQtWidgets::ViewportWindow::load_files(
		const QStringList &file_names)
{
	d_read_errors_dialog.clear();
	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog.read_errors();

	QStringList::const_iterator iter = file_names.begin();
	QStringList::const_iterator end = file_names.end();


	bool have_loaded_new_rotation_file = false;

	for ( ; iter != end; ++iter) {

		GPlatesFileIO::FileInfo file(*iter);

		try
		{
			if (file.get_qfileinfo().suffix().endsWith(QString("dat"), Qt::CaseInsensitive))
			{
				GPlatesFileIO::PlatesLineFormatReader reader;
				reader.read_file(file, *d_model_ptr, read_errors);

				// All loaded files are added to the set of loaded files.
				GPlatesAppState::ApplicationState::file_info_iterator new_file =
					GPlatesAppState::ApplicationState::instance()->push_back_loaded_file(file);

				// Line format files are made active by default.
				d_active_reconstructable_files.push_back(new_file);
			}
			else if (file.get_qfileinfo().suffix().endsWith(QString("rot"), Qt::CaseInsensitive))
			{
				GPlatesFileIO::PlatesRotationFormatReader reader;
				reader.read_file(file, *d_model_ptr, read_errors);

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
	if ( ! read_errors.is_empty())
	{
		d_read_errors_dialog.update();
		d_read_errors_dialog.show();
	}
}


namespace
{
	void
	get_features_collection_from_file_info_collection(
			GPlatesQtWidgets::ViewportWindow::ActiveFilesCollection &active_files,
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &features_collection) {

		GPlatesQtWidgets::ViewportWindow::ActiveFilesCollection::iterator
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
			GPlatesQtWidgets::ViewportWindow::ActiveFilesCollection &active_reconstructable_files,
			GPlatesQtWidgets::ViewportWindow::ActiveFilesCollection &active_reconstruction_files,
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
			GPlatesQtWidgets::ViewportWindow::ActiveFilesCollection &active_reconstructable_files,
			GPlatesQtWidgets::ViewportWindow::ActiveFilesCollection &active_reconstruction_files,
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
					// XXX: For lack of anything better to do, we will colour data that lacks a plate id grey
					colour = &GPlatesGui::Colour::GREY;
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
					// XXX: For lack of anything better to do, we will colour data that lacks a plate id grey
					colour = &GPlatesGui::Colour::GREY;
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
}


GPlatesQtWidgets::ViewportWindow::ViewportWindow(
		const QStringList &plates_line_fnames,
		const QStringList &plates_rot_fnames):
	d_model_ptr(new GPlatesModel::Model()),
	d_reconstruction_ptr(d_model_ptr->create_empty_reconstruction(0.0, 0)),
	d_recon_time(0.0),
	d_recon_root(0),
	d_reconstruction_view_widget(*this, this),
	d_specify_fixed_plate_dialog(d_recon_root, this),
	d_animate_dialog(*this, this),
	d_about_dialog(*this, this),
	d_license_dialog(&d_about_dialog),
	d_query_feature_properties_dialog(this),
	d_read_errors_dialog(this),
	d_animate_dialog_has_been_shown(false)
{
	setupUi(this);

	load_files(plates_line_fnames);
	load_files(plates_rot_fnames);

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
			d_canvas_ptr, SLOT(zoom_in()));
	QObject::connect(action_Zoom_Out, SIGNAL(triggered()),
			d_canvas_ptr, SLOT(zoom_out()));
	QObject::connect(action_Reset_Zoom_Level, SIGNAL(triggered()),
			d_canvas_ptr, SLOT(reset_zoom()));
	
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

	QObject::connect(action_File_Errors, SIGNAL(triggered()),
			this, SLOT(pop_up_read_errors_dialog()));

	setCentralWidget(&d_reconstruction_view_widget);

	// Render everything on the screen in present-day positions.
	d_canvas_ptr->clear_data();
	render_model(d_canvas_ptr, d_model_ptr, d_reconstruction_ptr, d_active_reconstructable_files, 
			d_active_reconstruction_files, 0.0, d_recon_root);
	d_canvas_ptr->update_canvas();

	GPlatesGui::FeatureWeakRefSequence::non_null_ptr_type clicked_features =
			GPlatesGui::FeatureWeakRefSequence::create();

	// FIXME:  This is, of course, very exception-unsafe.  This whole class needs to be nuked.
	d_canvas_tool_choice_ptr =
			new GPlatesGui::CanvasToolChoice(
					d_canvas_ptr->globe(),
					*d_canvas_ptr,
					*this,
					clicked_features,
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
