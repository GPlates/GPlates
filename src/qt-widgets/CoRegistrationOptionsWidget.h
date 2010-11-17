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

#include "CoRegLayerConfigurationDialog.h"
#include "CoRegistrationOptionsWidgetUi.h"
#include "LayerOptionsWidget.h"

#include "file-io/File.h"

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
						new CoRegLayerConfigurationDialog(d_current_visual_layer));
			}
			d_coreg_layer_config_dialog->pop_up();

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
			}

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;

		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;
		boost::shared_ptr<CoRegLayerConfigurationDialog> d_coreg_layer_config_dialog;
	};
}

#endif  // GPLATES_QTWIDGETS_COREGISTRATIONOPTIONSWIDGET_H
