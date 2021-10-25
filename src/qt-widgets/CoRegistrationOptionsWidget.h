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

#include "CoRegistrationLayerConfigurationDialog.h"
#include "CoRegistrationOptionsWidgetUi.h"
#include "LayerOptionsWidget.h"
#include "CoRegistrationResultTableDialog.h"

#include "app-logic/CoRegistrationLayerTask.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

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
					viewport_window,
					parent_);
		}


		virtual
		inline
		void
		set_data(
				const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
		{
			d_current_visual_layer = visual_layer;
			
			boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = visual_layer.lock();
			
			if(locked_visual_layer->get_reconstruct_graph_layer().get_all_inputs().size() > 0)
			{
				//disable the "view result" button if there is no "seed" input. Nothing to "view".
				view_result_button->setEnabled(true);
			}
			else
			{
				view_result_button->setEnabled(false);
			}
			//
			// Create the dialogs in 'set_data()' since it's the only place we know what
			// layer to associate with the dialogs.
			//

			if (d_coreg_layer_config_dialog)
			{
				d_coreg_layer_config_dialog->set_visual_layer(d_current_visual_layer);
			}
			else
			{
				d_coreg_layer_config_dialog.reset(
						new CoRegistrationLayerConfigurationDialog(
								d_view_state,
								d_viewport_window,
								d_current_visual_layer));
			}

			if (d_result_dialog)
			{
				d_result_dialog->set_visual_layer(d_current_visual_layer);
			}
			else
			{
				d_result_dialog.reset(
						new CoRegistrationResultTableDialog(
								d_view_state,
								d_viewport_window,
								d_current_visual_layer));
			}

			// NOTE: Each dialog is responsible for communicating with the layer when either the
			// co-registration configuration has changed or new co-registration results are available.
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
		
	
	private Q_SLOTS:

		void
		handle_co_registration_configuration_button_clicked()
		{
			// 'set_data()' should have created the dialog before it's possible for the user to
			// click the co-registration configuration button.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_coreg_layer_config_dialog,
					GPLATES_ASSERTION_SOURCE);

			d_coreg_layer_config_dialog->pop_up();

		}

		void
		handle_view_result_button_clicked()
		{
			// 'set_data()' should have created the dialog before it's possible for the user to
			// click the view result button.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_result_dialog,
					GPLATES_ASSERTION_SOURCE);
			
			d_result_dialog->pop_up();
		}

	private:
		CoRegistrationOptionsWidget(
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
				
				view_result_button->setDisabled(true);
				
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
			}

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;

		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;
		boost::shared_ptr<CoRegistrationLayerConfigurationDialog> d_coreg_layer_config_dialog;
		boost::shared_ptr<CoRegistrationResultTableDialog> d_result_dialog;
	};
}

#endif  // GPLATES_QTWIDGETS_COREGISTRATIONOPTIONSWIDGET_H
