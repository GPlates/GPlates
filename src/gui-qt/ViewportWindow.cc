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
#include <boost/format.hpp>

#include "ViewportWindow.h"
#include "InformationDialog.h"

#include "global/Exception.h"
#include "maths/LatLonPointConversions.h"
#include "model/ModelUtility.h"
#include "model/DummyTransactionHandle.h"
#include "fileio/ReadErrorAccumulation.h"
#include "fileio/ErrorOpeningFileForReadingException.h"
#include "fileio/PlatesLineFormatReader.h"
#include "fileio/PlatesRotationFormatReader.h"

namespace
{
	void
	load_plates_files(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &reconstructable_features,
			GPlatesModel::FeatureCollectionHandle::weak_ref &reconstruction_features,
			const std::string &plates_line_fname,
			const std::string &plates_rot_fname)
	{
		GPlatesFileIO::ReadErrorAccumulation read_errors;

		// FIXME:  Handle this ErrorOpeningFileForReadingException properly.
		try
		{
			reconstructable_features =
					GPlatesFileIO::PlatesLineFormatReader::read_file(
							plates_line_fname, model, read_errors);

			reconstruction_features =
					GPlatesFileIO::PlatesRotationFormatReader::read_file(
							plates_rot_fname, model, read_errors);
		}
		catch (GPlatesFileIO::ErrorOpeningFileForReadingException &e)
		{
			std::cerr << "Unable to open file '" << e.filename() << "' for reading." << std::endl;
		}
	}


	void
	render_model(
			GPlatesGui::GlobeCanvas *canvas_ptr, 
			GPlatesModel::Model *model_ptr, 
			GPlatesModel::FeatureCollectionHandle::weak_ref isochrons,
			GPlatesModel::FeatureCollectionHandle::weak_ref total_recon_seqs,
			double recon_time,
			GPlatesModel::GpmlPlateId::integer_plate_id_type recon_root)
	{
		try {
			GPlatesModel::Reconstruction::non_null_ptr_type reconstruction =
					model_ptr->create_reconstruction(isochrons, total_recon_seqs,
							recon_time, recon_root);

			std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> >::iterator iter =
					reconstruction->point_geometries().begin();
			std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> >::iterator finish =
					reconstruction->point_geometries().end();
			while (iter != finish) {
				canvas_ptr->draw_point(*iter->geometry());
				++iter;
			}
			std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> >::iterator iter2 =
					reconstruction->polyline_geometries().begin();
			std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> >::iterator finish2 =
					reconstruction->polyline_geometries().end();
			while (iter2 != finish2) {
				canvas_ptr->draw_polyline(*iter2->geometry());
				++iter2;
			}
			//render(reconstruction->point_geometries().begin(), reconstruction->point_geometries().end(), &GPlatesGui::GlobeCanvas::draw_point, canvas_ptr);
			//for_each(reconstruction->point_geometries().begin(), reconstruction->point_geometries().end(), render(canvas_ptr, &GlobeCanvas::draw_point, point_colour))
			// for_each(reconstruction->polyline_geometries().begin(), reconstruction->polyline_geometries().end(), polyline_point);
		} catch (GPlatesGlobal::Exception &e) {
			std::cerr << e << std::endl;
		}
	}
}


GPlatesGui::ViewportWindow::ViewportWindow(
		const std::string &plates_line_fname,
		const std::string &plates_rot_fname):
	d_recon_time(0.0),
	d_recon_root(0),
	d_reconstruct_to_time_dialog(d_recon_time, this),
	d_specify_fixed_plate_dialog(d_recon_root, this),
	d_animate_dialog(*this, this),
	d_animate_dialog_has_been_shown(false)
{
	setupUi(this);

	d_model_ptr = new GPlatesModel::Model();
	load_plates_files(*d_model_ptr, d_isochrons, d_total_recon_seqs, plates_line_fname, plates_rot_fname);

	d_canvas_ptr = new GlobeCanvas(this);
#if 0
	QObject::connect(d_canvas_ptr, SIGNAL(items_selected()), this, SLOT(selection_handler()));
	QObject::connect(d_canvas_ptr, SIGNAL(left_mouse_button_clicked()), this, SLOT(mouse_click_handler()));
#endif
	QObject::connect(action_Reconstruct_to_Time, SIGNAL(activated()),
			this, SLOT(pop_up_reconstruct_to_time_dialog()));
	QObject::connect(&d_reconstruct_to_time_dialog, SIGNAL(value_changed(double)),
			this, SLOT(set_reconstruction_time_and_reconstruct(double)));

	QObject::connect(action_Specify_Fixed_Plate, SIGNAL(activated()),
			this, SLOT(pop_up_specify_fixed_plate_dialog()));
	QObject::connect(&d_specify_fixed_plate_dialog, SIGNAL(value_changed(unsigned long)),
			this, SLOT(set_reconstruction_root_and_reconstruct(unsigned long)));

	QObject::connect(action_Animate, SIGNAL(activated()),
			this, SLOT(pop_up_animate_dialog()));
	QObject::connect(&d_animate_dialog, SIGNAL(current_time_changed(double)),
			this, SLOT(set_reconstruction_time_and_reconstruct(double)));
	
	QObject::connect(action_Increment_Reconstruction_Time, SIGNAL(activated()),
			this, SLOT(increment_reconstruction_time_and_reconstruct()));
	QObject::connect(action_Decrement_Reconstruction_Time, SIGNAL(activated()),
			this, SLOT(decrement_reconstruction_time_and_reconstruct()));
	
	centralwidget = d_canvas_ptr;
	setCentralWidget(centralwidget);

	// Render everything on the screen in present-day positions.
	const QString message(boost::str(boost::format("%1% MYA") % d_recon_time).c_str());
	statusbar->showMessage(message);
	d_canvas_ptr->clear_data();
	::render_model(d_canvas_ptr, d_model_ptr, d_isochrons, d_total_recon_seqs, 0.0, d_recon_root);
	d_canvas_ptr->update_canvas();
}


void
GPlatesGui::ViewportWindow::set_reconstruction_time_and_reconstruct(
		double recon_time)
{
	d_recon_time = recon_time;

	const QString message(boost::str(boost::format("%1% MYA") % d_recon_time).c_str());
	statusbar->showMessage(message);
	d_canvas_ptr->clear_data();
	::render_model(d_canvas_ptr, d_model_ptr, d_isochrons, d_total_recon_seqs, d_recon_time, d_recon_root);
	d_canvas_ptr->update_canvas();
}


void
GPlatesGui::ViewportWindow::set_reconstruction_root_and_reconstruct(
		unsigned long recon_root)
{
	d_recon_root = recon_root;

	d_canvas_ptr->clear_data();
	::render_model(d_canvas_ptr, d_model_ptr, d_isochrons, d_total_recon_seqs, d_recon_time, d_recon_root);
	d_canvas_ptr->update_canvas();
}


void
GPlatesGui::ViewportWindow::increment_reconstruction_time_and_reconstruct()
{
	double recon_time = d_recon_time + 1.0;
	if (recon_time <= 1000000.0) {
		d_recon_time = recon_time;
	}

	const QString message(boost::str(boost::format("%1% MYA") % d_recon_time).c_str());
	statusbar->showMessage(message);
	d_canvas_ptr->clear_data();
	::render_model(d_canvas_ptr, d_model_ptr, d_isochrons, d_total_recon_seqs, d_recon_time, d_recon_root);
	d_canvas_ptr->update_canvas();
}


void
GPlatesGui::ViewportWindow::decrement_reconstruction_time_and_reconstruct()
{
	double recon_time = d_recon_time - 1.0;
	// FIXME:  Use approx comparison for equality to zero.
	if (recon_time >= 0.0) {
		d_recon_time = recon_time;
	}

	const QString message(boost::str(boost::format("%1% MYA") % d_recon_time).c_str());
	statusbar->showMessage(message);
	d_canvas_ptr->clear_data();
	::render_model(d_canvas_ptr, d_model_ptr, d_isochrons, d_total_recon_seqs, d_recon_time, d_recon_root);
	d_canvas_ptr->update_canvas();
}


void
GPlatesGui::ViewportWindow::pop_up_reconstruct_to_time_dialog()
{
	d_reconstruct_to_time_dialog.show();
}


void
GPlatesGui::ViewportWindow::pop_up_specify_fixed_plate_dialog()
{
	d_specify_fixed_plate_dialog.show();
}


void
GPlatesGui::ViewportWindow::pop_up_animate_dialog()
{
	if ( ! d_animate_dialog_has_been_shown) {
		d_animate_dialog.set_start_time_value_to_viewport_time();
		d_animate_dialog.set_current_time_value_to_viewport_time();
		d_animate_dialog_has_been_shown = true;
	}
	d_animate_dialog.show();
}
