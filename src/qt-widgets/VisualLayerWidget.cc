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

#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <QPalette>
#include <QResizeEvent>
#include <QFont>
#include <QString>
#include <QMetaType>
#include <QIcon>
#include <QMessageBox>
#include <QMimeData>
#include <QByteArray>
#include <QDataStream>
#include <QDrag>
#include <QInputDialog>
#include <QDebug>

#include "VisualLayerWidget.h"

#include "ElidedLabel.h"
#include "LinkWidget.h"
#include "RasterLayerOptionsWidget.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"
#include "VisualLayersDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/LayerTaskType.h"

#include "file-io/File.h"
#include "file-io/FileInfo.h"

#include "gui/Dialogs.h"
#include "gui/VisualLayersListModel.h"
#include "gui/VisualLayersProxy.h"

#include "presentation/ViewState.h"
#include "presentation/VisualLayer.h"
#include "presentation/VisualLayerInputChannelName.h"
#include "presentation/VisualLayerType.h"
#include "presentation/VisualLayerRegistry.h"


namespace
{
	static const char *NEW_FEATURE_COLLECTION = "New Feature Collection";

	const QIcon &
	get_feature_collection_icon()
	{
		static const QIcon FEATURE_COLLECTION_ICON(":/gnome_text_x_preview_16.png");
		return FEATURE_COLLECTION_ICON;
	}

	const QPixmap &
	get_collapsed_icon()
	{
		static const QPixmap COLLAPSED_ICON(":/gnome_stock_data_next_16.png");
		return COLLAPSED_ICON;
	}

	const QPixmap &
	get_expanded_icon()
	{
		static const QPixmap EXPANDED_ICON(":/gnome_stock_data_next_down_16.png");
		return EXPANDED_ICON;
	}

	const QPixmap &
	get_visible_icon()
	{
		static const QPixmap VISIBLE_ICON(":/inkscape_object_visible_16.png");
		return VISIBLE_ICON;
	}

	const QPixmap &
	get_hidden_icon()
	{
		static const QPixmap HIDDEN_ICON(":/blank_16.png");
		return HIDDEN_ICON;
	}

	const QPixmap &
	get_is_default_icon()
	{
		static const QPixmap IS_DEFAULT_ICON(":/gnome_emblem_default_yellow_16.png");
		return IS_DEFAULT_ICON;
	}
}


Q_DECLARE_METATYPE( boost::function<void ()> )


GPlatesQtWidgets::VisualLayerWidgetInternals::ToggleIcon::ToggleIcon(
		const QPixmap &on_icon,
		const QPixmap &off_icon,
		bool is_clickable,
		bool show_frame_when_clickable,
		QWidget *parent_) :
	QLabel(parent_),
	d_on_icon(on_icon),
	d_off_icon(off_icon),
	d_is_clickable(is_clickable),
	d_show_frame_when_clickable(show_frame_when_clickable)
{
	set_clickable(is_clickable);
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::ToggleIcon::show_icon(
		bool on)
{
	setPixmap(on ? d_on_icon : d_off_icon);
	set_cursor();
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::ToggleIcon::set_clickable(
		bool is_clickable)
{
	d_is_clickable = is_clickable;
	set_cursor();
	setFrameStyle(is_clickable && d_show_frame_when_clickable ?
			QFrame::Panel | QFrame::Sunken :
			QFrame::NoFrame);
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::ToggleIcon::mousePressEvent(
		QMouseEvent *event_)
{
	if (event_->button() == Qt::LeftButton && d_is_clickable)
	{
		Q_EMIT clicked();
	}
	else
	{
		event_->ignore();
	}
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::ToggleIcon::changeEvent(
		QEvent *event_)
{
	if (event_->type() == QEvent::EnabledChange)
	{
		set_cursor();
	}
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::ToggleIcon::set_cursor()
{
	setCursor(isEnabled() && d_is_clickable ?
			Qt::PointingHandCursor : parentWidget()->cursor());
}


GPlatesQtWidgets::VisualLayerWidget::VisualLayerWidget(
		GPlatesGui::VisualLayersProxy &visual_layers,
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	QWidget(parent_),
	d_visual_layers(visual_layers),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window),
	d_row(-1),
	d_left_widget(
			new QWidget(this)),
	d_expand_icon(
			new VisualLayerWidgetInternals::ToggleIcon(
				get_expanded_icon(),
				get_collapsed_icon(),
				true,
				false,
				this)),
	d_visibility_icon(
			new VisualLayerWidgetInternals::ToggleIcon(
				get_visible_icon(),
				get_hidden_icon(),
				true,
				true,
				this)),
	d_is_default_icon(
			new VisualLayerWidgetInternals::ToggleIcon(
				get_is_default_icon(),
				get_hidden_icon(),
				true,
				true,
				this)),
	d_expand_input_channels_icon(
			new VisualLayerWidgetInternals::ToggleIcon(
				get_expanded_icon(),
				get_collapsed_icon(),
				true,
				false,
				this)),
	d_expand_layer_options_icon(
			new VisualLayerWidgetInternals::ToggleIcon(
				get_expanded_icon(),
				get_collapsed_icon(),
				true,
				false,
				this)),
	d_expand_advanced_options_icon(
			new VisualLayerWidgetInternals::ToggleIcon(
				get_expanded_icon(),
				get_collapsed_icon(),
				true,
				false,
				this)),
	d_visibility_default_stackedwidget(
			new QStackedWidget(this)),
	d_name_label(
			new ElidedLabel(Qt::ElideMiddle, this)),
	d_type_label(
			new ElidedLabel(Qt::ElideRight, this)),
	d_input_channels_widget_layout(NULL),
	d_current_layer_options_widget(NULL),
	d_layer_options_widget_layout(NULL),
	d_enable_layer_link(new LinkWidget(this)),
	d_rename_layer_link(new LinkWidget(tr("Rename layer..."), this)),
	d_delete_layer_link(new LinkWidget(tr("Delete layer..."), this))
{
	setupUi(this);
	setCursor(QCursor(Qt::OpenHandCursor));

	// Give the input_channels_widget a layout.
	d_input_channels_widget_layout = new QVBoxLayout(input_channels_widget);
	d_input_channels_widget_layout->setContentsMargins(26, 4, 0, 4);
	d_input_channels_widget_layout->setSpacing(4);

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

	// Install the top icons into their placeholders.
	d_visibility_default_stackedwidget->addWidget(d_visibility_icon);
	d_visibility_default_stackedwidget->addWidget(d_is_default_icon);
	QtWidgetUtils::add_widget_to_placeholder(
			d_visibility_default_stackedwidget,
			visibility_icon_placeholder_widget);
	d_visibility_icon->setToolTip(tr("Toggle Visibility"));
	d_is_default_icon->setToolTip(tr("Set as Default Reconstruction Tree"));

	// Install the other expand icons into their placeholders.
	QtWidgetUtils::add_widget_to_placeholder(
			d_expand_input_channels_icon,
			expand_input_channels_icon_placeholder_widget);
	QtWidgetUtils::add_widget_to_placeholder(
			d_expand_layer_options_icon,
			expand_layer_options_icon_placeholder_widget);
	QtWidgetUtils::add_widget_to_placeholder(
			d_expand_advanced_options_icon,
			expand_advanced_options_icon_placeholder_widget);

	// Give the layer_options_widget a layout.
	d_layer_options_widget_layout = new QVBoxLayout(layer_options_widget);
	d_layer_options_widget_layout->setContentsMargins(26, 0, 0, 0);

	// Install the links.
	QtWidgetUtils::add_widget_to_placeholder(
			d_enable_layer_link,
			enable_layer_placeholder_widget);
	QtWidgetUtils::add_widget_to_placeholder(
			d_rename_layer_link,
			rename_layer_placeholder_widget);
	QtWidgetUtils::add_widget_to_placeholder(
			d_delete_layer_link,
			delete_layer_placeholder_widget);

	make_signal_slot_connections();
}


namespace
{
	GPlatesGui::Colour
	lighten(
			const GPlatesGui::Colour &colour)
	{
		return GPlatesGui::Colour::linearly_interpolate(colour, GPlatesGui::Colour::get_white(), 0.8);
	}

	GPlatesGui::Colour
	darken(
			const GPlatesGui::Colour &colour)
	{
		GPlatesGui::HSVColour hsv = GPlatesGui::Colour::to_hsv(colour);
		hsv.v = 0.25;
		hsv.s *= 0.75;
		return GPlatesGui::Colour::from_hsv(hsv);
	}
}


void
GPlatesQtWidgets::VisualLayerWidget::set_data(
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer,
		int row)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			visual_layer.lock())
	{
		GPlatesPresentation::VisualLayerType::Type visual_layer_type = locked_visual_layer->get_layer_type();
		const GPlatesPresentation::VisualLayerRegistry &visual_layer_registry =
			d_view_state.get_visual_layer_registry();
		GPlatesAppLogic::Layer reconstruct_graph_layer = locked_visual_layer->get_reconstruct_graph_layer();

		// Enable or disable widgets based on whether the layer is active.
		bool is_active = reconstruct_graph_layer.is_active();
		advanced_options_header_widget->setEnabled(is_active);
		other_advanced_options_widget->setEnabled(is_active);
		input_channels_widget->setEnabled(is_active);
		input_channels_header_widget->setEnabled(is_active);
		layer_options_widget->setEnabled(is_active);
		layer_options_header_widget->setEnabled(is_active);
		top_widget->setEnabled(is_active);

		d_enable_layer_link->set_link_text(is_active ? tr("Disable layer") : tr("Enable layer"));

		// Make sure the advanced options are expanded if the layer is disabled.
		// Because this is the only way the user can re-enable the layer.
		// Can happen if disable layer, quit GPlates, open GPlates, restore session - in which case
		// the default is an un-expanded advanced options.
		if (!is_active)
		{
			locked_visual_layer->set_expanded(GPlatesPresentation::VisualLayer::ADVANCED_OPTIONS);
		}

		// Set the expand/collapse icons.
		bool expanded = locked_visual_layer->is_expanded(GPlatesPresentation::VisualLayer::ALL);
		bool input_channels_expanded = locked_visual_layer->is_expanded(GPlatesPresentation::VisualLayer::INPUT_CHANNELS);
		bool layer_options_expanded = locked_visual_layer->is_expanded(GPlatesPresentation::VisualLayer::LAYER_OPTIONS);
		bool advanced_options_expanded = locked_visual_layer->is_expanded(GPlatesPresentation::VisualLayer::ADVANCED_OPTIONS);
		d_expand_icon->show_icon(expanded);
		d_expand_input_channels_icon->show_icon(input_channels_expanded);
		d_expand_layer_options_icon->show_icon(layer_options_expanded);
		d_expand_advanced_options_icon->show_icon(advanced_options_expanded);

		// Set the background colour of various widgets depending on what type of layer it is.
		GPlatesGui::Colour layer_colour = visual_layer_registry.get_colour(visual_layer_type);
		GPlatesGui::Colour light_layer_colour = is_active ? lighten(layer_colour) : GPlatesGui::Colour(0.9f, 0.9f, 0.9f);
		GPlatesGui::Colour dark_layer_colour = is_active ? darken(layer_colour) : GPlatesGui::Colour(0.25f, 0.25f, 0.25f);

		QPalette basic_info_palette;
		basic_info_palette.setColor(QPalette::Text, dark_layer_colour);
		d_name_label->setPalette(basic_info_palette);
		d_type_label->setPalette(basic_info_palette);

		QPalette left_widget_palette;
		left_widget_palette.setColor(QPalette::Base, layer_colour);
		d_left_widget->setPalette(left_widget_palette);

		QPalette section_header_palette;
		section_header_palette.setColor(QPalette::Base, light_layer_colour);
		section_header_palette.setColor(QPalette::Text, dark_layer_colour);
		input_channels_header_widget->setPalette(section_header_palette);
		layer_options_header_widget->setPalette(section_header_palette);
		advanced_options_header_widget->setPalette(section_header_palette);

		bool is_recon_tree_layer = (visual_layer_type ==
			static_cast<GPlatesPresentation::VisualLayerType::Type>(
					GPlatesAppLogic::LayerTaskType::RECONSTRUCTION));
		if (is_recon_tree_layer)
		{
			d_visibility_default_stackedwidget->setCurrentIndex(1);

			// Default reconstruction tree icon.
			bool is_default = (reconstruct_graph_layer ==
					d_application_state.get_reconstruct_graph().get_default_reconstruction_tree_layer());
			d_is_default_icon->show_icon(is_default);
		}
		else
		{
			d_visibility_default_stackedwidget->setCurrentIndex(0);

			// Set the hide/show icon.
			if (visual_layer_registry.produces_rendered_geometries(visual_layer_type))
			{
				d_visibility_icon->show_icon(locked_visual_layer->is_visible());
				d_visibility_icon->set_clickable(true);
			}
			else
			{
				d_visibility_icon->show_icon(false);
				d_visibility_icon->set_clickable(false);
			}
		}

		// Update the basic info.
		d_name_label->setText(locked_visual_layer->get_name());
		d_type_label->setText(visual_layer_registry.get_name(visual_layer_type));

		// Show or hide the details panel as necessary.
		details_widget->setVisible(expanded);

		// Change the layers option widget if type changed since last time.
		if (d_visual_layer.expired() || d_visual_layer.lock()->get_layer_type() != visual_layer_type)
		{
			// Remove the existing widget if there is one.
			if (d_current_layer_options_widget)
			{
				d_layer_options_widget_layout->removeWidget(d_current_layer_options_widget);
				delete d_current_layer_options_widget;
			}

			d_current_layer_options_widget = visual_layer_registry.create_options_widget(
					visual_layer_type,
					d_application_state,
					d_view_state,
					d_viewport_window,
					this);
			if (d_current_layer_options_widget)
			{
				int right;
				d_current_layer_options_widget->layout()->getContentsMargins(NULL, NULL, &right, NULL);
				d_current_layer_options_widget->layout()->setContentsMargins(0, 4, right, 4);

				d_layer_options_widget_layout->addWidget(d_current_layer_options_widget);
				layer_options_header_label->setText(d_current_layer_options_widget->get_title());
				layer_options_header_widget->show();
			}
			else
			{
				layer_options_header_widget->hide();
			}
		}

		// Show or hide the various sections.
		input_channels_widget->setVisible(input_channels_expanded);
		layer_options_widget->setVisible(layer_options_expanded && d_current_layer_options_widget);
		advanced_options_widget->setVisible(advanced_options_expanded);

		// Populate the details panel only if shown.
		if (expanded)
		{
			if (input_channels_expanded)
			{
				// Update the input channel info.
				set_input_channel_data(reconstruct_graph_layer, light_layer_colour);
			}

			// Update the layers option widget.
			if (layer_options_expanded && d_current_layer_options_widget)
			{
				// Need to set the data to that of the visual layer that this VisualLayerWidget will
				// be referencing (note that the reference is not set until the end of this method).
				d_current_layer_options_widget->set_data(visual_layer);

				d_current_layer_options_widget->updateGeometry();
				layer_options_widget->updateGeometry();
			}
		}

		details_widget->updateGeometry();
		right_widget->updateGeometry();

		// Reduces flickering.
		resize(sizeHint());
	}

	// This must be done after the widget has refreshed itself.
	d_visual_layer = visual_layer;
	d_row = row;
}


void
GPlatesQtWidgets::VisualLayerWidget::mousePressEvent(
		QMouseEvent *event_)
{
	if (event_->button() == Qt::LeftButton)
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
	else
	{
		QWidget::mousePressEvent(event_);
	}
}


namespace
{
	template<class InputChannelContainerType>
	void
	move_main_input_channel_to_front(
			InputChannelContainerType &input_channels,
			GPlatesAppLogic::LayerInputChannelName::Type main_input_channel)
	{
		typedef typename InputChannelContainerType::iterator channel_iterator_type;
		channel_iterator_type channel_iter = input_channels.begin();
		for (; channel_iter != input_channels.end(); ++channel_iter)
		{
			if (channel_iter->get_input_channel_name() == main_input_channel)
			{
				// FIXME: This is not efficient if the container is a vector.
				typedef GPlatesAppLogic::LayerInputChannelType input_channel_definition_type;
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
		const GPlatesAppLogic::Layer &layer,
		const GPlatesGui::Colour &light_layer_colour)
{
	typedef GPlatesAppLogic::LayerInputChannelType input_channel_definition_type;

	std::vector<input_channel_definition_type> input_channels =
		layer.get_input_channel_types();

	// Make sure we have enough widgets in our pool to display all input channels.
	if (input_channels.size() > d_input_channel_widgets.size())
	{
		unsigned int num_new_widgets = input_channels.size() - d_input_channel_widgets.size();
		for (unsigned int i = 0; i != num_new_widgets; ++i)
		{
			VisualLayerWidgetInternals::InputChannelWidget *new_widget =
				new VisualLayerWidgetInternals::InputChannelWidget(
						d_visual_layers,
						d_application_state,
						d_view_state,
						this);
			d_input_channel_widgets.push_back(new_widget);
			d_input_channels_widget_layout->addWidget(new_widget);
		}
	}

	// List the main input channel first.
	const GPlatesAppLogic::LayerInputChannelName::Type main_input_channel =
			layer.get_main_input_feature_collection_channel();
	move_main_input_channel_to_front(input_channels, main_input_channel);

	// Display one input channel in one widget.
	typedef std::vector<input_channel_definition_type>::const_iterator channel_iterator_type;
	typedef std::vector<VisualLayerWidgetInternals::InputChannelWidget *>::const_iterator widget_iterator_type;
	channel_iterator_type channel_iter = input_channels.begin();
	widget_iterator_type widget_iter = d_input_channel_widgets.begin();
	for (; channel_iter != input_channels.end(); ++channel_iter, ++widget_iter)
	{
		const input_channel_definition_type &layer_input_channel_type = *channel_iter;
		VisualLayerWidgetInternals::InputChannelWidget *input_channel_widget = *widget_iter;
		input_channel_widget->set_data(
				layer,
				layer_input_channel_type,
				layer.get_channel_inputs(layer_input_channel_type.get_input_channel_name()),
				light_layer_colour);
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

	input_channels_widget->updateGeometry();
}


void
GPlatesQtWidgets::VisualLayerWidget::handle_expand_icon_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_visual_layer.lock())
	{
		locked_visual_layer->toggle_expanded(GPlatesPresentation::VisualLayer::ALL);
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
GPlatesQtWidgets::VisualLayerWidget::handle_is_default_icon_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		d_application_state.get_reconstruct_graph().set_default_reconstruction_tree_layer(layer);
	}
}


void
GPlatesQtWidgets::VisualLayerWidget::handle_expand_input_channels_icon_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_visual_layer.lock())
	{
		locked_visual_layer->toggle_expanded(GPlatesPresentation::VisualLayer::INPUT_CHANNELS);
	}
}


void
GPlatesQtWidgets::VisualLayerWidget::handle_expand_layer_options_icon_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_visual_layer.lock())
	{
		locked_visual_layer->toggle_expanded(GPlatesPresentation::VisualLayer::LAYER_OPTIONS);
	}
}


void
GPlatesQtWidgets::VisualLayerWidget::handle_expand_advanced_options_icon_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_visual_layer.lock())
	{
		locked_visual_layer->toggle_expanded(GPlatesPresentation::VisualLayer::ADVANCED_OPTIONS);
	}
}


void
GPlatesQtWidgets::VisualLayerWidget::handle_enable_layer_link_activated()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		layer.activate(!layer.is_active());
	}
}


void
GPlatesQtWidgets::VisualLayerWidget::handle_rename_layer_link_activated()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_visual_layer.lock())
	{
		QString existing = locked_visual_layer->get_custom_name() ?
			*(locked_visual_layer->get_custom_name()) : QString();
		bool ok;
		QString new_name = QInputDialog::getText(
				&d_viewport_window->dialogs().visual_layers_dialog(),
				tr("Rename Layer"),
				tr("Enter a custom name for the %1 layer.\n"
					"Leave the field blank if you would like GPlates to assign a name automatically.")
					.arg(locked_visual_layer->get_name()),
				QLineEdit::Normal,
				existing,
				&ok);
		if (ok)
		{
			boost::optional<QString> opt_new_name;
			if (!new_name.isEmpty())
			{
				opt_new_name = new_name;
			}
			locked_visual_layer->set_custom_name(opt_new_name);
		}
	}
}


void
GPlatesQtWidgets::VisualLayerWidget::handle_delete_layer_link_activated()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		if (QMessageBox::question(
					&d_viewport_window->dialogs().visual_layers_dialog(),
					tr("Delete Layer"),
					tr("Deleting this layer does not unload any corresponding feature collections. "
						"To unload feature collections, click on Manage Feature Collections on the File menu.\n"
						"Are you sure you want to delete this layer?"),
					QMessageBox::Yes | QMessageBox::No,
					QMessageBox::No) == QMessageBox::Yes)
		{
			d_application_state.get_reconstruct_graph().remove_layer(layer);
		}
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
	QObject::connect(
			d_is_default_icon,
			SIGNAL(clicked()),
			this,
			SLOT(handle_is_default_icon_clicked()));
	QObject::connect(
			d_expand_input_channels_icon,
			SIGNAL(clicked()),
			this,
			SLOT(handle_expand_input_channels_icon_clicked()));
	QObject::connect(
			d_expand_layer_options_icon,
			SIGNAL(clicked()),
			this,
			SLOT(handle_expand_layer_options_icon_clicked()));
	QObject::connect(
			d_expand_advanced_options_icon,
			SIGNAL(clicked()),
			this,
			SLOT(handle_expand_advanced_options_icon_clicked()));

	// Connect to signals from links.
	QObject::connect(
			d_enable_layer_link,
			SIGNAL(link_activated()),
			this,
			SLOT(handle_enable_layer_link_activated()));
	QObject::connect(
			d_rename_layer_link,
			SIGNAL(link_activated()),
			this,
			SLOT(handle_rename_layer_link_activated()));

	QObject::connect(
			d_delete_layer_link,
			SIGNAL(link_activated()),
			this,
			SLOT(handle_delete_layer_link_activated()));
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
	d_input_connection_label->setAutoFillBackground(true);
	d_disconnect_icon->setPixmap(get_disconnect_pixmap());
	d_disconnect_icon->setCursor(QCursor(Qt::PointingHandCursor));
	d_disconnect_icon->setToolTip(tr("Disconnect"));

	// Lay out the internal label and the disconnect icon.
	QHBoxLayout *widget_layout = new QHBoxLayout(this);
	widget_layout->setContentsMargins(0, 0, 0, 0);
	widget_layout->setSpacing(4);
	widget_layout->addWidget(d_input_connection_label);
	widget_layout->addWidget(d_disconnect_icon);
	QSizePolicy label_size_policy = d_input_connection_label->sizePolicy();
	label_size_policy.setHorizontalPolicy(QSizePolicy::Expanding);
	d_input_connection_label->setSizePolicy(label_size_policy);
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::InputConnectionWidget::set_data(
		const GPlatesAppLogic::Layer::InputConnection &input_connection,
		const GPlatesGui::Colour &background_colour)
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
			filename = tr(NEW_FEATURE_COLLECTION);
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

	// Set the background colour.
	QPalette label_palette = d_input_connection_label->palette();
	label_palette.setColor(QPalette::Base, background_colour);
	d_input_connection_label->setPalette(label_palette);
}


const QPixmap &
GPlatesQtWidgets::VisualLayerWidgetInternals::InputConnectionWidget::get_disconnect_pixmap()
{
	static const QPixmap DISCONNECT_PIXMAP(":/tango_list_remove_16.png");
	return DISCONNECT_PIXMAP;
}


GPlatesQtWidgets::VisualLayerWidgetInternals::AddNewConnectionWidget::AddNewConnectionWidget(
		const QString &display_text,
		QMenu *menu,
		QWidget *parent_) :
	QLabel(display_text, parent_),
	d_menu(menu),
	d_menu_open(false)
{
	setAutoFillBackground(true);
	setCursor(Qt::PointingHandCursor);
	d_menu->setCursor(Qt::PointingHandCursor);
	QFont this_font = font();
	this_font.setItalic(true);
	setFont(this_font);
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::AddNewConnectionWidget::mousePressEvent(
		QMouseEvent *ev)
{
	d_menu_open = true;

	QAction *clicked_action = d_menu->exec(mapToGlobal(QPoint(0, height())));
	if (clicked_action)
	{
		typedef boost::function<void ()> fn_type;
		QVariant qv = clicked_action->data();
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
						tr("Add new connection"),
						tr("The requested connection could not be made because it would introduce a cycle."));
			}
		}
	}

	d_menu_open = false;
	setPalette(QPalette());
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::AddNewConnectionWidget::enterEvent(
		QEvent *ev)
{
	if (isEnabled())
	{
		QPalette this_palette = palette();
		this_palette.setColor(QPalette::Base, d_highlight_colour);
		setPalette(this_palette);
	}
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::AddNewConnectionWidget::leaveEvent(
		QEvent *ev)
{
	if (!d_menu_open)
	{
		setPalette(QPalette());
	}
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::AddNewConnectionWidget::changeEvent(
		QEvent *ev)
{
	if (ev->type() == QEvent::EnabledChange)
	{
		// So it doesn't look so fugly on the Mac.
		setAutoFillBackground(isEnabled());
	}
}


GPlatesQtWidgets::VisualLayerWidgetInternals::InputChannelWidget::InputChannelWidget(
		GPlatesGui::VisualLayersProxy &visual_layers,
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_) :
	QWidget(parent_),
	d_visual_layers(visual_layers),
	d_application_state(application_state),
	d_view_state(view_state),
	d_input_channel_name_label(
			new ElidedLabel(Qt::ElideRight, this)),
	d_yet_another_container(
			new QWidget(this)),
	d_input_connection_widgets_container(
			new QWidget(this)),
	d_add_new_connection_menu(
			new QMenu(this)),
	d_add_new_connection_widget(
			new AddNewConnectionWidget(
				tr("Add new connection"),
				d_add_new_connection_menu,
				this)),
	d_input_connection_widgets_layout(NULL)
{
	// This widget has the following subwidgets:
	//  - The name label
	//  - An indented container (d_yet_another_container) that contains in turn:
	//     - A container of input connection widgets
	//     - A widget to add new input connections
	QVBoxLayout *yet_another_layout = new QVBoxLayout(d_yet_another_container);
	yet_another_layout->setContentsMargins(15, 0, 0, 0);
	yet_another_layout->setSpacing(0);
	yet_another_layout->addWidget(d_input_connection_widgets_container);
	QHBoxLayout *add_new_connection_layout = new QHBoxLayout();
	add_new_connection_layout->setContentsMargins(0, 0, 20 /* no disconnect icon on this row */, 0);
	add_new_connection_layout->addWidget(d_add_new_connection_widget);
	yet_another_layout->addLayout(add_new_connection_layout);

	QVBoxLayout *this_layout = new QVBoxLayout(this);
	this_layout->setContentsMargins(0, 0, 0, 0);
	this_layout->setSpacing(4);
	this_layout->addWidget(d_input_channel_name_label);
	this_layout->addWidget(d_yet_another_container);

	// Create a layout for the input connection widgets container.
	d_input_connection_widgets_layout = new QVBoxLayout(d_input_connection_widgets_container);
	d_input_connection_widgets_layout->setContentsMargins(0, 0, 0, 4);
	d_input_connection_widgets_layout->setSpacing(4);
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::InputChannelWidget::set_data(
		const GPlatesAppLogic::Layer &layer,
		const GPlatesAppLogic::LayerInputChannelType &layer_input_channel_type,
		const std::vector<GPlatesAppLogic::Layer::InputConnection> &input_connections,
		const GPlatesGui::Colour &light_layer_colour)
{
	// Compute the connection background colour from the light_layer_colour.
	GPlatesGui::HSVColour grey = GPlatesGui::Colour::to_hsv(light_layer_colour);
	grey.s = 0;
	GPlatesGui::Colour background_colour = GPlatesGui::Colour::linearly_interpolate(
			light_layer_colour,
			GPlatesGui::Colour::from_hsv(grey),
			0.5);
	d_add_new_connection_widget->set_highlight_colour(background_colour);

	// Update the channel name.
	d_input_channel_name_label->setText(
			GPlatesPresentation::VisualLayerInputChannelName::get_input_channel_name(
					layer_input_channel_type.get_input_channel_name())
			+ ":");

	// Disable the add new connection button if the channel only takes one
	// connection and we already have that.
	if (layer_input_channel_type.get_channel_data_arity() == GPlatesAppLogic::LayerInputChannelType::ONE_DATA_IN_CHANNEL &&
			input_connections.size() >= 1)
	{
		d_add_new_connection_widget->setEnabled(false);
		d_add_new_connection_widget->setToolTip(tr("This input channel only accepts one connection."));
	}
	else
	{
		const boost::optional< std::vector<GPlatesAppLogic::LayerTaskType::Type> > &input_data_types =
				layer_input_channel_type.get_layer_input_data_types();
		if (input_data_types)
		{
			populate_with_layers(
					layer,
					layer_input_channel_type.get_input_channel_name(),
					input_data_types.get());
		}
		else
		{
			populate_with_feature_collections(
					layer,
					layer_input_channel_type.get_input_channel_name());
		}
	}

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
		input_connection_widget->set_data(input_connection, background_colour);
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

	d_input_connection_widgets_container->setVisible(input_connections.size() > 0);
	d_input_connection_widgets_container->updateGeometry();
	d_yet_another_container->updateGeometry();
	updateGeometry();

	// Reduces flickering.
	d_input_connection_widgets_container->resize(d_input_connection_widgets_container->sizeHint());
	resize(sizeHint());
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::InputChannelWidget::populate_with_feature_collections(
		const GPlatesAppLogic::Layer &layer,
		const GPlatesAppLogic::LayerInputChannelName::Type input_data_channel)
{
	d_add_new_connection_menu->clear();

	typedef GPlatesAppLogic::FeatureCollectionFileState::file_reference file_reference;
	std::vector<file_reference> loaded_files = d_application_state.get_feature_collection_file_state().get_loaded_files();
	if (loaded_files.size() == 0)
	{
		d_add_new_connection_widget->setEnabled(false);
		d_add_new_connection_widget->setToolTip(tr("No feature collections have been loaded."));
		return;
	}
	else
	{
		d_add_new_connection_widget->setEnabled(true);
		d_add_new_connection_widget->setToolTip(QString());
	}

	GPlatesAppLogic::ReconstructGraph &reconstruct_graph = d_application_state.get_reconstruct_graph();

	BOOST_FOREACH(const file_reference &loaded_file, loaded_files)
	{
		QString display_name = loaded_file.get_file().get_file_info().get_display_name(false);
		if (display_name.isEmpty())
		{
			display_name = tr(NEW_FEATURE_COLLECTION);
		}
		QAction *action = new QAction(display_name, d_add_new_connection_menu);
		boost::function<void ()> fn = boost::bind(
				&GPlatesAppLogic::Layer::connect_input_to_file,
				layer,
				reconstruct_graph.get_input_file(loaded_file),
				input_data_channel);
		QVariant qv;
		qv.setValue(fn);
		action->setData(qv);
		action->setIcon(get_feature_collection_icon());
		d_add_new_connection_menu->addAction(action);
	}
}


void
GPlatesQtWidgets::VisualLayerWidgetInternals::InputChannelWidget::populate_with_layers(
		const GPlatesAppLogic::Layer &layer,
		const GPlatesAppLogic::LayerInputChannelName::Type input_data_channel,
		const std::vector<GPlatesAppLogic::LayerTaskType::Type> &input_data_types)
{
	d_add_new_connection_menu->clear();

	const GPlatesAppLogic::ReconstructGraph &reconstruct_graph = d_application_state.get_reconstruct_graph();
	const GPlatesPresentation::VisualLayerRegistry &visual_layer_registry =
		d_view_state.get_visual_layer_registry();
	unsigned int count = 0;
	BOOST_FOREACH(const GPlatesAppLogic::Layer &outputting_layer, reconstruct_graph)
	{
		// If the current layer matches one of the types in the list of supported input data types.
		if (std::find(input_data_types.begin(), input_data_types.end(), outputting_layer.get_type()) !=
			input_data_types.end())
		{
			boost::weak_ptr<GPlatesPresentation::VisualLayer> outputting_visual_layer =
				d_visual_layers.get_visual_layer(outputting_layer);
			if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_outputting_visual_layer =
					outputting_visual_layer.lock())
			{
				QString outputting_layer_name = locked_outputting_visual_layer->get_name();
				QAction *action = new QAction(outputting_layer_name, d_add_new_connection_menu);
				boost::function<void ()> fn = boost::bind(
						&GPlatesAppLogic::Layer::connect_input_to_layer_output,
						layer,
						outputting_layer,
						input_data_channel);
				QVariant qv;
				qv.setValue(fn);
				action->setData(qv);
				action->setIcon(
						visual_layer_registry.get_icon(
							locked_outputting_visual_layer->get_layer_type()));
				d_add_new_connection_menu->addAction(action);

				++count;
			}
		}
	}

	if (count == 0)
	{
		d_add_new_connection_widget->setEnabled(false);
		d_add_new_connection_widget->setToolTip(tr("There are no layers that can supply input to this connection."));
	}
	else
	{
		d_add_new_connection_widget->setEnabled(true);
		d_add_new_connection_widget->setToolTip(QString());
	}
}

