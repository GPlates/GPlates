/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#include "GlobeCanvas.h"

#include <vector>
#include <cmath>

#include <QLocale>
#include <QtGui/QMouseEvent>

#include "ViewportWindow.h"  // Remove this when there is a ViewState class.
#include "QueryFeaturePropertiesDialog.h"
#include "feature-visitors/QueryFeaturePropertiesDialogPopulator.h"
#include "feature-visitors/PlateIdFinder.h"
#include "gui/ProximityTests.h"

#include "maths/types.h"
#include "maths/UnitVector3D.h"
#include "maths/LatLonPointConversions.h"

#include "global/Exception.h"
#include "state/Layout.h"
#include "model/FeatureHandle.h"
#include "utils/UnicodeStringUtils.h"


/**
 * At the initial zoom, the smaller dimension of the GlobeCanvas will be @a FRAMING_RATIO times the
 * diameter of the Globe.  Obviously, when the GlobeCanvas is resized, the Globe will be scaled
 * accordingly.
 *
 * The value of this constant is purely cosmetic.
 */
static const GLfloat FRAMING_RATIO = 1.07;

static const GLfloat EYE_X = 0.0, EYE_Y = 0.0, EYE_Z = -5.0;


namespace 
{
	/**
	 * Calculate the globe-position discriminant for the universe coordinates @a y and @a z.
	 */
	inline
	double
	calc_globe_pos_discrim(
			const double &y,
			const double &z) 
	{
		return (y * y + z * z);
	}
	

	/**
	 * Return whether the globe-position discriminant indicates that a position is on the
	 * globe.
	 */
	inline
	bool
	is_on_globe(
			const double &discrim) 
	{
		return (discrim < 1.0);
	}


	/**
	 * Given universe coordinates @a y and @a z and discriminant @a discrim, calculate the
	 * corresponding position on the globe (@a x, @a y, @a z).
	 *
	 * This assumes that (@a discrim >= 0 && @a discrim <= 1) and (@a y * @a y + @a z * @a z +
	 * @a discrim == 1).
	 */
	const GPlatesMaths::PointOnSphere
	on_globe(
			const double &y,
			const double &z,
			const double &discrim) 
	{
		// Be wary of floating-point error, which could result in calculating the sqrt of a
		// (very slightly) negative value.  (Yes, this is something I actually observed in
		// this code.)
		double one_minus_discrim = 1.0 - discrim;
		if (one_minus_discrim < 0.0) {
			one_minus_discrim = 0.0;
		}
		double x = sqrt(one_minus_discrim);

		return GPlatesMaths::PointOnSphere(
				GPlatesMaths::UnitVector3D(x, y, z));
	}


	/**
	 * Given universe coordinates @a y and @a z and a discriminant @a discrim which together
	 * indicate that a position is @em not on the globe, calculate the closest position which
	 * @em is on the globe.
	 *
	 * This assumes that (@a discrim >= 1).
	 */
	const GPlatesMaths::PointOnSphere
	at_intersection_with_globe(
			const double &y,
			const double &z,
			const double &discrim) 
	{
		double norm_reciprocal = 1.0 / sqrt(discrim);
		return GPlatesMaths::PointOnSphere(
				GPlatesMaths::UnitVector3D(
					0.0, y * norm_reciprocal, z * norm_reciprocal));
	}


	/**
	 * Given universe coordinates @a y and @a z, calculate and return a position which is on
	 * the globe (a unit sphere).
	 *
	 * This position might be the position determined by @a y and @a z, or the closest position
	 * on the globe to the position determined by @a y and @a z.
	 */
	const GPlatesMaths::PointOnSphere
	virtual_globe_position(
			const double &y,
			const double &z) 
	{
		double discrim = calc_globe_pos_discrim(y, z);
		
		if (is_on_globe(discrim)) {
			// Universe coords y and z do in fact determine a position on the globe.
			return on_globe(y, z, discrim);
		}

		// Universe coords y and z do not determine a position on the globe.  Find the
		// closest point which *is* on the globe.
		return at_intersection_with_globe(y, z, discrim);
	}
	
}


GPlatesQtWidgets::GlobeCanvas::GlobeCanvas(
		ViewportWindow &view_state,
		QWidget *parent_):
	QGLWidget(parent_),
	d_view_state_ptr(&view_state),
	d_query_feature_properties_dialog_ptr(new QueryFeaturePropertiesDialog(this->parentWidget()))
{
	handle_zoom_change();
}


void
GPlatesQtWidgets::GlobeCanvas::draw_polyline(
		const GPlatesMaths::PolylineOnSphere &polyline)
{
	GPlatesState::Layout::InsertLineDataPos(polyline);
}


void
GPlatesQtWidgets::GlobeCanvas::draw_point(
		const GPlatesMaths::PointOnSphere &point)
{
	GPlatesState::Layout::InsertPointDataPos(point);
}


void
GPlatesQtWidgets::GlobeCanvas::update_canvas()
{
	updateGL();
}


void
GPlatesQtWidgets::GlobeCanvas::clear_data()
{
	GPlatesState::Layout::Clear();
}


void
GPlatesQtWidgets::GlobeCanvas::zoom_in() 
{
	unsigned zoom_percent = d_viewport_zoom.zoom_percent();

	d_viewport_zoom.zoom_in();
	
	if (zoom_percent != d_viewport_zoom.zoom_percent()) {
		handle_zoom_change();
	}
}


void
GPlatesQtWidgets::GlobeCanvas::zoom_out() 
{
	unsigned zoom_percent = d_viewport_zoom.zoom_percent();

	d_viewport_zoom.zoom_out();
	
	if (zoom_percent != d_viewport_zoom.zoom_percent()) {
		handle_zoom_change();
	}
}


void
GPlatesQtWidgets::GlobeCanvas::zoom_reset() 
{
	d_viewport_zoom.reset_zoom();
	handle_zoom_change();
}


void 
GPlatesQtWidgets::GlobeCanvas::initializeGL() 
{
	glEnable(GL_DEPTH_TEST);
	
	// FIXME: Enable polygon offset here or in Globe?
	
	clear_canvas();
}


void 
GPlatesQtWidgets::GlobeCanvas::resizeGL(
		int new_width,
		int new_height) 
{
	try {
		set_view();
	} catch (const GPlatesGlobal::Exception &e) {
		// FIXME: Use new exception system which doesn't involve strings.
	}
}


void 
GPlatesQtWidgets::GlobeCanvas::paintGL() 
{
	try {
		clear_canvas();
		
		glLoadIdentity();
		glTranslatef(EYE_X, EYE_Y, EYE_Z);

		// Set up our universe coordinate system (the standard geometric one):
		//   Z points up
		//   Y points right
		//   X points out of the screen
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		glRotatef(-90.0, 0.0, 0.0, 1.0);

		// FIXME: Globe uses wrong naming convention for methods.
		d_globe.Paint();

	} catch (const GPlatesGlobal::Exception &e) {
		// FIXME: Use new exception system which doesn't involve strings.
	}
}


void
GPlatesQtWidgets::GlobeCanvas::mousePressEvent(
		QMouseEvent *press_event) 
{
	d_mouse_x = press_event->x();
	d_mouse_y = press_event->y();
	
	switch (press_event->button()) {
	case Qt::LeftButton:
		handle_left_mouse_down();
		break;
		
	case Qt::RightButton:
		handle_right_mouse_down();
		break;
		
	default:
		break;
	}
}


void
GPlatesQtWidgets::GlobeCanvas::mouseMoveEvent(
		QMouseEvent *move_event) 
{
	d_mouse_x = move_event->x();
	d_mouse_y = move_event->y();
	
	if (move_event->buttons() & Qt::RightButton) {
		handle_right_mouse_drag();
	}
}


void 
GPlatesQtWidgets::GlobeCanvas::mouseReleaseEvent(
		QMouseEvent *release_event)
{
	if (release_event->button() == Qt::LeftButton) {
		emit left_mouse_button_clicked();
	}
}


void GPlatesQtWidgets::GlobeCanvas::wheelEvent(
		QWheelEvent *wheel_event) 
{
	handle_wheel_rotation(wheel_event->delta());
}


void
GPlatesQtWidgets::GlobeCanvas::handle_zoom_change() 
{
	emit current_zoom_changed(d_viewport_zoom.zoom_percent());

	set_view();
	update();
	
	handle_mouse_motion();
}

		
void
GPlatesQtWidgets::GlobeCanvas::set_view() 
{
	static const GLdouble depth_near_clipping = 0.5;

	// Always fill up all of the available space.
	get_dimensions();
	glViewport(0, 0, static_cast<GLsizei>(d_width), static_cast<GLsizei>(d_height));
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// This is used for the coordinates of the symmetrical clipping planes which bound the
	// smaller dimension.
	GLdouble smaller_dim_clipping =
			FRAMING_RATIO / d_viewport_zoom.zoom_factor();

	// This is used for the coordinates of the symmetrical clipping planes which bound the
	// larger dimension.
	GLdouble dim_ratio = d_larger_dim / d_smaller_dim;
	GLdouble larger_dim_clipping = smaller_dim_clipping * dim_ratio;

	// This is used for the coordinate of the further clipping plane in the depth dimension.
	GLdouble depth_far_clipping = fabsf(EYE_Z);

	if (d_width <= d_height) {
		glOrtho(-smaller_dim_clipping, smaller_dim_clipping,
			-larger_dim_clipping, larger_dim_clipping, depth_near_clipping, 
			depth_far_clipping);

	} else {
		glOrtho(-larger_dim_clipping, larger_dim_clipping,
			-smaller_dim_clipping, smaller_dim_clipping, depth_near_clipping, 
			depth_far_clipping);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


void
GPlatesQtWidgets::GlobeCanvas::get_dimensions() 
{
	d_width = width();
	d_height = height();

	if (d_width <= d_height) {
		d_smaller_dim = static_cast< GLdouble >(d_width);
		d_larger_dim = static_cast< GLdouble >(d_height);
	} else {
		d_smaller_dim = static_cast< GLdouble >(d_height);
		d_larger_dim = static_cast< GLdouble >(d_width);
	}
}


void
GPlatesQtWidgets::GlobeCanvas::handle_mouse_motion() 
{
	using namespace GPlatesMaths;

	double y_pos = get_universe_coord_y(d_mouse_x);
	double z_pos = get_universe_coord_z(d_mouse_y);

	double discrim = calc_globe_pos_discrim(y_pos, z_pos);
	
	if (is_on_globe(discrim)) {
		PointOnSphere p = on_globe(y_pos, z_pos, discrim);

		// FIXME: Globe uses wrong naming convention for methods.
		PointOnSphere rotated_p = d_globe.Orient(p);

		// FIXME: LatLonPoint uses wrong naming convention for functions.
		LatLonPoint llp =
			LatLonPointConversions::convertPointOnSphereToLatLonPoint(rotated_p);

		emit current_global_pos_changed(llp.latitude().dval(),
			llp.longitude().dval());

	} else {
		emit current_global_pos_off_globe();
	}
}


void
GPlatesQtWidgets::GlobeCanvas::handle_right_mouse_down() 
{
	using namespace GPlatesMaths;

	double y_pos = get_universe_coord_y(d_mouse_x);
	double z_pos = get_universe_coord_z(d_mouse_y);

	PointOnSphere p = virtual_globe_position(y_pos, z_pos);
	
	// FIXME: Globe uses wrong naming convention for methods.
	d_globe.SetNewHandlePos(p);
}


void
GPlatesQtWidgets::GlobeCanvas::handle_left_mouse_down() 
{
	using namespace GPlatesGui;

	double y_pos = get_universe_coord_y(d_mouse_x);
	double z_pos = get_universe_coord_z(d_mouse_y);
	GPlatesMaths::PointOnSphere click_pos = virtual_globe_position(y_pos, z_pos);

	// Compensate for the rotated globe.
	// FIXME: Globe uses wrong naming convention for methods.
	GPlatesMaths::PointOnSphere rotated_click_pos = d_globe.Orient(click_pos);

	// The larger the value of this constant, the more relaxed the proximity inclusion
	// threshold.  Read the comment preceding 'diameter_ratio' for the purpose of this value.
	static const GLdouble epsilon_diameter = 6.0;

	// Say we pick an epsilon radius of 3 pixels around the click position.  That's a diameter
	// of 6 pixels (the value of the constant 'epsilon_diameter' above).
	//
	// The value of 'd_smaller_dim' is the value of whichever of the width or height of the
	// canvas is smaller; the smaller dimension of the canvas will play a role in determining
	// the size of the globe.
	//
	// The value of 'zoom_factor' starts at 1 for no zoom, then increases to 1.12202, 1.25893,
	// etc.  The product (d_smaller_dim * zoom_factor) gives the current size of the globe in
	// (floating-point) pixels, taking into account canvas size and zoom.
	//
	// So, (epsilon_diameter / (d_smaller_dim * zoom_factor)) is the ratio of the diameter of
	// the epsilon circle to the diameter of the globe.  We want to convert this to an angle,
	// so we should pipe this value through an inverse-sine function, to convert from the
	// on-screen projection size of the epsilon circle to the angle at the centre of the globe
	// (but for arguments of such small magnitude (less than 0.05), the value of asin(x) is
	// practically equal to the value of 'x' anyway.  (No really -- try it!)
	//
	// Then take the cosine, and we have the dot-product-related closeness inclusion threshold.

	// A new addition, 2007-10-16:  If you look at a cross-section of the globe, sliced
	// vertically, you'll notice that we only see the high latitutes (whether positive or
	// negative latitudes) tangentially.  This means that a small mouse-position displacement
	// on-screen results in a significantly larger mouse-position displacement on the globe.
	// Thus, at a high latitude, displacing the mouse-position by a single pixel on-screen will
	// have a more significant displacement on the globe position than it would at the Equator.
	//
	// Since the mouse-position on-screen will vary by whole pixels, at the high latitudes it
	// might not even be possible to reach a mouse-position on the globe which is close enough
	// to a geometry to click on it -- moving by a single pixel on-screen might cause the
	// mouse-position on the globe to "skip over" the necessary position on the globe.
	//
	// For this reason, let's increase the epsilon diameter proportional to (1 - cos of the
	// "latitude" (really the angular distance from the centre of the projection of the globe
	// in its current orientation)), so that it is still possible to click on geometries at the
	// edge of the globe.
	//
	// The value of 3 below is pretty much arbitrary, however.  I just tried different values
	// till things behaved as I wanted in the GUI.

	// Since our globe is a unit sphere, the x-coordinate of the virtual click point is equal
	// to the cos of the "latitude" (angular distance from the centre).
	double cos_lat = click_pos.position_vector().x().dval();
	double lat_scaled_epsilon_diameter = epsilon_diameter + 3 * epsilon_diameter * (1 - cos_lat);
	GLdouble diameter_ratio =
			lat_scaled_epsilon_diameter / (d_smaller_dim * d_viewport_zoom.zoom_factor());
	double proximity_inclusion_threshold = cos(static_cast<double>(diameter_ratio));

#if 0
	std::cout << "\nEpsilon diameter: " << epsilon_diameter << std::endl;
	std::cout << "cos(lat): " << cos_lat << std::endl;
	std::cout << "Latitude-scaled epsilon diameter: " << lat_scaled_epsilon_diameter << std::endl;
	std::cout << "Smaller canvas dim: " << d_smaller_dim << std::endl;
	std::cout << "Zoom factor: " << d_viewport_zoom.zoom_factor() << std::endl;
	std::cout << "Globe size: " << (d_smaller_dim * d_viewport_zoom.zoom_factor()) << std::endl;
	std::cout << "Diameter ratio: " << diameter_ratio << std::endl;
	std::cout << "asin(diameter ratio): " << asin(diameter_ratio) << std::endl;
	std::cout << "Proximity inclusion threshold: " << proximity_inclusion_threshold << std::endl;
	std::cout << "Proximity inclusion threshold from asin(diameter ratio): "
			<< cos(asin(diameter_ratio)) << std::endl;
#endif

	if ( ! d_reconstruction_ptr) {
		emit no_items_selected_by_click();
		return;
	}
	std::priority_queue<ProximityTests::ProximityHit> sorted_hits;
	ProximityTests::find_close_rfgs(sorted_hits, *d_reconstruction_ptr, rotated_click_pos,
			proximity_inclusion_threshold);
	if (sorted_hits.size() == 0) {
		emit no_items_selected_by_click();
		return;
	}
	GPlatesModel::FeatureHandle::weak_ref feature_ref(sorted_hits.top().d_feature->reference());
	if ( ! feature_ref.is_valid()) {
		// FIXME:  How did this happen?  Throw an exception!
		return;  // FIXME:  Should throw an exception instead.
	}

	d_query_feature_properties_dialog_ptr->set_feature_type(
			GPlatesUtils::make_qstring(feature_ref->feature_type()));

	// These next few fields only make sense if the feature is reconstructable, ie. if it has a
	// reconstruction plate ID.
	GPlatesModel::PropertyName plate_id_property_name(UnicodeString("gpml:reconstructionPlateId"));
	GPlatesFeatureVisitors::PlateIdFinder plate_id_finder(plate_id_property_name);
	plate_id_finder.visit_feature_handle(*feature_ref);
	if (plate_id_finder.found_plate_ids_begin() != plate_id_finder.found_plate_ids_end()) {
		// The feature has a reconstruction plate ID.
		GPlatesModel::integer_plate_id_type recon_plate_id =
				*plate_id_finder.found_plate_ids_begin();
		d_query_feature_properties_dialog_ptr->set_plate_id(recon_plate_id);

		GPlatesModel::integer_plate_id_type root_plate_id =
				d_view_state_ptr->reconstruction_root();
		d_query_feature_properties_dialog_ptr->set_root_plate_id(root_plate_id);
		d_query_feature_properties_dialog_ptr->set_reconstruction_time(
				d_view_state_ptr->reconstruction_time());

		// Now let's use the reconstruction plate ID of the feature to find the appropriate 
		// absolute rotation in the reconstruction tree.
		GPlatesModel::ReconstructionTree &recon_tree =
				d_reconstruction_ptr->reconstruction_tree();
		std::pair<GPlatesMaths::FiniteRotation,
				GPlatesModel::ReconstructionTree::ReconstructionCircumstance>
				absolute_rotation =
						recon_tree.get_composed_absolute_rotation(recon_plate_id);

		// FIXME:  Do we care about the reconstruction circumstance?
		// (For example, there may have been no match for the reconstruction plate ID.)
		GPlatesMaths::UnitQuaternion3D uq = absolute_rotation.first.unit_quat();
		if (GPlatesMaths::represents_identity_rotation(uq)) {
			d_query_feature_properties_dialog_ptr->set_euler_pole(QObject::tr("indeterminate"));
			d_query_feature_properties_dialog_ptr->set_angle(0.0);
		} else {
			using namespace GPlatesMaths;
			using LatLonPointConversions::convertPointOnSphereToLatLonPoint;

			UnitQuaternion3D::RotationParams params = uq.get_rotation_params();

			PointOnSphere euler_pole(params.axis);
			LatLonPoint llp = convertPointOnSphereToLatLonPoint(euler_pole);

			// Use the default locale for the floating-point-to-string conversion.
			// (We need the underscore at the end of the variable name, because
			// apparently there is already a member of GlobeCanvas named 'locale'.)
			QLocale locale_;
			QString euler_pole_lat = locale_.toString(llp.latitude().dval());
			QString euler_pole_lon = locale_.toString(llp.longitude().dval());
			QString euler_pole_as_string;
			euler_pole_as_string.append(euler_pole_lat);
			euler_pole_as_string.append(QObject::tr(" ; "));
			euler_pole_as_string.append(euler_pole_lon);

			d_query_feature_properties_dialog_ptr->set_euler_pole(euler_pole_as_string);

			const double &angle = radiansToDegrees(params.angle).dval();
			d_query_feature_properties_dialog_ptr->set_angle(angle);
		}
	}

	GPlatesFeatureVisitors::QueryFeaturePropertiesDialogPopulator populator(
			d_query_feature_properties_dialog_ptr->property_tree());
	populator.visit_feature_handle(*feature_ref);

	d_query_feature_properties_dialog_ptr->show();

	// FIXME:  We should re-enable this.
	// emit items_selected(items);
}


void
GPlatesQtWidgets::GlobeCanvas::handle_right_mouse_drag() 
{
	using namespace GPlatesMaths;

	double y_pos = get_universe_coord_y(d_mouse_x);
	double z_pos = get_universe_coord_z(d_mouse_y);

	PointOnSphere p = virtual_globe_position(y_pos, z_pos);
	
	// FIXME: Globe uses wrong naming convention for methods.
	d_globe.UpdateHandlePos(p);
	
	updateGL();
}


void
GPlatesQtWidgets::GlobeCanvas::handle_wheel_rotation(
		int delta) 
{
	const int num_degrees = delta / 8;
	
	int num_steps = num_degrees / 15;

	if (num_steps > 0) {
		while (num_steps--) {
			zoom_in();
		}
	} else {
		while (num_steps++) {
			zoom_out();
		}
	}
}


double
GPlatesQtWidgets::GlobeCanvas::get_universe_coord_y(
		int screen_x) 
{
	// Scale the screen to a "unit square".
	double y_pos = (2.0 * screen_x - d_width) / d_smaller_dim;

	return (y_pos * FRAMING_RATIO / d_viewport_zoom.zoom_factor());
}


double
GPlatesQtWidgets::GlobeCanvas::get_universe_coord_z(
		int screen_y) 
{
	// Scale the screen to a "unit square".
	double z_pos = (d_height - 2.0 * screen_y) / d_smaller_dim;
	
	return (z_pos * FRAMING_RATIO / d_viewport_zoom.zoom_factor());
}


void
GPlatesQtWidgets::GlobeCanvas::clear_canvas(
		const QColor& c) 
{
	// Set the colour buffer's clearing colour.
	glClearColor(c.red(), c.green(), c.blue(), c.alpha());

	// Clear the window to the current clearing colour.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
