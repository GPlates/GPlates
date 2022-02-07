/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include "ReconstructScalarCoverageLayerOptionsWidget.h"

#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/ReconstructScalarCoverageLayerTask.h"
#include "app-logic/ReconstructScalarCoverageParams.h"

#include "global/GPlatesAssert.h"

#include "presentation/ReconstructScalarCoverageVisualLayerParams.h"
#include "presentation/VisualLayer.h"

#include "property-values/ValueObjectType.h"


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new ReconstructScalarCoverageLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::ReconstructScalarCoverageLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window)
{
	setupUi(this);
	scalar_type_combobox->setCursor(QCursor(Qt::ArrowCursor));

	make_signal_slot_connections();
}


void
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (layer_task_params)
		{
			// Populate the scalar type combobox with the list of scalar types, and ensure that
			// the correct one is selected.
			int scalar_type_index = -1;
			const GPlatesPropertyValues::ValueObjectType &selected_scalar_type =
					layer_task_params->get_scalar_type();

			scalar_type_combobox->clear();
			const std::vector<GPlatesPropertyValues::ValueObjectType> &scalar_types =
				layer_task_params->get_scalar_types();
			for (int i = 0; i != static_cast<int>(scalar_types.size()); ++i)
			{
				const GPlatesPropertyValues::ValueObjectType &curr_scalar_type = scalar_types[i];
				if (curr_scalar_type == selected_scalar_type)
				{
					scalar_type_index = i;
				}
				scalar_type_combobox->addItem(convert_qualified_xml_name_to_qstring(curr_scalar_type));
			}

			scalar_type_combobox->setCurrentIndex(scalar_type_index);
		}

		GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
		}
	}
}


const QString &
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::get_title()
{
	static const QString TITLE = tr("Scalar Coverage options");
	return TITLE;
}


void
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::handle_scalar_type_combobox_activated(
		const QString &text)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		// Set the scalar type in the app-logic layer params.
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (layer_task_params)
		{
			boost::optional<GPlatesPropertyValues::ValueObjectType> scalar_type =
					GPlatesModel::convert_qstring_to_qualified_xml_name<
							GPlatesPropertyValues::ValueObjectType>(text);
			if (scalar_type)
			{
				layer_task_params->set_scalar_type(scalar_type.get());
			}
		}
	}
}


void
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::make_signal_slot_connections()
{
	QObject::connect(
			scalar_type_combobox,
			SIGNAL(activated(const QString &)),
			this,
			SLOT(handle_scalar_type_combobox_activated(const QString &)));
}
