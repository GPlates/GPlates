/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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
 
#include <unistd.h>
#include <boost/format.hpp>

#include "maths/LatLonPointConversions.h"

#include "Document.h"
#include "InformationDialog.h"



namespace {

	void
	reconstruct_fake_model(GPlatesModel::Model &model, double time)
	{
		std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> > point_reconstructions;
		std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> > polyline_reconstructions;
		model.create_reconstruction(point_reconstructions, polyline_reconstructions, time, 0);
	}

#if 0
	template<class render_functor_type>
	class render
	{
	private:
		render_functor_type d_render_functor;
		typedef typename render_functor_type::argument_type::element_type geometry_type;
		GPlatesGui::GlobeCanvas *d_canvas_ptr;
		GPlatesGui::Colour d_colour;
	public:
		render(
				GPlatesGui::GlobeCanvas *canvas_ptr,
				render_functor_type render_functor,
				GPlatesGui::Colour colour) :
			d_render_functor(render_functor),
			d_canvas_ptr(canvas_ptr),
			d_colour(colour)
		{  }

		void
		operator()(
				std::vector<GPlatesModel::ReconstructedFeatureGeometry<geometry_type> >::iterator rfg)
		{
			render_functor(rfg->geometry(), d_colour);
		}
	}
#endif
	template<typename In, typename F>
	void
	render(In start, In end, F functor, GPlatesGui::GlobeCanvas *canvas_ptr)
	{
		for (In i = start; i != end; ++i)
			canvas_ptr->*functor(i->geometry(), GPlatesGui::Colour::RED);
	}

	void
	render_model(GPlatesGui::GlobeCanvas *canvas_ptr, GPlatesModel::Model *model_ptr, double time)
	{
		std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> > point_reconstructions;
		std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> > polyline_reconstructions;
		model_ptr->create_reconstruction(point_reconstructions, polyline_reconstructions, time, 501);

		std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> >::iterator iter = point_reconstructions.begin();
		std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> >::iterator finish = point_reconstructions.end();
		while (iter != finish) {
			canvas_ptr->draw_point(*iter->geometry());
			++iter;
		}
		std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> >::iterator iter2 = polyline_reconstructions.begin();
		std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> >::iterator finish2 = polyline_reconstructions.end();
		while (iter2 != finish2) {
			canvas_ptr->draw_polyline(*iter2->geometry());
			++iter2;
		}
		//render(point_reconstructions.begin(), point_reconstructions.end(), &GPlatesGui::GlobeCanvas::draw_point, canvas_ptr);
		//for_each(point_reconstructions.begin(), point_reconstructions.end(), render(canvas_ptr, &GlobeCanvas::draw_point, point_colour))
		// for_each(polyline_reconstructions.begin(), polyline_reconstructions.end(), polyline_point);
	}
	
	void
	draw_sample_points_and_lines(
			GPlatesGui::GlobeCanvas *canvas_ptr)
	{
		using namespace GPlatesMaths;
	
		std::list< LatLonPoint > points;
	
		points.push_back(LatLonPoint(30, 40));
		points.push_back(LatLonPoint(43, 41));
		points.push_back(LatLonPoint(47, 42));
	
		PolylineOnSphere line = 
			LatLonPointConversions::convertLatLonPointListToPolylineOnSphere(points);
	
		canvas_ptr->draw_polyline(line);
	
		canvas_ptr->draw_point(PointOnSphere(UnitVector3D(1, 0, 0)));
		canvas_ptr->draw_point(PointOnSphere(UnitVector3D(0, 1, 0)));
		canvas_ptr->draw_point(PointOnSphere(UnitVector3D(0, 0, 1)));
	
		canvas_ptr->update_canvas();
	}
}

GPlatesGui::Document::Document()
{
	setupUi(this);

	d_timer_ptr = new QTimer(this);
	connect(d_timer_ptr, SIGNAL(timeout()), this, SLOT(animation_step()));

	d_model_ptr = new GPlatesModel::Model();
	//reconstruct_fake_model(model, 0);
	
	d_canvas_ptr = new GlobeCanvas(this);
	QObject::connect(d_canvas_ptr, SIGNAL(items_selected()), this, SLOT(selection_handler()));
	QObject::connect(d_canvas_ptr, SIGNAL(left_mouse_button_clicked()), this, SLOT(mouse_click_handler()));
	
	centralwidget = d_canvas_ptr;
	setCentralWidget(centralwidget);
}

void 
GPlatesGui::Document::selection_handler(
		std::vector< GlobeCanvas::line_header_type > &items)
{
	// Do nothing here for this demo: see the method below.
	// Or: Use InformationDialog here.
}

void
GPlatesGui::Document::mouse_click_handler()
{
	d_time = 100.0;
	if (d_timer_ptr->isActive()) {
		return;
	}
	const int timerInterval = 25;
	d_timer_ptr->start(timerInterval);
}

void
GPlatesGui::Document::animation_step()
{
	// Move the animation on by a fraction
	if (d_time <= 80) {
		d_timer_ptr->stop();
		statusbar->clearMessage();
		return;
	}
	d_time -= 0.1;
	const QString message(boost::str(boost::format("%1% MYA") % d_time).c_str());
	statusbar->showMessage(message);
	d_canvas_ptr->clear_data();
	render_model(d_canvas_ptr, d_model_ptr, d_time);
	d_canvas_ptr->update_canvas();
}
