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
#include <QPainterPath>
#include <QtGlobal>

#include "VelocityLegendOverlay.h"

#include "VelocityLegendOverlaySettings.h"

#include "global/GPlatesAssert.h"

#include "opengl/GLMatrix.h"
#include "opengl/GLProjectionUtils.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLViewport.h"
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

	void
	render(
			GPlatesOpenGL::GLRenderer &renderer,
			const GPlatesGui::VelocityLegendOverlaySettings &settings,
			float x,
			float y,
			double legend_width,
			double legend_height,
			int legend_margin,
			const QString &text,
			int text_width,
			double arrow_length,
			double arrow_height,
			double arrow_angle,
			float scale)
	{
		using namespace GPlatesOpenGL;

		// Before we suspend GLRenderer (and resume QPainter) we'll get the scissor rectangle
		// if scissoring is enabled and use that as a clip rectangle.
		boost::optional<GLViewport> scissor_rect;
		if (renderer.gl_get_enable(GL_SCISSOR_TEST))
		{
			scissor_rect = renderer.gl_get_scissor();
		}

		// And before we suspend GLRenderer (and resume QPainter) we'll get the viewport,
		// model-view transform and projection transform.
		const GLViewport viewport = renderer.gl_get_viewport();
		const GLMatrix model_view_transform = renderer.gl_get_matrix(GL_MODELVIEW);
		const GLMatrix projection_transform = renderer.gl_get_matrix(GL_PROJECTION);

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
					"VelocityLegendOverlay: attempted to render text using a GLRenderer that does not have a QPainter attached.");

		// The QPainter's paint device.
		const QPaintDevice *qpaint_device = qpainter->device();
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					qpaint_device,
					GPLATES_ASSERTION_SOURCE);

		const int qpaint_device_pixel_ratio = qpaint_device->devicePixelRatio();

		// Set the identity world transform since our input position is specified in *window* coordinates
		// and we don't want it transformed by the current world transform.
		qpainter->setWorldTransform(QTransform()/*identity*/);

		// Set the clip rectangle if the GLRenderer has scissor testing enabled.
		if (scissor_rect)
		{
			// Note that the scissor rectangle is in OpenGL device pixel coordinates, but parameters to QPainter
			// should be in device *independent* coordinates (hence the divide by device pixel ratio).
			//
			// Note: Using floating-point QRectF to avoid rounding to nearest 'qpaint_device_pixel_ratio' device pixel
			//       if scissor rect has, for example, odd coordinates (and device pixel ratio is the integer 2).
			qpainter->setClipRect(
					QRectF(
							scissor_rect->x() / qreal(qpaint_device_pixel_ratio),
							// Also need to convert scissor rectangle from OpenGL to Qt (ie, invert y-axis)...
							qpaint_device->height() - (scissor_rect->y() + scissor_rect->height()) / qreal(qpaint_device_pixel_ratio),
							scissor_rect->width() / qreal(qpaint_device_pixel_ratio),
							scissor_rect->height() / qreal(qpaint_device_pixel_ratio)));
		}

		// Pass our (x, y) window position through the model-view-projection transform and viewport
		// to get our new viewport coordinates.
		GLdouble win_x, win_y, win_z;
		GLProjectionUtils::glu_project(
				viewport, model_view_transform, projection_transform,
				// Convert from device-independent pixels to device pixels (used by OpenGL)...
				x * qpaint_device_pixel_ratio, y * qpaint_device_pixel_ratio, 0.0/*world_z*/,
				&win_x, &win_y, &win_z);

		// Get the Qt window coordinates.
		//
		// Note that win_x and win_y are in OpenGL device pixel coordinates, but parameters to QPainter
		// should be in device *independent* coordinates (hence the divide by device pixel ratio).
		const float qt_win_x = win_x / qpaint_device_pixel_ratio;
		// Also note that OpenGL and Qt y-axes are the reverse of each other.
		const float qt_win_y = qpaint_device->height() - win_y / qpaint_device_pixel_ratio;

		// Determine the bounding box rectangle. We also use this to position the arrow and text, whether or
		// not we draw the bounding box.
		const QRectF background_box(qt_win_x, qt_win_y, legend_width, legend_height);

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
		qpainter->drawText(background_box.center().x() - text_width / 2, background_box.bottom() - legend_margin, text);

		qpainter->setPen(settings.get_arrow_colour());
		qpainter->setBrush(QBrush(settings.get_arrow_colour()));
		double arrow_head_size = 5.0 * scale;

		QTransform transform;
		transform.translate(
				background_box.center().x(),
				background_box.top() + (arrow_height + 2 * legend_margin) / 2);
		transform.rotate(arrow_angle);

		QPainterPath arrow_painter_path;
		arrow_painter_path.moveTo(-arrow_length / 2, 0);
		arrow_painter_path.lineTo(arrow_length / 2, 0);

		arrow_painter_path.lineTo(arrow_length / 2 - arrow_head_size, -arrow_head_size);
		arrow_painter_path.lineTo(arrow_length / 2 - arrow_head_size, arrow_head_size);
		arrow_painter_path.lineTo(arrow_length / 2, 0);

		qpainter->setTransform(transform);

		qpainter->drawPath(arrow_painter_path);

		// Turn off clipping if it was turned on.
		if (scissor_rect)
		{
			qpainter->setClipRect(QRect(), Qt::NoClip);
		}
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

	// Here onwards we should be able to draw as desired with the QPainter.


	const double arrow_angle = settings.get_arrow_angle();
	const double max_arrow_length = settings.get_arrow_length();
	const double angle_rad = GPlatesMaths::convert_deg_to_rad(arrow_angle);


	// The length of an arrow representing 2 cm per year. See comments in the GlobeCanvas class for information about the FRAMING_RATIO
	double two_cm_per_year = layer_scale.get() * (std::min)(paint_device_width, paint_device_height) /
			GPlatesQtWidgets::GlobeCanvas::FRAMING_RATIO;

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
	default:
		velocity_scale = settings.get_arrow_scale();
		arrow_length = two_cm_per_year / 2. * velocity_scale;
		break;
	}

	arrow_length *= scale;

	double arrow_height = arrow_length*std::abs(std::sin(angle_rad));
	double arrow_width = arrow_length*std::abs(std::cos(angle_rad));

	int legend_margin = BOX_MARGIN * (std::min)(paint_device_width, paint_device_height);
	legend_margin = (std::max)(legend_margin,MIN_MARGIN);
	legend_margin *= scale;

	int arrow_box_width = arrow_width + 2*legend_margin;
	int arrow_box_height = arrow_height + 2*legend_margin;

	QString text = QString("%1 cm/yr").arg(velocity_scale);
	QFontMetrics fm(settings.get_scale_text_font());

#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
	int text_width = fm.horizontalAdvance(text) * scale;
#else
	int text_width = fm.width(text) * scale;
#endif
	int text_height = fm.height() * scale;

	const double legend_height = arrow_box_height + text_height + legend_margin;
	const double legend_width = (std::max)(arrow_box_width,(text_width + 2*legend_margin));

	// Scale the x and y offsets.
	const float x_offset = settings.get_x_offset() * scale;
	const float y_offset = settings.get_y_offset() * scale;

	// Work out position of text.
	//
	// Note: We're using OpenGL co-ordinates where OpenGL and Qt y-axes are the reverse of each other.
	//       We're using OpenGL because we then pass these coordinates through the OpenGL model-view-projection transform.
	float x;
	if (settings.get_anchor() == GPlatesGui::VelocityLegendOverlaySettings::TOP_LEFT ||
		settings.get_anchor() == GPlatesGui::VelocityLegendOverlaySettings::BOTTOM_LEFT)
	{
		x = x_offset;
	}
	else // TOP_RIGHT, BOTTOM_RIGHT
	{
		x = paint_device_width - x_offset - legend_width;
	}

	float y;
	if (settings.get_anchor() == GPlatesGui::VelocityLegendOverlaySettings::TOP_LEFT ||
		settings.get_anchor() == GPlatesGui::VelocityLegendOverlaySettings::TOP_RIGHT)
	{
		y = paint_device_height - y_offset;
	}
	else // BOTTOM_LEFT, BOTTOM_RIGHT
	{
		y = y_offset + legend_height;
	}

	// Render the velocity legend.
	render(
			renderer,
			settings,
			x, y,
			legend_width, legend_height, legend_margin,
			text, text_width,
			arrow_length, arrow_height, arrow_angle,
			scale);
}
