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
#include "gui/GlobeCamera.h"
#include "gui/GlobeVisibilityTester.h"
#include "gui/SimpleGlobeOrientation.h"
#include "gui/TextOverlay.h"
#include "gui/VelocityLegendOverlay.h"

#include "maths/LatLonPoint.h"
#include "maths/Rotation.h"
#include "maths/types.h"
#include "maths/UnitVector3D.h"

#include "model/FeatureHandle.h"

#include "opengl/GLContext.h"
#include "opengl/GLContextImpl.h"
#include "opengl/GLImageUtils.h"
#include "opengl/GLIntersect.h"
#include "opengl/GLIntersectPrimitives.h"
#include "opengl/GLProjection.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLTileRender.h"

#include "presentation/ViewState.h"

#include "utils/Profile.h"
#include "utils/UnicodeStringUtils.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace 
{
	/**
	 * Given the scene view's dimensions (eg, canvas dimensions) generate projection transforms
	 * needed to display the scene, and generate the orthographic/perspective view transform.
	 *
	 * NOTE: The view/projection transforms are post-multiplied (ie, not initialised to identity first)
	 * so set them to identity first, if necessary, otherwise the previous values will be included.
	 */
	void
	calc_scene_view_projection_transforms(
			unsigned int viewport_width_in_device_pixels,
			unsigned int viewport_height_in_device_pixels,
			const GPlatesGui::GlobeCamera &camera,
			GPlatesOpenGL::GLMatrix &view_transform,
			// Note that these projection transforms are 'orthographic' or 'perspective', and hence are
			// only affected by viewport *aspect ratio*, so they are independent of whether we're using
			// device pixels or device *independent* pixels...
			GPlatesOpenGL::GLMatrix &projection_transform_include_front_half_globe,
			GPlatesOpenGL::GLMatrix &projection_transform_include_rear_half_globe,
			GPlatesOpenGL::GLMatrix &projection_transform_include_full_globe,
			GPlatesOpenGL::GLMatrix &projection_transform_include_stars,
			// This is the only projection transform affected by viewport *pixel* dimensions,
			// so it depends on whether we're using device pixels or device *independent* pixels.
			// Here we need to use device pixels (since using OpenGL, and its viewport is in device pixels)..
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

		const GPlatesMaths::UnitVector3D &camera_view_direction = camera.get_view_direction();
		const GPlatesMaths::UnitVector3D &camera_look_at = camera.get_look_at_position();
		const GPlatesMaths::UnitVector3D &camera_up = camera.get_up_direction();

		if (camera.get_projection_type() == GPlatesGui::GlobeProjection::ORTHOGRAPHIC)
		{
			//
			// View transform.
			//
			// Note that, for 'orthographic' viewing (as opposed to 'perspective'), the 'eye' can be anywhere
			// along the view direction. This is because the view rays are parallel and hence only the direction
			// matters (not the position). The position does affect the near/far clip plane distances, but
			// they're all adjusted based on the eye position anyway, so the near/far clip planes always end up
			// in the correct position regardless of the eye position.
			// The end result is moving the eye position along the view direction does not affect the rendered scene
			// for 'orthographic' viewing (whereas it does affect the scene for 'perspective' viewing).
			//
			// We'll just (arbitrarily) choose the eye position to be the look-at position moved back one unit along the view direction.
			const GPlatesMaths::Vector3D camera_eye = GPlatesMaths::Vector3D(camera_look_at) - GPlatesMaths::Vector3D(camera_view_direction);
			view_transform.glu_look_at(
					camera_eye.x().dval(), camera_eye.y().dval(), camera_eye.z().dval(), // eye
					camera_look_at.x().dval(), camera_look_at.y().dval(), camera_look_at.z().dval(), // centre
					camera_up.x().dval(), camera_up.y().dval(), camera_up.z().dval()); // up

			// Distance from eye to globe centre projected along the view direction.
			const GPlatesMaths::Vector3D globe_centre(0, 0, 0);
			const GLdouble eye_to_globe_centre_distance_along_view_direction = dot(globe_centre - camera_eye, camera_view_direction).dval();

			// The 1.0 is the globe radius.
			// The 0.5 is arbitrary and because we don't want to put the near clipping plane too close
			// to the globe because some objects are outside the globe such as rendered arrows.
			// Same applies to the far clipping plane.
			const GLdouble depth_in_front_of_globe = eye_to_globe_centre_distance_along_view_direction - 1.0 - 0.5;
			const GLdouble depth_behind_globe = eye_to_globe_centre_distance_along_view_direction + 1.0 + 0.5;
			// The stars need a far clip plane further away.
			const GLdouble depth_behind_globe_and_including_stars = depth_behind_globe + 10;
			const GLdouble depth_globe_centre = eye_to_globe_centre_distance_along_view_direction;
			// The 'depth_globe_centre' will need adjustment so that the circumference of the globe doesn't get clipped.
			// Also the opaque sphere is now rendered as a flat disk facing the camera and
			// positioned through the globe centre - so we don't want that to get clipped away either.
			const GLdouble depth_epsilon_to_avoid_clipping_globe_circumference = 0.0001;

			//
			// Projection transform.
			//
			// Note that, counter-intuitively, zooming into an orthographic view is not accomplished by moving
			// the eye closer to the globe. Instead it's accomplished by reducing the width and height of
			// the orthographic viewing frustum (rectangular prism).
			//

			// The aspect ratio (width/height) of the screen.
			const double aspect_ratio = double(viewport_width_in_device_pixels) / viewport_height_in_device_pixels;

			double ortho_left;
			double ortho_right;
			double ortho_bottom;
			double ortho_top;
			camera.get_orthographic_left_right_bottom_top(
					aspect_ratio,
					ortho_left, ortho_right, ortho_bottom, ortho_top);

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
		}
		else // perspective...
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					camera.get_projection_type() == GPlatesGui::GlobeProjection::PERSPECTIVE,
					GPLATES_ASSERTION_SOURCE);

			//
			// View transform.
			//
			// In contrast to orthographic viewing, zooming in 'perspective' viewing is accomplished by moving the eye position.
			// Alternatively zooming could also be accomplished by narrowing the field-of-view, but it's better
			// to keep the field-of-view constant since that is how we view the real world with the naked eye
			// (as opposed to a telephoto lens where the viewing rays become more parallel with greater zoom).
			//
			const GPlatesMaths::Vector3D camera_eye = camera.get_perspective_eye_position();
			view_transform.glu_look_at(
					camera_eye.x().dval(), camera_eye.y().dval(), camera_eye.z().dval(), // eye
					camera_look_at.x().dval(), camera_look_at.y().dval(), camera_look_at.z().dval(), // centre
					camera_up.x().dval(), camera_up.y().dval(), camera_up.z().dval()); // up

			// Distance from eye to globe centre projected along the view direction.
			const GPlatesMaths::Vector3D globe_centre(0, 0, 0);
			const GLdouble eye_to_globe_centre_distance_along_view_direction =
					dot(globe_centre - camera_eye, camera_view_direction).dval();

			//
			// For perspective viewing it's generally advised to keep the near plane as far away as
			// possible in order to get better precision from the depth buffer (ie, quantised 32-bit
			// depths in hardware depth buffer, or 24 bits if using 8 bits for stencil, spread over a
			// shorter near-to-far distance).
			// Actually most of the loss of precision occurs in the far distance (since it's essentially
			// the post-projection '1/z' that's quantised into the 32-bit depth buffer), so depths
			// close to the near clip plane get mapped to more quantised values than further away.
			// So if there's any z-fighting (different objects mapped to same depth buffer value)
			// it'll happen more at the back of the globe where it's not as noticeable
			// (since projected to a smaller area in viewport).
			// According to https://www.khronos.org/opengl/wiki/Depth_Buffer_Precision the z value
			// in eye coordinates (ie, eye at z=0) is related to the near 'n' and far 'f' distances
			// and the integer z-buffer value 'z_w' and the number of integer depth buffer values 's':
			//
			//   z_eye =         f * n
			//           -----------------------
			//           (z_w / s) * (f - n) - f
			//
			// ...for z_w equal to 0 and s this gives a z_eye of -n and -f.
			//
			// Since the eye position moves quite close to the globe in perspective view
			// (to accomplish viewport zooming) we also don't want to clip away the closest part of the globe,
			// and we don't want to clip away any objects sticking outside the globe as much as we
			// can avoid it (such as rendered arrows). So we can't keep the near plane too far away.
			// If we set it to half the distance between the eye and the globe at maximum zoom (factor 1000)
			// then near distance n=0.5*(1.4-1.0)/1000=0.0002. And if, for a far distance f=(1.4 + 1.0 + 0.5)=2.9
			// and s=2^24 for a 24-bit depth buffer, we plug in the z_w values of 0, 1 and s, s-1
			// (which are the two closest and two furthest integer z-buffer values respectively)
			// then we get z_eye(0)-z_eye(1) = 1.2e-11 and z_eye(s)-z_eye(s-1) = 2.5e-3.
			// This corresponds to 0.08mm and 16km respectively.
			// This also shows we get about 2e+8 times more z-buffer precision at the near plane compared to the far plane.
			//

			const GLdouble depth_in_front_of_globe = 0.001;  // ~6km (z-buffer resolution: ~0.37mm near and ~3.2km far)
			// The 1.0 is the globe radius.
			// The 0.5 is arbitrary and because we don't want to put the far clipping plane too close
			// to the globe because some objects are outside the globe such as rendered arrows.
			// Note that this only really matters if you can see them (if the globe is translucent).
			const GLdouble depth_behind_globe = eye_to_globe_centre_distance_along_view_direction + 1.0 + 0.5;
			// The stars need a far clip plane further away.
			const GLdouble depth_behind_globe_and_including_stars = depth_behind_globe + 10;
			const GLdouble depth_globe_centre = eye_to_globe_centre_distance_along_view_direction;
			// The 'depth_globe_centre' will need adjustment so that the circumference of the globe doesn't get clipped.
			// Also the opaque sphere is now rendered as a flat disk facing the camera and
			// positioned through the globe centre - so we don't want that to get clipped away either.
			const GLdouble depth_epsilon_to_avoid_clipping_globe_circumference = 0.0001;

			// The aspect ratio (width/height) of the screen.
			const double aspect_ratio = double(viewport_width_in_device_pixels) / viewport_height_in_device_pixels;

			double fovy_degrees;
			camera.get_perspective_fovy(aspect_ratio, fovy_degrees);

			//
			// Projection transforms.
			//

			projection_transform_include_front_half_globe.glu_perspective(
					fovy_degrees,
					aspect_ratio,
					depth_in_front_of_globe,
					depth_globe_centre + depth_epsilon_to_avoid_clipping_globe_circumference);

			projection_transform_include_rear_half_globe.glu_perspective(
					fovy_degrees,
					aspect_ratio,
					depth_globe_centre - depth_epsilon_to_avoid_clipping_globe_circumference,
					depth_behind_globe);

			projection_transform_include_full_globe.glu_perspective(
					fovy_degrees,
					aspect_ratio,
					depth_in_front_of_globe,
					depth_behind_globe);

			projection_transform_include_stars.glu_perspective(
					fovy_degrees,
					aspect_ratio,
					depth_in_front_of_globe,
					depth_behind_globe_and_including_stars);
		}

		// The text overlay coordinates are specified in window coordinates.
		// The near and far values only need to include z=0 so [-1,1] will do fine.
		projection_transform_text_overlay.gl_ortho(
				0, viewport_width_in_device_pixels,
				0, viewport_height_in_device_pixels,
				-1,
				1);
	}
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
			colour_scheme),
	d_globe_camera(view_state.get_globe_camera()),
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
			colour_scheme_),
	d_globe_camera(existing_globe_canvas->d_globe_camera),
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

	// Update our view whenever the camera changes.
	//
	// Note that the camera is updated when the zoom changes.
	QObject::connect(
			&d_globe_camera, SIGNAL(camera_changed()),
			this, SLOT(handle_camera_change()));

	handle_camera_change();

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
	// Say we pick an epsilon radius of 3 pixels around the click position.
	// The larger this radius, the more relaxed the proximity inclusion threshold.
	//
	// FIXME:  Do we want this constant to instead be a variable set by a per-user preference,
	// to enable users to specify their own epsilon radius?  (For example, users with shaky
	// hands or very high-resolution displays might prefer a larger epsilon radius.)
	const double pixel_inclusion_threshold = 3.0;
	// Limit the maximum angular distance on unit sphere. When the click point is at the circumference
	// of the visible globe, a one viewport pixel variation can result in a large traversal on the
	// globe since the globe surface is tangential to the view there.
	const double max_distance_inclusion_threshold = GPlatesMaths::convert_deg_to_rad(5.0);

	// At the circumference of the visible globe we see the globe surface of the globe almost-tangentially.
	// At these locations, a small mouse-pointer displacement on-screen will result in a significantly
	// larger mouse-pointer displacement on the surface of the globe than would the equivalent
	// mouse-pointer away from the visible circumference.
	//
	// To take this into account we use the current view and projection transforms (and viewport) to
	// project one screen pixel area onto the globe and find the maximum deviation of this area
	// projected onto the globe (in terms of angular distance on the globe).
	//
	// Calculate the maximum distance on the unit-sphere subtended by one viewport pixel projected onto it.
	GPlatesOpenGL::GLProjection gl_projection(
			// Note: We don't multiply dimensions by device-pixel-ratio since we want our max pixel size to be
			// in device *independent* coordinates. This way if a user has a high DPI display (like Apple Retina)
			// the higher pixel resolution does not force them to have more accurate mouse clicks...
			GPlatesOpenGL::GLViewport(0, 0, width(), height()),
			d_gl_view_transform,
			// Also note that this projection transform is 'orthographic' or 'perspective', and hence is
			// only affected by viewport *aspect ratio*, so it is independent of whether we're using
			// device pixels or device *independent* pixels...
			d_gl_projection_transform_include_full_globe);
	boost::optional< std::pair<double/*min*/, double/*max*/> > min_max_pixel_size =
			gl_projection.get_min_max_pixel_size_on_unit_sphere(click_point.position_vector());
	// If unable to determine maximum pixel size then just return the maximum allowed proximity threshold.
	if (!min_max_pixel_size)
	{
		return std::cos(max_distance_inclusion_threshold);  // Proximity threshold is expected to be a cosine.
	}

	// Multiply the inclusive distance on unit-sphere (associated with one viewport pixel) by the
	// number of inclusive viewport pixels.
	double distance_inclusion_threshold = pixel_inclusion_threshold * min_max_pixel_size->second/*max*/;

	// Clamp to range to the maximum distance inclusion threshold (if necessary).
	if (distance_inclusion_threshold > max_distance_inclusion_threshold)
	{
		distance_inclusion_threshold = max_distance_inclusion_threshold;
	}

	// Proximity threshold is expected to be a cosine.
	return std::cos(distance_inclusion_threshold);
}


GPlatesMaths::PointOnSphere
GPlatesQtWidgets::GlobeCanvas::centre_of_viewport() const
{
	return GPlatesMaths::PointOnSphere(d_globe_camera.get_look_at_position());
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
	std::pair<bool, GPlatesMaths::PointOnSphere> new_pos_result =
			calc_virtual_globe_position(
					d_mouse_pointer_screen_pos_x,
					d_mouse_pointer_screen_pos_y);

	bool is_now_on_globe = new_pos_result.first;
	const GPlatesMaths::PointOnSphere &new_pos = new_pos_result.second;

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
	std::pair<bool, GPlatesMaths::PointOnSphere> new_pos_result =
			calc_virtual_globe_position(
					d_mouse_pointer_screen_pos_x,
					d_mouse_pointer_screen_pos_y);

	bool is_now_on_globe = new_pos_result.first;
	const GPlatesMaths::PointOnSphere &new_pos = new_pos_result.second;

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
	// that call our 'initializeGL()' method - it's now also when a client wants to render the
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

	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = render_scene(
			*renderer,
			d_gl_view_transform,
			d_gl_projection_transform_include_front_half_globe,
			d_gl_projection_transform_include_rear_half_globe,
			d_gl_projection_transform_include_full_globe,
			d_gl_projection_transform_include_stars,
			d_gl_projection_transform_text_overlay,
			*this);
}


QSize
GPlatesQtWidgets::GlobeCanvas::get_viewport_size() const
{
	return QSize(width(), height());
}


QSize
GPlatesQtWidgets::GlobeCanvas::get_viewport_size_in_device_pixels() const
{
	// QWidget::width() and QWidget::height() are in device independent pixels.
	// Convert from widget size to device pixels (OpenGL).
	return devicePixelRatio() * get_viewport_size();
}


QImage
GPlatesQtWidgets::GlobeCanvas::render_to_qimage(
		const QSize &image_size)
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

	// The image to render the scene into.
	QImage image(image_size, QImage::Format_ARGB32);
	if (image.isNull())
	{
		// Most likely a memory allocation failure - return the null image.
		return QImage();
	}

	// Fill the image with transparent black in case there's an exception during rendering
	// of one of the tiles and the image is incomplete.
	image.fill(QColor(0,0,0,0).rgba());

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
					image_size.width(),
					image_size.height())/*destination_viewport*/,
			tile_border);

	// Keep track of the cache handles of all rendered tiles.
	boost::shared_ptr< std::vector<cache_handle_type> > frame_cache_handle(
			new std::vector<cache_handle_type>());

	// Render the scene tile-by-tile.
	for (tile_render.first_tile(); !tile_render.finished(); tile_render.next_tile())
	{
		// Render the scene to the feedback paint device.
		// This will use the main framebuffer for intermediate rendering in some cases.
		// Hold onto the previous frame's cached resources *while* generating the current frame.
		const cache_handle_type tile_cache_handle =	
				render_scene_tile_into_image(*renderer, tile_render, image);
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

	// Calculate the projection matrices associated with the current image dimensions.
	GPlatesOpenGL::GLMatrix tile_view_transform;
	GPlatesOpenGL::GLMatrix tile_projection_transform_include_front_half_globe(projection_matrix_tile);
	GPlatesOpenGL::GLMatrix tile_projection_transform_include_rear_half_globe(projection_matrix_tile);
	GPlatesOpenGL::GLMatrix tile_projection_transform_include_full_globe(projection_matrix_tile);
	GPlatesOpenGL::GLMatrix tile_projection_transform_include_stars(projection_matrix_tile);
	GPlatesOpenGL::GLMatrix tile_projection_transform_text_overlay(projection_matrix_tile);
	calc_scene_view_projection_transforms(
			image.width(),
			image.height(),
			d_globe_camera,
			tile_view_transform,
			tile_projection_transform_include_front_half_globe,
			tile_projection_transform_include_rear_half_globe,
			tile_projection_transform_include_full_globe,
			tile_projection_transform_include_stars,
			tile_projection_transform_text_overlay);

	//
	// Render the scene.
	//
	const cache_handle_type tile_cache_handle = render_scene(
			renderer,
			tile_view_transform,
			tile_projection_transform_include_front_half_globe,
			tile_projection_transform_include_rear_half_globe,
			tile_projection_transform_include_full_globe,
			tile_projection_transform_include_stars,
			tile_projection_transform_text_overlay,
			image);

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
	GPlatesOpenGL::GLMatrix view_transform;
	GPlatesOpenGL::GLMatrix projection_transform_include_front_half_globe;
	GPlatesOpenGL::GLMatrix projection_transform_include_rear_half_globe;
	GPlatesOpenGL::GLMatrix projection_transform_include_full_globe;
	GPlatesOpenGL::GLMatrix projection_transform_include_stars;
	GPlatesOpenGL::GLMatrix projection_transform_text_overlay;
	calc_scene_view_projection_transforms(
			// Convert from widget size to device pixels (used by OpenGL)...
			feedback_paint_device.width() * feedback_paint_device.devicePixelRatio(),
			feedback_paint_device.height() * feedback_paint_device.devicePixelRatio(),
			d_globe_camera,
			view_transform,
			projection_transform_include_front_half_globe,
			projection_transform_include_rear_half_globe,
			projection_transform_include_full_globe,
			projection_transform_include_stars,
			projection_transform_text_overlay);

	// Render the scene to the feedback paint device.
	// This will use the main framebuffer for intermediate rendering in some cases.
	// Hold onto the previous frame's cached resources *while* generating the current frame.
	d_gl_frame_cache_handle = render_scene(
			*renderer,
			view_transform,
			projection_transform_include_front_half_globe,
			projection_transform_include_rear_half_globe,
			projection_transform_include_full_globe,
			projection_transform_include_stars,
			projection_transform_text_overlay,
			feedback_paint_device);
}


GPlatesQtWidgets::GlobeCanvas::cache_handle_type
GPlatesQtWidgets::GlobeCanvas::render_scene(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesOpenGL::GLMatrix &view_transform,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_front_half_globe,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_rear_half_globe,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_full_globe,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_stars,
		const GPlatesOpenGL::GLMatrix &projection_transform_text_overlay,
		const QPaintDevice &paint_device)
{
	PROFILE_FUNC();

	// Clear the colour buffer of the main framebuffer.
	// NOTE: We leave the depth clears to class Globe since it can do multiple
	// depth buffer clears per render depending on the projection matrices it uses.
	renderer.gl_clear_color(); // Clear colour to (0,0,0,0).
	renderer.gl_clear(GL_COLOR_BUFFER_BIT);

	// NOTE: We only set the model-view transform here.
	// The projection transform is set inside the globe renderer.
	// This is because there are two projection transforms (with differing far clip planes)
	// and the choice is determined by the globe renderer.
	renderer.gl_load_matrix(GL_MODELVIEW, view_transform);

	const double viewport_zoom_factor = d_view_state.get_viewport_zoom().zoom_factor();
	const float scale = calculate_scale(paint_device);
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
			scale,
			projection_transform_include_front_half_globe,
			projection_transform_include_rear_half_globe,
			projection_transform_include_full_globe,
			projection_transform_include_stars);

	// The text overlay is rendered in screen window coordinates (ie, no model-view transform needed).
	renderer.gl_load_matrix(GL_MODELVIEW, GPlatesOpenGL::GLMatrix::IDENTITY);
	renderer.gl_load_matrix(GL_PROJECTION, projection_transform_text_overlay);

	// Paint the text overlay.
	// We use the paint device dimensions (and not the canvas dimensions) in case the paint device
	// is not the canvas (eg, when rendering to a larger dimension SVG paint device).
	d_text_overlay->paint(
			renderer,
			d_view_state.get_text_overlay_settings(),
			paint_device,
			scale);

	// Paint the velocity legend overlay
	d_velocity_legend_overlay->paint(
			renderer,
			d_view_state.get_velocity_legend_overlay_settings(),
			// These are widget dimensions (not device pixels)...
			paint_device.width(),
			paint_device.height(),
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
GPlatesQtWidgets::GlobeCanvas::handle_camera_change() 
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

	// Update the projection matrices.
	d_gl_view_transform.gl_load_identity();
	d_gl_projection_transform_include_front_half_globe.gl_load_identity();
	d_gl_projection_transform_include_rear_half_globe.gl_load_identity();
	d_gl_projection_transform_include_full_globe.gl_load_identity();
	d_gl_projection_transform_include_stars.gl_load_identity();
	d_gl_projection_transform_text_overlay.gl_load_identity();
	calc_scene_view_projection_transforms(
			// Convert from widget size to device pixels (used by OpenGL)...
			width() * devicePixelRatio(),
			height() * devicePixelRatio(),
			d_globe_camera,
			d_gl_view_transform,
			d_gl_projection_transform_include_front_half_globe,
			d_gl_projection_transform_include_rear_half_globe,
			d_gl_projection_transform_include_full_globe,
			d_gl_projection_transform_include_stars,
			d_gl_projection_transform_text_overlay);
}


void
GPlatesQtWidgets::GlobeCanvas::update_mouse_pointer_pos(
		QMouseEvent *mouse_event) 
{
	d_mouse_pointer_screen_pos_x = mouse_event->x();
	d_mouse_pointer_screen_pos_y = mouse_event->y();

	handle_mouse_pointer_pos_change();
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

std::pair<bool, GPlatesMaths::PointOnSphere>
GPlatesQtWidgets::GlobeCanvas::calc_virtual_globe_position(
		int screen_x,
		int screen_y) const
{
	// Note that OpenGL and Qt y-axes are the reverse of each other.
	screen_y = height() - screen_y;

	// Project screen coordinates into a ray into 3D scene.
	GPlatesOpenGL::GLProjection gl_projection(
			// Note: We don't multiply dimensions by device-pixel-ratio since we're using this for
			// mouse/screen coordinates which are device *independent* pixels...
			GPlatesOpenGL::GLViewport(0, 0, width(), height()),
			d_gl_view_transform,
			// Also note that this projection transform is 'orthographic' or 'perspective', and hence is
			// only affected by viewport *aspect ratio*, so it is independent of whether we're using
			// device pixels or device *independent* pixels...
			d_gl_projection_transform_include_full_globe);
	boost::optional<GPlatesOpenGL::GLIntersect::Ray> ray =
			gl_projection.project_window_coords_into_ray(screen_x, screen_y);
	if (!ray)
	{
		// Shouldn't really get here. Only happens when unable to invert model-view-projection transform.
		// Just return arbitrary point on sphere (centre of viewport) and log a warning message.
		qWarning() << "Unable to project screen pixel onto scene ray:" << __FILE__ << ":" << __LINE__;
		return std::make_pair(false, centre_of_viewport());
	}

	// Create a unit sphere in model space representing the globe.
	const GPlatesOpenGL::GLIntersect::Sphere sphere(GPlatesMaths::Vector3D(0,0,0), 1);

	// Intersect the ray with the globe.
	const boost::optional<GPlatesMaths::real_t> ray_distance_to_sphere = intersect_ray_sphere(ray.get(), sphere);

	// Did the ray intersect the globe ?
	if (ray_distance_to_sphere)
	{
		// Return the point on the sphere where the ray first intersects.
		// Due to numerical precision the ray may be slightly off the sphere so we'll
		// normalise it (otherwise can provide out-of-range for 'acos' later on).
		const GPlatesMaths::UnitVector3D projected_point_on_sphere =
				ray->get_point_on_ray(ray_distance_to_sphere.get()).get_normalisation();

		// Return true to indicate ray intersected the globe.
		return std::make_pair(true, GPlatesMaths::PointOnSphere(projected_point_on_sphere));
	}

	//
	// Screen pixel does not intersect unit sphere.
	//
	// We need to find the nearest point on the horizon (visible circumference) of the globe.
	//

	if (d_globe_camera.get_projection_type() == GPlatesGui::GlobeProjection::ORTHOGRAPHIC)
	{
		// Find the position, along ray of projected screen pixel, closest to the globe (unit sphere).
		// We'll project this towards the origin onto the globe, and consider that the horizon of the globe.
		// Note that this works well for orthographic viewing since all screen pixel rays are parallel and
		// hence all perpendicular to the horizon (circumference around globe).

		// Project line segment from globe origin to ray origin onto the ray direction.
		// This gives us the distance along ray to that point on the ray that is closest to the globe.
		const GPlatesMaths::real_t ray_distance_to_closest_point =
				dot(GPlatesMaths::Vector3D(0,0,0) - ray->get_origin(), ray->get_direction());
		const GPlatesMaths::Vector3D closest_point = ray->get_point_on_ray(ray_distance_to_closest_point);

		// Normalise the closest point, to project it towards the origin onto the unit-sphere globe.
		// Return false to indicate ray did not intersect the globe.
		return std::make_pair(false, GPlatesMaths::PointOnSphere(closest_point.get_normalisation()));
	}
	else // perspective...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_globe_camera.get_projection_type() == GPlatesGui::GlobeProjection::PERSPECTIVE,
				GPLATES_ASSERTION_SOURCE);

		// For perspective viewing we want the equivalent of a ray, emanating from the eye location,
		// that touches the globe surface tangentially - call this the horizon ray.
		// Our current ray misses the surface, so we can't use that.
		//
		// First we find the normal to the plane that contains our current ray and the globe centre.
		// The plane intersects the globe surface to form a circle. We find the point on this circle
		// that is perpendicular to the eye location (both with respect to globe centre). We then find
		// the angle between this point and the globe centre (at the eye location). We then rotate
		// this point by the angle around the globe centre (along the plane). The rotated point is
		// the horizon point where the horizon ray touches the globe surface tangentially.
		
		const GPlatesMaths::Vector3D ray_origin_to_globe_centre = GPlatesMaths::Vector3D(0, 0, 0) - ray->get_origin();
		// Note that normalisation should never fail here since the current ray is not pointing
		// at the globe centre because we already know it missed the entire globe.
		const GPlatesMaths::UnitVector3D horizon_rotation_axis =
				cross(ray_origin_to_globe_centre, ray->get_direction()).get_normalisation();
		const GPlatesMaths::UnitVector3D ray_origin_to_globe_centre_perpendicular =
				cross(horizon_rotation_axis, ray_origin_to_globe_centre).get_normalisation();
		// The 1.0 is the radius of globe (unit sphere).
		// And distance from globe centre to ray origin (eye) should be greater than 1.0 since eye is outside globe.
		const GPlatesMaths::real_t horizon_rotation_angle = asin(1.0 / ray_origin_to_globe_centre.magnitude());
		const GPlatesMaths::Rotation horizon_rotation =
				GPlatesMaths::Rotation::create(horizon_rotation_axis, horizon_rotation_angle);
		const GPlatesMaths::UnitVector3D horizon_point = horizon_rotation * ray_origin_to_globe_centre_perpendicular;

		// Return false to indicate ray did not intersect the globe.
		return std::make_pair(false, GPlatesMaths::PointOnSphere(horizon_point));
	}
}

float
GPlatesQtWidgets::GlobeCanvas::calculate_scale(
		const QPaintDevice &paint_device) const
{
	// Note that we use regular device *independent* sizes not high-DPI device pixels
	// (ie, not using device pixel ratio) to calculate scale because font sizes, etc, are
	// based on these coordinates (it's only OpenGL, really, that deals with device pixels).
	const int paint_device_dimension = (std::min)(paint_device.width(), paint_device.height());
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
