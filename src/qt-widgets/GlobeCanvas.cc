/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
 *  (under the name "GLCanvas.cc")
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
 *  (under the name "GlobeCanvas.cc")
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
#include <utility>
#include <cmath>
#include <iostream>
#include <boost/none.hpp>

#include <QLinearGradient>
#include <QLocale>
#include <QPainter>
#include <QtGui/QMouseEvent>
#include <QSizePolicy>

#include "gui/ColourScheme.h"
#include "gui/GlobeVisibilityTester.h"
#include "gui/QGLWidgetTextRenderer.h"
#include "gui/SvgExport.h"
#include "gui/Texture.h"

#include "maths/types.h"
#include "maths/UnitVector3D.h"
#include "maths/LatLonPoint.h"

#include "global/GPlatesException.h"
#include "model/FeatureHandle.h"

#include "presentation/ViewState.h"

#include "utils/UnicodeStringUtils.h"

#include "view-operations/RenderedGeometryCollection.h"


/**
 * At the initial zoom, the smaller dimension of the GlobeCanvas will be @a FRAMING_RATIO times the
 * diameter of the Globe.  Obviously, when the GlobeCanvas is resized, the Globe will be scaled
 * accordingly.
 *
 * The value of this constant is purely cosmetic.
 */
static const GLfloat FRAMING_RATIO = static_cast<GLfloat>(1.07);

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
	discrim_signifies_on_globe(
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
	calc_pos_on_globe(
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
	calc_pos_at_intersection_with_globe(
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
	calc_virtual_globe_position(
			const double &y,
			const double &z) 
	{
		double discrim = calc_globe_pos_discrim(y, z);
		
		if (discrim_signifies_on_globe(discrim)) {
			// Universe coords y and z do in fact determine a position on the globe.
			return calc_pos_on_globe(y, z, discrim);
		}

		// Universe coords y and z do not determine a position on the globe.  Find the
		// closest point which *is* on the globe.
		return calc_pos_at_intersection_with_globe(y, z, discrim);
	}
	
}


const GPlatesMaths::PointOnSphere &
GPlatesQtWidgets::GlobeCanvas::centre_of_viewport()
{
	static const GPlatesMaths::PointOnSphere centre_point(GPlatesMaths::UnitVector3D(1, 0, 0));
	return centre_point;
}


// Public constructor
GPlatesQtWidgets::GlobeCanvas::GlobeCanvas(
		GPlatesPresentation::ViewState &view_state,
		GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme,
		QWidget *parent_):
	QGLWidget(parent_),
	d_view_state(view_state),
	// The following unit-vector initialisation value is arbitrary.
	d_virtual_mouse_pointer_pos_on_globe(GPlatesMaths::UnitVector3D(1, 0, 0)),
	d_mouse_pointer_is_on_globe(false),
	d_globe(
			view_state.get_rendered_geometry_collection(),
			view_state.get_render_settings(),
			GPlatesGui::QGLWidgetTextRenderer::create(this),
			GPlatesGui::GlobeVisibilityTester(*this),
			colour_scheme),
	d_viewport_zoom(view_state.get_viewport_zoom()),
	d_mouse_wheel_enabled(true)
{
	init();
}


// Private constructor
GPlatesQtWidgets::GlobeCanvas::GlobeCanvas(
		GlobeCanvas *existing_globe_canvas,
		GPlatesPresentation::ViewState &view_state_,
		GPlatesMaths::PointOnSphere &virtual_mouse_pointer_pos_on_globe_,
		bool mouse_pointer_is_on_globe_,
		GPlatesGui::Globe &existing_globe_,
		bool mouse_wheel_enabled_,
		GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme_,
		QWidget *parent_) :
	QGLWidget(parent_, existing_globe_canvas /* share display lists and texture objects */),
	d_view_state(view_state_),
	d_virtual_mouse_pointer_pos_on_globe(virtual_mouse_pointer_pos_on_globe_),
	d_mouse_pointer_is_on_globe(mouse_pointer_is_on_globe_),
	d_globe(
			existing_globe_,
			GPlatesGui::QGLWidgetTextRenderer::create(this),
			GPlatesGui::GlobeVisibilityTester(*this),
			colour_scheme_),
	d_viewport_zoom(view_state_.get_viewport_zoom()),
	d_mouse_wheel_enabled(mouse_wheel_enabled_)
{
	init();
}


void
GPlatesQtWidgets::GlobeCanvas::init()
{
	// QWidget::setMouseTracking:
	//   If mouse tracking is disabled (the default), the widget only receives mouse move
	//   events when at least one mouse button is pressed while the mouse is being moved.
	//
	//   If mouse tracking is enabled, the widget receives mouse move events even if no buttons
	//   are pressed.
	//    -- http://doc.trolltech.com/4.3/qwidget.html#mouseTracking-prop
	setMouseTracking(true);
	
	// Ensure the globe will always expand to fill available space.
	// A minumum size and non-collapsibility is set on the globe basically so users
	// can't obliterate it and then wonder (read:complain) where their globe went.
	QSizePolicy globe_size_policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	globe_size_policy.setHorizontalStretch(255);
	setSizePolicy(globe_size_policy);
	setMinimumSize(100, 100);

	QObject::connect(&(d_globe.orientation()), SIGNAL(orientation_changed()),
			this, SLOT(notify_of_orientation_change()));
	QObject::connect(&(d_globe.orientation()), SIGNAL(orientation_changed()),
			this, SLOT(force_mouse_pointer_pos_change()));

	// Update our canvas whenever the RenderedGeometryCollection gets updated.
	// This will cause 'paintGL()' to be called which will visit the rendered
	// geometry collection and redraw it.
	QObject::connect(
			&(d_view_state.get_rendered_geometry_collection()),
			SIGNAL(collection_was_updated(
					GPlatesViewOperations::RenderedGeometryCollection &,
					GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type)),
			this,
			SLOT(update_canvas()));

	// Update canvas whenever RenderSettings gets changed.
	QObject::connect(
			&(d_view_state.get_render_settings()),
			SIGNAL(settings_changed()),
			this,
			SLOT(update_canvas()));

	// Update canvas whenever Texture gets changed.
	QObject::connect(
			&(d_globe.texture()),
			SIGNAL(texture_changed()),
			this,
			SLOT(update_canvas()));

	handle_zoom_change();

	setAttribute(Qt::WA_NoSystemBackground);
}


GPlatesQtWidgets::GlobeCanvas *
GPlatesQtWidgets::GlobeCanvas::clone(
		GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme,
		QWidget *parent_)
{
	return new GlobeCanvas(
			this,
			d_view_state,
			d_virtual_mouse_pointer_pos_on_globe,
			d_mouse_pointer_is_on_globe,
			d_globe,
			d_mouse_wheel_enabled,
			colour_scheme,
			parent_);
}


double
GPlatesQtWidgets::GlobeCanvas::current_proximity_inclusion_threshold(
		const GPlatesMaths::PointOnSphere &click_point) const
{
	// Say we pick an epsilon radius of 3 pixels around the click position.  That's an epsilon
	// diameter of 6 pixels.  (Obviously, the larger this diameter, the more relaxed the
	// proximity inclusion threshold.)
	// FIXME:  Do we want this constant to instead be a variable set by a per-user preference,
	// to enable users to specify their own epsilon radius?  (For example, users with shaky
	// hands or very high-resolution displays might prefer a larger epsilon radius.)
	static const GLdouble epsilon_diameter = 6.0;

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
	// on-screen projection size of the epsilon circle to the angle at the centre of the globe.
	// (However, we won't bother with the inverse-sine:  For arguments of as small a magnitude
	// as these (less than 0.05), the value of asin(x) is practically equal to the value of 'x'
	// anyway.  No, really -- don't take my word for it -- try it yourself!)
	//
	// Then take the cosine, and we have the dot-product-related closeness inclusion threshold.

	// Algorithm modification, 2007-10-16:  If you look at a cross-section of the globe (sliced
	// vertically) you'll notice that at high latitudes (whether positive or negative), we see
	// the surface of the globe almost-tangentially.  At these high latitudes, a small
	// mouse-pointer displacement on-screen will result in a significantly larger mouse-pointer
	// displacement on the surface of the globe than would the equivalent mouse-pointer
	// displacement at the Equator.
	//
	// Since the mouse-pointer position on-screen will vary by whole pixels, at the high
	// latitudes of the cross-section it might not even be *possible* to reach a mouse-pointer
	// position on the globe which is close enough to a geometry to click on it:  Moving by a
	// single pixel on-screen might cause the mouse-pointer position on the globe to "skip
	// over" the necessary location on the globe.
	//
	// If we generalise this reasoning to the 3-D sphere, we can state that the "problem areas"
	// of the sphere are those which are "far" from the point which appears at the centre of
	// the projection of the globe.  Mathematically, if we let 'lambda' be the angular distance
	// of the current point from the point which appears at the centre of the projection of the
	// globe, then it is the "high" values of lambda (those which are close to 90 degrees)
	// which correspond to the high latitudes of the cross-section case.
	// 
	// So, let's increase the epsilon diameter proportional to (1 - cos(lambda)) so that it is
	// still possible to click on geometries at the edge of the globe.
	//
	// The value of 3 below is pretty much arbitrary, however.  I just tried different values
	// till things behaved as I wanted in the GUI.

	// Since our globe is a unit sphere, the x-coordinate of the virtual click point is equal
	// to the cos of 'lambda'.
	double cos_lambda = click_point.position_vector().x().dval();
	double lambda_scaled_epsilon_diameter = epsilon_diameter + 3 * epsilon_diameter * (1 - cos_lambda);
	GLdouble diameter_ratio =
			lambda_scaled_epsilon_diameter / (d_smaller_dim * d_viewport_zoom.zoom_factor());
	double proximity_inclusion_threshold = cos(static_cast<double>(diameter_ratio));

#if 0  // Change 0 to 1 if you want this informative output.
	std::cout << "\nEpsilon diameter: " << epsilon_diameter << std::endl;
	std::cout << "cos(lambda): " << cos_lambda << std::endl;
	std::cout << "Lambda-scaled epsilon diameter: " << lambda_scaled_epsilon_diameter << std::endl;
	std::cout << "Smaller canvas dim: " << d_smaller_dim << std::endl;
	std::cout << "Zoom factor: " << d_viewport_zoom.zoom_factor() << std::endl;
	std::cout << "Globe size: " << (d_smaller_dim * d_viewport_zoom.zoom_factor()) << std::endl;
	std::cout << "Diameter ratio: " << diameter_ratio << std::endl;
	std::cout << "asin(diameter ratio): " << asin(diameter_ratio) << std::endl;
	std::cout << "Proximity inclusion threshold: " << proximity_inclusion_threshold << std::endl;
	std::cout << "Proximity inclusion threshold from asin(diameter ratio): "
			<< cos(asin(diameter_ratio)) << std::endl;
#endif

	return proximity_inclusion_threshold;
}


void
GPlatesQtWidgets::GlobeCanvas::update_canvas()
{
	update();
}


void
GPlatesQtWidgets::GlobeCanvas::draw_svg_output()
{

	try {
		clear_canvas();
		glLoadIdentity();
		glTranslatef(EYE_X,EYE_Y,EYE_Z);
		// Set up our universe coordinate system (the standard geometric one):
		//   Z points up
		//   Y points right
		//   X points out of the screen
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		glRotatef(-90.0, 0.0, 0.0, 1.0);

		const double viewport_zoom_factor = d_viewport_zoom.zoom_factor();
		d_globe.paint_vector_output(viewport_zoom_factor, calculate_scale());
	}
	catch (const GPlatesGlobal::Exception &e){
			std::cerr << e << std::endl;
	}
}

void
GPlatesQtWidgets::GlobeCanvas::notify_of_orientation_change() 
{
	update();
}


void
GPlatesQtWidgets::GlobeCanvas::handle_mouse_pointer_pos_change()
{
	double y_pos = get_universe_coord_y_of_mouse();
	double z_pos = get_universe_coord_z_of_mouse();
	GPlatesMaths::PointOnSphere new_pos = calc_virtual_globe_position(y_pos, z_pos);

	bool is_now_on_globe = discrim_signifies_on_globe(calc_globe_pos_discrim(y_pos, z_pos));

	if (new_pos != d_virtual_mouse_pointer_pos_on_globe ||
			is_now_on_globe != d_mouse_pointer_is_on_globe) {

		d_virtual_mouse_pointer_pos_on_globe = new_pos;
		d_mouse_pointer_is_on_globe = is_now_on_globe;

		GPlatesMaths::PointOnSphere oriented_new_pos = d_globe.orient(new_pos);
		emit mouse_pointer_position_changed(oriented_new_pos, is_now_on_globe);
	}
}


void
GPlatesQtWidgets::GlobeCanvas::force_mouse_pointer_pos_change()
{
	double y_pos = get_universe_coord_y_of_mouse();
	double z_pos = get_universe_coord_z_of_mouse();
	GPlatesMaths::PointOnSphere new_pos = calc_virtual_globe_position(y_pos, z_pos);

	bool is_now_on_globe = discrim_signifies_on_globe(calc_globe_pos_discrim(y_pos, z_pos));

	d_virtual_mouse_pointer_pos_on_globe = new_pos;
	d_mouse_pointer_is_on_globe = is_now_on_globe;

	GPlatesMaths::PointOnSphere oriented_new_pos = d_globe.orient(new_pos);
	emit mouse_pointer_position_changed(oriented_new_pos, is_now_on_globe);
}


void 
GPlatesQtWidgets::GlobeCanvas::initializeGL() 
{
	glEnable(GL_DEPTH_TEST);

	// Right now the only filled polygons we draw are rendered arrow heads and
	// we draw both sides of polygons for now to avoid having to close the 3d mesh
	// used to render the arrow head.
	// This is the default - just making it explicit.
	glDisable(GL_CULL_FACE);

	// FIXME: Enable polygon offset here or in Globe?
	
	clear_canvas();

}


void 
GPlatesQtWidgets::GlobeCanvas::resizeGL(
		int new_width,
		int new_height) 
{
	try
	{
		set_view();
	}
	catch (const GPlatesGlobal::Exception &e)
	{
		std::cerr << e << std::endl;
	}
}


void 
GPlatesQtWidgets::GlobeCanvas::paintGL() 
{
// This paintGL method should be enabled, and the paintEvent() method disabled, when we wish to draw *without* overpainting
//	(  http://doc.trolltech.com/4.3/opengl-overpainting.html )
//

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

		const double viewport_zoom_factor = d_viewport_zoom.zoom_factor();
		d_globe.paint(viewport_zoom_factor, calculate_scale());

	} catch (const GPlatesGlobal::Exception &e){
			std::cerr << e << std::endl;
	}

	// If d_mouse_press_info is not boost::none, then mouse is down.
	emit repainted(d_mouse_press_info);
}


void
GPlatesQtWidgets::GlobeCanvas::mousePressEvent(
		QMouseEvent *press_event) 
{
	update_mouse_pointer_pos(press_event);

	// Let's ignore all mouse buttons except the left mouse button.
	if (press_event->button() != Qt::LeftButton) {
		return;
	}
	d_mouse_press_info =
			MousePressInfo(
					press_event->x(),
					press_event->y(),
					virtual_mouse_pointer_pos_on_globe(),
					mouse_pointer_is_on_globe(),
					press_event->button(),
					press_event->modifiers());

}

void
GPlatesQtWidgets::GlobeCanvas::mouseMoveEvent(
		QMouseEvent *move_event) 
{
	update_mouse_pointer_pos(move_event);
	
	if (d_mouse_press_info) {
		// Call it a drag if EITHER:
		//  * the mouse moved at least 2 pixels in one direction and 1 pixel in the other;
		// OR:
		//  * the mouse moved at least 3 pixels in one direction.
		//
		// Otherwise, the user just has shaky hands or a very high-res screen.
		int mouse_delta_x = move_event->x() - d_mouse_press_info->d_mouse_pointer_screen_pos_x;
		int mouse_delta_y = move_event->y() - d_mouse_press_info->d_mouse_pointer_screen_pos_y;
		if (mouse_delta_x*mouse_delta_x + mouse_delta_y*mouse_delta_y > 4) {
			d_mouse_press_info->d_is_mouse_drag = true;
		}

		if (d_mouse_press_info->d_is_mouse_drag) {
			emit mouse_dragged(
					d_mouse_press_info->d_mouse_pointer_pos,
					d_globe.orient(d_mouse_press_info->d_mouse_pointer_pos),
					d_mouse_press_info->d_is_on_globe,
					virtual_mouse_pointer_pos_on_globe(),
					d_globe.orient(virtual_mouse_pointer_pos_on_globe()),
					mouse_pointer_is_on_globe(),
					d_globe.orient(centre_of_viewport()),
					d_mouse_press_info->d_button,
					d_mouse_press_info->d_modifiers);
		}
	}
	else
	{
		//
		// The mouse has moved but the left mouse button is not currently pressed.
		// This could mean no mouse buttons are currently pressed or it could mean a
		// button other than the left mouse button is currently pressed.
		// Either way it is an mouse movement that is not currently invoking a
		// canvas tool operation.
		//
		emit mouse_moved_without_drag(
				virtual_mouse_pointer_pos_on_globe(),
				d_globe.orient(virtual_mouse_pointer_pos_on_globe()),
				mouse_pointer_is_on_globe(),
				d_globe.orient(centre_of_viewport()));
	}
}


void 
GPlatesQtWidgets::GlobeCanvas::mouseReleaseEvent(
		QMouseEvent *release_event)
{
	// Let's ignore all mouse buttons except the left mouse button.
	if (release_event->button() != Qt::LeftButton) {
		return;
	}

	// Let's do our best to avoid crash-inducing Boost assertions.
	if ( ! d_mouse_press_info) {
		// OK, something strange happened:  Our boost::optional MousePressInfo is not
		// initialised.  Rather than spontaneously crashing with a Boost assertion error,
		// let's log a warning on the console and NOT crash.
		std::cerr << "Warning (GlobeCanvas::mouseReleaseEvent, "
				<< __FILE__
				<< " line "
				<< __LINE__
				<< "):\nUninitialised mouse press info!"
				<< std::endl;
		return;
	}

	if (abs(release_event->x() - d_mouse_press_info->d_mouse_pointer_screen_pos_x) > 3 &&
			abs(release_event->y() - d_mouse_press_info->d_mouse_pointer_screen_pos_y) > 3) {
		d_mouse_press_info->d_is_mouse_drag = true;
	}
	if (d_mouse_press_info->d_is_mouse_drag) {
		emit mouse_released_after_drag(
				d_mouse_press_info->d_mouse_pointer_pos,
				d_globe.orient(d_mouse_press_info->d_mouse_pointer_pos),
				d_mouse_press_info->d_is_on_globe,
				virtual_mouse_pointer_pos_on_globe(),
				d_globe.orient(virtual_mouse_pointer_pos_on_globe()),
				mouse_pointer_is_on_globe(),
				d_globe.orient(centre_of_viewport()),
				d_mouse_press_info->d_button,
				d_mouse_press_info->d_modifiers);
	} else {
		emit mouse_clicked(
				d_mouse_press_info->d_mouse_pointer_pos,
				d_globe.orient(d_mouse_press_info->d_mouse_pointer_pos),
				d_mouse_press_info->d_is_on_globe,
				d_mouse_press_info->d_button,
				d_mouse_press_info->d_modifiers);
	}
	d_mouse_press_info = boost::none;

	// Emit repainted signal with mouse_down = false so that those listeners who
	// didn't care about intermediate repaints can now deal with the repaint.
	emit repainted(false);
}


void GPlatesQtWidgets::GlobeCanvas::wheelEvent(
		QWheelEvent *wheel_event) 
{
	if (d_mouse_wheel_enabled)
	{
		handle_wheel_rotation(wheel_event->delta());
	}
	else
	{
		wheel_event->ignore();
	}
}


void
GPlatesQtWidgets::GlobeCanvas::handle_zoom_change() 
{
	// switch context before we do any GL stuff
	makeCurrent();

	set_view();

	// QWidget::update:
	//   Updates the widget unless updates are disabled or the widget is hidden.
	//
	//   This function does not cause an immediate repaint; instead it schedules a paint event
	//   for processing when Qt returns to the main event loop.
	//    -- http://doc.trolltech.com/4.3/qwidget.html#update
	update();

	handle_mouse_pointer_pos_change();
}


void
GPlatesQtWidgets::GlobeCanvas::set_view() 
{
	static const GLdouble depth_near_clipping = 0.5;

	// Always fill up all of the available space.
	update_dimensions();
	glViewport(0, 0, static_cast<GLsizei>(d_canvas_screen_width), static_cast<GLsizei>(d_canvas_screen_height));
	
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

	if (d_canvas_screen_width <= d_canvas_screen_height) {
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
GPlatesQtWidgets::GlobeCanvas::update_dimensions() 
{
	d_canvas_screen_width = width();
	d_canvas_screen_height = height();

	if (d_canvas_screen_width <= d_canvas_screen_height) {
		d_smaller_dim = static_cast<GLdouble>(d_canvas_screen_width);
		d_larger_dim = static_cast<GLdouble>(d_canvas_screen_height);
	} else {
		d_smaller_dim = static_cast<GLdouble>(d_canvas_screen_height);
		d_larger_dim = static_cast<GLdouble>(d_canvas_screen_width);
	}
}


void
GPlatesQtWidgets::GlobeCanvas::update_mouse_pointer_pos(
		QMouseEvent *mouse_event) 
{
	d_mouse_pointer_screen_pos_x = mouse_event->x();
	d_mouse_pointer_screen_pos_y = mouse_event->y();

	handle_mouse_pointer_pos_change();
}


void
GPlatesQtWidgets::GlobeCanvas::handle_wheel_rotation(
		int delta) 
{
	// These integer values (8 and 15) are copied from the Qt reference docs for QWheelEvent:
	//  http://doc.trolltech.com/4.3/qwheelevent.html#delta
	const int num_degrees = delta / 8;
	int num_steps = num_degrees / 15;

	if (num_steps > 0) {
		while (num_steps--) {
			d_viewport_zoom.zoom_in();
		}
	} else {
		while (num_steps++) {
			d_viewport_zoom.zoom_out();
		}
	}
}


double
GPlatesQtWidgets::GlobeCanvas::get_universe_coord_y(
		int screen_x) const
{
	// Scale the screen to a "unit square".
	double y_pos = (2.0 * screen_x - d_canvas_screen_width) / d_smaller_dim;

	return (y_pos * FRAMING_RATIO / d_viewport_zoom.zoom_factor());
}


double
GPlatesQtWidgets::GlobeCanvas::get_universe_coord_z(
		int screen_y) const
{
	// Scale the screen to a "unit square".
	double z_pos = (d_canvas_screen_height - 2.0 * screen_y) / d_smaller_dim;
	
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


// raster display
void
GPlatesQtWidgets::GlobeCanvas::toggle_raster_image()
{
	d_globe.toggle_raster_display();
	update_canvas();
}


void
GPlatesQtWidgets::GlobeCanvas::enable_raster_display()
{
	d_globe.enable_raster_display();
}

void
GPlatesQtWidgets::GlobeCanvas::disable_raster_display()
{
	d_globe.disable_raster_display();
}


#if 0
void
GPlatesQtWidgets::GlobeCanvas::paintEvent(QPaintEvent *paint_event)
{
// This paintEvent() method should be enabled, and the paintGL method disabled, when we wish to use Qt overpainting
//  ( http://doc.trolltech.com/4.3/opengl-overpainting.html )

	try {

		qglClearColor(Qt::black);

		QPainter painter;
		painter.begin(this);

		painter.setRenderHint(QPainter::Antialiasing);
	
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glPushAttrib(GL_ALL_ATTRIB_BITS);

		clear_canvas();

		set_view();

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

		glPopAttrib();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		draw_colour_legend(&painter,d_globe.texture());

		painter.end();

	} catch (const GPlatesGlobal::Exception &e){
			std::cerr << e << std::endl;
	}

}
#endif

void
GPlatesQtWidgets::GlobeCanvas::draw_colour_legend(
	QPainter *painter,
	GPlatesGui::Texture &texture)
{
	if (!(texture.is_enabled() && texture.corresponds_to_data())){
		return;
	}

	// The position of the bottom-right of the colour legend w.r.t the bottom right of the window.  
	QPoint margin(30,30);

	// The size of the colour legend background box.
	QSize box(80,220);

	// The size of the colour bar. 
	QSize bar(20,200);

	// The offset of the colour bar w.r.t. the colour legend background box. 
	QPoint bar_margin(5,
			static_cast<int>((box.height()-bar.height())/2.0));


	// The background box.
	QRect box_rect(width()- margin.x() - box.width(),
					height() - margin.y() - box.height(),
					box.width(),
					box.height());
			
	// The colour bar box.
	QRect bar_rect(box_rect.topLeft()+bar_margin,
					bar);


	// Draw the gray background box. 
	painter->setBrush(Qt::gray);
	painter->drawRect(box_rect);

	QLinearGradient gradient(
					bar_rect.left() + bar_rect.width()/2,
					bar_rect.top(),
					bar_rect.left() + bar_rect.width()/2,
					bar_rect.bottom());

	gradient.setColorAt(0,Qt::blue);
	gradient.setColorAt(0.25,Qt::cyan);
	gradient.setColorAt(0.5,Qt::green);
	gradient.setColorAt(0.75,Qt::yellow);
	gradient.setColorAt(1.0,Qt::red);

	// Draw the colour bar. 
	painter->setBrush(gradient);
	painter->drawRect(bar_rect);

	// Add text at the bottom, middle, and top of the colour bar. 
	QString min_string,max_string,med_string;
	float min = texture.get_min();
	float max = texture.get_max();
	float med = (max + min)/2.;

	QLocale locale_;
	min_string = locale_.toString(min,'g',4);
	med_string = locale_.toString(med,'g',4);
	max_string = locale_.toString(max,'g',4);

	painter->setPen(Qt::black);
	painter->drawText(bar_rect.right()+5, bar_rect.bottom()+2, min_string);
	painter->drawText(bar_rect.right()+5,
			static_cast<int>((bar_rect.bottom()+bar_rect.top())/2.0+2),
			med_string);
	painter->drawText(bar_rect.right()+5, bar_rect.top()+2, max_string);

}


void
GPlatesQtWidgets::GlobeCanvas::create_svg_output(
	QString filename)
{
	GPlatesGui::SvgExport::create_svg_output(filename,this);
}

void
GPlatesQtWidgets::GlobeCanvas::set_camera_viewpoint(
	const GPlatesMaths::LatLonPoint &desired_centre)
{
	static const GPlatesMaths::PointOnSphere centre_of_canvas =
			GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(0, 0));

	GPlatesMaths::PointOnSphere oriented_desired_centre = 
	d_globe.orientation().orient_point(
		GPlatesMaths::make_point_on_sphere(desired_centre));
	d_globe.set_new_handle_pos(oriented_desired_centre);
	d_globe.update_handle_pos(centre_of_canvas);

	update_canvas();
}

boost::optional<GPlatesMaths::LatLonPoint>
GPlatesQtWidgets::GlobeCanvas::camera_llp() const
{
// This returns a boost::optional for consistency with the virtual function. The globe
// should always return a valid camera llp. 
	static const GPlatesMaths::PointOnSphere centre_of_canvas =
			GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(0, 0));

	GPlatesMaths::PointOnSphere oriented_centre = globe().orient(centre_of_canvas);
	return GPlatesMaths::make_lat_lon_point(oriented_centre);
}

void
GPlatesQtWidgets::GlobeCanvas::move_camera_up()
{
	globe().orientation().move_camera_up();
}

void
GPlatesQtWidgets::GlobeCanvas::move_camera_down()
{
	globe().orientation().move_camera_down();
}

void
GPlatesQtWidgets::GlobeCanvas::move_camera_left()
{
	globe().orientation().move_camera_left();
}

void
GPlatesQtWidgets::GlobeCanvas::move_camera_right()
{
	globe().orientation().move_camera_right();
}

void
GPlatesQtWidgets::GlobeCanvas::rotate_camera_clockwise()
{
	globe().orientation().rotate_camera_clockwise();
}

void
GPlatesQtWidgets::GlobeCanvas::rotate_camera_anticlockwise()
{
	globe().orientation().rotate_camera_anticlockwise();
}

void
GPlatesQtWidgets::GlobeCanvas::reset_camera_orientation()
{
	globe().orientation().orient_poles_vertically();
}

float
GPlatesQtWidgets::GlobeCanvas::calculate_scale()
{
	int min_dimension = (std::min)(size().width(), size().height());
	return static_cast<float>(min_dimension) /
		static_cast<float>(d_view_state.get_main_viewport_min_dimension());
}

QImage
GPlatesQtWidgets::GlobeCanvas::grab_frame_buffer()
{
	return grabFrameBuffer();
}
