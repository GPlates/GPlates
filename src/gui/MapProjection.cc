/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The Geological Survey of Norway
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

#include <cmath>
#include <iostream>
#include <boost/none.hpp>
#include <QDebug>
#include <QString>

#include "MapProjection.h"

#include "maths/GreatCircle.h"
#include "maths/Real.h"
#include "maths/MathsUtils.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif

namespace 
{
	// This is used for initialising d_boundary_great_circle in the constructor.
	// It should correspond to the initial central llp value. 
	const GPlatesMaths::UnitVector3D INITIAL_BOUNDARY_AXIS(0,1,0);

	const double MIN_SCALE_FACTOR = 1e-8;

	// The Proj library has issues with the Mercator projection at the poles (ie, latitudes -90 and 90).
	// So we clamp latitude slightly inside the poles.
	// And we do this for all projections for consistency.
	//
	// Note: The clamping epsilon also determines the height range of the Mercator map projection.
	//       Eg, changing from 1e-3  to 1e-6 increases the range quite noticeably.
	const double CLAMP_LATITUDE_NEAR_POLES_EPSILON = 1e-6;
	const double MIN_LATITUDE = -90.0 + CLAMP_LATITUDE_NEAR_POLES_EPSILON;
	const double MAX_LATITUDE = 90.0 - CLAMP_LATITUDE_NEAR_POLES_EPSILON;

	struct MapProjectionParameters 
	{
		GPlatesGui::MapProjection::Type projection_name;
		const char *label_name;
		const char* proj_name;
		const char* proj_ellipse;
		double scaling_factor;
		// Whether we need to check for longitude wrapping to range [-180, 180] for inverse transform.
		//
		// For example, with Mercator the proj library inverse transform returns valid longitudes even
		// when the map coordinates are far to the right, or left, of the map itself (and this doesn't happen
		// with Mollweide and Robinson). So we need to explicitly detect and prevent this with Mercator.
		bool detect_inverse_longitude_wrapping;
	};

	static MapProjectionParameters projection_table[] =
	{
		// Don't really need a scale for "Rectangular" since handling ourselves (directly in degrees) and not using proj library...
		{GPlatesGui::MapProjection::RECTANGULAR, "Rectangular", "proj=latlong", "ellps=WGS84", 1.0, true},
		// The remaining projections are handled by the proj library and the scale converts metres to degrees (purely to match "Rectangular")...
		{GPlatesGui::MapProjection::MERCATOR, "Mercator", "proj=merc", "ellps=WGS84", 0.0000070, true},
		{GPlatesGui::MapProjection::MOLLWEIDE, "Mollweide", "proj=moll", "ellps=WGS84", 0.0000095, false},
		{GPlatesGui::MapProjection::ROBINSON, "Robinson", "proj=robin", "ellps=WGS84", 0.0000095, false}
		// This was never used as a map projection, probably because it's not a standard projection, so we'll remove it as a choice...
		//{GPlatesGui::MapProjection::LAMBERT_CONIC, "LambertConic", "proj=lcc", "ellps=WGS84", 0.000003, false}
	};

}


const char *
GPlatesGui::MapProjection::get_display_name(
		Type projection_type)
{
	return projection_table[projection_type].label_name;
}


GPlatesGui::MapProjection::MapProjection():
#if defined(GPLATES_USING_PROJ4)
	d_projection(0),
	d_latlon_projection(0),
#else // using proj5+...
	d_transformation(0),
	d_proj_info(proj_info()),
#endif
	d_scale(1.),
	d_projection_type(RECTANGULAR),
	d_central_meridian(0),
	d_boundary_great_circle(INITIAL_BOUNDARY_AXIS)
{
	set_projection_type(d_projection_type);
}

GPlatesGui::MapProjection::MapProjection(
		MapProjection::Type projection_type_):
#if defined(GPLATES_USING_PROJ4)
	d_projection(0),
	d_latlon_projection(0),
#else // using proj5+...
	d_transformation(0),
	d_proj_info(proj_info()),
#endif
	d_scale(1.),
	d_projection_type(RECTANGULAR),
	d_central_meridian(0),
	d_boundary_great_circle(INITIAL_BOUNDARY_AXIS)
{
	set_projection_type(projection_type_);
}

GPlatesGui::MapProjection::MapProjection(
		const MapProjectionSettings &projection_settings):
#if defined(GPLATES_USING_PROJ4)
	d_projection(0),
	d_latlon_projection(0),
#else // using proj5+...
	d_transformation(0),
	d_proj_info(proj_info()),
#endif
	d_scale(1.),
	d_projection_type(RECTANGULAR),
	d_central_meridian(projection_settings.get_central_meridian()),
	d_boundary_great_circle(INITIAL_BOUNDARY_AXIS)
{
	set_projection_type(projection_settings.get_projection_type());

	// The central llp is not the default so we need to update the boundary great circle.
	update_boundary_great_circle();
}

GPlatesGui::MapProjection::~MapProjection()
{
#if defined(GPLATES_USING_PROJ4)
	if (d_projection)
	{
		pj_free(d_projection);
	}
	if (d_latlon_projection)
	{
		pj_free(d_latlon_projection);
	}
#else // using proj5+...
	if (d_transformation)
	{
		proj_destroy(d_transformation);
	}
#endif
}


GPlatesGui::MapProjectionSettings
GPlatesGui::MapProjection::get_projection_settings() const
{
	return MapProjectionSettings(d_projection_type, d_central_meridian);
}

void
GPlatesGui::MapProjection::set_projection_type(
		MapProjection::Type projection_type_)
{
	// Set up the central longitude string.
	QString lon_string("lon_0=");
	double lon = d_central_meridian;
	lon_string += QString("%1").arg(lon);

	const int num_projection_args = 3;
	const int num_latlon_args = 3;
	char *projection_args[num_projection_args];
	char *latlon_args[num_latlon_args];

	projection_args[0] = strdup(projection_table[projection_type_].proj_name);
	projection_args[1] = strdup(projection_table[projection_type_].proj_ellipse);
	projection_args[2] = strdup(lon_string.toStdString().c_str());

	// Set up a zero central longitude string.
	lon_string = QString("lon_0=");
	lon = 0.;
	lon_string += QString("%1").arg(lon);

	latlon_args[0] = strdup("proj=latlong");
	latlon_args[1] = strdup(projection_table[projection_type_].proj_ellipse);
	latlon_args[2] = strdup(lon_string.toStdString().c_str());

#if defined(GPLATES_USING_PROJ4)

	if (d_projection)
	{
		pj_free(d_projection);
		pj_free(d_latlon_projection);
		d_projection = 0;
		d_latlon_projection = 0;
	}

	if (!(d_projection = pj_init(num_projection_args,projection_args)))
	{
		QString message = QString("Proj4 initialisation failed. ");
		message.append(projection_args[0]);
		throw ProjectionException(GPLATES_EXCEPTION_SOURCE,message.toStdString().c_str());
	}

	if (!(d_latlon_projection = pj_init(num_latlon_args,latlon_args)))
	{
		QString message = QString("Proj4 initialisation failed. ");
		message.append(latlon_args[0]);
		throw ProjectionException(GPLATES_EXCEPTION_SOURCE,message.toStdString().c_str());
	}

#else // using proj5+...

	if (d_transformation)
	{
		proj_destroy(d_transformation);
		d_transformation = 0;
	}

	// For proj5+, concatenate the separate strings and insert '+' in front of each string.
	QString projection_args_string;
	for (unsigned int i = 0; i < num_projection_args; ++i)
	{
		projection_args_string += QString(" +%1").arg(projection_args[i]);
	}
	projection_args_string = projection_args_string.trimmed();
	QString latlon_args_string;
	for (unsigned int i = 0; i < num_latlon_args; ++i)
	{
		latlon_args_string += QString(" +%1").arg(latlon_args[i]);
	}
	latlon_args_string = latlon_args_string.trimmed();

	// Create a single transformation object that converts between the two projections.
	// This is a fundamental difference compared to Proj4.
	if (d_proj_info.major == 5)
	{
		// Transformation between 'latlong' and the selected projection.
		// No need for source 'latlong' CRS since destination CRS accepts geodetic input.
		if (!(d_transformation = proj_create(PJ_DEFAULT_CTX, projection_args_string.toStdString().c_str())))
		{
			QString message = QString("Proj initialisation failed: %1: %2")
				.arg(projection_args[0])
				.arg(proj_errno_string(proj_context_errno(PJ_DEFAULT_CTX)));
			throw ProjectionException(GPLATES_EXCEPTION_SOURCE, message.toStdString().c_str());
		}
	}
	else // proj6+...
	{
		// Transformation between 'latlong' and the selected projection.
		if (!(d_transformation = proj_create_crs_to_crs(PJ_DEFAULT_CTX,
				latlon_args_string.toStdString().c_str(),
				projection_args_string.toStdString().c_str(),
				NULL)))
		{
			QString message = QString("Proj initialisation failed: %1: %2")
				.arg(projection_args[0])
				.arg(proj_errno_string(proj_context_errno(PJ_DEFAULT_CTX)));
			throw ProjectionException(GPLATES_EXCEPTION_SOURCE, message.toStdString().c_str());
		}

#if 0 // For proj6.1+, to get a consistent longitude/latitude ordering, but we don't need this...
		PJ* transformation_for_GIS = proj_normalize_for_visualization(PJ_DEFAULT_CTX, d_transformation);
		if (0 == transformation_for_GIS)
		{
			proj_destroy(d_transformation);

			QString message = QString("Proj initialisation failed: %1: %2")
				.arg(projection_args[0])
				.arg(proj_errno_string(proj_context_errno(PJ_DEFAULT_CTX)));
			throw ProjectionException(GPLATES_EXCEPTION_SOURCE, message.toStdString().c_str());
		}
		proj_destroy(d_transformation);
		d_transformation = transformation_for_GIS;
#endif
	}

#endif

	d_scale = projection_table[projection_type_].scaling_factor;
	if (d_scale < MIN_SCALE_FACTOR)
	{
		d_scale = MIN_SCALE_FACTOR;
	}

	d_projection_type = projection_type_;
}


QPointF
GPlatesGui::MapProjection::forward_transform(
		const GPlatesMaths::LatLonPoint &lat_lon_point) const
{
	double x_coordinate = lat_lon_point.longitude();
	double y_coordinate = lat_lon_point.latitude();
	forward_transform(x_coordinate, y_coordinate);

	return QPointF(x_coordinate, y_coordinate);
}


void
GPlatesGui::MapProjection::forward_transform(
		double &input_longitude_output_x,
		double &input_latitude_output_y) const
{

#if defined(GPLATES_USING_PROJ4)
	if (!d_projection)
	{
		return;
	}
#else // using proj5+...
	if (!d_transformation)
	{
		return;
	}
#endif

	// Input (longitude, latitude).
	double longitude = input_longitude_output_x;
	double latitude = input_latitude_output_y;

	// The Proj library has issues with the Mercator projection at the poles (ie, latitudes -90 and 90).
	// So we clamp latitude slightly inside the poles.
	// And we do this for all projections for consistency.
	if (latitude <= MIN_LATITUDE) latitude = MIN_LATITUDE;
	if (latitude >= MAX_LATITUDE) latitude = MAX_LATITUDE;

	//
	// Handle rectangular projection ourselves (instead of using the proj library).
	//
	// There were a few issues with non-zero central meridians using earlier proj library versions.
	// Also the 'latlong' projection is treated as a special case by proj (having units of degrees instead of metres)
	// and this varies across the proj versions.
	//
	if (d_projection_type == RECTANGULAR)
	{
		// Output (x, y).
		double x = longitude;
		double y = latitude;

		// Handle non-zero central meridians.
		//
		// Note: Also exporting of global grid-line-registered rasters depends on latitude and longitude
		// extents being exactly [-90, 90] and [-180, 180] after subtracting central longitude since the
		// export expands the map projection very slightly (using OpenGL model-view transform) to ensure
		// border pixels get rendered.
		// So if this code path changes then should check that those rasters are exported correctly.
		x -= d_central_meridian;

		if (x > 180.)
		{
			x -= 360.;
		}
		if (x < -180.)
		{
			x += 360.;
		}
		// Output x should now be in the range [-180, 180].
		// And output x corresponding to central meridian input should be zero (to match what proj library does).

		// Return forward transformed (x,y) to the caller.
		input_longitude_output_x = x;
		input_latitude_output_y = y;

		return;
	}

#if defined(GPLATES_USING_PROJ4)

	// Convert degrees to radians.
	// DEG_TO_RAD is defined in the <proj_api.h> header. 
	longitude *= DEG_TO_RAD;
	latitude *= DEG_TO_RAD;

	// Output (x, y).
	double x = longitude;
	double y = latitude;

	// Projection transformation.
	if (0 != pj_transform(d_latlon_projection, d_projection, 1, 0, &x, &y, NULL))
	{
		throw ProjectionException(GPLATES_EXCEPTION_SOURCE, "Error in pj_transform.");
	}

#else // using proj5+...

	if (d_proj_info.major == 5)
	{
		// Convert degrees to radians.
		latitude = proj_torad(latitude);
		longitude = proj_torad(longitude);
	}
	else // proj6+...
	{
		// NOTE: There's no need to convert degrees to radians since Proj6+ recognises "+proj=latlong" as degrees.
	}

	// Projection transformation.
	PJ_COORD c;
	c.lp.lam = longitude;
	c.lp.phi = latitude;
	c = proj_trans(d_transformation, PJ_FWD, c);

	// Output (x, y).
	double x = c.lp.lam;
	double y = c.lp.phi;

	// Debugging...
	//
	//int err = proj_errno(d_transformation);
	//if (err != 0)
	//{
	//	qDebug() << proj_errno_string(err);
	//}

#endif

	if (GPlatesMaths::is_infinity(x) || GPlatesMaths::is_infinity(y))
	{
		throw ProjectionException(GPLATES_EXCEPTION_SOURCE, "HUGE_VAL returned from proj transform.");
	}

	x *= d_scale;
	y *= d_scale;

	// Return forward transformed (x,y) to the caller.
	input_longitude_output_x = x;
	input_latitude_output_y = y;
}


boost::optional<GPlatesMaths::LatLonPoint>
GPlatesGui::MapProjection::inverse_transform(
		const QPointF &map_point) const
{
	double longitude = map_point.x();
	double latitude = map_point.y();
	if (!inverse_transform(longitude, latitude))
	{
		return boost::none;
	}

	return GPlatesMaths::LatLonPoint(latitude, longitude);
}


bool
GPlatesGui::MapProjection::inverse_transform(
		double &input_x_output_longitude,
		double &input_y_output_latitude) const
{
#if defined(GPLATES_USING_PROJ4)
	if (!d_projection)
	{
		return false;
	}
#else // using proj5+...
	if (!d_transformation)
	{
		return false;
	}
#endif

	// Input (x, y).
	double x = input_x_output_longitude;
	double y = input_y_output_latitude;

	//
	// Handle rectangular projection ourselves (instead of using the proj library).
	//
	// There were a few issues with non-zero central meridians using earlier proj library versions.
	// Also the 'latlong' projection is treated as a special case by proj (having units of degrees instead of metres)
	// and this varies across the proj versions.
	//
	if (d_projection_type == RECTANGULAR)
	{
		// Check the input x is within the valid range associated with longitude range [-180, 180].
		// Anything outside that range is outside the rectangular map (left or right of it).
		//
		// Note that input x corresponding to central meridian is zero.
		//
		// Note that this does the check required by 'MapProjectionParameters::detect_inverse_longitude_wrapping'.
		if (!GPlatesMaths::is_in_range(x, -180.0, 180.0))
		{
			return false;
		}

		// Check the input y is within the valid range associated with latitude range [-90, 90].
		// Anything outside that range is outside the rectangular map (above or below it).
		if (!GPlatesMaths::is_in_range(y, -90.0, 90.0))
		{
			return false;
		}

		// Output (longitude, latitude).
		double longitude = x;
		double latitude = y;

		// Handle non-zero central meridians (x=0 in map projection space should map to longitude=central_meridian).
		longitude += d_central_meridian;

		// This shouldn't really be necessary but we'll keep longitude within a reasonable range
		// in case central meridian is large.
		if (longitude > 180.)
		{
			longitude -= 360.;
		}
		else if (longitude < -180.)
		{
			longitude += 360.;
		}

		// Return inverse transformed (longitude,latitude) to the caller.
		input_x_output_longitude = longitude;
		input_y_output_latitude = latitude;

		return true;
	}

	x /= d_scale;
	y /= d_scale;

#if defined(GPLATES_USING_PROJ4)

	// Output (longitude, latitude).
	double longitude = x;
	double latitude = y;

	// Projection inverse transformation.
	if (0 != pj_transform(d_projection,d_latlon_projection,1,0,&longitude,&latitude,NULL))
	{
		return false;
	}

	// Convert radians to degrees.
	// RAD_TO_DEG is defined in the <proj_api.h> header. 
	longitude *= RAD_TO_DEG;
	latitude *= RAD_TO_DEG;

#else // using proj5+...

	// Projection inverse transformation.
	PJ_COORD c;
	c.lp.lam = x;
	c.lp.phi = y;
	c = proj_trans(d_transformation, PJ_INV, c);

	// Output (longitude, latitude).
	double longitude = c.lp.lam;
	double latitude = c.lp.phi;

	if (d_proj_info.major == 5)
	{
		// Output is in radians - convert to degrees.
		longitude = proj_todeg(longitude);
		latitude = proj_todeg(latitude);
	}
	else // proj6+...
	{
		// NOTE: There's no need to convert radians to degrees since Proj6+ recognises "+proj=latlong" as degrees.
	}

	// Debugging...
	//
	//int err = proj_errno(d_transformation);
	//if (err != 0)
	//{
	//	qDebug() << proj_errno_string(err);
	//}

#endif

	if (GPlatesMaths::is_infinity(longitude) || GPlatesMaths::is_infinity(latitude))
	{
		return false;
	}
	
	if (!GPlatesMaths::LatLonPoint::is_valid_latitude(latitude) ||
		!GPlatesMaths::LatLonPoint::is_valid_longitude(longitude))
	{
		return false;
	}

	// See if we need to check if the inverse transform wrapped longitude to range [-180, 180].
	if (projection_table[d_projection_type].detect_inverse_longitude_wrapping)
	{
		// For example, with Mercator the proj library inverse transform returns valid longitudes even
		// when the map coordinates are far to the right, or left, of the map itself (and this doesn't happen
		// with Mollweide and Robinson). So we need to explicitly detect and prevent this with Mercator.
		//
		// To check this we transform the inverted longitude back into an x-coordinate and compare
		// with the original x-coordinate. If they don't match then we can assume we're off the map.

		const double original_x = input_x_output_longitude;
		const double original_y = input_y_output_latitude;

		// Forward transform the inverted longitude and latitude.
		double inverted_and_transformed_x = longitude;
		double inverted_and_transformed_y = latitude;
		forward_transform(inverted_and_transformed_x, inverted_and_transformed_y);

		// If we don't end up at the same transformed x-coordinate then we're off the map. 
		if (std::fabs(inverted_and_transformed_x - original_x) > 1e-6 ||
			std::fabs(inverted_and_transformed_y - original_y) > 1e-6)
		{
			return false;
		}
	}

	// Return inverse transformed (longitude,latitude) to the caller.
	input_x_output_longitude = longitude;
	input_y_output_latitude = latitude;

	return true;
}


void
GPlatesGui::MapProjection::set_central_meridian(
		const double &central_meridian_)
{
	d_central_meridian = central_meridian_;

	// We've changed some projection parameters, so reset the projection. 
	set_projection_type(d_projection_type);

	// We need to update the boundary great circle as well.
	update_boundary_great_circle();
}

void
GPlatesGui::MapProjection::update_boundary_great_circle()
{
	// We need 2 points to do this. We can use:
	// 1) The central llp of the map projection, and
	// 2) The "north pole" of the map projection (which will not necessarily coincide with the real north pole). 
	//	  We're not handling oblique projections here, i.e. we're only handling lat-lon offsets to the 
	//	  centre of the map, so anywhere on the central meridian will give us a suitable point. 
	//
	GPlatesMaths::PointOnSphere central_pos = GPlatesMaths::make_point_on_sphere(
			GPlatesMaths::LatLonPoint(0.0, d_central_meridian));
	GPlatesMaths::PointOnSphere second_pos = GPlatesMaths::make_point_on_sphere(
			GPlatesMaths::LatLonPoint(90.0, d_central_meridian));
	d_boundary_great_circle = GPlatesMaths::GreatCircle(central_pos, second_pos);
}


GPlatesGui::MapProjectionSettings::MapProjectionSettings(
		MapProjection::Type projection_type_,
		const double &central_meridian_) :
	d_projection_type(projection_type_),
	d_central_meridian(central_meridian_)
{
}
