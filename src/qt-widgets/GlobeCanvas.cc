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
 * Copyright (C) 2011 Geological Survey of Norway
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
#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include <QDebug>
#include <QLinearGradient>
#include <QLocale>
#include <QPainter>
#include <QtGui/QMouseEvent>
#include <QSizePolicy>

#include "GlobeCanvas.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/GPlatesException.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"
#include "gui/ColourScheme.h"
#include "gui/GlobeVisibilityTester.h"
#include "gui/SimpleGlobeOrientation.h"
#include "gui/TextOverlay.h"
#include "gui/VelocityLegendOverlay.h"

#include "maths/types.h"
#include "maths/UnitVector3D.h"
#include "maths/LatLonPoint.h"

#include "model/FeatureHandle.h"

#include "opengl/GLContext.h"
#include "opengl/GLContextImpl.h"
#include "opengl/GLImageUtils.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLTileRender.h"

#include "presentation/ViewState.h"

#include "utils/Profile.h"
#include "utils/UnicodeStringUtils.h"

#include "view-operations/RenderedGeometryCollection.h"

/**
 * At the initial zoom, the smaller dimension of the GlobeCanvas will be @a FRAMING_RATIO times the
 * diameter of the Globe.  Obviously, when the GlobeCanvas is resized, the Globe will be scaled
 * accordingly.
 *
 * The value of this constant is purely cosmetic.
 */
const GLfloat GPlatesQtWidgets::GlobeCanvas::FRAMING_RATIO = static_cast<GLfloat>(1.07);

namespace 
{
	/**
	 * The view is initially oriented such that the global x-axis points out of the screen.
	 * So set our viewpoint to be along the positive x-axis (looking at the origin).
	 *
	 * This should be outside the globe (centred at origin) - should have magnitude greater than 1.0.
	 */
	static const GPlatesMaths::Vector3D EYE_POSITION(5.0, 0, 0);

	/**
	 * Calculate the globe-position discriminant for the universe coordinates @a y and @a z.
	 */
	inline
	double
	calc_globe_pos_discrim(
			double y,
			double z) 
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
			double discrim) 
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
			double y,
			double z,
			double discrim) 
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
			double y,
			double z,
			double discrim) 
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
			double y,
			double z) 
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


	/**
	 * Calculate the size of one device-independent pixel in world space coordinates.
	 */
	double
	calc_device_independent_pixel_to_world_space_ratio(
			int scene_view_width_in_device_independent_pixels,
			int scene_view_height_in_device_independent_pixels,
			const double &zoom_factor)
	{
		// Smallest dimension (view width or height) in device-independent pixels.
		double smaller_dim_in_device_independent_pixels;
		if (scene_view_width_in_device_independent_pixels <= scene_view_height_in_device_independent_pixels)
		{
			smaller_dim_in_device_independent_pixels = static_cast<GLdouble>(scene_view_width_in_device_independent_pixels);
		}
		else
		{
			smaller_dim_in_device_independent_pixels = static_cast<GLdouble>(scene_view_height_in_device_independent_pixels);
		}

		// The smallest dimension (view width or height) is bounded by this size in world space.
		const GLdouble smaller_dim_world_space = 2 * GPlatesQtWidgets::GlobeCanvas::FRAMING_RATIO / zoom_factor;

		return smaller_dim_world_space / smaller_dim_in_device_independent_pixels;
	}


	/**
	 * Given the scene view's dimensions (eg, canvas dimensions) generate projection transforms
	 * needed to display the scene.
	 *
	 * NOTE: The projection transforms are post-multiplied (ie, not initialised to identity first)
	 * so set them to identity first, if necessary, otherwise the previous values will be included.
	 */
	void
	calc_scene_projection_transforms(
			int scene_view_width_in_device_independent_pixels,
			int scene_view_height_in_device_independent_pixels,
			const double &zoom_factor,
			GPlatesOpenGL::GLMatrix &projection_transform_include_front_half_globe,
			GPlatesOpenGL::GLMatrix &projection_transform_include_rear_half_globe,
			GPlatesOpenGL::GLMatrix &projection_transform_include_full_globe,
			GPlatesOpenGL::GLMatrix &projection_transform_include_stars,
			GPlatesOpenGL::GLMatrix &projection_transform_text_overlay)
	{
		// NOTE: We ensure that the projection transforms calculated here can also be used when
		// rendering to SVG output. This is because SVG output ignores depth buffering -
		// it uses the OpenGL feedback mechanism which bypasses rasterisation - and hence the
		// opaque disc through the centre of the globe does not occlude vector geometry
		// on the back side of the globe).
		// The solution is to set the far or near clip planes to pass through the globe centre
		// (depending on whether rendering front or rear half of globe.
		// This is effectively does the equivalent culling of the opaque disc but in the
		// transformation pipeline instead of the rasterisation pipeline.
		//
		static const GLdouble eye_to_globe_centre_distance = EYE_POSITION.magnitude().dval();
		// The 1.0 is the globe radius.
		// The 0.5 is arbitrary and because we don't want to put the near clipping plane too close
		// to the globe because some objects are outside the globe such as rendered arrows.
		static const GLdouble depth_in_front_of_globe = eye_to_globe_centre_distance - 1.0 - 0.5;
		static const GLdouble depth_behind_globe = eye_to_globe_centre_distance + 1.0 + 0.5;
		// The stars need a far clip plane further away.
		static const GLdouble depth_behind_globe_and_including_stars = depth_behind_globe + 10;
		static const GLdouble depth_globe_centre = eye_to_globe_centre_distance;
		// The 'depth_globe_centre' will need adjustment so that the circumference of the globe doesn't get clipped.
		// Also the opaque sphere is now rendered as a flat disk facing the camera and
		// positioned through the globe centre - so we don't want that to get clipped away either.
		static const GLdouble depth_epsilon_to_avoid_clipping_globe_circumference = 0.0001;

		// The smaller/larger of the dimensions (width/height) of the screen.
		double smaller_dim;
		double larger_dim;
		if (scene_view_width_in_device_independent_pixels <= scene_view_height_in_device_independent_pixels)
		{
			smaller_dim = static_cast<GLdouble>(scene_view_width_in_device_independent_pixels);
			larger_dim = static_cast<GLdouble>(scene_view_height_in_device_independent_pixels);
		}
		else
		{
			smaller_dim = static_cast<GLdouble>(scene_view_height_in_device_independent_pixels);
			larger_dim = static_cast<GLdouble>(scene_view_width_in_device_independent_pixels);
		}
		
		// This is used for the coordinates of the symmetrical clipping planes which bound the
		// smaller dimension.
		GLdouble smaller_dim_clipping = GPlatesQtWidgets::GlobeCanvas::FRAMING_RATIO / zoom_factor;

		// This is used for the coordinates of the symmetrical clipping planes which bound the
		// larger dimension.
		GLdouble dim_ratio = larger_dim / smaller_dim;
		GLdouble larger_dim_clipping = smaller_dim_clipping * dim_ratio;

		GLdouble ortho_left, ortho_right, ortho_bottom, ortho_top;
		if (scene_view_width_in_device_independent_pixels <= scene_view_height_in_device_independent_pixels)
		{
			ortho_left = -smaller_dim_clipping;
			ortho_right = smaller_dim_clipping;
			ortho_bottom = -larger_dim_clipping;
			ortho_top = larger_dim_clipping;
		}
		else
		{
			ortho_left = -larger_dim_clipping;
			ortho_right = larger_dim_clipping;
			ortho_bottom = -smaller_dim_clipping;
			ortho_top = smaller_dim_clipping;
		}

		projection_transform_include_front_half_globe.gl_ortho(
				ortho_left, ortho_right,
				ortho_bottom, ortho_top,
				depth_in_front_of_globe,
				depth_globe_centre + depth_epsilon_to_avoid_clipping_globe_circumference);

		projection_transform_include_rear_half_globe.gl_ortho(
				ortho_left, ortho_right,
				ortho_bottom, ortho_top,
				depth_globe_centre - depth_epsilon_to_avoid_clipping_globe_circumference,
				depth_behind_globe);

		projection_transform_include_full_globe.gl_ortho(
				ortho_left, ortho_right,
				ortho_bottom, ortho_top,
				depth_in_front_of_globe,
				depth_behind_globe);

		projection_transform_include_stars.gl_ortho(
				ortho_left, ortho_right,
				ortho_bottom, ortho_top,
				depth_in_front_of_globe,
				depth_behind_globe_and_including_stars);

		// The text overlay coordinates are specified in window coordinates.
		// The near and far values only need to include z=0 so [-1,1] will do fine.
		projection_transform_text_overlay.gl_ortho(
				0, scene_view_width_in_device_independent_pixels,
				0, scene_view_height_in_device_independent_pixels,
				-1,
				1);
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
	QGLWidget(
			GPlatesOpenGL::GLContext::get_qgl_format_to_create_context_with(),
			parent_),
	d_view_state(view_state),
	d_gl_context(
			GPlatesOpenGL::GLContext::create(
					boost::shared_ptr<GPlatesOpenGL::GLContext::Impl>(
							new GPlatesOpenGL::GLContextImpl::QGLWidgetImpl(*this)))),
	d_make_context_current(*d_gl_context),
	d_initialisedGL(false),
	d_gl_visual_layers(
			GPlatesOpenGL::GLVisualLayers::create(
					d_gl_context, view_state.get_application_state())),
	// The following unit-vector initialisation value is arbitrary.
	d_virtual_mouse_pointer_pos_on_globe(GPlatesMaths::UnitVector3D(1, 0, 0)),
	d_mouse_pointer_is_on_globe(false),
	d_globe(
			view_state,
			d_gl_visual_layers,
			view_state.get_rendered_geometry_collection(),
			view_state.get_visual_layers(),
			GPlatesGui::GlobeVisibilityTester(*this),
			colour_scheme,
			devicePixelRatio()),
	d_text_overlay(
			new GPlatesGui::TextOverlay(
				view_state.get_application_state())),
	d_velocity_legend_overlay(
			new GPlatesGui::VelocityLegendOverlay())
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
		GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme_,
		QWidget *parent_) :
	QGLWidget(
			GPlatesOpenGL::GLContext::get_qgl_format_to_create_context_with(),
			parent_,
			// Share texture objects, vertex buffer objects, etc...
			existing_globe_canvas),
	d_view_state(view_state_),
	d_gl_context(isSharing() // Mirror the sharing of OpenGL context state (if sharing)...
			? GPlatesOpenGL::GLContext::create(
					boost::shared_ptr<GPlatesOpenGL::GLContext::Impl>(
							new GPlatesOpenGL::GLContextImpl::QGLWidgetImpl(*this)),
					*existing_globe_canvas->d_gl_context)
			: GPlatesOpenGL::GLContext::create(
					boost::shared_ptr<GPlatesOpenGL::GLContext::Impl>(
							new GPlatesOpenGL::GLContextImpl::QGLWidgetImpl(*this)))),
	d_make_context_current(*d_gl_context),
	d_initialisedGL(false),
	d_gl_visual_layers(
			// Attempt to share OpenGL resources across contexts.
			// This will depend on whether the two 'GLContext's share any state.
			GPlatesOpenGL::GLVisualLayers::create(
					d_gl_context,
					existing_globe_canvas->d_gl_visual_layers,
					view_state_.get_application_state())),
	d_virtual_mouse_pointer_pos_on_globe(virtual_mouse_pointer_pos_on_globe_),
	d_mouse_pointer_is_on_globe(mouse_pointer_is_on_globe_),
	d_globe(
			existing_globe_,
			d_gl_visual_layers,
			GPlatesGui::GlobeVisibilityTester(*this),
			colour_scheme_,
			devicePixelRatio()),
	d_text_overlay(
			new GPlatesGui::TextOverlay(
				d_view_state.get_application_state())),
	d_velocity_legend_overlay(
			new GPlatesGui::VelocityLegendOverlay())
{
	if (!isSharing())
	{
		qWarning() << "Unable to share an OpenGL context between QGLWidgets.";
	}

	init();
}


GPlatesQtWidgets::GlobeCanvas::~GlobeCanvas()
{  }


void
GPlatesQtWidgets::GlobeCanvas::init()
{
	// Since we're using a QPainter inside 'paintEvent()' or more specifically 'paintGL()'
	// (which is called from 'paintEvent()') then we turn off automatic swapping of the OpenGL
	// front and back buffers after each 'paintGL()' call. This is because QPainter::end(),
	// or QPainter's destructor, automatically calls QGLWidget::swapBuffers() if auto buffer swap
	// is enabled - and this results in two calls to QGLWidget::swapBuffers() - one from QPainter
	// and one from 'paintEvent()'. So we disable auto buffer swapping and explicitly call it ourself.
	//
	// Also we don't want to swap buffers when we're just rendering to a QImage (using OpenGL)
	// and not rendering to the QGLWidget itself, otherwise the widget will have the wrong content.
	setAutoBufferSwap(false);

	// Don't fill the background - we already clear the background using OpenGL in 'render_scene()' anyway.
	//
	// NOTE: Also there's a problem where QPainter (used in 'paintGL()') uses the background role
	// of the canvas widget to fill the background using glClearColor/glClear - but the clear colour
	// does not get reset to black (default OpenGL state) in 'QPainter::beginNativePainting()' which
	// GLRenderer requires (the default OpenGL state) and hence it assumes the clear colour is black
	// when it is not - and hence the background (behind the globe) is *not* black.
	setAutoFillBackground(false);

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
	setFocusPolicy(Qt::StrongFocus);
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
			lambda_scaled_epsilon_diameter / (d_smaller_dim * d_view_state.get_viewport_zoom().zoom_factor());
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
GPlatesQtWidgets::GlobeCanvas::notify_of_orientation_change() 
{
	update_canvas();
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
		Q_EMIT mouse_pointer_position_changed(oriented_new_pos, is_now_on_globe);
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
	Q_EMIT mouse_pointer_position_changed(oriented_new_pos, is_now_on_globe);
}


void 
GPlatesQtWidgets::GlobeCanvas::initializeGL_if_necessary() 
{
	// Return early if we've already initialised OpenGL.
	// This is now necessary because it's not only 'paintEvent()' and other QGLWidget methods
	// that call our 'initializeGL()' method - we also now it when a client wants to render the
	// scene to an image (instead of render/update the QGLWidget itself).
	if (d_initialisedGL)
	{
		return;
	}

	// Make sure the OpenGL context is current.
	// We can't use 'd_gl_context' yet because it hasn't been initialised.
	makeCurrent();

	initializeGL();
}


void 
GPlatesQtWidgets::GlobeCanvas::initializeGL() 
{
	// Initialise our context-like object first.
	d_gl_context->initialise();

	// Create the off-screen context that's used when rendering OpenGL outside the paint event.
	d_gl_off_screen_context =
			GPlatesOpenGL::GLOffScreenContext::create(
					GPlatesOpenGL::GLOffScreenContext::QGLWidgetContext(this, d_gl_context));

	//
	// Set up the initial model-view transform.
	//

	// Set up our universe coordinate system (the standard geometric one):
	//   Z points up
	//   Y points right
	//   X points out of the screen
	d_gl_model_view_transform.gl_load_identity();
	d_gl_model_view_transform.glu_look_at(
			EYE_POSITION.x().dval(), EYE_POSITION.y().dval(), EYE_POSITION.z().dval(), // eye
			0, 0, 0,  // centre
			0, 0, 1); // up

	// Get a renderer - it'll be used to initialise some OpenGL objects.
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = d_gl_context->create_renderer();

	// Start a begin_render/end_render scope.
	// Pass in the viewport of the window currently attached to the OpenGL context.
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer);

	// NOTE: We don't actually 'glClear()' the framebuffer because:
	//  1) It's not necessary to do this before calling Globe::initialiseGL(), and
	//  2) It appears to generate an OpenGL error when GPlatesOpenGL::GLUtils::assert_no_gl_errors()
	//     is next called ("invalid framebuffer operation") on a Mac-mini system (all other systems
	//     seem fine) - it's possibly due to the main framebuffer not set up correctly yet - retrieving
	//     the viewport parameters returns 100x100 pixels which is not the actual size of the canvas.

	// Initialise those parts of globe that require a valid OpenGL context to be bound.
	d_globe.initialiseGL(*renderer);

	// 'initializeGL()' should only be called once.
	d_initialisedGL = true;
}


void 
GPlatesQtWidgets::GlobeCanvas::resizeGL(
		int new_width,
		int new_height) 
{
	set_view();
}


void 
GPlatesQtWidgets::GlobeCanvas::paintGL() 
{
// This paintGL method should be enabled, and the paintEvent() method disabled, when we wish to draw *without* overpainting
//	(  http://doc.trolltech.com/4.3/opengl-overpainting.html )
//

	// We use a QPainter (attached to the canvas) since it is used for (OpenGL) text rendering.
	QPainter painter(this);

	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = d_gl_context->create_renderer();

	// Start a begin_render/end_render scope.
	//
	// By default the current render target of 'renderer' is the main frame buffer (of the window).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	//
	// We're currently in an active QPainter so we need to let the GLRenderer know about that.
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer, painter);

	// Clear colour buffer of the main framebuffer.
	//
	// Note that we clear the colour to (0,0,0,1) and not (0,0,0,0) because we want any parts of
	// the scene, that are not rendered, to have *opaque* alpha (=1). This appears to be needed on
	// Mac with Qt5 (alpha=0 is fine on Qt5 Windows/Ubuntu, and on Qt4 for all platforms). Perhaps because
	// QGLWidget rendering (on Qt5 Mac) is first done to a framebuffer object which is then blended into the
	// window framebuffer (where having a source alpha of zero would result in the black background not showing).
	// Or, more likely, maybe a framebuffer object is used on all platforms but the window framebuffer is
	// white on Mac but already black on Windows/Ubuntu.
	const GPlatesGui::Colour clear_colour(0, 0, 0, 1);

	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = render_scene(
			*renderer,
			d_gl_projection_transform_include_front_half_globe,
			d_gl_projection_transform_include_rear_half_globe,
			d_gl_projection_transform_include_full_globe,
			d_gl_projection_transform_include_stars,
			d_gl_projection_transform_text_overlay,
			clear_colour,
			// Using device-independent pixels (eg, widget dimensions)...
			width(),
			height());
}


QSize
GPlatesQtWidgets::GlobeCanvas::get_viewport_size() const
{
	return QSize(width(), height());
}


QImage
GPlatesQtWidgets::GlobeCanvas::render_to_qimage(
		const QSize &image_size_in_device_independent_pixels,
		const GPlatesGui::Colour &image_clear_colour)
{
	// Initialise OpenGL if we haven't already.
	initializeGL_if_necessary();

	// We use a QPainter (attached to the canvas) since it is used for (OpenGL) text rendering.
	QPainter painter(this);

	// Start a render scope.
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	//
	// Where possible, force drawing to an off-screen render target.
	// It seems making the OpenGL context current is not enough to prevent Snow Leopard systems
	// with ATI graphics from hanging/crashing - this appears to be due to modifying/accessing the
	// main/default framebuffer (which is intimately tied to the windowing system).
	// Using an off-screen render target appears to avoid this issue.
	//
	// Set the off-screen render target to the size of the QGLWidget main framebuffer.
	// This is because we use QPainter to render text and it sets itself up using the dimensions
	// of the main framebuffer - if we change the dimensions then the text is rendered incorrectly.
	//
	// We're currently in an active QPainter so we need to let the GLRenderer know about that.
	GPlatesOpenGL::GLOffScreenContext::RenderScope off_screen_render_scope(
			*d_gl_off_screen_context.get(),
			// Convert from widget size to device pixels (used by OpenGL)...
			width() * devicePixelRatio(),
			height() * devicePixelRatio(),
			painter);

	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = off_screen_render_scope.get_renderer();


	// The image to render/copy the scene into.
	//
	// Handle high DPI displays (eg, Apple Retina) by rendering image in high-res device pixels.
	// The image will still be it's original size in device *independent* pixels.
	//
	// TODO: We're using the device pixel ratio of current canvas since we're rendering into that and
	// then copying into image. This might not be ideal if this canvas is displayed on one monitor and
	// the QImage (eg, Colouring previews) will be displayed on another with a different device pixel ratio.
	const QSize image_size_in_device_pixels(
			image_size_in_device_independent_pixels.width() * devicePixelRatio(),
			image_size_in_device_independent_pixels.height() * devicePixelRatio());
	QImage image(image_size_in_device_pixels, QImage::Format_ARGB32);
	image.setDevicePixelRatio(devicePixelRatio());

	if (image.isNull())
	{
		// Most likely a memory allocation failure - return the null image.
		return QImage();
	}

	// Fill the image with the clear colour in case there's an exception during rendering
	// of one of the tiles and the image is incomplete.
	image.fill(QColor(image_clear_colour).rgba());

	// Get the frame buffer dimensions (in device pixels).
	const std::pair<unsigned int/*width*/, unsigned int/*height*/> frame_buffer_dimensions =
			renderer->get_current_frame_buffer_dimensions();

	// The border is half the point size or line width, rounded up to nearest pixel.
	// TODO: Use the actual maximum point size or line width to calculate this.
	const unsigned int tile_border = 10;
	// Set up for rendering the scene into tiles.
	// The tile render target dimensions match the frame buffer dimensions.
	GPlatesOpenGL::GLTileRender tile_render(
			frame_buffer_dimensions.first/*tile_render_target_width*/,
			frame_buffer_dimensions.second/*tile_render_target_height*/,
			GPlatesOpenGL::GLViewport(
					0,
					0,
					// Use image size in device pixels (used by OpenGL)...
					image_size_in_device_pixels.width(),
					image_size_in_device_pixels.height())/*destination_viewport*/,
			tile_border);

	// Calculate the projection matrices associated with the image dimensions.
	GPlatesOpenGL::GLMatrix projection_transform_include_front_half_globe;
	GPlatesOpenGL::GLMatrix projection_transform_include_rear_half_globe;
	GPlatesOpenGL::GLMatrix projection_transform_include_full_globe;
	GPlatesOpenGL::GLMatrix projection_transform_include_stars;
	GPlatesOpenGL::GLMatrix projection_transform_text_overlay;
	calc_scene_projection_transforms(
			// Using device-independent pixels (eg, widget dimensions)...
			image_size_in_device_independent_pixels.width(),
			image_size_in_device_independent_pixels.height(),
			d_view_state.get_viewport_zoom().zoom_factor(),
			projection_transform_include_front_half_globe,
			projection_transform_include_rear_half_globe,
			projection_transform_include_full_globe,
			projection_transform_include_stars,
			projection_transform_text_overlay);

	// Keep track of the cache handles of all rendered tiles.
	boost::shared_ptr< std::vector<cache_handle_type> > frame_cache_handle(
			new std::vector<cache_handle_type>());

	// Render the scene tile-by-tile.
	for (tile_render.first_tile(); !tile_render.finished(); tile_render.next_tile())
	{
		// Render the scene to the feedback paint device.
		// This will use the main framebuffer for intermediate rendering in some cases.
		// Hold onto the previous frame's cached resources *while* generating the current frame.
		const cache_handle_type tile_cache_handle =	 render_scene_tile_into_image(
				*renderer,
				tile_render,
				projection_transform_include_front_half_globe,
				projection_transform_include_rear_half_globe,
				projection_transform_include_full_globe,
				projection_transform_include_stars,
				projection_transform_text_overlay,
				image_clear_colour,
				image);
		frame_cache_handle->push_back(tile_cache_handle);
	}

	// The previous cached resources were kept alive *while* in the rendering loop above.
	d_gl_frame_cache_handle = frame_cache_handle;

	return image;
}


GPlatesQtWidgets::GlobeCanvas::cache_handle_type
GPlatesQtWidgets::GlobeCanvas::render_scene_tile_into_image(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesOpenGL::GLTileRender &tile_render,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_front_half_globe,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_rear_half_globe,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_full_globe,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_stars,
		const GPlatesOpenGL::GLMatrix &projection_transform_text_overlay,
		const GPlatesGui::Colour &image_clear_colour,
		QImage &image)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

	GPlatesOpenGL::GLViewport current_tile_render_target_viewport;
	tile_render.get_tile_render_target_viewport(current_tile_render_target_viewport);

	GPlatesOpenGL::GLViewport current_tile_render_target_scissor_rect;
	tile_render.get_tile_render_target_scissor_rectangle(current_tile_render_target_scissor_rect);

	// Mask off rendering outside the current tile region in case the tile is smaller than the
	// render target. Note that the tile's viewport is slightly larger than the tile itself
	// (the scissor rectangle) in order that fat points and wide lines just outside the tile
	// have pixels rasterised inside the tile (the projection transform has also been expanded slightly).
	//
	// This includes 'gl_clear()' calls which clear the entire framebuffer.
	renderer.gl_enable(GL_SCISSOR_TEST);
	renderer.gl_scissor(
			current_tile_render_target_scissor_rect.x(),
			current_tile_render_target_scissor_rect.y(),
			current_tile_render_target_scissor_rect.width(),
			current_tile_render_target_scissor_rect.height());
	renderer.gl_viewport(
			current_tile_render_target_viewport.x(),
			current_tile_render_target_viewport.y(),
			current_tile_render_target_viewport.width(),
			current_tile_render_target_viewport.height());

	//
	// Adjust the various projection transforms for the current tile.
	//

	const GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type projection_transform_tile =
			tile_render.get_tile_projection_transform();
	const GPlatesOpenGL::GLMatrix &projection_matrix_tile = projection_transform_tile->get_matrix();

	GPlatesOpenGL::GLMatrix tile_projection_transform_include_front_half_globe(projection_matrix_tile);
	GPlatesOpenGL::GLMatrix tile_projection_transform_include_rear_half_globe(projection_matrix_tile);
	GPlatesOpenGL::GLMatrix tile_projection_transform_include_full_globe(projection_matrix_tile);
	GPlatesOpenGL::GLMatrix tile_projection_transform_include_stars(projection_matrix_tile);
	GPlatesOpenGL::GLMatrix tile_projection_transform_text_overlay(projection_matrix_tile);

	tile_projection_transform_include_front_half_globe.gl_mult_matrix(projection_transform_include_front_half_globe);
	tile_projection_transform_include_rear_half_globe.gl_mult_matrix(projection_transform_include_rear_half_globe);
	tile_projection_transform_include_full_globe.gl_mult_matrix(projection_transform_include_full_globe);
	tile_projection_transform_include_stars.gl_mult_matrix(projection_transform_include_stars);
	tile_projection_transform_text_overlay.gl_mult_matrix(projection_transform_text_overlay);

	//
	// Render the scene.
	//
	const cache_handle_type tile_cache_handle = render_scene(
			renderer,
			tile_projection_transform_include_front_half_globe,
			tile_projection_transform_include_rear_half_globe,
			tile_projection_transform_include_full_globe,
			tile_projection_transform_include_stars,
			tile_projection_transform_text_overlay,
			image_clear_colour,
			// Since QImage is just raw pixels its dimensions are in device pixels, but
			// we need device-independent pixels here (eg, widget dimensions)...
			image.width() / image.devicePixelRatio(),
			image.height() / image.devicePixelRatio());

	//
	// Copy the rendered tile into the appropriate sub-rect of the image.
	//

	GPlatesOpenGL::GLViewport current_tile_source_viewport;
	tile_render.get_tile_source_viewport(current_tile_source_viewport);

	GPlatesOpenGL::GLViewport current_tile_destination_viewport;
	tile_render.get_tile_destination_viewport(current_tile_destination_viewport);

	GPlatesOpenGL::GLImageUtils::copy_rgba8_frame_buffer_into_argb32_qimage(
			renderer,
			image,
			current_tile_source_viewport,
			current_tile_destination_viewport);

	return tile_cache_handle;
}


void
GPlatesQtWidgets::GlobeCanvas::render_opengl_feedback_to_paint_device(
		QPaintDevice &feedback_paint_device)
{
	// Initialise OpenGL if we haven't already.
	initializeGL_if_necessary();

	// Note that we're not rendering to the OpenGL canvas here.
	// The OpenGL rendering gets redirected into the QPainter (using OpenGL feedback) and
	// ends up in the specified paint device.
	QPainter feedback_painter(&feedback_paint_device);

	// Start a render scope.
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	//
	// Where possible, force drawing to an off-screen render target.
	// It seems making the OpenGL context current is not enough to prevent Snow Leopard systems
	// with ATI graphics from hanging/crashing - this appears to be due to modifying/accessing the
	// main/default framebuffer (which is intimately tied to the windowing system).
	// Using an off-screen render target appears to avoid this issue.
	//
	// Set the off-screen render target to the size of the QGLWidget main framebuffer.
	// This is because we use QPainter to render text and it sets itself up using the dimensions
	// of the main framebuffer - actually that doesn't apply when painting to a device other than
	// the main framebuffer (in our case the feedback paint device, eg, SVG) - but we'll leave the
	// restriction in for now.
	// TODO: change to a larger size render target for more efficient rendering.
	//
	// We're currently in an active QPainter so we need to let the GLRenderer know about that.
	GPlatesOpenGL::GLOffScreenContext::RenderScope off_screen_render_scope(
			*d_gl_off_screen_context.get(),
			// Convert from widget size to device pixels (used by OpenGL)...
			width() * devicePixelRatio(),
			height() * devicePixelRatio(),
			feedback_painter,
			false/*paint_device_is_framebuffer*/);

	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = off_screen_render_scope.get_renderer();

	// Set the viewport (and scissor rectangle) to the size of the feedback paint device instead
	// of the globe canvas because OpenGL feedback uses the viewport to generate projected vertices.
	// Also text rendering uses the viewport.
	// And we want all this to be positioned correctly within the feedback paint device.
	renderer->gl_viewport(0, 0,
			// Convert from widget size to device pixels (used by OpenGL)...
			feedback_paint_device.width() * feedback_paint_device.devicePixelRatio(),
			feedback_paint_device.height() * feedback_paint_device.devicePixelRatio());
	renderer->gl_scissor(0, 0,
			// Convert from widget size to device pixels (used by OpenGL)...
			feedback_paint_device.width() * feedback_paint_device.devicePixelRatio(),
			feedback_paint_device.height() * feedback_paint_device.devicePixelRatio());

	// Calculate the projection matrices associated with the feedback paint device dimensions.
	GPlatesOpenGL::GLMatrix projection_transform_include_front_half_globe;
	GPlatesOpenGL::GLMatrix projection_transform_include_rear_half_globe;
	GPlatesOpenGL::GLMatrix projection_transform_include_full_globe;
	GPlatesOpenGL::GLMatrix projection_transform_include_stars;
	GPlatesOpenGL::GLMatrix projection_transform_text_overlay;
	calc_scene_projection_transforms(
			// Using device-independent pixels (eg, widget dimensions)...
			feedback_paint_device.width(),
			feedback_paint_device.height(),
			d_view_state.get_viewport_zoom().zoom_factor(),
			projection_transform_include_front_half_globe,
			projection_transform_include_rear_half_globe,
			projection_transform_include_full_globe,
			projection_transform_include_stars,
			projection_transform_text_overlay);

	// Clear colour buffer of the framebuffer (set to transparent black).
	const GPlatesGui::Colour clear_colour(0, 0, 0, 0);

	// Render the scene to the feedback paint device.
	// This will use the main framebuffer for intermediate rendering in some cases.
	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = render_scene(
			*renderer,
			projection_transform_include_front_half_globe,
			projection_transform_include_rear_half_globe,
			projection_transform_include_full_globe,
			projection_transform_include_stars,
			projection_transform_text_overlay,
			clear_colour,
			// Using device-independent pixels (eg, widget dimensions)...
			feedback_paint_device.width(),
			feedback_paint_device.height());
}


GPlatesQtWidgets::GlobeCanvas::cache_handle_type
GPlatesQtWidgets::GlobeCanvas::render_scene(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_front_half_globe,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_rear_half_globe,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_full_globe,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_stars,
		const GPlatesOpenGL::GLMatrix &projection_transform_text_overlay,
		const GPlatesGui::Colour &clear_colour,
		int paint_device_width_in_device_independent_pixels,
		int paint_device_height_in_device_independent_pixels)
{
	PROFILE_FUNC();

	// Clear the colour buffer of the framebuffer.
	// NOTE: We leave the depth clears to class Globe since it can do multiple
	// depth buffer clears per render depending on the projection matrices it uses.
	renderer.gl_clear_color(clear_colour.red(), clear_colour.green(), clear_colour.blue(), clear_colour.alpha());
	renderer.gl_clear(GL_COLOR_BUFFER_BIT);

	// NOTE: We only set the model-view transform here.
	// The projection transform is set inside the globe renderer.
	// This is because there are two projection transforms (with differing far clip planes)
	// and the choice is determined by the globe renderer.
	renderer.gl_load_matrix(GL_MODELVIEW, d_gl_model_view_transform);

	const double viewport_zoom_factor = d_view_state.get_viewport_zoom().zoom_factor();
	const float scale = calculate_scale(
			paint_device_width_in_device_independent_pixels,
			paint_device_height_in_device_independent_pixels);
	const double device_independent_pixel_to_world_space_ratio =
			calc_device_independent_pixel_to_world_space_ratio(
					paint_device_width_in_device_independent_pixels,
					paint_device_height_in_device_independent_pixels,
					viewport_zoom_factor);


	//
	// Paint the globe and its contents.
	//
	// NOTE: We hold onto the previous frame's cached resources *while* generating the current frame
	// and then release our hold on the previous frame (by assigning the current frame's cache).
	// This just prevents a render frame from invalidating cached resources of the previous frame
	// in order to avoid regenerating the same cached resources unnecessarily each frame.
	// Since the view direction usually differs little from one frame to the next there is a lot
	// of overlap that we want to reuse (and not recalculate).
	//
	const cache_handle_type frame_cache_handle = d_globe.paint(
			renderer,
			viewport_zoom_factor,
			device_independent_pixel_to_world_space_ratio,
			scale,
			projection_transform_include_front_half_globe,
			projection_transform_include_rear_half_globe,
			projection_transform_include_full_globe,
			projection_transform_include_stars);

	// The text overlay is rendered in screen window coordinates (ie, no model-view transform needed).
	renderer.gl_load_matrix(GL_MODELVIEW, GPlatesOpenGL::GLMatrix::IDENTITY);
	// The text overlay projection transform is in device-independent pixels (suitable for QPainter).
	renderer.gl_load_matrix(GL_PROJECTION, projection_transform_text_overlay);

	// Paint the text overlay.
	// We use the paint device dimensions (and not the canvas dimensions) in case the paint device
	// is not the canvas (eg, when rendering to a larger dimension SVG paint device).
	d_text_overlay->paint(
			renderer,
			d_view_state.get_text_overlay_settings(),
			// These are widget dimensions (not device pixels)...
			paint_device_width_in_device_independent_pixels,
			paint_device_height_in_device_independent_pixels,
			scale);

	// Paint the velocity legend overlay
	d_velocity_legend_overlay->paint(
			renderer,
			d_view_state.get_velocity_legend_overlay_settings(),
			// These are widget dimensions (not device pixels)...
			paint_device_width_in_device_independent_pixels,
			paint_device_height_in_device_independent_pixels,
			scale);

	return frame_cache_handle;
}


void
GPlatesQtWidgets::GlobeCanvas::paintEvent(
		QPaintEvent *paint_event)
{
// This paintEvent() method should be enabled, and the paintGL method disabled, when we wish to use Qt overpainting
//  ( http://doc.trolltech.com/4.3/opengl-overpainting.html )

	QGLWidget::paintEvent(paint_event);

	// Explicitly swap the OpenGL front and back buffers.
	// Note that we have already disabled auto buffer swapping because otherwise both the QPainter
	// in 'paintGL()' and 'QGLWidget::paintEvent()' will call 'QGLWidget::swapBuffers()'
	// essentially canceling each other out (or causing flickering).
	if (doubleBuffer() && !autoBufferSwap())
	{
		swapBuffers();
	}

	// If d_mouse_press_info is not boost::none, then mouse is down.
	Q_EMIT repainted(static_cast<bool>(d_mouse_press_info));
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

	Q_EMIT mouse_pressed(
		d_mouse_press_info->d_mouse_pointer_pos,
		d_globe.orient(d_mouse_press_info->d_mouse_pointer_pos),
		d_mouse_press_info->d_is_on_globe,
		d_mouse_press_info->d_button,
		d_mouse_press_info->d_modifiers);

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
			Q_EMIT mouse_dragged(
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
		Q_EMIT mouse_moved_without_drag(
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
		qWarning() << "Warning (GlobeCanvas::mouseReleaseEvent, "
				<< __FILE__
				<< " line "
				<< __LINE__
				<< "):\nUninitialised mouse press info!";
		return;
	}

	if (abs(release_event->x() - d_mouse_press_info->d_mouse_pointer_screen_pos_x) > 3 &&
			abs(release_event->y() - d_mouse_press_info->d_mouse_pointer_screen_pos_y) > 3) {
		d_mouse_press_info->d_is_mouse_drag = true;
	}
	if (d_mouse_press_info->d_is_mouse_drag) {
		Q_EMIT mouse_released_after_drag(
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
		Q_EMIT mouse_clicked(
				d_mouse_press_info->d_mouse_pointer_pos,
				d_globe.orient(d_mouse_press_info->d_mouse_pointer_pos),
				d_mouse_press_info->d_is_on_globe,
				d_mouse_press_info->d_button,
				d_mouse_press_info->d_modifiers);
	}
	d_mouse_press_info = boost::none;

	// Emit repainted signal with mouse_down = false so that those listeners who
	// didn't care about intermediate repaints can now deal with the repaint.
	Q_EMIT repainted(false);
}


void
GPlatesQtWidgets::GlobeCanvas::keyPressEvent(
		QKeyEvent *key_event)
{
	// Note that the arrow keys are handled here instead of being set as shortcuts
	// to the corresponding actions in ViewportWindow because when they were set as
	// shortcuts, they were interfering with the arrow keys on other widgets.
	switch (key_event->key())
	{
		case Qt::Key_Up:
			move_camera_up();
			break;

		case Qt::Key_Down:
			move_camera_down();
			break;

		case Qt::Key_Left:
			move_camera_left();
			break;

		case Qt::Key_Right:
			move_camera_right();
			break;

		default:
			QGLWidget::keyPressEvent(key_event);
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
	update_canvas();

	handle_mouse_pointer_pos_change();
}


void
GPlatesQtWidgets::GlobeCanvas::set_view() 
{
	//
	// Set up the projection transforms for the current canvas dimensions.
	//

	// Always fill up all of the available space.
	update_dimensions();

	// Update the projection matrices.
	d_gl_projection_transform_include_front_half_globe.gl_load_identity();
	d_gl_projection_transform_include_rear_half_globe.gl_load_identity();
	d_gl_projection_transform_include_full_globe.gl_load_identity();
	d_gl_projection_transform_include_stars.gl_load_identity();
	d_gl_projection_transform_text_overlay.gl_load_identity();
	calc_scene_projection_transforms(
			// Using device-independent pixels (eg, widget dimensions)...
			width(),
			height(),
			d_view_state.get_viewport_zoom().zoom_factor(),
			d_gl_projection_transform_include_front_half_globe,
			d_gl_projection_transform_include_rear_half_globe,
			d_gl_projection_transform_include_full_globe,
			d_gl_projection_transform_include_stars,
			d_gl_projection_transform_text_overlay);
}


void
GPlatesQtWidgets::GlobeCanvas::update_dimensions() 
{
	if (width() <= height())
	{
		d_smaller_dim = static_cast<GLdouble>(width());
		d_larger_dim = static_cast<GLdouble>(height());
	}
	else
	{
		d_smaller_dim = static_cast<GLdouble>(height());
		d_larger_dim = static_cast<GLdouble>(width());
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


double
GPlatesQtWidgets::GlobeCanvas::get_universe_coord_y(
		int screen_x) const
{
	// Scale the screen to a "unit square".
	double y_pos = (2.0 * screen_x - width()) / d_smaller_dim;

	return (y_pos * FRAMING_RATIO / d_view_state.get_viewport_zoom().zoom_factor());
}


double
GPlatesQtWidgets::GlobeCanvas::get_universe_coord_z(
		int screen_y) const
{
	// Scale the screen to a "unit square".
	double z_pos = (height() - 2.0 * screen_y) / d_smaller_dim;
	
	return (z_pos * FRAMING_RATIO / d_view_state.get_viewport_zoom().zoom_factor());
}


#if 0
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
#endif

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
	globe().orientation().move_camera_up(d_view_state.get_viewport_zoom().zoom_factor());
}

void
GPlatesQtWidgets::GlobeCanvas::move_camera_down()
{
	globe().orientation().move_camera_down(d_view_state.get_viewport_zoom().zoom_factor());
}

void
GPlatesQtWidgets::GlobeCanvas::move_camera_left()
{
	globe().orientation().move_camera_left(d_view_state.get_viewport_zoom().zoom_factor());
}

void
GPlatesQtWidgets::GlobeCanvas::move_camera_right()
{
	globe().orientation().move_camera_right(d_view_state.get_viewport_zoom().zoom_factor());
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
GPlatesQtWidgets::GlobeCanvas::calculate_scale(
		int paint_device_width_in_device_independent_pixels,
		int paint_device_height_in_device_independent_pixels)
{
	// Note that we use regular device *independent* sizes not high-DPI device pixels
	// (ie, not using device pixel ratio) to calculate scale because font sizes, etc, are
	// based on these coordinates (it's only OpenGL, really, that deals with device pixels).
	const int paint_device_dimension = (std::min)(
			paint_device_width_in_device_independent_pixels,
			paint_device_height_in_device_independent_pixels);
	const int min_viewport_dimension = (std::min)(width(), height());

	// If paint device is larger than the viewport then don't scale - this avoids having
	// too large point/line sizes when exporting large screenshots.
	if (paint_device_dimension >= min_viewport_dimension)
	{
		return 1.0f;
	}

	// This is useful when rendering the small colouring previews - avoids too large point/line sizes.
	return static_cast<float>(paint_device_dimension) / static_cast<float>(min_viewport_dimension);
}

void
GPlatesQtWidgets::GlobeCanvas::set_orientation(
	const GPlatesMaths::Rotation &rotation
	/*bool should_emit_external_signal */)
{
	d_globe.orientation().set_rotation(rotation);

	update_canvas();
}

boost::optional<GPlatesMaths::Rotation>
GPlatesQtWidgets::GlobeCanvas::orientation() const
{
	return d_globe.orientation().rotation();
}
