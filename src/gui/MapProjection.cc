/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The Geological Survey of Norway
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
#include <limits> // epsilon
#include <cmath_ext.h> // fabs, isinf
// #include <float.h>

#include <QDebug>
#include <QString>

#include <boost/none.hpp>

#include "maths/GreatCircle.h"
#include "maths/Real.h"
#include "maths/MathsUtils.h"

#include "MapProjection.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif

namespace 
{
	// This is used for initialising d_boundary_great_circle in the constructor.
	// It should correspond to the initial central llp value. 
	const GPlatesMaths::UnitVector3D INITIAL_BOUNDARY_AXIS(0,1,0);

	const double MIN_SCALE_FACTOR = 1e-8;

	struct ProjectionParameters 
	{
		GPlatesGui::ProjectionType projection_name;
		const char* proj4_name;
		const char* proj4_ellipse;
		double scaling_factor;
		bool inverse_defined;
	};

	static ProjectionParameters projection_table[] = {
		{GPlatesGui::ORTHOGRAPHIC,"","",0.,false},
		{GPlatesGui::RECTANGULAR,"proj=latlong","ellps=WGS84",RAD_TO_DEG,true},
		{GPlatesGui::MERCATOR,"proj=merc","ellps=WGS84",0.0000070,true},
		{GPlatesGui::MOLLWEIDE,"proj=moll","ellps=WGS84",0.0000095,true},
		{GPlatesGui::ROBINSON,"proj=robin","ellps=WGS84",0.0000095,true},
		{GPlatesGui::LAMBERT_CONIC,"proj=lcc","ellps=WGS84",0.000003,true}
	};

}

GPlatesGui::MapProjection::MapProjection():
	d_projection(0),
	d_scale(1.),
	d_projection_type(ORTHOGRAPHIC),
	d_central_llp(0,0),
	d_boundary_great_circle(INITIAL_BOUNDARY_AXIS)
{
	set_projection_type(d_projection_type);
}

GPlatesGui::MapProjection::MapProjection(
	ProjectionType projection_type_):
	d_projection(0),
	d_latlon_projection(0),
	d_scale(1.),
	d_projection_type(ORTHOGRAPHIC),
	d_central_llp(0,0),
	d_boundary_great_circle(INITIAL_BOUNDARY_AXIS)
{
	if ((projection_type_ < MIN_PROJECTION_INDEX) || (projection_type_ > NUM_PROJECTIONS-1))
	{
		// An invalid projection type was set. 
		d_projection_type = ORTHOGRAPHIC;
		return;
	}
	else{
		set_projection_type(projection_type_);
	}
}

GPlatesGui::MapProjection::~MapProjection()
{
	if (d_projection){
		pj_free(d_projection);
	}
}

void
GPlatesGui::MapProjection::set_projection_type(
		ProjectionType projection_type_)
{

	if ((projection_type_ < MIN_PROJECTION_INDEX) || (projection_type_ > NUM_PROJECTIONS-1))
	{
		// An invalid projection type was set. 	
		d_projection_type = ORTHOGRAPHIC;
		return;
	}

	// Set up the central longitude string.
	QString lon_string("lon_0=");
	double lon = d_central_llp.longitude();
	lon_string += QString("%1").arg(lon);

	const int num_projection_args = 3;
	const int num_latlon_args = 3;
	char *projection_args[num_projection_args];
	char *latlon_args[num_latlon_args];

	projection_args[0] = strdup(projection_table[projection_type_].proj4_name);
	projection_args[1] = strdup(projection_table[projection_type_].proj4_ellipse);
	projection_args[2] = strdup(lon_string.toStdString().c_str());

	if (d_projection){
		pj_free(d_projection);
		pj_free(d_latlon_projection);
	}

	if (!(d_projection = pj_init(num_projection_args,projection_args)))
	{
		QString message = QString("Proj4 initialisation failed. ");
		message.append(projection_args[0]);
		throw ProjectionException(GPLATES_EXCEPTION_SOURCE,message.toStdString().c_str());
	}

	// Set up a zero central longitude string.
	lon_string = QString("lon_0=");
	lon = 0.;
	lon_string += QString("%1").arg(lon);


	latlon_args[0] = strdup("proj=latlong");
	latlon_args[1] = strdup(projection_table[projection_type_].proj4_ellipse);
	latlon_args[2] = strdup(lon_string.toStdString().c_str());

	if (!(d_latlon_projection = pj_init(num_latlon_args,latlon_args)))
	{
		QString message = QString("Proj4 initialisation failed. ");
		message.append(latlon_args[0]);
		throw ProjectionException(GPLATES_EXCEPTION_SOURCE,message.toStdString().c_str());
	}


	d_scale = projection_table[projection_type_].scaling_factor;
	if (d_scale < MIN_SCALE_FACTOR) {
		d_scale = MIN_SCALE_FACTOR;
	}

	d_projection_type = projection_type_;

#if 0
	free(args[0]);
	free(args[1]);
	free(args[2]);
#endif
}


void
GPlatesGui::MapProjection::forward_transform(
	 double &longitude,
	 double &latitude) const
{

	if (!d_projection){
		return;
	}

	if (d_projection_type == RECTANGULAR)
	{
		// The rectangular projection is playing silly buggers under non-zero central meridians,
		// so I'm going to handle this case explicitly. 
		longitude -= d_central_llp.longitude();

		if (longitude > 180.)
		{
			longitude -= 360.;
		}
		if (longitude < -180.)
		{
			longitude += 360.;
		}
		return;
	}

	// The mercator projection (and quite possibly some others that we don't use yet)
	// doesn't like +/-90 latitude values. 
	if (latitude <= -90.) latitude = -89.999;
	if (latitude >= 90.) latitude = 89.999;


	// DEG_TO_RAD is defined in the <proj_api.h> header. 
	longitude *= DEG_TO_RAD;
	latitude *= DEG_TO_RAD;

	int result = pj_transform(d_latlon_projection,d_projection,1,0,&longitude,&latitude,NULL);

	if (result != 0)
	{
		throw ProjectionException(GPLATES_EXCEPTION_SOURCE,"Error in pj_transform.");
	}
	if (std::isinf(longitude) || std::isinf(latitude))
	{
		throw ProjectionException(GPLATES_EXCEPTION_SOURCE,"HUGE_VAL returned from pj_transform.");
	}

	longitude *= d_scale;
	latitude *= d_scale;

}


boost::optional<GPlatesMaths::LatLonPoint>
GPlatesGui::MapProjection::inverse_transform(
	double &longitude,
	double &latitude) const
{
	if (!d_projection){
		return boost::none;
	}

	if (d_projection_type == RECTANGULAR)
	{
		// The inverse transform rectangular->rectangular 
		// is playing silly buggers under non-zero central meridians, so
		// I'm handling this projection explicitly.
		longitude += d_central_llp.longitude();

		if (longitude > 180.)
		{
			longitude -= 360.;
		}
		else if (longitude < -180.)
		{
			longitude += 360.;
		}

		if (GPlatesMaths::LatLonPoint::is_valid_latitude(latitude) && 
			(GPlatesMaths::LatLonPoint::is_valid_longitude(longitude)))
		{
			return boost::optional<GPlatesMaths::LatLonPoint>(GPlatesMaths::LatLonPoint(latitude,longitude));		
		}
		else
		{
			return boost::none;
		}
	}

	longitude /= d_scale;
	latitude /= d_scale;

	if  (!projection_table[d_projection_type].inverse_defined)
	{	
		// On Fedora the following message is output when you switch from the map to the globe. This 
		// doesn't happen on XP. The reason this is being called is that if you click on the part of
		// the projection combobox that is on top of the viewport (e.g. on the word "Globe"), a
		// mouse down event is issued for the MapView (even though it's covered up by the list),
		// which in turn calls inverse_projection. There does not seem to be an easy way to stop the
		// MapView from getting the mouse down event unfortunately, so let's suppress the error.
		// qWarning("No inverse is defined for this projection in the proj4 library.");
		return boost::none;
	}

	int result = pj_transform(d_projection,d_latlon_projection,1,0,&longitude,&latitude,NULL);

	if (result != 0)
	{
		return boost::none;
	}
	if (std::isinf(longitude) || std::isinf(latitude))
	{
		return boost::none;
	}

	longitude *= RAD_TO_DEG;
	latitude *= RAD_TO_DEG;
	
	if (GPlatesMaths::LatLonPoint::is_valid_latitude(latitude) && 
		(GPlatesMaths::LatLonPoint::is_valid_longitude(longitude)))
	{
		return boost::optional<GPlatesMaths::LatLonPoint>(GPlatesMaths::LatLonPoint(latitude,longitude));		
	}
	else
	{
		return boost::none;
	}
}


void
GPlatesGui::MapProjection::set_central_llp(
	GPlatesMaths::LatLonPoint &llp)
{
	d_central_llp = llp;

	// We've changed some projection parameters, so reset the projection. 
	set_projection_type(d_projection_type);

	// We need to update the boundary great circle as well.

	// We need 2 points to do this. We can use:
	// 1) The central llp of the map projection, and
	// 2) The "north pole" of the map projection (which will not necessarily coincide with the real north pole). 
	//	  We're not handling oblique projections here, i.e. we're only handling lat-lon offsets to the 
	//	  centre of the map, so anywhere on the central meridian will give us a suitable point. 
	//
	double central_lat = d_central_llp.latitude();
	double central_lon = d_central_llp.longitude();

	GPlatesMaths::PointOnSphere central_pos = GPlatesMaths::make_point_on_sphere(d_central_llp);
	double lat_of_second_llp = 0.;
	if (central_lat <= 0.)
	{
		lat_of_second_llp = central_lat + 90.;
	}
	else
	{
		lat_of_second_llp = central_lat - 90.;
	}

	GPlatesMaths::LatLonPoint second_llp(lat_of_second_llp,central_lon);
	GPlatesMaths::PointOnSphere second_pos = GPlatesMaths::make_point_on_sphere(second_llp);
	d_boundary_great_circle = GPlatesMaths::GreatCircle(central_pos,second_pos);
}

const
GPlatesMaths::LatLonPoint &
GPlatesGui::MapProjection::central_llp() const
{
	return d_central_llp;
}

const
GPlatesMaths::GreatCircle &
GPlatesGui::MapProjection::boundary_great_circle() const
{
	return d_boundary_great_circle;
}

void
GPlatesGui::MapProjection::forward_transform(
	const GPlatesMaths::PointOnSphere &point_on_sphere, 
	double &x_coordinate, 
	double &y_coordinate) const
{

	if (!d_projection){
		return;
	}

	const GPlatesMaths::Real
		&x = point_on_sphere.position_vector().x(),
		&y = point_on_sphere.position_vector().y(),
		&z = point_on_sphere.position_vector().z();

	//std::cerr << "--\nx: " << x << ", y: " << y << ", z: " << z << std::endl;
	y_coordinate = asin(z).dval();
	x_coordinate = atan2(y, x).dval();
	if (x_coordinate < -GPlatesMaths::Pi) {
		x_coordinate = GPlatesMaths::Pi;
	}

	int result = pj_transform(d_latlon_projection,d_projection,1,0,&x_coordinate,&y_coordinate,NULL);
	if (result != 0)
	{
		throw ProjectionException(GPLATES_EXCEPTION_SOURCE,"Error in pj_transform.");
	}
	if (std::isinf(x_coordinate) || std::isinf(y_coordinate))
	{
		throw ProjectionException(GPLATES_EXCEPTION_SOURCE,"HUGE_VAL returned from pj_transform.");
	}

	x_coordinate *= d_scale;
	y_coordinate *= d_scale;

}
