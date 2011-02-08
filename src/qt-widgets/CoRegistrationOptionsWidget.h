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
 
#ifndef GPLATES_QTWIDGETS_COREGISTRATIONOPTIONSWIDGET_H
#define GPLATES_QTWIDGETS_COREGISTRATIONOPTIONSWIDGET_H

#include <boost/bind.hpp>

#include "CoRegLayerConfigurationDialog.h"
#include "CoRegistrationOptionsWidgetUi.h"
#include "LayerOptionsWidget.h"
#include "ResultTableDialog.h"

#include "app-logic/CoRegistrationLayerTask.h"
#include "file-io/File.h"
#include "presentation/VisualLayer.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	// Forward declaration.
	class ReadErrorAccumulationDialog;
	class ViewportWindow;

	/**
	 * CoRegistrationOptionsWidget is used to show additional options for
	 * co-registration layers in the visual layers widget.
	 */
	class CoRegistrationOptionsWidget :
			public LayerOptionsWidget,
			protected Ui_CoRegistrationOptionsWidget
	{
		Q_OBJECT
		
	public:

		static
		LayerOptionsWidget *
		create(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_)
		{
			return new CoRegistrationOptionsWidget(
					application_state,
					view_state,
					parent_);
		}


		virtual
		inline
		void
		set_data(
				const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
		{
			d_current_visual_layer = visual_layer;
			
			if(d_coreg_layer_config_dialog)
			{
				d_coreg_layer_config_dialog->set_virtual_layer(d_current_visual_layer);
			}
			else
			{
				d_coreg_layer_config_dialog.reset(
						new CoRegLayerConfigurationDialog(
								d_view_state,
								d_current_visual_layer));
			}

			boost::shared_ptr<GPlatesPresentation::VisualLayer> layer = d_current_visual_layer.lock();
				GPlatesAppLogic::CoRegistrationLayerTaskParams* params = 
					dynamic_cast<GPlatesAppLogic::CoRegistrationLayerTaskParams*> 
							(&layer->get_reconstruct_graph_layer().get_layer_task_params());
			if(params)
			{
				params->set_cfg_table(
						d_coreg_layer_config_dialog->cfg_table());
				params->set_call_back(
						boost::bind(
								&ResultTableDialog::data_arrived, d_result_dialog.get(),_1));
			}
			
		}


		virtual
		inline
		const QString &
		get_title()
		{
			static const QString TITLE = "Co-Registration options";
			return TITLE;
		}
	private:
		
	
	private slots:

		void
		handle_co_registration_configuration_button_clicked()
		{
			if(!d_coreg_layer_config_dialog)
			{
				d_coreg_layer_config_dialog.reset(
						new CoRegLayerConfigurationDialog(
								d_view_state,
								d_current_visual_layer));
			}
			d_coreg_layer_config_dialog->pop_up();

		}

		void
		handle_view_result_button_clicked()
		{
			if(!d_result_dialog)
			{
				d_result_dialog.reset(
						new ResultTableDialog(
								std::vector< DataTable >(),
								d_view_state,
								this,
								false));
			}
			d_result_dialog->show();
 			d_result_dialog->activateWindow();
 			d_result_dialog->raise();
		}

	private:
		CoRegistrationOptionsWidget(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_) :
			LayerOptionsWidget(parent_),
			d_application_state(application_state),
			d_view_state(view_state)
			{
				setupUi(this);
				QObject::connect(
						co_registration_configuration_button,
						SIGNAL(clicked()),
						this,
						SLOT(handle_co_registration_configuration_button_clicked()));

				QObject::connect(
						view_result_button,
						SIGNAL(clicked()),
						this,
						SLOT(handle_view_result_button_clicked()));
				
				if(!d_result_dialog)
				{
					d_result_dialog.reset(
							new ResultTableDialog(
									std::vector< DataTable >(),
									d_view_state,
									this,
									false));
				}
			}

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;

		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;
		boost::shared_ptr<CoRegLayerConfigurationDialog> d_coreg_layer_config_dialog;
		boost::shared_ptr<ResultTableDialog> d_result_dialog;
	};
}

#endif  // GPLATES_QTWIDGETS_COREGISTRATIONOPTIONSWIDGET_H
