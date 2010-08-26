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
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <QPalette>
#include <QResizeEvent>
#include <QFont>
#include <QString>
#include <QMetaType>
#include <QIcon>
#include <QPixmap>
#include <QMessageBox>
#include <QMimeData>
#include <QByteArray>
#include <QDataStream>
#include <QDrag>
#include <QDebug>

#include "VisualLayerWidget.h"

#include "ElidedLabel.h"
#include "RasterLayerOptionsWidget.h"
#include "QtWidgetUtils.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/LayerTaskType.h"

#include "file-io/File.h"
#include "file-io/FileInfo.h"

#include "gui/Colour.h"
#include "gui/HTMLColourNames.h"
#include "gui/LayerTaskTypeInfo.h"
#include "gui/VisualLayersListModel.h"
#include "gui/VisualLayersProxy.h"

#include "presentation/VisualLayer.h"


namespace
{
	using namespace GPlatesGui;

	static const QString NEW_FEATURE_COLLECTION = "New Feature Collection";

	const Colour &
	get_layer_colour(
			GPlatesAppLogic::LayerTaskType::Type layer_type)
	{
		static const HTMLColourNames &html_colours = HTMLColourNames::instance();

		static const Colour RECONSTRUCTION_COLOUR = *html_colours.get_colour("gold");
		static const Colour RECONSTRUCT_COLOUR = *html_colours.get_colour("yellowgreen");
		static const Colour RASTER_COLOUR = *html_colours.get_colour("tomato");
		static const Colour AGE_GRID_COLOUR = *html_colours.get_colour("darkturquoise");
		static const Colour TOPOLOGY_BOUNDARY_RESOLVER_COLOUR = *html_colours.get_colour("plum");
		static const Colour TOPOLOGY_NETWORK_RESOLVER_COLOUR = *html_colours.get_colour("darkkhaki");

		static const Colour DEFAULT_COLOUR = Colour::get_white();

		using namespace GPlatesAppLogic::LayerTaskType;

		switch (layer_type)
		{
			case RECONSTRUCTION:
				return RECONSTRUCTION_COLOUR;

			case RECONSTRUCT:
				return RECONSTRUCT_COLOUR;

			case RASTER:
				return RASTER_COLOUR;

			case AGE_GRID:
				return AGE_GRID_COLOUR;

			case TOPOLOGY_BOUNDARY_RESOLVER:
				return TOPOLOGY_BOUNDARY_RESOLVER_COLOUR;

			case TOPOLOGY_NETWORK_RESOLVER:
				return TOPOLOGY_NETWORK_RESOLVER_COLOUR;

			default:
				return DEFAULT_COLOUR;
		}
	}

	QPixmap
	get_filled_pixmap(
			int width,
			int height,
			const Colour &colour)
	{
		QPixmap result(width, height);
		result.fill(colour);
		return result;
	}

	const QIcon &
	get_feature_collection_icon()
	{
		static const QIcon FEATURE_COLLECTION_ICON(":/gnome_text_x_preview_16.png");
		return FEATURE_COLLECTION_ICON;
	}

	const QIcon &
	get_layer_icon(
			GPlatesAppLogic::LayerTaskType::Type layer_type)
	{
		using namespace GPlatesAppLogic::LayerTaskType;

		static const int ICON_SIZE = 16;

		static const QIcon RECONSTRUCTION_ICON(get_filled_pixmap(
					ICON_SIZE, ICON_SIZE, get_layer_colour(RECONSTRUCTION)));
		static const QIcon RECONSTRUCT_ICON(get_filled_pixmap(
					ICON_SIZE, ICON_SIZE, get_layer_colour(RECONSTRUCT)));
		static const QIcon RASTER_ICON(get_filled_pixmap(
					ICON_SIZE, ICON_SIZE, get_layer_colour(RASTER)));
		static const QIcon AGE_GRID_ICON(get_filled_pixmap(
					ICON_SIZE, ICON_SIZE, get_layer_colour(AGE_GRID)));
		static const QIcon TOPOLOGY_BOUNDARY_RESOLVER_ICON(get_filled_pixmap(
					ICON_SIZE, ICON_SIZE, get_layer_colour(TOPOLOGY_BOUNDARY_RESOLVER)));
		static const QIcon TOPOLOGY_NETWORK_RESOLVER_ICON(get_filled_pixmap(
					ICON_SIZE, ICON_SIZE, get_layer_colour(TOPOLOGY_NETWORK_RESOLVER)));

		static const QIcon DEFAULT_ICON;

		switch (layer_type)
		{
			case RECONSTRUCTION:
				return RECONSTRUCTION_ICON;

			case RECONSTRUCT:
				return RECONSTRUCT_ICON;

			case RASTER:
				return RASTER_ICON;

			case AGE_GRID:
			 	return AGE_GRID_ICON;

			case TOPOLOGY_BOUNDARY_RESOLVER:
				return TOPOLOGY_BOUNDARY_RESOLVER_ICON;

			case TOPOLOGY_NETWORK_RESOLVER:
			 	return TOPOLOGY_NETWORK_RESOLVER_ICON;

			default:
				return DEFAULT_ICON;
		}
	}
}


Q_DECLARE_METATYPE( boost::function<void ()> )


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
	if (event_->button() == Qt::LeftButton)
	{
		emit clicked();
	}
}


const QPixmap &
GPlatesQtWidgets::VisualLayerWidgetInternals::get_collapsed_icon()
{
	static const QPixmap COLLAPSED_ICON(":/gnome_stock_data_next_16.png");
	return COLLAPSED_ICON;
}


const QPixmap &
GPlatesQtWidgets::VisualLayerWidgetInternals::get_expanded_icon()
{
	static const QPixmap EXPANDED_ICON(":/gnome_stock_data_next_down_16.png");
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
	static const QPixmap HIDDEN_ICON(":/blank_16.png");
	return HIDDEN_ICON;
}


GPlatesQtWidgets::VisualLayerWidget::VisualLayerWidget(
		GPlatesGui::VisualLayersProxy &visual_layers,
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		QString &open_file_path,
		ReadErrorAccumulationDialog *read_errors_dialog,
		QWidget *parent_) :
	QWidget(parent_),
	d_visual_layers(visual_layers),
	d_application_state(application_state),
	d_view_state(view_state),
	d_open_file_path(open_file_path),
	d_read_errors_dialog(read_errors_dialog),
	d_left_widget(
			new DraggableWidget(this)),
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
	d_name_label(
			new ElidedLabel(Qt::ElideMiddle, this)),
	d_type_label(
			new ElidedLabel(Qt::ElideRight, this)),
	d_input_channels_widget_layout(NULL),
	d_raster_layer_options_widget(NULL)
{
	setupUi(this);

	// Give the input_channels_widget a layout.
	d_input_channels_widget_layout = new QVBoxLayout(input_channels_widget);
	d_input_channels_widget_layout->setContentsMargins(2, 6, 2, 0);

	// Install labels for the layer name and type.
	QtWidgetUtils::add_widget_to_placeholder(
			d_name_label,
			name_label_placeholder_widget);
	QFont name_label_font = d_name_label->font();
	name_label_font.setBold(true);
	d_name_label->setFont(name_label_font);
	QtWidgetUtils::add_widget_to_placeholder(
			d_type_label,
			type_label_placeholder_widget);

	// Create the left widget, which shows the stripe of colour.
	QtWidgetUtils::add_widget_to_placeholder(
			d_left_widget,
			left_placeholder_widget);
	d_left_widget->setAutoFillBackground(true);
	QVBoxLayout *left_layout = new QVBoxLayout(d_left_widget);
	left_layout->setContentsMargins(2, 5, 2, 2);
	left_layout->addWidget(d_expand_icon);
	QWidget *left_filler_widget = new QWidget(this);
	left_filler_widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
	left_layout->addWidget(left_filler_widget);

	// Install the icons into their placeholders.
	QtWidgetUtils::add_widget_to_placeholder(
			d_visibility_icon,
			visibility_icon_placeholder_widget);
	d_visibility_icon->setFrameStyle(QFrame::Panel | QFrame::Sunken);

	make_signal_slot_connections();

	// Hide things for now...
	advanced_options_groupbox->hide();
	colouring_groupbox->hide();
}


void
GPlatesQtWidgets::VisualLayerWidget::set_data(
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer,
		int row)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			visual_layer.lock())
	{
		const GPlatesAppLogic::Layer &layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::LayerTaskType::Type layer_type = layer.get_type();

		// Store pointer to visual layer for later use.
		d_visual_layer = visual_layer;

		// Set the expand/collapse icon.
		bool expanded = locked_visual_layer->is_expanded();
		d_expand_icon->show_icon(expanded);

		// Set the background colour of d_left_widget depending on what type of layer it is.
		QPalette left_widget_palette;
		left_widget_palette.setColor(QPalette::Base, get_layer_colour(layer_type));
		d_left_widget->setPalette(left_widget_palette);
		d_left_widget->set_row(row);

		// Set the hide/show icon.
		d_visibility_icon->show_icon(locked_visual_layer->is_visible());

		// Update the basic info.
		d_name_label->setText(locked_visual_layer->get_name());
		d_type_label->setText(GPlatesGui::LayerTaskTypeInfo::get_name(layer_type));
		top_widget->setToolTip(GPlatesGui::LayerTaskTypeInfo::get_description(layer_type));

		// Show or hide the details panel as necessary.
		details_placeholder_widget->setVisible(expanded);

		// Populate the details panel only if shown.
		if (expanded)
		{
			// Update the input channel info.
			set_input_channel_data(locked_visual_layer->get_reconstruct_graph_layer());

			// Show or hide the special groupbox for raster layers.
			if (layer_type == GPlatesAppLogic::LayerTaskType::RASTER)
			{
				if (!d_raster_layer_options_widget)
				{
					d_raster_layer_options_widget = new RasterLayerOptionsWidget(
							d_application_state,
							d_view_state,
							d_open_file_path,
							d_read_errors_dialog,
							this);
					QtWidgetUtils::add_widget_to_placeholder(
							d_raster_layer_options_widget,
							raster_groupbox);
				}
				d_raster_layer_options_widget->set_data(
						locked_visual_layer->get_reconstruct_graph_layer());
				raster_groupbox->show();
			}
			else
			{
				raster_groupbox->hide();
			}
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
		unsigned int num_new_widgets = input_channels.size() - d_input_channel_widgets.size();
		for (unsigned int i = 0; i != num_new_widgets; ++i)
		{
			VisualLayerWidgetInternals::InputChannelWidget *new_widget =
				new VisualLayerWidgetInternals::InputChannelWidget(d_visual_layers, d_application_state, this);
			d_input_channel_widgets.push_back(new_widget);
			d_input_channels_widget_layout->addWidget(new_widget);
		}
	}

	// List the main input channel first.
	QString main_input_channel = layer.get_main_input_feature_collection_channel();
	move_main_input_channel_to_front(input_channels, main_input_channel);

	// Display one input channel in one widget.
	typedef std::vector<input_channel_definition_type>::const_iterator channel_iterator_type;
	typedef std::vector<VisualLayerWidgetInternals::InputChannelWidget *>::const_iterator widget_iterator_type;
	channel_iterator_type channel_iter = input_channels.begin();
	widget_iterator_type widget_iter = d_input_channel_widgets.begin();
	for (; channel_iter != input_channels.end(); ++channel_iter, ++widget_iter)
	{
		const input_channel_definition_type &input_channel_definition = *channel_iter;
		VisualLayerWidgetInternals::InputChannelWidget *input_channel_widget = *widget_iter;
		input_channel_widget->set_data(
				layer,
				input_channel_definition,
				layer.get_channel_inputs(input_channel_definition.get<0>()));
		input_channel_widget->show();
	}

	// Hide the excess widgets in the pool.
	if (input_channels.size() < d_input_channel_widgets.size())
	{
		for (widget_iter = d_input_channel_widgets.begin() + input_channels.size();
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


namespace
{
	class DisconnectInputConnectionLabel :
			public QLabel
	{
	public:

		DisconnectInputConnectionLabel(
				GPlatesAppLogic::Layer::InputConnection &current_input_connection,
				QWidget *parent_) :
			QLabel(parent_),
			d_current_input_connection(current_input_connection)
		{  }

	protected:

		void
		mousePressEvent(
				QMouseEvent *event_)
		{
			if (d_current_input_connection.is_valid())
			{
				d_current_input_connection.disconnect();
			}
		}

	private:

		GPlatesAppLogic::Layer::InputConnection &d_current_input_connection;
	};
}


GPlatesQtWidgets::VisualLayerWidgetInternals::InputConnectionWidget::InputConnectionWidget(
		GPlatesGui::VisualLayersProxy &visual_layers,
		QWidget *parent_) :
	QWidget(parent_),
	d_visual_layers(visual_layers),
	d_input_connection_label(
			new ElidedLabel(Qt::ElideMiddle, this)),
	d_disconnect_icon(
			new DisconnectInputConnectionLabel(d_current_input_connection, this))
{
	d_input_connection_label->setFrameStyle(QFrame::Panel | QFrame::Plain);
	d_disconnect_icon->setPixmap(get_disconnect_pixmap());
	d_disconnect_icon->setCursor(QCursor(Qt::PointingHandCursor));

	static const QString DISCONNECT_ICON_TOOLTIP = "Disconnect";
	d_disconnect_icon->setToolTip(DISCONNECT_ICON_TOOLTIP);

	// Lay out the internal label and the disconnect icon.
	QHBoxLayout *widget_layout = new QHBoxLayout(this);
	widget_layout->setContentsMargins(0, 0, 0, 0);
	widget_layout->addWidget(d_input_connection_label);
	widget_layout->addWidget(d_disconnect_icon);
	QSizePolicy label_size_policy = d_input_connection_label->sizePolicy();
	label_size_policy.setHorizontalPolicy(QSizePolicy::Expanding);
	d_input_connection_label->setSizePolicy(label_size_policy);
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::InputConnectionWidget::set_data(
		const GPlatesAppLogic::Layer::InputConnection &input_connection)
{
	// Save the input connection, in case the user wants to disconnect.
	d_current_input_connection = input_connection;

	boost::optional<GPlatesAppLogic::Layer::InputFile> input_file = input_connection.get_input_file();
	if (input_file)
	{
		// Display the filename if the input connection is a file.
		QString filename = input_file->get_file_info().get_display_name(false /* no absolute path */);
		if (filename.isEmpty())
		{
			filename = NEW_FEATURE_COLLECTION;
		}
		d_input_connection_label->setText(filename);
	}
	else
	{
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
}


const QPixmap &
GPlatesQtWidgets::VisualLayerWidgetInternals::InputConnectionWidget::get_disconnect_pixmap()
{
	static const QPixmap DISCONNECT_PIXMAP(":/tango_list_remove_16.png");
	return DISCONNECT_PIXMAP;
}


GPlatesQtWidgets::VisualLayerWidgetInternals::InputChannelWidget::InputChannelWidget(
		GPlatesGui::VisualLayersProxy &visual_layers,
		GPlatesAppLogic::ApplicationState &application_state,
		QWidget *parent_) :
	QWidget(parent_),
	d_visual_layers(visual_layers),
	d_application_state(application_state),
	d_input_channel_name_label(
			new ElidedLabel(Qt::ElideRight, this)),
	d_input_connection_widgets_container(
			new QWidget(this)),
	d_add_new_connection_widget(
			new QToolButton(this)),
	d_input_connection_widgets_layout(NULL)
{
	static const QString ADD_NEW_CONNECTION = "Add new connection";
	d_add_new_connection_widget->setText(ADD_NEW_CONNECTION);
	d_add_new_connection_widget->setAutoRaise(true);
	d_add_new_connection_widget->setPopupMode(QToolButton::InstantPopup);
	QMenu *add_new_connection_menu = new QMenu(d_add_new_connection_widget);
	d_add_new_connection_widget->setMenu(add_new_connection_menu);
	QFont add_new_connection_font = d_add_new_connection_widget->font();
	add_new_connection_font.setItalic(true);
	d_add_new_connection_widget->setFont(add_new_connection_font);

	// This widget has the following subwidgets:
	//  - The name label
	//  - A container that contains in turn:
	//     - A container of input connection widgets
	//     - A button to add new input connections
	QWidget *yet_another_container = new QWidget(this);
	QVBoxLayout *yet_another_layout = new QVBoxLayout(yet_another_container);
	yet_another_layout->setContentsMargins(15, 0, 0, 0);
	yet_another_layout->setSpacing(1);
	yet_another_layout->addWidget(d_input_connection_widgets_container);
	yet_another_layout->addWidget(d_add_new_connection_widget);

	QVBoxLayout *this_layout = new QVBoxLayout(this);
	this_layout->setContentsMargins(0, 0, 0, 0);
	this_layout->setSpacing(4);
	this_layout->addWidget(d_input_channel_name_label);
	this_layout->addWidget(yet_another_container);
	this_layout->addStretch();

	// Create a layout for the input connection widgets container.
	d_input_connection_widgets_layout = new QVBoxLayout(d_input_connection_widgets_container);
	d_input_connection_widgets_layout->setContentsMargins(0, 0, 0, 0);
	d_input_connection_widgets_layout->setSpacing(4);

	QObject::connect(
			add_new_connection_menu,
			SIGNAL(triggered(QAction *)),
			this,
			SLOT(handle_menu_triggered(QAction *)));
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::InputChannelWidget::set_data(
		const GPlatesAppLogic::Layer &layer,
		const GPlatesAppLogic::Layer::input_channel_definition_type &input_channel_definition,
		const std::vector<GPlatesAppLogic::Layer::InputConnection> &input_connections)
{
	// Update the channel name.
	d_input_channel_name_label->setText(input_channel_definition.get<0>() + ":");

	// Disable the add new connection button if the channel only takes one
	// connection and we already have that.
	if (input_channel_definition.get<2>() == GPlatesAppLogic::Layer::ONE_DATA_IN_CHANNEL &&
			input_connections.size() >= 1)
	{
		d_add_new_connection_widget->setEnabled(false);
		static const QString ONLY_ONE_CONNECTION_TOOLTIP = tr("This input channel only accepts one connection.");
		d_add_new_connection_widget->setToolTip(ONLY_ONE_CONNECTION_TOOLTIP);
	}
	else
	{
		GPlatesAppLogic::Layer::LayerInputDataType input_data_type = input_channel_definition.get<1>();
		if (input_data_type == GPlatesAppLogic::Layer::INPUT_FEATURE_COLLECTION_DATA)
		{
			populate_with_feature_collections(
					layer,
					input_channel_definition.get<0>(),
					d_add_new_connection_widget);
		}
		else
		{
			populate_with_layers(
					layer,
					input_channel_definition.get<0>(),
					input_data_type,
					d_add_new_connection_widget);
		}
	}

	// Easy code path if no input connections.
	if (input_connections.size() == 0)
	{
		d_input_connection_widgets_container->hide();
		return;
	}
	d_input_connection_widgets_container->show();

	// Make sure we have enough widgets in our pool to display all input channels.
	if (input_connections.size() > d_input_connection_widgets.size())
	{
		unsigned int num_new_widgets = input_connections.size() - d_input_connection_widgets.size();
		for (unsigned int i = 0; i != num_new_widgets; ++i)
		{
			InputConnectionWidget *new_widget = new InputConnectionWidget(d_visual_layers);
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
		input_connection_widget->show();
	}

	// Hide the excess widgets in the pool.
	if (input_connections.size() < d_input_connection_widgets.size())
	{
		for (widget_iter = d_input_connection_widgets.begin() + input_connections.size();
				widget_iter != d_input_connection_widgets.end();
				++widget_iter)
		{
			(*widget_iter)->hide();
		}
	}
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::InputChannelWidget::handle_menu_triggered(
		QAction *action)
{
	typedef boost::function<void ()> fn_type;
	QVariant qv = action->data();
	if (qv.canConvert<fn_type>())
	{
		fn_type fn = qv.value<fn_type>();

		try
		{
			fn();
		}
		catch (const GPlatesAppLogic::Layer::CycleDetectedInReconstructGraph &)
		{
			QMessageBox::critical(
					this,
					"Add new connection",
					"The requested connection could not be made because it would introduce a cycle.");
		}
	}
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::InputChannelWidget::populate_with_feature_collections(
		const GPlatesAppLogic::Layer &layer,
		const QString &input_data_channel,
		QToolButton *button)
{
	QMenu *menu = button->menu();
	menu->clear();

	typedef GPlatesAppLogic::FeatureCollectionFileState::file_reference file_reference;
	std::vector<file_reference> loaded_files = d_application_state.get_feature_collection_file_state().get_loaded_files();
	if (loaded_files.size() == 0)
	{
		button->setEnabled(false);
		static const QString NO_FEATURE_COLLECTIONS_TOOLTIP = tr("No feature collections have been loaded.");
		button->setToolTip(NO_FEATURE_COLLECTIONS_TOOLTIP);
		return;
	}
	else
	{
		button->setEnabled(true);
		button->setToolTip(QString());
	}

	GPlatesAppLogic::ReconstructGraph &reconstruct_graph = d_application_state.get_reconstruct_graph();

	BOOST_FOREACH(const file_reference &loaded_file, loaded_files)
	{
		QString display_name = loaded_file.get_file().get_file_info().get_display_name(false);
		if (display_name.isEmpty())
		{
			display_name = NEW_FEATURE_COLLECTION;
		}
		QAction *action = new QAction(display_name, menu);
		boost::function<void ()> fn = boost::bind(
				&GPlatesAppLogic::Layer::connect_input_to_file,
				layer,
				reconstruct_graph.get_input_file(loaded_file),
				input_data_channel);
		QVariant qv;
		qv.setValue(fn);
		action->setData(qv);
		action->setIcon(get_feature_collection_icon());
		menu->addAction(action);
	}
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::InputChannelWidget::populate_with_layers(
		const GPlatesAppLogic::Layer &layer,
		const QString &input_data_channel,
		GPlatesAppLogic::Layer::LayerInputDataType input_data_type,
		QToolButton *button)
{
	QMenu *menu = button->menu();
	menu->clear();

	GPlatesAppLogic::Layer::LayerOutputDataType output_data_type;
	if (input_data_type == GPlatesAppLogic::Layer::INPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA)
	{
		output_data_type = GPlatesAppLogic::Layer::OUTPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA;
	}
	else if (input_data_type == GPlatesAppLogic::Layer::INPUT_RECONSTRUCTION_TREE_DATA)
	{
		output_data_type = GPlatesAppLogic::Layer::OUTPUT_RECONSTRUCTION_TREE_DATA;
	}
	else
	{
		return;
	}

	const GPlatesAppLogic::ReconstructGraph &reconstruct_graph = d_application_state.get_reconstruct_graph();
	unsigned int count = 0;
	BOOST_FOREACH(const GPlatesAppLogic::Layer &outputting_layer, reconstruct_graph)
	{
		if (outputting_layer.get_output_definition() == output_data_type)
		{
			boost::weak_ptr<GPlatesPresentation::VisualLayer> outputting_visual_layer =
				d_visual_layers.get_visual_layer(outputting_layer);
			if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_outputting_visual_layer =
					outputting_visual_layer.lock())
			{
				QString outputting_layer_name = locked_outputting_visual_layer->get_name();
				QAction *action = new QAction(outputting_layer_name, menu);
				boost::function<void ()> fn = boost::bind(
						&GPlatesAppLogic::Layer::connect_input_to_layer_output,
						layer,
						outputting_layer,
						input_data_channel);
				QVariant qv;
				qv.setValue(fn);
				action->setData(qv);
				action->setIcon(
						get_layer_icon(
							locked_outputting_visual_layer->get_reconstruct_graph_layer().get_type()));
				menu->addAction(action);

				++count;
			}
		}
	}

	if (count == 0)
	{
		button->setEnabled(false);
		static const QString NO_APPROPRIATE_LAYERS_TOOLTIP = tr("There are no layers that can supply input to this connection.");
		button->setToolTip(NO_APPROPRIATE_LAYERS_TOOLTIP);
	}
	else
	{
		button->setEnabled(true);
		button->setToolTip(QString());
	}
}


GPlatesQtWidgets::VisualLayerWidget::DraggableWidget::DraggableWidget(
		QWidget *parent_) :
	QWidget(parent_),
	d_row(-1)
{
	setCursor(QCursor(Qt::OpenHandCursor));
}


void
GPlatesQtWidgets::VisualLayerWidget::DraggableWidget::mousePressEvent(
		QMouseEvent *event_)
{
	QMimeData *mime_data = new QMimeData(); // Qt responsible for memory.
	QByteArray encoded_data;
	QDataStream stream(&encoded_data, QIODevice::WriteOnly);
	stream << d_row;
	mime_data->setData(GPlatesGui::VisualLayersListModel::VISUAL_LAYERS_MIME_TYPE, encoded_data);

	QDrag *drag = new QDrag(this);
	drag->setMimeData(mime_data);
	drag->exec();
}


void
GPlatesQtWidgets::VisualLayerWidget::DraggableWidget::set_row(
		int row)
{
	d_row = row;
}

