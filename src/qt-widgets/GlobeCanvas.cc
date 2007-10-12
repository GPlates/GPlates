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

static const GLfloat FRAMING_RATIO = 1.07;

static const GLfloat EYE_X = 0.0, EYE_Y = 0.0, EYE_Z = -5.0;

namespace 
{
	static
	GPlatesMaths::real_t
	calc_globe_pos_discrim(
			const GPlatesMaths::real_t &y,
			const GPlatesMaths::real_t &z) 
	{
		return (y * y + z * z);
	}
	
	static
	bool
	is_on_globe(
			const GPlatesMaths::real_t &discrim) 
	{
		return (discrim <= 1.0);
	}

	static
	const GPlatesMaths::PointOnSphere
	on_globe(
			const GPlatesMaths::real_t &y,
			const GPlatesMaths::real_t &z,
			const GPlatesMaths::real_t &discrim) 
	{
		using namespace GPlatesMaths;

		real_t x = sqrt(1.0 - discrim);

		return PointOnSphere(UnitVector3D(x, y, z));
	}

	static
	const GPlatesMaths::PointOnSphere
	at_intersection_with_globe(
			const GPlatesMaths::real_t &y,
			const GPlatesMaths::real_t &z,
			const GPlatesMaths::real_t &discrim) 
	{
		using namespace GPlatesMaths;

		real_t norm_reciprocal = 1.0 / sqrt(discrim);

		return PointOnSphere(
			UnitVector3D(0.0, y * norm_reciprocal, z * norm_reciprocal));
	}

	static
	const GPlatesMaths::PointOnSphere
	virtual_globe_position(
			const GPlatesMaths::real_t &y,
			const GPlatesMaths::real_t &z) 
	{
		GPlatesMaths::real_t discrim = calc_globe_pos_discrim(y, z);
		
		if (is_on_globe(discrim)) {
			return on_globe(y, z, discrim);
		}
		
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

	get_dimensions();
	glViewport(0, 0, static_cast<GLsizei>(d_width), static_cast<GLsizei>(d_height));
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	GLdouble smaller_dim_clipping =
		FRAMING_RATIO / d_viewport_zoom.zoom_factor();

	GLdouble dim_ratio = d_larger_dim / d_smaller_dim;
	GLdouble larger_dim_clipping = smaller_dim_clipping * dim_ratio;

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

	real_t y_pos = get_universe_coord_y(d_mouse_x);
	real_t z_pos = get_universe_coord_z(d_mouse_y);

	real_t discrim = calc_globe_pos_discrim(y_pos, z_pos);
	
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

	real_t y_pos = get_universe_coord_y(d_mouse_x);
	real_t z_pos = get_universe_coord_z(d_mouse_y);

	PointOnSphere p = virtual_globe_position(y_pos, z_pos);
	
	// FIXME: Globe uses wrong naming convention for methods.
	d_globe.SetNewHandlePos(p);
}
		
void
GPlatesQtWidgets::GlobeCanvas::handle_left_mouse_down() 
{
	using namespace GPlatesMaths;
	using namespace GPlatesGui;

	real_t y_pos = get_universe_coord_y(d_mouse_x);
	real_t z_pos = get_universe_coord_z(d_mouse_y);

	PointOnSphere p = virtual_globe_position(y_pos, z_pos);

	// FIXME: Globe uses wrong naming convention for methods.
	PointOnSphere rotated_p = d_globe.Orient(p);

#if 0
	std::cout << "If 2.0, proximity inclusion threshold is " << cos(static_cast<double>(2.0 / (d_smaller_dim * d_viewport_zoom.zoom_factor()))) << std::endl;
	std::cout << "If 3.0, proximity inclusion threshold is " << cos(static_cast<double>(3.0 / (d_smaller_dim * d_viewport_zoom.zoom_factor()))) << std::endl;
	std::cout << "If 4.0, proximity inclusion threshold is " << cos(static_cast<double>(4.0 / (d_smaller_dim * d_viewport_zoom.zoom_factor()))) << std::endl;
	std::cout << "If 5.0, proximity inclusion threshold is " << cos(static_cast<double>(5.0 / (d_smaller_dim * d_viewport_zoom.zoom_factor()))) << std::endl;
	std::cout << "If 10.0, proximity inclusion threshold is " << cos(static_cast<double>(10.0 / (d_smaller_dim * d_viewport_zoom.zoom_factor()))) << std::endl;
	std::cout << "If 20.0, proximity inclusion threshold is " << cos(static_cast<double>(20.0 / (d_smaller_dim * d_viewport_zoom.zoom_factor()))) << std::endl;
	std::cout << "If 40.0, proximity inclusion threshold is " << cos(static_cast<double>(40.0 / (d_smaller_dim * d_viewport_zoom.zoom_factor()))) << std::endl;
#endif

	// Unfortunately, I have no idea how this diameter ratio works anymore...
	// All I have been able to work out is that, the larger the value (currently 10.0), the
	// more relaxed the proximity inclusion threshold.
	GLdouble diameter_ratio = 10.0 / (d_smaller_dim * d_viewport_zoom.zoom_factor());
	double proximity_inclusion_threshold = cos(static_cast<double>(diameter_ratio));
	std::priority_queue<ProximityTests::ProximityHit> sorted_hits;

	if ( ! d_reconstruction_ptr) {
		emit no_items_selected_by_click();
		return;
	}
	ProximityTests::find_close_rfgs(sorted_hits, *d_reconstruction_ptr, rotated_p, proximity_inclusion_threshold);
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
		GPlatesMaths::FiniteRotation absolute_rotation =
				recon_tree.get_composed_absolute_rotation(recon_plate_id);

		GPlatesMaths::UnitQuaternion3D uq = absolute_rotation.unit_quat();
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

	real_t y_pos = get_universe_coord_y(d_mouse_x);
	real_t z_pos = get_universe_coord_z(d_mouse_y);

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
				
GPlatesMaths::real_t
GPlatesQtWidgets::GlobeCanvas::get_universe_coord_y(
		int screen_x) 
{
	GPlatesMaths::real_t y_pos = (2.0 * screen_x - d_width) / d_smaller_dim;

	return (y_pos * FRAMING_RATIO / d_viewport_zoom.zoom_factor());
}
				
GPlatesMaths::real_t
GPlatesQtWidgets::GlobeCanvas::get_universe_coord_z(
		int screen_y) 
{
	GPlatesMaths::real_t z_pos = (d_height - 2.0 * screen_y) / d_smaller_dim;
	
	return (z_pos * FRAMING_RATIO / d_viewport_zoom.zoom_factor());
}
				
void
GPlatesQtWidgets::GlobeCanvas::clear_canvas(
		const QColor& c) 
{
	glClearColor(c.red(), c.green(), c.blue(), c.alpha());
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
