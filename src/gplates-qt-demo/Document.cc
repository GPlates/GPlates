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
 
#include "Document.h"
#include "InformationDialog.h"
#include "model/Model.h"


#if 0
namespace {

	void
	reconstruct_fake_model(GPlatesModel::Model &model, double time)
	{
		std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> > point_reconstructions;
		std::vector<GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> > polyline_reconstructions;
		model.create_reconstruction(point_reconstructions, polyline_reconstructions, time, 0);
	}

	template<class render_functor_type>
	class render
	{
	private:
		render_functor_type render_functor;
		GPlatesGui::GlobeCanvas *d_canvas_ptr;
		GPlatesGui::Colour point_colour;
		GPlatesGui::Colour polyline_colour;
	public:
		render(
				render_functor_type render_functor,
				GPlatesGui::GlobeCanvas *canvas_ptr) :
			d_render_functor(render_functor),
			d_canvas_ptr(canvas_ptr),
			point_colour(GPlatesGui::Colour::RED),
			polyline_colour(GPlatesGui::Colour::GREEN),
		{  }

		void
		operator()(
				std::vector<GPlatesModel::ReconstructedFeatureGeometry<geometry_type> >::iterator *g)
		{
			static render_functor_type render_functor;
			render_functor(g->geometry());
		}
	}

	void
	render_model(GlobeCanvas *canvas_ptr, const GPlatesModel::Model &model)
	{
		for_each(point_reconstructions.begin(), point_reconstructions.end(), render<GPlatesMaths::PointOnSphere, bind2nd(mem_fun(&GlobeCanvas::draw_point)(), point_colour)>(canvas_ptr))
		// for_each(polyline_reconstructions.begin(), polyline_reconstructions.end(), polyline_point);
	}
}
#endif

GPlatesGui::Document::Document()
{
	setupUi(this);

	GPlatesModel::Model model;
	//reconstruct_fake_model(model, 0);
	
	GlobeCanvas *canvas = new GlobeCanvas(this);
	QObject::connect(canvas, SIGNAL(items_selected), this, SLOT(selection_handler()));
	
	centralwidget = canvas;
	setCentralWidget(centralwidget);	
}

void 
GPlatesGui::Document::selection_handler(
		std::vector< GlobeCanvas::line_header_type > &items)
{
	// Use InformationDialog here.
}
