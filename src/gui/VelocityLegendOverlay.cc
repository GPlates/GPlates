/* $Id$ */

/**
 * @file
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2017 Geological Survey of Norway
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

#include <QFontMetrics>
#include <QLocale>
#include <QPainter>

#include "VelocityLegendOverlay.h"

#include "VelocityLegendOverlaySettings.h"

#include "global/GPlatesAssert.h"

#include "opengl/GLMatrix.h"
#include "opengl/GLProjectionUtils.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLViewport.h"
#include "opengl/GLText.h"
#include "opengl/OpenGLException.h"

#include "presentation/VelocityFieldCalculatorVisualLayerParams.h"
#include "presentation/ViewState.h"
#include "presentation/VisualLayers.h"

#include "qt-widgets/GlobeCanvas.h"

namespace
{

	/**
	 * @brief BOX_MARGIN - Fraction of window size used as margin around arrow.
	 */
	const double BOX_MARGIN = 0.05;

	/**
	 * @brief MIN_MARGIN - minimum margin in pixels
	 */
	const int MIN_MARGIN = 20;

	/**
	 * Returns a scaled version of the specified font.
	 *
	 * Borrowed from anonymous namespace of GLText.cc
	 */
	QFont
	scale_font(
			const QFont &font,
			float scale)
	{
		QFont ret = font;

		const qreal min_point_size = 2.0;
		qreal point_size = QFontInfo(font).pointSizeF();
		ret.setPointSizeF((std::max)(min_point_size, point_size * scale));

		return ret;
	}

	void
	set_glu_projection(
			GPlatesOpenGL::GLRenderer &renderer,
			const int &world_x,
			const int &world_y,
			const int &world_z,
			GLdouble &win_x,
			GLdouble &win_y,
			GLdouble &win_z)
	{
		using namespace GPlatesOpenGL;
		const GLViewport &viewport = renderer.gl_get_viewport();
		const GLMatrix &model_view_transform = renderer.gl_get_matrix(GL_MODELVIEW);
		const GLMatrix &projection_transform = renderer.gl_get_matrix(GL_PROJECTION);

		GLProjectionUtils::glu_project(
					viewport, model_view_transform, projection_transform,
					world_x, world_y, world_z,
					&win_x, &win_y, &win_z);

	}

	/**
	 * @brief get_scale_from_uppermost_velocity_layer
	 * @param view_state
	 * @return the scale factor (i.e. the length of an arrow representing 2cm/yr) of the
	 * last velocity layer we come across in the layers collection, i.e. the uppermost velocity layer.
	 */
	boost::optional<double>
	get_scale_from_uppermost_velocity_layer(
			const GPlatesPresentation::ViewState &view_state)
	{
		typedef std::pair<unsigned int, double> index_scale_pair_type;
		std::vector<index_scale_pair_type> velocity_layer_indices;
		const GPlatesPresentation::VisualLayers &vl = view_state.get_visual_layers();
		static const GPlatesAppLogic::LayerTaskType::Type layer_type =
				GPlatesAppLogic::LayerTaskType::VELOCITY_FIELD_CALCULATOR;

		for (unsigned int n = 0; n < vl.size() ; ++n)
		{

			boost::shared_ptr<const GPlatesPresentation::VisualLayer> visual_layer =
					vl.visual_layer_at(n).lock();
			if (visual_layer->get_layer_type() == layer_type)
			{
				GPlatesPresentation::VisualLayerParams::non_null_ptr_to_const_type params =
						visual_layer->get_visual_layer_params();
				const GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *velocity_params =
						dynamic_cast<const GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *>
						(params.get());
				if (velocity_params)
				{
					velocity_layer_indices.push_back(index_scale_pair_type(n,velocity_params->get_arrow_body_scale()));
				}
			}

		}
		BOOST_FOREACH(index_scale_pair_type x, velocity_layer_indices)
		{
			qDebug() << "Layer index: " << x.first << x.second;
		}

		/**
				  From the comments in the "Help" box in the Layer window:

						These parameters control the scaling of arrows (both the body and the head).
						Both parameters are specified as log10(scale) which has a range of [-3, 0] corresponding
						to a 'scale' range of [0.001, 1.0]. A scale of 1.0 (or log10 of 0.0) renders a velocity
						of 2cm/year such that it is about as high or wide as the GPlates viewport.

						The scaling of arrows on the screen is independent of the zoom level. That is, the size
						of the arrows on the screen remains constant across the zoom levels.
				 */

		if (!velocity_layer_indices.empty())
		{
			double sf = velocity_layer_indices.back().second;
			return sf;
		}

		return boost::none;
	}

	boost::optional<double>
	get_scale_from_selected_layer(
			const boost::weak_ptr<GPlatesPresentation::VisualLayer> &selected_visual_layer)
	{

		// The provided index should correspond to a velocity layer, but no harm in checking
		static const GPlatesAppLogic::LayerTaskType::Type layer_type =
				GPlatesAppLogic::LayerTaskType::VELOCITY_FIELD_CALCULATOR;

		boost::shared_ptr<const GPlatesPresentation::VisualLayer> visual_layer =
				selected_visual_layer.lock();

		if (!visual_layer)
		{
			return boost::none;
		}

		if (visual_layer->get_layer_type() == layer_type)
		{
			GPlatesPresentation::VisualLayerParams::non_null_ptr_to_const_type params =
					visual_layer->get_visual_layer_params();
			const GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *velocity_params =
					dynamic_cast<const GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *>
					(params.get());
			if (velocity_params)
			{
				return velocity_params->get_arrow_body_scale();
			}
		}
		return boost::none;
	}

	/**
	 * @brief get_background_qrect
	 * On return @param rect contains the QRect representing the background rectangle, given the
	 * desired parameters provided by the user via @param settings: anchor position, horizontal and vertical offsets,
	 * and width and height.
	 */
	void
	get_background_qrect(
			QRect &rect,
			const int &device_width,
			const int &device_height,
			const double &scale,
			const GPlatesGui::VelocityLegendOverlaySettings &settings,
			const double &width,
			const double &height)
	{
		GPlatesGui::VelocityLegendOverlaySettings::Anchor anchor =
				settings.get_anchor();
		double x,y;
		switch(anchor)
		{
		case GPlatesGui::VelocityLegendOverlaySettings::TOP_LEFT:
			x = settings.get_x_offset();
			y = settings.get_y_offset();
			break;
		case GPlatesGui::VelocityLegendOverlaySettings::TOP_RIGHT:
			x = device_width - settings.get_x_offset() - width*scale;
			y = settings.get_y_offset();
			break;
		case GPlatesGui::VelocityLegendOverlaySettings::BOTTOM_LEFT:
			x = settings.get_x_offset();
			y = device_height - settings.get_y_offset() - height*scale;
			break;
		case GPlatesGui::VelocityLegendOverlaySettings::BOTTOM_RIGHT:
			x = device_width - settings.get_x_offset() - width*scale;
			y = device_height - settings.get_y_offset() - height*scale;
			break;
		default:
			x = settings.get_x_offset();
			y = settings.get_y_offset();
			break;
		}

		int width_ = width*scale;
		int height_ = height*scale;
		rect.setRect(x,y,width_,height_);


	}

	/**
	 * @brief reduce_to_fit
	 * @param length and @param scale are reduced successively by factors 2, 2 and 2.5
	 * until @param length is less than or equal to @param max_width.
	 */
	void
	reduce_to_fit(
			double &length,
			double &scale,
			const double max_width)
	{
		while (length > max_width)
		{
			length /= 2.;
			scale /= 2.;
			if (length < max_width)
			{
				break;
			}
			length /= 2.;
			scale /= 2.;
			if (length < max_width)
			{
				break;
			}
			length /= 2.5;
			scale /= 2.5;
			if (length < max_width)
			{
				break;
			}
		}
	}

	void
	increase_to_fit(
			double &length,
			double &scale,
			const double max_width)
	{
		while (length < max_width)
		{
			length *= 10.;
			scale *= 10.;
		}
		reduce_to_fit(length,scale,max_width);
	}



}


GPlatesGui::VelocityLegendOverlay::VelocityLegendOverlay()
{  }


void
GPlatesGui::VelocityLegendOverlay::paint(
		GPlatesOpenGL::GLRenderer &renderer,
		const VelocityLegendOverlaySettings &settings,
		int paint_device_width,
		int paint_device_height,
		float scale)
{

	// We set up openGL and a QPainter as per the TextOverlay class.

	if (!settings.is_enabled())
	{
		return;
	}

	// This will get the scale of the last velocity-type layer we meet in the layers collection, i.e.
	// the uppermost velocity layer. Later we can add in a combo-box layer selector in the Configure... dialog.
//	boost::optional<double> layer_scale = get_scale_from_uppermost_velocity_layer(d_view_state);

	boost::optional<double> layer_scale = get_scale_from_selected_layer(settings.get_selected_velocity_layer());
	if (!layer_scale)
	{
		return;
	}

	// Scale the x and y offsets.
	float x_offset = settings.get_x_offset() * scale;
	float y_offset = settings.get_y_offset() * scale;

	GLdouble win_x, win_y, win_z;
	set_glu_projection(renderer,x_offset,y_offset,0,win_x,win_y,win_z);

	if (win_z < 0 || win_z > 1)
	{
		return;
	}

	using namespace GPlatesOpenGL;

	// Before we suspend GLRenderer (and resume QPainter) we'll get the scissor rectangle
	// if scissoring is enabled and use that as a clip rectangle.
	boost::optional<GLViewport> scissor_rect;
	if (renderer.gl_get_enable(GL_SCISSOR_TEST))
	{
		scissor_rect = renderer.gl_get_scissor();
	}

	// Suspend rendering with 'GLRenderer' so we can resume painting with 'QPainter'.
	// At scope exit we can resume rendering with 'GLRenderer'.
	//
	// We do this because the QPainter's paint engine might be OpenGL and we need to make sure
	// it's OpenGL state does not interfere with the OpenGL state of 'GLRenderer' and vice versa.
	// This also provides a means to retrieve the QPainter for rendering text.
	GLRenderer::QPainterBlockScope qpainter_block_scope(renderer);

	boost::optional<QPainter &> qpainter = qpainter_block_scope.get_qpainter();

	// We need a QPainter - one should have been specified to 'GLRenderer::begin_render'.
	GPlatesGlobal::Assert<OpenGLException>(
				qpainter,
				GPLATES_ASSERTION_SOURCE,
				"GLText: attempted to render text using a GLRenderer that does not have a QPainter attached.");

	// The QPainter's paint device.
	const QPaintDevice *qpaint_device = qpainter->device();
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				qpaint_device,
				GPLATES_ASSERTION_SOURCE);

	// Set the identity world transform since our input position is specified in *window* coordinates
	// and we don't want it transformed by the current world transform.
	qpainter->setWorldTransform(QTransform()/*identity*/);

	// Set the clip rectangle if the GLRenderer has scissor testing enabled.
	if (scissor_rect)
	{
		qpainter->setClipRect(
					scissor_rect->x(),
					// Also need to convert scissor rectangle from OpenGL to Qt (ie, invert y-axis)...
					qpaint_device->height() - scissor_rect->y() - scissor_rect->height(),
					scissor_rect->width(),
					scissor_rect->height());
	}

	// Here onwards we should be able to draw as desired with the QPainter.



	const double angle = settings.get_arrow_angle();
	const double max_arrow_length = settings.get_arrow_length();
	const double angle_rad = GPlatesMaths::convert_deg_to_rad(angle);


	// The length of an arrow representing 2 cm per year. See comments in the GlobeCanvas class for information about the FRAMING_RATIO
	double two_cm_per_year = *layer_scale * (std::min)(paint_device_width, paint_device_height)/ GPlatesQtWidgets::GlobeCanvas::FRAMING_RATIO;

	double arrow_length, velocity_scale;

	switch(settings.get_arrow_length_type())
	{
	case GPlatesGui::VelocityLegendOverlaySettings::MAXIMUM_ARROW_LENGTH:
		velocity_scale = 2. ; // cm per year
		arrow_length = two_cm_per_year;
		// If our arrow length exceeds the maximum length specified by the user, reduce it
		// (and the scale) so that the scale is the biggest multiple of 1, 2, 5, 10 etc
		// for which the arrow length is less than or equal to the max length.
		if (arrow_length > max_arrow_length)
		{
			reduce_to_fit(arrow_length, velocity_scale,max_arrow_length);
		}
		else
		{
			// If we are already within the max length, make sure we use the biggest scale possible.
			increase_to_fit(arrow_length, velocity_scale,max_arrow_length);
		}
		break;
	case GPlatesGui::VelocityLegendOverlaySettings::DYNAMIC_ARROW_LENGTH:
		velocity_scale = settings.get_arrow_scale();
		arrow_length = two_cm_per_year / 2. * velocity_scale;
		break;
	}

	double arrow_height = arrow_length*std::abs(std::sin(angle_rad));
	double arrow_width = arrow_length*std::abs(std::cos(angle_rad));

	int margin = BOX_MARGIN * (std::min)(paint_device_width, paint_device_height);;
	margin = (std::max)(margin,MIN_MARGIN);
	int arrow_box_width = arrow_width + 2*margin;
	int arrow_box_height = arrow_height + 2*margin;

	QString text = QString("%1 cm/yr").arg(velocity_scale);
	QFontMetrics fm(settings.get_scale_text_font());

	int text_width = fm.width(text);
	int text_height = fm.height()*scale;

	double height = arrow_box_height + text_height + margin;
	double width = (std::max)(arrow_box_width,(text_width + 2*margin));

	// Determine the bounding box rectangle. We also use this to position the arrow and text, whether or
	// not we draw the bounding box.
	QRect background_box;
	get_background_qrect(background_box,paint_device_width,paint_device_height,scale,settings,width,height);

	// Draw background box
	if (settings.background_enabled())
	{
		qpainter->setBrush(QBrush(settings.get_background_colour()));
		qpainter->setPen(settings.get_background_colour());
		qpainter->drawRect(background_box);
	}


	// Draw legend text
	qpainter->setPen(settings.get_scale_text_colour());
	qpainter->setFont(scale_font(settings.get_scale_text_font(), scale));
	qpainter->drawText(background_box.center().x()-text_width/2, background_box.bottom()-margin,text);

	qpainter->setPen(settings.get_arrow_colour());
	qpainter->setBrush(QBrush(settings.get_arrow_colour()));
	double arrow_head_size = 5.;

	QTransform transform;
	transform.translate(background_box.center().x(),
						background_box.top()+(arrow_height+2*margin)/2);
	transform.rotate(angle);

	QPainterPath arrow_painter_path;
	arrow_painter_path.moveTo(-arrow_length/2,0);
	arrow_painter_path.lineTo(arrow_length/2,0);

	arrow_painter_path.lineTo(arrow_length/2 - arrow_head_size, - arrow_head_size);
	arrow_painter_path.lineTo(arrow_length/2 - arrow_head_size,arrow_head_size);
	arrow_painter_path.lineTo(arrow_length/2,0);

	qpainter->setTransform(transform);

	qpainter->drawPath(arrow_painter_path);

	// Turn off clipping if it was turned on.
	if (scissor_rect)
	{
		qpainter->setClipRect(QRect(), Qt::NoClip);
	}

}




