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
#include <QVBoxLayout>

#include "app-logic/Layer.h"

#include "VisualLayerWidgetUi.h"


namespace GPlatesPresentation
{
	class VisualLayer;
	class VisualLayers;
}

namespace GPlatesQtWidgets
{
	class ElidedLabel;

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
				GPlatesPresentation::VisualLayers &visual_layers,
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

		/**
		 * The BasicInfoWidget is the widget at the top that holds the name of the
		 * layer and its type.
		 */
		class BasicInfoWidget :
				public QWidget
		{
		public:

			BasicInfoWidget(
					QWidget *parent_ = NULL);

			void
			set_name(
					const QString &name);

			void
			set_type(
					const QString &type);

		private:

			ElidedLabel *d_name_label;
			ElidedLabel *d_type_label;
		};


		/**
		 * Displays an existing input connection.
		 */
		class InputConnectionWidget :
				public QWidget
		{
		public:

			InputConnectionWidget(
					GPlatesPresentation::VisualLayers &visual_layers,
					QWidget *parent_ = NULL);

			/**
			 * Causes this widget to display the @a input_connection.
			 */
			void
			set_data(
					const GPlatesAppLogic::Layer::InputConnection &input_connection);

		private:

			GPlatesPresentation::VisualLayers &d_visual_layers;

			ElidedLabel *d_input_connection_label;
		};


		/**
		 * Displays the input connections on a particular input channel, and allows
		 * the user to add or remove input connections.
		 */
		class InputChannelWidget :
				public QWidget
		{
		public:

			InputChannelWidget(
					GPlatesPresentation::VisualLayers &visual_layers,
					QWidget *parent_ = NULL);

			/**
			 * Causes this widget to display the @a input_connections for the input
			 * channel defined by @a input_channel_definition.
			 */
			void
			set_data(
					const GPlatesAppLogic::Layer::input_channel_definition_type &input_channel_definition,
					const std::vector<GPlatesAppLogic::Layer::InputConnection> &input_connections);

		private:

			GPlatesPresentation::VisualLayers &d_visual_layers;

			ElidedLabel *d_input_channel_name_label;

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

		void
		make_signal_slot_connections();

		/**
		 * Called by @a set_data to set up the input channel widgets.
		 */
		void
		set_input_channel_data(
				const GPlatesAppLogic::Layer &layer);

		GPlatesPresentation::VisualLayers &d_visual_layers;

		/**
		 * A weak pointer to the visual layer that we're currently displaying.
		 *
		 * This is invalid if @a set_data has not yet been called to provide us with
		 * a visual layer.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_visual_layer;

		/**
		 * Displays the name and the type of the layer.
		 *
		 * Memory managed by Qt.
		 */
		BasicInfoWidget *d_basic_info_widget;

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

		/**
		 * A pointer to the layout of the input_channels_groupbox.
		 *
		 * Memory managed by Qt.
		 */
		QVBoxLayout *d_input_channels_groupbox_layout;

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
		std::vector<InputChannelWidget *> d_input_channel_widgets;
	};
}

#endif  // GPLATES_QTWIDGETS_VISUALLAYERWIDGET_H
