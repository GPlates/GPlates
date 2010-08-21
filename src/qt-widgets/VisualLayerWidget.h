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
 
#ifndef GPLATES_QTWIDGETS_VISUALLAYERWIDGET_H
#define GPLATES_QTWIDGETS_VISUALLAYERWIDGET_H

#include <vector>
#include <boost/weak_ptr.hpp>
#include <QLabel>
#include <QPixmap>
#include <QWidget>
#include <QFormLayout>
#include <QToolButton>
#include <QMenu>

#include "app-logic/Layer.h"

#include "VisualLayerWidgetUi.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesGui
{
	class VisualLayersProxy;
}

namespace GPlatesPresentation
{
	class ViewState;
	class VisualLayer;
}

namespace GPlatesQtWidgets
{
	// Forward declarations.
	class ElidedLabel;
	class RasterLayerOptionsWidget;
	class ReadErrorAccumulationDialog;

	namespace VisualLayerWidgetInternals
	{
		/**
		 * ToggleIcon is an icon that has two states, on and off, and can display a
		 * different icon for each of these two states.
		 *
		 * This cannot be an inner class of VisualLayerWidget because moc is unable to
		 * process inner classes.
		 */
		class ToggleIcon :
				public QLabel
		{
			Q_OBJECT
	
		public:

			ToggleIcon(
					const QPixmap &on_icon,
					const QPixmap &off_icon,
					QWidget *parent_ = NULL);

			void
			show_icon(
					bool on = true);
		
		signals:

			void
			clicked();

		protected:

			void
			mousePressEvent(
					QMouseEvent *event_);

		private:

			const QPixmap &d_on_icon;
			const QPixmap &d_off_icon;
		};


		/**
		 * Returns an icon that indicates that a layer is visually expanded.
		 */
		const QPixmap &
		get_expanded_icon();


		/**
		 * Returns an icon that indicates that a layer is visually collapsed.
		 */
		const QPixmap &
		get_collapsed_icon();


		/**
		 * Returns an icon that indicates that a layer is visible.
		 */
		const QPixmap &
		get_visible_icon();


		/**
		 * Returns an icon that indicates that a layer is hidden.
		 */
		const QPixmap &
		get_hidden_icon();


		/**
		 * Displays an existing input connection.
		 */
		class InputConnectionWidget :
				public QWidget
		{
		public:

			InputConnectionWidget(
					GPlatesGui::VisualLayersProxy &visual_layers,
					QWidget *parent_ = NULL);

			/**
			 * Causes this widget to display the @a input_connection.
			 */
			void
			set_data(
					const GPlatesAppLogic::Layer::InputConnection &input_connection);

		private:

			static
			const QPixmap &
			get_disconnect_pixmap();

			GPlatesGui::VisualLayersProxy &d_visual_layers;
			ElidedLabel *d_input_connection_label;
			QLabel *d_disconnect_icon;

			GPlatesAppLogic::Layer::InputConnection d_current_input_connection;
		};


		/**
		 * Displays the input connections on a particular input channel, and allows
		 * the user to add or remove input connections.
		 */
		class InputChannelWidget :
				public QWidget
		{
			Q_OBJECT

		public:

			InputChannelWidget(
					GPlatesGui::VisualLayersProxy &visual_layers,
					GPlatesAppLogic::ApplicationState &file_state,
					QWidget *parent_ = NULL);

			/**
			 * Causes this widget to display the @a input_connections for the input
			 * channel defined by @a input_channel_definition.
			 */
			void
			set_data(
					const GPlatesAppLogic::Layer &layer,
					const GPlatesAppLogic::Layer::input_channel_definition_type &input_channel_definition,
					const std::vector<GPlatesAppLogic::Layer::InputConnection> &input_connections);

		private slots:

			void
			handle_menu_triggered(
					QAction *action);

		private:

			void
			populate_with_feature_collections(
					const GPlatesAppLogic::Layer &layer,
					const QString &input_data_channel,
					QMenu *menu);

			void
			populate_with_layers(
					const GPlatesAppLogic::Layer &layer,
					const QString &input_data_channel,
					GPlatesAppLogic::Layer::LayerInputDataType input_data_type,
					QMenu *menu);

			GPlatesGui::VisualLayersProxy &d_visual_layers;

			GPlatesAppLogic::ApplicationState &d_application_state;

			ElidedLabel *d_input_channel_name_label;

			QWidget *d_input_connection_widgets_container;

			QToolButton *d_add_new_connection_widget;

			/**
			 * A pointer to the layout of the Qt container that holds the widgets that
			 * display input connections.
			 */
			QFormLayout *d_input_connection_widgets_layout;

			/**
			 * A pool of InputConnectionWidgets that can be used to display information
			 * about the input connections for the current input channel.
			 *
			 * Additional InputConnectionWidgets are created and added to this pool if
			 * the existing number of InputConnectionWidgets is insufficient to display
			 * all of the input connections for the input channel. However,
			 * InputConnectionWidgets are not dstroyed until 'this' widget is destroyed.
			 *
			 * InputConnectionWidget memory is managed by Qt.
			 */
			std::vector<InputConnectionWidget *> d_input_connection_widgets;
		};
	}


	/**
	 * The VisualLayerWidget displays information about a single VisualLayer, and
	 * is contained within a VisualLayersWidget.
	 */
	class VisualLayerWidget :
			public QWidget,
			protected Ui_VisualLayerWidget
	{
		Q_OBJECT
		
	public:

		explicit
		VisualLayerWidget(
				GPlatesGui::VisualLayersProxy &visual_layers,
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				QString &open_file_path,
				ReadErrorAccumulationDialog *read_errors_dialog,
				QWidget *parent_ = NULL);

		void
		set_data(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

	private slots:

		void
		handle_expand_icon_clicked();

		void
		handle_visibility_icon_clicked();

	private:

		void
		make_signal_slot_connections();

		/**
		 * Called by @a set_data to set up the input channel widgets.
		 */
		void
		set_input_channel_data(
				const GPlatesAppLogic::Layer &layer);

		GPlatesGui::VisualLayersProxy &d_visual_layers;
		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		QString &d_open_file_path;
		ReadErrorAccumulationDialog *d_read_errors_dialog;

		/**
		 * A weak pointer to the visual layer that we're currently displaying.
		 *
		 * This is invalid if @a set_data has not yet been called to provide us with
		 * a visual layer.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_visual_layer;

		/**
		 * A pointer to the expand/collapse icon on the left. For this icon, 'on'
		 * corresponds to 'expanded' and 'off' corresponds to 'collapsed'.
		 *
		 * Memory managed by Qt.
		 */
		VisualLayerWidgetInternals::ToggleIcon *d_expand_icon;

		/**
		 * A pointer to the hide/show icon on the right. For this icon, 'on'
		 * corresponds to 'visible' and 'off' corresponds to 'hidden'.
		 *
		 * Memory managed by Qt.
		 */
		VisualLayerWidgetInternals::ToggleIcon *d_visibility_icon;

		ElidedLabel *d_name_label;

		ElidedLabel *d_type_label;

		/**
		 * A pointer to the layout of the input_channels_widget.
		 *
		 * Memory managed by Qt.
		 */
		QFormLayout *d_input_channels_widget_layout;

		/**
		 * A pool of InputChannelWidgets that can be used to display information about
		 * the input channels for the current visual layer.
		 *
		 * Additional InputChannelWidgets are created and added to this pool if the
		 * existing number of InputChannelWidgets is insufficient to display all of
		 * the input channels for the visual layer. However, InputChannelWidgets are
		 * not destroyed until 'this' widget is destroyed (this shouldn't be too bad
		 * because layers should have fairly similar numbers of input channels).
		 *
		 * InputChannelWidget memory managed by Qt.
		 */
		std::vector<VisualLayerWidgetInternals::InputChannelWidget *> d_input_channel_widgets;

		RasterLayerOptionsWidget *d_raster_layer_options_widget;
	};
}

#endif  // GPLATES_QTWIDGETS_VISUALLAYERWIDGET_H
