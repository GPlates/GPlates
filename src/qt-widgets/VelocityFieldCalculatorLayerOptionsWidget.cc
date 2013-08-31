/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
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
#include <boost/static_assert.hpp>

#include "VelocityFieldCalculatorLayerOptionsWidget.h"

#include "TotalReconstructionPolesDialog.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/VelocityFieldCalculatorLayerTask.h"
#include "app-logic/VelocityParams.h"

#include "global/GPlatesAssert.h"

#include "presentation/VelocityFieldCalculatorVisualLayerParams.h"


namespace
{
	const QString HELP_SOLVE_VELOCITIES_METHOD_DIALOG_TITLE = QObject::tr("Calculating velocities");
	const QString HELP_SOLVE_VELOCITIES_METHOD_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<h3>Select the method used to calculate velocities</h3>"
			"A velocity is calculated at each point in the <i>velocity domains</i>.\n"
			"The <i>velocity domains</i> can contain point, multi-point, polyline and polygon geometries.\n"
			"<p>The options for the calculating velocity at each point in these domain geometries are:</p>"
			"<h4>Calculate velocities of surfaces:</h4>"
			"<ul>"
			"<li>At each time step the points in the <i>velocity domains</i> are intersected with "
			"the static/dynamic polygons/networks in the <i>velocity surfaces</i>. The velocities of "
			"the surfaces are then calculated at those intersecting domain points.<li>"
			"<li>The <i>velocity surfaces</i> can be static polygons or topological plate polygons/networks.</li>"
			"<li>To use this option, layers containing the surfaces should be connected to the "
			"<i>velocity surfaces</i> layer input.<li>"
			"</ul>"
			"<h4>Calculate velocities of domain points:</h4>\n"
			"<ul>"
			"<li>Any layers currently connected to the <i>velocity surfaces</i> layer input are <b>ignored</b>.<li>"
			"<li>The velocities of the domain points are then calculated as they reconstruct through time.<li>"
			"</ul>"
			"</body></html>\n");

	const QString HELP_ARROW_SPACING_DIALOG_TITLE = QObject::tr("Spacing between arrows");
	const QString HELP_ARROW_SPACING_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>This parameter limits the number of velocity arrows that can be displayed on the screen or monitor.</p>"
			"<p>This is achieved by dividing the globe into equal area regions where the area of each region "
			"is controlled by this parameter. If there is more than one arrow in a region then only the arrow closest to "
			"the centre of the region is displayed and this rule is repeated for each region. "
			"In this way only a limited number of arrows are rendered and they are distributed evenly across the globe.</p>"
			"<p>The density of arrows on the screen is <i>independent</i> of the zoom level. "
			"That is, the number of arrows per unit screen area remains constant across the zoom levels.</p>"
			"<p>Select the 'X' button to remove any limit to the number of arrows on the screen.</p>"
			"</body></html>\n");

	const QString HELP_ARROW_SCALE_DIALOG_TITLE = QObject::tr("Arrow body and head scaling");
	const QString HELP_ARROW_SCALE_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>These parameters control the scaling of arrows (both the body and the head).</p>"
			"<p>Both parameters are specified as log10(scale) which has a range of [-3, 0] corresponding "
			"to a 'scale' range of [0.001, 1.0]. A scale of 1.0 (or log10 of 0.0) renders a velocity "
			"of 2cm/year such that it is about as high or wide as the GPlates viewport.</p>"
			"<p>The scaling of arrows on the screen is <i>independent</i> of the zoom level. "
			"That is, the size of the arrows on the screen remains constant across the zoom levels.</p>"
			"</body></html>\n");

	const QString HELP_VELOCITY_SMOOTHING_DIALOG_TITLE = QObject::tr("Plate boundary velocity smoothing");
	const QString HELP_VELOCITY_SMOOTHING_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>If enabled, specifies the angular distance (degrees) over which velocities are smoothed "
			"across a plate/network boundary.</p>"
			"<p>Any domain points that lie within this distance from a boundary will have their velocity "
			"smoothed across this region to minimize velocity discontinuities across a plate boundary.</p>"
			"</body></html>\n");
}


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new VelocityFieldCalculatorLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::VelocityFieldCalculatorLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window),
	d_help_solve_velocities_method_dialog(
			new InformationDialog(
					HELP_SOLVE_VELOCITIES_METHOD_DIALOG_TEXT,
					HELP_SOLVE_VELOCITIES_METHOD_DIALOG_TITLE,
					viewport_window)),
	d_help_arrow_spacing_dialog(
			new InformationDialog(
					HELP_ARROW_SPACING_DIALOG_TEXT,
					HELP_ARROW_SPACING_DIALOG_TITLE,
					viewport_window)),
	d_help_arrow_scale_dialog(
			new InformationDialog(
					HELP_ARROW_SCALE_DIALOG_TEXT,
					HELP_ARROW_SCALE_DIALOG_TITLE,
					viewport_window)),
	d_help_velocity_smoothing_dialog(
			new InformationDialog(
					HELP_VELOCITY_SMOOTHING_DIALOG_TEXT,
					HELP_VELOCITY_SMOOTHING_DIALOG_TITLE,
					viewport_window))
{
	setupUi(this);
	solve_velocities_method_combobox->setCursor(QCursor(Qt::ArrowCursor));
	push_button_help_solve_velocities_method->setCursor(QCursor(Qt::ArrowCursor));
	arrow_spacing_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	push_button_help_arrow_spacing->setCursor(QCursor(Qt::ArrowCursor));
	push_button_unlimited_arrow_spacing->setCursor(QCursor(Qt::ArrowCursor));
	arrow_body_scale_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	arrowhead_scale_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	push_button_help_arrow_scale->setCursor(QCursor(Qt::ArrowCursor));
	velocity_smoothing_check_box->setCursor(QCursor(Qt::ArrowCursor));
	velocity_smoothing_distance_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	push_button_help_velocity_smoothing->setCursor(QCursor(Qt::ArrowCursor));

	QObject::connect(
			solve_velocities_method_combobox, SIGNAL(activated(int)),
			this, SLOT(handle_solve_velocity_method_combobox_activated(int)));
	QObject::connect(
			arrow_spacing_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_arrow_spacing_value_changed(double)));
	QObject::connect(
			push_button_unlimited_arrow_spacing, SIGNAL(clicked()),
			this, SLOT(handle_unlimited_arrow_spacing_clicked()));
	QObject::connect(
			arrow_body_scale_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_arrow_body_scale_value_changed(double)));
	QObject::connect(
			arrowhead_scale_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_arrowhead_scale_value_changed(double)));
	QObject::connect(
			velocity_smoothing_check_box, SIGNAL(stateChanged(int)),
			this, SLOT(handle_velocity_smoothing_check_box_changed()));
	QObject::connect(
			velocity_smoothing_distance_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_velocity_smoothing_distance_spinbox_changed(double)));

	// Connect the help dialogs.
	QObject::connect(
			push_button_help_solve_velocities_method, SIGNAL(clicked()),
			d_help_solve_velocities_method_dialog, SLOT(show()));
	QObject::connect(
			push_button_help_arrow_spacing, SIGNAL(clicked()),
			d_help_arrow_spacing_dialog, SLOT(show()));
	QObject::connect(
			push_button_help_arrow_scale, SIGNAL(clicked()),
			d_help_arrow_scale_dialog, SLOT(show()));
	QObject::connect(
			push_button_help_velocity_smoothing, SIGNAL(clicked()),
			d_help_velocity_smoothing_dialog, SLOT(show()));
}


void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;

	// Set the state of the checkboxes.
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (layer_task_params)
		{
			// Update this source code if more 'solve velocities' enumeration values have been added (or removed).
			BOOST_STATIC_ASSERT(GPlatesAppLogic::VelocityParams::NUM_SOLVE_VELOCITY_METHODS == 2);

			const GPlatesAppLogic::VelocityParams &velocity_params =
					layer_task_params->get_velocity_params();

			// Populate the 'solve velocities' combobox.
			solve_velocities_method_combobox->clear();
			for (int i = 0; i < GPlatesAppLogic::VelocityParams::NUM_SOLVE_VELOCITY_METHODS; ++i)
			{
				const GPlatesAppLogic::VelocityParams::SolveVelocitiesMethodType solve_velocities_method =
						static_cast<GPlatesAppLogic::VelocityParams::SolveVelocitiesMethodType>(i);

				// "Calculate velocities "....
				switch (solve_velocities_method)
				{
				case GPlatesAppLogic::VelocityParams::SOLVE_VELOCITIES_OF_SURFACES_AT_DOMAIN_POINTS:
					solve_velocities_method_combobox->addItem("of surfaces");
					break;
				case GPlatesAppLogic::VelocityParams::SOLVE_VELOCITIES_OF_DOMAIN_POINTS:
					solve_velocities_method_combobox->addItem("of domain points");
					break;
				default:
					GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
					break;
				}
			}

			solve_velocities_method_combobox->setCurrentIndex(velocity_params.get_solve_velocities_method());

			QObject::disconnect(
					velocity_smoothing_check_box, SIGNAL(stateChanged(int)),
					this, SLOT(handle_velocity_smoothing_check_box_changed()));
			velocity_smoothing_check_box->setChecked(velocity_params.get_is_boundary_smoothing_enabled());
			QObject::connect(
					velocity_smoothing_check_box, SIGNAL(stateChanged(int)),
					this, SLOT(handle_velocity_smoothing_check_box_changed()));

			QObject::disconnect(
					velocity_smoothing_distance_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_velocity_smoothing_distance_spinbox_changed(double)));
			velocity_smoothing_distance_spinbox->setValue(
					velocity_params.get_boundary_smoothing_angular_half_extent_degrees());
			QObject::connect(
					velocity_smoothing_distance_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_velocity_smoothing_distance_spinbox_changed(double)));

			// Only enable velocity smoothing controls if velocity smoothing is enabled.
			velocity_smoothing_controls->setEnabled(velocity_params.get_is_boundary_smoothing_enabled());

			// Only show velocity smoothing options if solve-velocities-of-surfaces is selected.
			velocity_smoothing_widget->setVisible(
					velocity_params.get_solve_velocities_method() ==
						GPlatesAppLogic::VelocityParams::SOLVE_VELOCITIES_OF_SURFACES_AT_DOMAIN_POINTS);
		}

		GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			// Setting values in a spin box will emit signals if the value changes
			// which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect the signals.

			QObject::disconnect(
					arrow_spacing_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_arrow_spacing_value_changed(double)));
			arrow_spacing_spinbox->setValue(visual_layer_params->get_arrow_spacing());
			QObject::connect(
					arrow_spacing_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_arrow_spacing_value_changed(double)));

			QObject::disconnect(
					arrow_body_scale_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_arrow_body_scale_value_changed(double)));
			// Convert from scale to log10(scale).
			arrow_body_scale_spinbox->setValue(std::log10(visual_layer_params->get_arrow_body_scale()));
			QObject::connect(
					arrow_body_scale_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_arrow_body_scale_value_changed(double)));

			QObject::disconnect(
					arrowhead_scale_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_arrowhead_scale_value_changed(double)));
			// Convert from scale to log10(scale).
			arrowhead_scale_spinbox->setValue(std::log10(visual_layer_params->get_arrowhead_scale()));
			QObject::connect(
					arrowhead_scale_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_arrowhead_scale_value_changed(double)));
		}
	}
}


const QString &
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::get_title()
{
	static const QString TITLE = tr("Velocity & Interpolation options");
	return TITLE;
}


void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::handle_solve_velocity_method_combobox_activated(
		int index)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (layer_task_params)
		{
			if (index < 0 || index >= GPlatesAppLogic::VelocityParams::NUM_SOLVE_VELOCITY_METHODS)
			{
				return;
			}

			// If the combobox choice has not changed then return early.
			if (index == layer_task_params->get_velocity_params().get_solve_velocities_method())
			{
				return;
			}

			GPlatesAppLogic::VelocityParams velocity_params = layer_task_params->get_velocity_params();

			velocity_params.set_solve_velocities_method(
					static_cast<GPlatesAppLogic::VelocityParams::SolveVelocitiesMethodType>(index));

			layer_task_params->set_velocity_params(velocity_params);

			// Only show velocity smoothing options if solve-velocities-of-surfaces is selected.
			velocity_smoothing_widget->setVisible(
					velocity_params.get_solve_velocities_method() ==
						GPlatesAppLogic::VelocityParams::SOLVE_VELOCITIES_OF_SURFACES_AT_DOMAIN_POINTS);
		}
	}
}


void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::handle_arrow_spacing_value_changed(
		double arrow_spacing)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_arrow_spacing(arrow_spacing);
		}
	}
}


void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::handle_unlimited_arrow_spacing_clicked()
{
	// Set to the minimum value.
	// This will also display the special value text of "Not limited".
	arrow_spacing_spinbox->setValue(0);
}


void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::handle_arrow_body_scale_value_changed(
		double arrow_body_scale_log10)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			// Convert from log10(scale) to scale.
			const float arrow_body_scale = static_cast<float>(std::pow(10.0, arrow_body_scale_log10));

			params->set_arrow_body_scale(arrow_body_scale);
		}
	}
}


void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::handle_arrowhead_scale_value_changed(
		double arrowhead_scale_log10)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			// Convert from log10(scale) to scale.
			const float arrowhead_scale = static_cast<float>(std::pow(10.0, arrowhead_scale_log10));

			params->set_arrowhead_scale(arrowhead_scale);
		}
	}
}


void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::handle_velocity_smoothing_check_box_changed()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (layer_task_params)
		{
			GPlatesAppLogic::VelocityParams velocity_params = layer_task_params->get_velocity_params();

			velocity_params.set_is_boundary_smoothing_enabled(
					velocity_smoothing_check_box->isChecked());

			layer_task_params->set_velocity_params(velocity_params);

			// Only enable velocity smoothing controls if velocity smoothing is enabled.
			velocity_smoothing_controls->setEnabled(velocity_params.get_is_boundary_smoothing_enabled());
		}
	}
}


void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::handle_velocity_smoothing_distance_spinbox_changed(
		double value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (layer_task_params)
		{
			GPlatesAppLogic::VelocityParams velocity_params = layer_task_params->get_velocity_params();

			velocity_params.set_boundary_smoothing_angular_half_extent_degrees(value);

			layer_task_params->set_velocity_params(velocity_params);
		}
	}
}
