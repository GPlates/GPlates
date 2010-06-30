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

#include <boost/shared_ptr.hpp>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/lambda/lambda.hpp>
#include <QPalette>
#include <QResizeEvent>
#include <QFont>
#include <QDebug>

#include "VisualLayerWidget.h"

#include "ElidedLabel.h"
#include "QtWidgetUtils.h"

#include "gui/Colour.h"

#include "presentation/VisualLayer.h"
#include "presentation/VisualLayers.h"


GPlatesQtWidgets::VisualLayerWidgetInternals::ToggleIcon::ToggleIcon(
		const QPixmap &on_icon,
		const QPixmap &off_icon,
		QWidget *parent_) :
	QLabel(parent_),
	d_on_icon(on_icon),
	d_off_icon(off_icon)
{
	setCursor(Qt::PointingHandCursor);
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::ToggleIcon::show_icon(
		bool on)
{
	setPixmap(on ? d_on_icon : d_off_icon);
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::ToggleIcon::mousePressEvent(
		QMouseEvent *event_)
{
	emit clicked();
}


const QPixmap &
GPlatesQtWidgets::VisualLayerWidgetInternals::get_collapsed_icon()
{
	static const QPixmap COLLAPSED_ICON(":/gnome_stock_data-next_16.png");
	return COLLAPSED_ICON;
}


const QPixmap &
GPlatesQtWidgets::VisualLayerWidgetInternals::get_expanded_icon()
{
	static const QPixmap EXPANDED_ICON(":/gnome_stock_data-next_down_16.png");
	return EXPANDED_ICON;
}


const QPixmap &
GPlatesQtWidgets::VisualLayerWidgetInternals::get_visible_icon()
{
	static const QPixmap VISIBLE_ICON(":/inkscape_object_visible_16.png");
	return VISIBLE_ICON;
}


const QPixmap &
GPlatesQtWidgets::VisualLayerWidgetInternals::get_hidden_icon()
{
	static const QPixmap HIDDEN_ICON(":/inkscape_object_hidden_16.png");
	return HIDDEN_ICON;
}


GPlatesQtWidgets::VisualLayerWidget::VisualLayerWidget(
		GPlatesPresentation::VisualLayers &visual_layers,
		QWidget *parent_) :
	QWidget(parent_),
	d_visual_layers(visual_layers),
	d_basic_info_widget(
			new BasicInfoWidget(this)),
	d_expand_icon(
			new VisualLayerWidgetInternals::ToggleIcon(
				VisualLayerWidgetInternals::get_expanded_icon(),
				VisualLayerWidgetInternals::get_collapsed_icon(),
				this)),
	d_visibility_icon(
			new VisualLayerWidgetInternals::ToggleIcon(
				VisualLayerWidgetInternals::get_visible_icon(),
				VisualLayerWidgetInternals::get_hidden_icon(),
				this)),
	d_input_channels_groupbox_layout(NULL)
{
	setupUi(this);

	// Give the input_channels_groupbox a layout.
	d_input_channels_groupbox_layout = new QVBoxLayout(input_channels_groupbox);

	// Install the basic info widget into its placeholder.
	QtWidgetUtils::add_widget_to_placeholder(
			d_basic_info_widget,
			basic_info_placeholder_widget);

	// Install the icons into their placeholders.
	QtWidgetUtils::add_widget_to_placeholder(
			d_expand_icon,
			expand_icon_placeholder_widget);
	QtWidgetUtils::add_widget_to_placeholder(
			d_visibility_icon,
			visibility_icon_placeholder_widget);
	d_visibility_icon->setFrameStyle(QFrame::Panel | QFrame::Sunken);

	// Set background colour to usual window background colour.
	QPalette widget_palette = palette();
	widget_palette.setColor(QPalette::Base, widget_palette.color(QPalette::Window));
	setPalette(widget_palette);

	// Set background colour of details panel to be lighter than widget background.
	QPalette details_palette = details_placeholder_widget->palette();
	details_palette.setColor(
			QPalette::Base,
			GPlatesGui::Colour::linearly_interpolate(
				details_palette.color(QPalette::Window),
				GPlatesGui::Colour::get_white(),
				0.5 /* mid-way */));
	details_placeholder_widget->setPalette(details_palette);

	// Hide this for now...
	additional_options_widget->hide();

	make_signal_slot_connections();
}


void
GPlatesQtWidgets::VisualLayerWidget::set_data(
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			visual_layer.lock())
	{
		const GPlatesAppLogic::Layer &layer = locked_visual_layer->get_reconstruct_graph_layer();

		// Store pointer to visual layer for later use.
		d_visual_layer = visual_layer;

		// Set the expand/collapse icon.
		bool expanded = locked_visual_layer->is_expanded();
		d_expand_icon->show_icon(expanded);

		// Set the hide/show icon.
		d_visibility_icon->show_icon(locked_visual_layer->is_visible());

		// Show or hide the details panel as necessary.
		details_placeholder_widget->setVisible(expanded);

		// Update the basic info.
		d_basic_info_widget->set_name(locked_visual_layer->get_name());
		std::pair<QString, QString> layer_type_name_and_description = layer.get_name_and_description();
		d_basic_info_widget->set_type(layer_type_name_and_description.first);

		if (expanded)
		{
			// Update the input channel info.
			set_input_channel_data(locked_visual_layer->get_reconstruct_graph_layer());
		}
	}
}


namespace
{
	template<class InputChannelContainerType>
	void
	move_main_input_channel_to_front(
			InputChannelContainerType &input_channels,
			const QString &main_input_channel)
	{
		typedef typename InputChannelContainerType::iterator channel_iterator_type;
		channel_iterator_type channel_iter = input_channels.begin();
		for (; channel_iter != input_channels.end(); ++channel_iter)
		{
			if (boost::get<0>(*channel_iter) == main_input_channel)
			{
				// FIXME: This is not efficient if the container is a vector.
				typedef GPlatesAppLogic::Layer::input_channel_definition_type input_channel_definition_type;
				input_channel_definition_type temp = *channel_iter;
				input_channels.erase(channel_iter);
				input_channels.insert(input_channels.begin(), temp);
				return;
			}
		}
	}
}


void
GPlatesQtWidgets::VisualLayerWidget::set_input_channel_data(
		const GPlatesAppLogic::Layer &layer)
{
	typedef GPlatesAppLogic::Layer::input_channel_definition_type input_channel_definition_type;

	std::vector<input_channel_definition_type> input_channels =
		layer.get_input_channel_definitions();

	// Make sure we have enough widgets in our pool to display all input channels.
	if (input_channels.size() > d_input_channel_widgets.size())
	{
		int num_new_widgets = static_cast<int>(input_channels.size() - d_input_channel_widgets.size());
		for (int i = 0; i != num_new_widgets; ++i)
		{
			InputChannelWidget *new_widget = new InputChannelWidget(d_visual_layers, this);
			d_input_channel_widgets.push_back(new_widget);
			d_input_channels_groupbox_layout->addWidget(new_widget);
		}
	}

	// List the main input channel first.
	QString main_input_channel = layer.get_main_input_feature_collection_channel();
	move_main_input_channel_to_front(input_channels, main_input_channel);

	// Display one input channel in one widget.
	typedef std::vector<input_channel_definition_type>::const_iterator channel_iterator_type;
	typedef std::vector<InputChannelWidget *>::const_iterator widget_iterator_type;
	channel_iterator_type channel_iter = input_channels.begin();
	widget_iterator_type widget_iter = d_input_channel_widgets.begin();
	for (; channel_iter != input_channels.end(); ++channel_iter, ++widget_iter)
	{
		const input_channel_definition_type &input_channel_definition = *channel_iter;
		InputChannelWidget *input_channel_widget = *widget_iter;
		input_channel_widget->set_data(
				input_channel_definition,
				layer.get_channel_inputs(input_channel_definition.get<0>()));
	}

	// Hide the excess widgets in the pool.
	if (input_channels.size() < d_input_channel_widgets.size())
	{
		for (widget_iter = d_input_channel_widgets.begin() +
				(d_input_channel_widgets.size() - input_channels.size());
				widget_iter != d_input_channel_widgets.end();
				++widget_iter)
		{
			(*widget_iter)->hide();
		}
	}
}


void
GPlatesQtWidgets::VisualLayerWidget::handle_expand_icon_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_visual_layer.lock())
	{
		// If collapsed, make expanded. If expanded, make collapsed.
		locked_visual_layer->toggle_expanded();
	}
}


void
GPlatesQtWidgets::VisualLayerWidget::handle_visibility_icon_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_visual_layer.lock())
	{
		// If visible, hide layer. If hidden, show layer.
		locked_visual_layer->toggle_visible();
	}
}


void
GPlatesQtWidgets::VisualLayerWidget::make_signal_slot_connections()
{
	// Connect to signals from the icons.
	QObject::connect(
			d_expand_icon,
			SIGNAL(clicked()),
			this,
			SLOT(handle_expand_icon_clicked()));
	QObject::connect(
			d_visibility_icon,
			SIGNAL(clicked()),
			this,
			SLOT(handle_visibility_icon_clicked()));
}


GPlatesQtWidgets::VisualLayerWidget::BasicInfoWidget::BasicInfoWidget(
		QWidget *parent_) :
	QWidget(parent_)
{
	d_name_label = new ElidedLabel("Global EarthByte GPlates Coastlines 20091014", this);
	d_name_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);

	d_type_label = new ElidedLabel("Reconstruction", this);
	d_type_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
	QFont type_label_font = d_type_label->font();
	type_label_font.setItalic(true);
	d_type_label->setFont(type_label_font);

	QVBoxLayout *this_layout = new QVBoxLayout(this);
	this_layout->setContentsMargins(0, 0, 0, 0);
	this_layout->setSpacing(0);
	this_layout->addWidget(d_name_label);
	this_layout->addWidget(d_type_label);

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}


void
GPlatesQtWidgets::VisualLayerWidget::BasicInfoWidget::set_name(
		const QString &name)
{
	d_name_label->setText(name);
}


void
GPlatesQtWidgets::VisualLayerWidget::BasicInfoWidget::set_type(
		const QString &type)
{
	d_type_label->setText(type);
}


GPlatesQtWidgets::VisualLayerWidget::InputConnectionWidget::InputConnectionWidget(
		GPlatesPresentation::VisualLayers &visual_layers,
		QWidget *parent_) :
	QWidget(parent_),
	d_visual_layers(visual_layers),
	d_input_connection_label(
			new ElidedLabel(this))
{
	// Put the internal label into a frame.
	QFrame *label_frame = new QFrame(this);
	label_frame->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	QtWidgetUtils::add_widget_to_placeholder(d_input_connection_label, label_frame);

	// Put the frame into this widget.
	QtWidgetUtils::add_widget_to_placeholder(label_frame, this);
}


void
GPlatesQtWidgets::VisualLayerWidget::InputConnectionWidget::set_data(
		const GPlatesAppLogic::Layer::InputConnection &input_connection)
{
	boost::optional<GPlatesAppLogic::Layer::InputFile> input_file = input_connection.get_input_file();
	if (input_file)
	{
		// Display the filename if the input connection is a file.
		QString filename = input_file->get_file_info().get_display_name(false /* no absolute path */);
		if (filename.isEmpty())
		{
			filename = "New Feature Collection";
		}
		d_input_connection_label->setText(filename);
		return;
	}

	boost::optional<GPlatesAppLogic::Layer> input_layer = input_connection.get_input_layer();
	if (input_layer)
	{
		// Display the visual layer name if the input connection is a layer.
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer =
			d_visual_layers.get_visual_layer(*input_layer);
		if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = visual_layer.lock())
		{
			d_input_connection_label->setText(locked_visual_layer->get_name());
		}
		else
		{
			d_input_connection_label->setText(QString());
		}
	}
}


GPlatesQtWidgets::VisualLayerWidget::InputChannelWidget::InputChannelWidget(
		GPlatesPresentation::VisualLayers &visual_layers,
		QWidget *parent_) :
	QWidget(parent_),
	d_visual_layers(visual_layers),
	d_input_channel_name_label(
			new ElidedLabel(this)),
	d_input_connection_widgets_layout(NULL)
{
	// The widget has two subwidgets: the name label and the container of input
	// connection widgets. The label is on top of the container widget.
	QWidget *input_connection_widgets_container = new QWidget(this);
	QVBoxLayout *this_layout = new QVBoxLayout(this);
	this_layout->setContentsMargins(0, 0, 0, 0);
	this_layout->setSpacing(0);
	this_layout->addWidget(d_input_channel_name_label);
	this_layout->addWidget(input_connection_widgets_container);

	// Create a layout for the input connection widgets container.
	d_input_connection_widgets_layout = new QVBoxLayout(input_connection_widgets_container);
	static const int LEFT_MARGIN = 10;
	d_input_connection_widgets_layout->setContentsMargins(LEFT_MARGIN, 0, 0, 0);
}


void
GPlatesQtWidgets::VisualLayerWidget::InputChannelWidget::set_data(
		const GPlatesAppLogic::Layer::input_channel_definition_type &input_channel_definition,
		const std::vector<GPlatesAppLogic::Layer::InputConnection> &input_connections)
{
	// Update the channel name.
	d_input_channel_name_label->setText(input_channel_definition.get<0>() + ":");

	// Make sure we have enough widgets in our pool to display all input channels.
	if (input_connections.size() > d_input_connection_widgets.size())
	{
		int num_new_widgets = static_cast<int>(input_connections.size() - d_input_connection_widgets.size());
		for (int i = 0; i != num_new_widgets; ++i)
		{
			InputConnectionWidget *new_widget = new InputConnectionWidget(d_visual_layers, this);
			d_input_connection_widgets.push_back(new_widget);
			d_input_connection_widgets_layout->addWidget(new_widget);
		}
	}

	// Display one input channel in one widget.
	typedef std::vector<GPlatesAppLogic::Layer::InputConnection>::const_iterator connection_iterator_type;
	typedef std::vector<InputConnectionWidget *>::const_iterator widget_iterator_type;
	connection_iterator_type connection_iter = input_connections.begin();
	widget_iterator_type widget_iter = d_input_connection_widgets.begin();
	for (; connection_iter != input_connections.end(); ++connection_iter, ++widget_iter)
	{
		const GPlatesAppLogic::Layer::InputConnection &input_connection = *connection_iter;
		InputConnectionWidget *input_connection_widget = *widget_iter;
		input_connection_widget->set_data(input_connection);
	}

	// Hide the excess widgets in the pool.
	if (input_connections.size() < d_input_connection_widgets.size())
	{
		for (widget_iter = d_input_connection_widgets.begin() +
				(d_input_connection_widgets.size() - input_connections.size());
				widget_iter != d_input_connection_widgets.end();
				++widget_iter)
		{
			(*widget_iter)->hide();
		}
	}
}

