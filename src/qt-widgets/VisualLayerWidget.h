/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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
#include <QVBoxLayout>
#include <QToolButton>
#include <QMenu>
#include <QStackedWidget>

#include "VisualLayerWidgetUi.h"

#include "app-logic/Layer.h"

#include "gui/Colour.h"


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
	class LayerOptionsWidget;
	class LinkWidget;
	class ViewportWindow;

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

			explicit
			ToggleIcon(
					const QPixmap &on_icon,
					const QPixmap &off_icon,
					bool is_clickable = true,
					bool show_frame_when_clickable = true,
					QWidget *parent_ = NULL);

			void
			show_icon(
					bool on = true);

			void
			set_clickable(
					bool is_clickable = true);
		
		Q_SIGNALS:

			void
			clicked();

		protected:

			virtual
			void
			mousePressEvent(
					QMouseEvent *event_);

			virtual
			void
			changeEvent(
					QEvent *event_);

		private:

			void
			set_cursor();

			const QPixmap &d_on_icon;
			const QPixmap &d_off_icon;
			bool d_is_clickable;
			bool d_show_frame_when_clickable;
		};


		/**
		 * Displays an existing input connection.
		 */
		class InputConnectionWidget :
				public QWidget
		{
		public:

			explicit
			InputConnectionWidget(
					GPlatesGui::VisualLayersProxy &visual_layers,
					QWidget *parent_ = NULL);

			/**
			 * Causes this widget to display the @a input_connection.
			 */
			void
			set_data(
					const GPlatesAppLogic::Layer::InputConnection &input_connection,
					const GPlatesGui::Colour &background_colour);

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
		 * A widget that allows the user to add new connections to a channel.
		 */
		class AddNewConnectionWidget :
				public QLabel
		{
		public:

			explicit
			AddNewConnectionWidget(
					const QString &display_text,
					QMenu *menu,
					QWidget *parent_ = NULL);

			void
			set_highlight_colour(
					const GPlatesGui::Colour &highlight_colour)
			{
				d_highlight_colour = highlight_colour;
			}

		protected:

			virtual
			void
			mousePressEvent(
					QMouseEvent *ev);

			virtual
			void
			enterEvent(
					QEvent *ev);

			virtual
			void
			leaveEvent(
					QEvent *ev);

			virtual
			void
			changeEvent(
					QEvent *ev);

		private:

			QMenu *d_menu;
			GPlatesGui::Colour d_highlight_colour;
			bool d_menu_open;
		};


		/**
		 * Displays the input connections on a particular input channel, and allows
		 * the user to add or remove input connections.
		 */
		class InputChannelWidget :
				public QWidget
		{
		public:

			explicit
			InputChannelWidget(
					GPlatesGui::VisualLayersProxy &visual_layers,
					GPlatesAppLogic::ApplicationState &application_state,
					GPlatesPresentation::ViewState &view_state,
					QWidget *parent_ = NULL);

			/**
			 * Causes this widget to display the @a input_connections for the input
			 * channel defined by @a layer_input_channel_type.
			 */
			void
			set_data(
					const GPlatesAppLogic::Layer &layer,
					const GPlatesAppLogic::LayerInputChannelType &layer_input_channel_type,
					const std::vector<GPlatesAppLogic::Layer::InputConnection> &input_connections,
					const GPlatesGui::Colour &light_layer_colour);

		private:

			void
			populate_with_feature_collections(
					const GPlatesAppLogic::Layer &layer,
					const GPlatesAppLogic::LayerInputChannelName::Type input_data_channel);

			void
			populate_with_layers(
					const GPlatesAppLogic::Layer &layer,
					const GPlatesAppLogic::LayerInputChannelName::Type input_data_channel,
					const std::vector<GPlatesAppLogic::LayerInputChannelType::InputLayerType> &input_layer_types);

			GPlatesGui::VisualLayersProxy &d_visual_layers;
			GPlatesAppLogic::ApplicationState &d_application_state;
			GPlatesPresentation::ViewState &d_view_state;

			ElidedLabel *d_input_channel_name_label;
			QWidget *d_yet_another_container;
			QWidget *d_input_connection_widgets_container;
			QMenu *d_add_new_connection_menu;
			AddNewConnectionWidget *d_add_new_connection_widget;

			/**
			 * A pointer to the layout of the Qt container that holds the widgets that
			 * display input connections.
			 */
			QVBoxLayout *d_input_connection_widgets_layout;

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
				ViewportWindow *viewport_window,
				QWidget *parent_ = NULL);

		void
		set_data(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer,
				int row);

	protected:

		virtual
		void
		mousePressEvent(
				QMouseEvent *event_);

	private Q_SLOTS:

		void
		handle_expand_icon_clicked();

		void
		handle_visibility_icon_clicked();

		void
		handle_is_default_icon_clicked();

		void
		handle_expand_input_channels_icon_clicked();

		void
		handle_expand_layer_options_icon_clicked();

		void
		handle_expand_advanced_options_icon_clicked();

		void
		handle_enable_layer_link_activated();

		void
		handle_rename_layer_link_activated();

		void
		handle_delete_layer_link_activated();

	private:

		void
		make_signal_slot_connections();

		/**
		 * Called by @a refresh to set up the input channel widgets.
		 */
		void
		set_input_channel_data(
				const GPlatesAppLogic::Layer &layer,
				const GPlatesGui::Colour &light_layer_colour);

		GPlatesGui::VisualLayersProxy &d_visual_layers;
		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;

		/**
		 * A weak pointer to the visual layer that we're currently displaying.
		 *
		 * This is invalid if @a set_data has not yet been called to provide us with
		 * a visual layer.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_visual_layer;

		/**
		 * The index of the row that this widget is showing.
		 */
		int d_row;

		QWidget *d_left_widget;

		/**
		 * The main expand/collapse icon on the left. For this icon, 'on' corresponds
		 * to 'expanded' and 'off' corresponds to 'collapsed'.
		 */
		VisualLayerWidgetInternals::ToggleIcon *d_expand_icon;

		/**
		 * The hide/show icon on the top. For this icon, 'on' corresponds to 'visible'
		 * and 'off' corresponds to 'hidden'.
		 */
		VisualLayerWidgetInternals::ToggleIcon *d_visibility_icon;

		/**
		 * The icon that shows whether the current layer is the default reconstruction tree.
		 */
		VisualLayerWidgetInternals::ToggleIcon *d_is_default_icon;

		/**
		 * The icon that allows the user to expand/collapse the input channels section.
		 */
		VisualLayerWidgetInternals::ToggleIcon *d_expand_input_channels_icon;

		/**
		 * The icon that allows the user to expand/collapse the layer options section.
		 */
		VisualLayerWidgetInternals::ToggleIcon *d_expand_layer_options_icon;

		/**
		 * The icon that allows the user to expand/collapse the advanced options section.
		 */
		VisualLayerWidgetInternals::ToggleIcon *d_expand_advanced_options_icon;

		/**
		 * The @a d_visibility_icon (page 0) and @a d_is_default_icon (page 1) are
		 * placed inside this; they occupy the same position on screen and this is
		 * used to switch between them.
		 */
		QStackedWidget *d_visibility_default_stackedwidget;

		/**
		 * The label showing the name of the layer in bold.
		 */
		ElidedLabel *d_name_label;

		/**
		 * The label showing the type of the layer.
		 */
		ElidedLabel *d_type_label;

		/**
		 * The layout of the @a input_channels_widget.
		 */
		QVBoxLayout *d_input_channels_widget_layout;

		/**
		 * A pool of InputChannelWidgets that can be used to display information about
		 * the input channels for the current visual layer.
		 *
		 * Additional InputChannelWidgets are created and added to this pool if the
		 * existing number of InputChannelWidgets is insufficient to display all of
		 * the input channels for the visual layer. However, InputChannelWidgets are
		 * not destroyed until 'this' widget is destroyed (this shouldn't be too bad
		 * because layers should have fairly similar numbers of input channels).
		 */
		std::vector<VisualLayerWidgetInternals::InputChannelWidget *> d_input_channel_widgets;

		LayerOptionsWidget *d_current_layer_options_widget;
		QVBoxLayout *d_layer_options_widget_layout;

		/**
		 * Shows the "Disable layer" or "Enable layer" link as appropriate.
		 */
		LinkWidget *d_enable_layer_link;

		/**
		 * Shows the "Rename layer" link.
		 */
		LinkWidget *d_rename_layer_link;

		/**
		 * Shows the "Delete layer" link.
		 */
		LinkWidget *d_delete_layer_link;
	};
}

#endif  // GPLATES_QTWIDGETS_VISUALLAYERWIDGET_H
