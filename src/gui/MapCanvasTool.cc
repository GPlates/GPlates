/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
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

#include "MapCanvasTool.h"

#include "MapTransform.h"

#include "maths/Real.h"
#include "maths/MathsUtils.h"
#include "maths/types.h"

#include "qt-widgets/MapView.h"


namespace
{
	double
	angle_between_vectors(
			const QPointF &v1,
			const QPointF &v2)
	{
		GPlatesMaths::real_t len1 = sqrt(v1.x()*v1.x() + v1.y()*v1.y());
		GPlatesMaths::real_t len2 = sqrt(v2.x()*v2.x() + v2.y()*v2.y());

		if ((len1 == 0) || (len2 == 0)) {
			return 0.0;
		}

		GPlatesMaths::real_t cosangle = (v1.x()*v2.x() + v1.y()*v2.y())/(len1*len2);
		GPlatesMaths::real_t cross = v1.x()*v2.y() - v2.x()*v1.y();

		if (!is_strictly_negative(cross))
		{
			//qDebug() << "Cross greater-than or equal to 0";
			return GPlatesMaths::convert_rad_to_deg(acos(cosangle.dval()));
		}
		else
		{
			//qDebug() << "Cross less than 0";
			return (-1.0 * GPlatesMaths::convert_rad_to_deg(acos(cosangle.dval())));
		}

	}

}


GPlatesGui::MapCanvasTool::~MapCanvasTool()
{  }


void
GPlatesGui::MapCanvasTool::handle_ctrl_left_drag(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation)
{
	map_transform().translate(-translation.x(), -translation.y());
}


void
GPlatesGui::MapCanvasTool::rotate_map_by_drag(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation)
{
	QPointF centre = map_view().mapToScene(map_view().viewport()->rect().center());

	QPointF current_vector = current_point_on_scene - centre;
	QPointF previous_vector = current_vector - translation;

	double angle = angle_between_vectors(previous_vector,current_vector);

	if (GPlatesMaths::is_nan(angle)) {
#if 0
		qDebug() << "isnan";
		qDebug() << "previous: " << previous_vector;
		qDebug() << "current: " << current_vector;
#endif
		return;
	}
	
	// FIXME: Setting setTransformationAnchor(QGraphicsView::AnchorViewCentre) here
	// rotates the map about the centre of the view, but also re-centres the whole 
	// map each time you rotate. 
	// Without setting the transformation anchor here, the map is not re-centred
	// each time, but the rotation is about the centre of the map rather than
	// the centre of the view. 
	// Ideally I want this to rotate about the centre of the view, but without 
	// re-centring the map each time. 
	map_transform().rotate(angle);

#if 0
	qDebug();
	qDebug() << "Centre: " << centre;
	qDebug() << "Translation: " << translation;
	qDebug() << "Current point: " << current_point_on_scene;
	qDebug() << "Current vector: " << current_vector;
	qDebug() << "Previous vector: " << previous_vector;
	qDebug() << "Angle: " << angle;
#endif
}

