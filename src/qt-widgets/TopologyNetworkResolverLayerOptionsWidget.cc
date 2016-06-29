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
#include <QFileInfo>
#include <QDir>
#include <QPalette>

#include "TopologyNetworkResolverLayerOptionsWidget.h"

#include "ColourScaleWidget.h"
#include "FriendlyLineEdit.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"

#include "file-io/CptReader.h"
#include "file-io/ReadErrorAccumulation.h"

#include "gui/Colour.h"
#include "gui/ColourPaletteUtils.h"
#include "gui/ColourSpectrum.h"
#include "gui/RasterColourPalette.h"

#include "presentation/TopologyNetworkVisualLayerParams.h"
#include "presentation/VisualLayer.h"

#include "utils/ComponentManager.h"
#include "property-values/XsString.h"


GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::TopologyNetworkResolverLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window),
	d_palette_filename_lineedit(
		new FriendlyLineEdit( 
			QString(), 
			tr("Default Palette"), 
			this)),
	d_open_file_dialog( 
		this, 
		tr("Open CPT File"), 
		tr("Regular CPT file (*.cpt);;All files (*)"), 
		view_state),
	d_colour_scale_widget(
			new ColourScaleWidget(
				view_state, 
				viewport_window, 
				this))
{
	setupUi(this);
	select_palette_filename_button->setCursor(QCursor(Qt::ArrowCursor));
	use_default_palette_button->setCursor(QCursor(Qt::ArrowCursor));
	mesh_checkbox->setCursor(QCursor(Qt::ArrowCursor));
	constrained_checkbox->setCursor(QCursor(Qt::ArrowCursor));
	triangulation_checkbox->setCursor(QCursor(Qt::ArrowCursor));
	total_triangulation_checkbox->setCursor(QCursor(Qt::ArrowCursor));
	fill_checkbox->setCursor(QCursor(Qt::ArrowCursor));
	segment_velocity_checkbox->setCursor(QCursor(Qt::ArrowCursor));	d_palette_filename_lineedit->setReadOnly(true);
	QtWidgetUtils::add_widget_to_placeholder(
		d_palette_filename_lineedit,
		palette_filename_placeholder_widget);

	// set up scale1
	QtWidgetUtils::add_widget_to_placeholder(
			d_colour_scale_widget,
			colour_scale_placeholder_widget);
	QPalette colour_scale_palette = d_colour_scale_widget->palette();
	colour_scale_palette.setColor(QPalette::Window, Qt::white);
	d_colour_scale_widget->setPalette(colour_scale_palette);

	// set up signals and slots
	QObject::connect(
			triangulation_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_triangulation_clicked()));

	QObject::connect(
			constrained_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_constrained_clicked()));

	QObject::connect(
			mesh_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_mesh_clicked()));

	QObject::connect(
			total_triangulation_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_total_triangulation_clicked()));

	QObject::connect(
			segment_velocity_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_segment_velocity_clicked()));

	QObject::connect(
			fill_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_fill_clicked()));

	QObject::connect(
			color_index_combobox,
			SIGNAL(activated(int)),
			this,
			SLOT(handle_color_index_combobox_activated()));

	QObject::connect(
			color_index_combobox,
			SIGNAL(currentIndexChanged(int)),
			this,
			SLOT(handle_color_index_combobox_activated()));

	// update button
	QObject::connect(
			update_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_update_button_clicked()));

	QObject::connect(
			select_palette_filename_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_select_palette_filename_button_clicked()));

	QObject::connect(
			use_default_palette_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_use_default_palette_button_clicked()));


#if 0
	LinkWidget *draw_style_link = new LinkWidget(
			tr("Set Draw style..."), this);

	QtWidgetUtils::add_widget_to_placeholder(
			draw_style_link,
			draw_style_placeholder_widget);
	QObject::connect(
			draw_style_link,
			SIGNAL(link_activated()),
			this,
			SLOT(open_draw_style_setting_dlg()));
	if(!GPlatesUtils::ComponentManager::instance().is_enabled(GPlatesUtils::ComponentManager::Component::python()))
	{
		draw_style_link->setVisible(false);
	}
#endif

}


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new TopologyNetworkResolverLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;

	// Set the state of the checkboxes.
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = 
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());

		if (params)
		{
			// Check boxes
			mesh_checkbox->setChecked(params->show_mesh_triangulation());
			constrained_checkbox->setChecked(params->show_constrained_triangulation());
			triangulation_checkbox->setChecked(params->show_delaunay_triangulation());
			total_triangulation_checkbox->setChecked(params->show_total_triangulation());
			fill_checkbox->setChecked(params->show_fill());
			segment_velocity_checkbox->setChecked(params->show_segment_velocity());

			// Colour Index
			//
			// Changing the current index will emit signals which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.
			QObject::disconnect(
					color_index_combobox, SIGNAL(currentIndexChanged(int)), this,
					SLOT(handle_color_index_combobox_activated()));
			color_index_combobox->setCurrentIndex( params->color_index() );
			QObject::connect(
					color_index_combobox, SIGNAL(currentIndexChanged(int)), this,
					SLOT(handle_color_index_combobox_activated()));

			// Populate the palette filename.
			d_palette_filename_lineedit->setText(params->get_colour_palette_filename());

			// Set the range values
			range1_max->setValue( params->get_range1_max() );
			range1_min->setValue( params->get_range1_min() );

			range2_max->setValue( params->get_range2_max() );
			range2_min->setValue( params->get_range2_min() );


			// Set the fg colour spinbox
			int fg_index = 4; // default is white 
			if 		( params->get_fg_colour() == GPlatesGui::Colour::get_red() ) 	{fg_index = 0;}
			else if ( params->get_fg_colour() == GPlatesGui::Colour::get_yellow() )	{fg_index = 1;}
			else if ( params->get_fg_colour() == GPlatesGui::Colour::get_green() ) 	{fg_index = 2;}
			else if ( params->get_fg_colour() == GPlatesGui::Colour::get_blue() )	{fg_index = 3;}
			else if ( params->get_fg_colour() == GPlatesGui::Colour::get_white() ) 	{fg_index = 4;}
			else if ( params->get_fg_colour() == GPlatesGui::Colour(0.35f, 0.35f, 0.35f) ) 	{fg_index = 5;}
			else if ( params->get_fg_colour() == GPlatesGui::Colour::get_black() )	{fg_index = 6;}
			fg_colour_combobox->setCurrentIndex( fg_index );

			// Set the max colour spinbox
			int max_index = 0;
			if ( 	  params->get_max_colour() == GPlatesGui::Colour::get_red() ) 	{max_index = 0;}
			else if ( params->get_max_colour() == GPlatesGui::Colour::get_yellow() ){max_index = 1;}
			else if ( params->get_max_colour() == GPlatesGui::Colour::get_green() ) {max_index = 2;}
			else if ( params->get_max_colour() == GPlatesGui::Colour::get_blue() ) 	{max_index = 3;}
			else if ( params->get_max_colour() == GPlatesGui::Colour::get_white() ) {max_index = 4;}
			else if ( params->get_max_colour() == GPlatesGui::Colour(0.35f, 0.35f, 0.35f) ) 	{max_index = 5;}
			else if ( params->get_max_colour() == GPlatesGui::Colour::get_black() )	{max_index = 6;}
			max_colour_combobox->setCurrentIndex( max_index );

			// Set the mid colour spinbox
			int mid_index = 0;
			if ( 	  params->get_mid_colour() == GPlatesGui::Colour::get_red() ) 	{mid_index = 0;}
			else if ( params->get_mid_colour() == GPlatesGui::Colour::get_yellow() ){mid_index = 1;}
			else if ( params->get_mid_colour() == GPlatesGui::Colour::get_green() ) {mid_index = 2;}
			else if ( params->get_mid_colour() == GPlatesGui::Colour::get_blue() ) 	{mid_index = 3;}
			else if ( params->get_mid_colour() == GPlatesGui::Colour::get_white() ) {mid_index = 4;}
			else if ( params->get_mid_colour() == GPlatesGui::Colour(0.35f, 0.35f, 0.35f) ) 	{mid_index = 5;}
			else if ( params->get_mid_colour() == GPlatesGui::Colour::get_black() )	{mid_index = 6;}
			mid_colour_combobox->setCurrentIndex( mid_index );

			// Set the min colour spinbox
			int min_index = 0;
			if ( 	  params->get_min_colour() == GPlatesGui::Colour::get_red() ) 	{min_index = 0;}
			else if ( params->get_min_colour() == GPlatesGui::Colour::get_yellow() ){min_index = 1;}
			else if ( params->get_min_colour() == GPlatesGui::Colour::get_green() ) {min_index = 2;}
			else if ( params->get_min_colour() == GPlatesGui::Colour::get_blue() ) 	{min_index = 3;}
			else if ( params->get_min_colour() == GPlatesGui::Colour::get_white() ) {min_index = 4;}
			else if ( params->get_min_colour() == GPlatesGui::Colour(0.35f, 0.35f, 0.35f) ) 	{min_index = 5;}
			else if ( params->get_min_colour() == GPlatesGui::Colour::get_black() )	{min_index = 6;}
			min_colour_combobox->setCurrentIndex( min_index );

			// Set the bg colour spinbox
			int bg_index = 4; // default is white 
			if 		( params->get_bg_colour() == GPlatesGui::Colour::get_red() ) 	{bg_index = 0;}
			else if ( params->get_bg_colour() == GPlatesGui::Colour::get_yellow() )	{bg_index = 1;}
			else if ( params->get_bg_colour() == GPlatesGui::Colour::get_green() ) 	{bg_index = 2;}
			else if ( params->get_bg_colour() == GPlatesGui::Colour::get_blue() )	{bg_index = 3;}
			else if ( params->get_bg_colour() == GPlatesGui::Colour::get_white() ) 	{bg_index = 4;}
			else if ( params->get_bg_colour() == GPlatesGui::Colour(0.35f, 0.35f, 0.35f) ) 	{bg_index = 5;}
			else if ( params->get_bg_colour() == GPlatesGui::Colour::get_black() )	{bg_index = 6;}
			bg_colour_combobox->setCurrentIndex( bg_index );

			if (params->get_colour_palette())
			{
				d_colour_scale_widget->populate(
						GPlatesGui::RasterColourPalette::create<double>(
								params->get_colour_palette().get()));
			}
			else
			{
				d_colour_scale_widget->populate(
						GPlatesGui::RasterColourPalette::create());
			}

			colour_scale_placeholder_widget->setVisible(true);
		}
	}
}


const QString &
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::get_title()
{
	static const QString TITLE = tr("Network & Triangulation options");
	return TITLE;
}


void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_mesh_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_show_mesh_triangulation(mesh_checkbox->isChecked());
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_constrained_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_show_constrained_triangulation(constrained_checkbox->isChecked());
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_triangulation_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_show_delaunay_triangulation(triangulation_checkbox->isChecked());
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_total_triangulation_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_show_total_triangulation(total_triangulation_checkbox->isChecked());
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_segment_velocity_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_show_segment_velocity(segment_velocity_checkbox->isChecked());
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_fill_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_show_fill(fill_checkbox->isChecked());
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_color_index_combobox_activated()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_color_index( color_index_combobox->currentIndex() );
		}
	}
}



void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_update_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!params)
		{
			return;
		}

		// Set the values
		params->set_range1_max( range1_max->value() );
		params->set_range1_min( range1_min->value() );

		params->set_range2_max( range2_max->value() );
		params->set_range2_min( range2_min->value() );

		// Get the fg colour from the combobox index	
		int fg_index = fg_colour_combobox->currentIndex();
		GPlatesGui::Colour fg_colour;
		if 		( fg_index == 0 ) { fg_colour = GPlatesGui::Colour::get_red(); }
		else if ( fg_index == 1 ) { fg_colour = GPlatesGui::Colour::get_yellow(); }
		else if ( fg_index == 2 ) { fg_colour = GPlatesGui::Colour::get_green(); }
		else if ( fg_index == 3 ) { fg_colour = GPlatesGui::Colour::get_blue(); }
		else if ( fg_index == 4 ) { fg_colour = GPlatesGui::Colour::get_white(); }
		else if ( fg_index == 5 ) { fg_colour = GPlatesGui::Colour(0.35f, 0.35f, 0.35f); }
		else if ( fg_index == 6 ) { fg_colour = GPlatesGui::Colour::get_black(); }
		else					  { fg_colour = GPlatesGui::Colour::get_black(); }
		// Set the colour
		params->set_fg_colour( fg_colour );

		// Get the max colour from the combobox index	
		int max_index = max_colour_combobox->currentIndex();
		GPlatesGui::Colour max_colour;
		if 		( max_index == 0 ) { max_colour = GPlatesGui::Colour::get_red(); }
		else if ( max_index == 1 ) { max_colour = GPlatesGui::Colour::get_yellow(); }
		else if ( max_index == 2 ) { max_colour = GPlatesGui::Colour::get_green(); }
		else if ( max_index == 3 ) { max_colour = GPlatesGui::Colour::get_blue(); }
		else if ( max_index == 4 ) { max_colour = GPlatesGui::Colour::get_white(); }
		else if ( max_index == 5 ) { max_colour = GPlatesGui::Colour(0.35f, 0.35f, 0.35f); }
		else if ( max_index == 6 ) { max_colour = GPlatesGui::Colour::get_black(); }
		else		               { max_colour = GPlatesGui::Colour::get_white(); }
		// Set the colour
		params->set_max_colour( max_colour );

		// Get the mid colour from the combobox index	
		int mid_index = mid_colour_combobox->currentIndex();
		GPlatesGui::Colour mid_colour;
		if 		( mid_index == 0 ) { mid_colour = GPlatesGui::Colour::get_red(); }
		else if ( mid_index == 1 ) { mid_colour = GPlatesGui::Colour::get_yellow(); }
		else if ( mid_index == 2 ) { mid_colour = GPlatesGui::Colour::get_green(); }
		else if ( mid_index == 3 ) { mid_colour = GPlatesGui::Colour::get_blue(); }
		else if ( mid_index == 4 ) { mid_colour = GPlatesGui::Colour::get_white(); }
		else if ( mid_index == 5 ) { mid_colour = GPlatesGui::Colour(0.35f, 0.35f, 0.35f); }
		else if ( mid_index == 6 ) { mid_colour = GPlatesGui::Colour::get_black(); }
		else					   { mid_colour = GPlatesGui::Colour::get_grey(); }
		// Set the colour
		params->set_mid_colour( mid_colour );

		// Get the min colour from the combobox index	
		int min_index = min_colour_combobox->currentIndex();
		GPlatesGui::Colour min_colour;
		if 		( min_index == 0 ) { min_colour = GPlatesGui::Colour::get_red(); }
		else if ( min_index == 1 ) { min_colour = GPlatesGui::Colour::get_yellow(); }
		else if ( min_index == 2 ) { min_colour = GPlatesGui::Colour::get_green(); }
		else if ( min_index == 3 ) { min_colour = GPlatesGui::Colour::get_blue(); }
		else if ( min_index == 4 ) { min_colour = GPlatesGui::Colour::get_white(); }
		else if ( min_index == 5 ) { min_colour = GPlatesGui::Colour(0.35f, 0.35f, 0.35f); }
		else if ( min_index == 6 ) { min_colour = GPlatesGui::Colour::get_black(); }
		else					   { min_colour = GPlatesGui::Colour::get_black(); }
		// Set the colour
		params->set_min_colour( min_colour );

		// Get the bg colour from the combobox index	
		int bg_index = bg_colour_combobox->currentIndex();
		GPlatesGui::Colour bg_colour;
		if 		( bg_index == 0 ) { bg_colour = GPlatesGui::Colour::get_red(); }
		else if ( bg_index == 1 ) { bg_colour = GPlatesGui::Colour::get_yellow(); }
		else if ( bg_index == 2 ) { bg_colour = GPlatesGui::Colour::get_green(); }
		else if ( bg_index == 3 ) { bg_colour = GPlatesGui::Colour::get_blue(); }
		else if ( bg_index == 4 ) { bg_colour = GPlatesGui::Colour::get_white(); }
		else if ( bg_index == 5 ) { bg_colour = GPlatesGui::Colour(0.35f, 0.35f, 0.35f); }
		else if ( bg_index == 6 ) { bg_colour = GPlatesGui::Colour::get_black(); }
		else					  { bg_colour = GPlatesGui::Colour::get_black(); }
		// Set the colour
		params->set_bg_colour( bg_colour );


		// Create the palette
		params->user_generated_colour_palette();
	}
}


void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_select_palette_filename_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!params)
		{
			return;
		}

		QString palette_file_name = d_open_file_dialog.get_open_file_name();
		if (palette_file_name.isEmpty())
		{
			return;
		}

		d_view_state.get_last_open_directory() = QFileInfo(palette_file_name).path();

		GPlatesFileIO::ReadErrorAccumulation cpt_read_errors;
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette =
				GPlatesGui::ColourPaletteUtils::read_cpt_raster_colour_palette(
						palette_file_name,
						// We only allow real-valued colour palettes since our data is real-valued...
						false/*allow_integer_colour_palette*/,
						cpt_read_errors);

		// If we successfully read a real-valued colour palette.
		boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> colour_palette =
				GPlatesGui::RasterColourPaletteExtract::get_colour_palette<double>(*raster_colour_palette);
		if (colour_palette)
		{
			params->set_colour_palette(palette_file_name, colour_palette.get());

			d_palette_filename_lineedit->setText(QDir::toNativeSeparators(palette_file_name));
		}

		// Show any read errors.
		if (cpt_read_errors.size() > 0)
		{
			d_viewport_window->handle_read_errors(cpt_read_errors);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_use_default_palette_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			handle_update_button_clicked();
		}
	}
}



void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::open_draw_style_setting_dlg()
{
	QtWidgetUtils::pop_up_dialog(d_draw_style_dialog_ptr);
	d_draw_style_dialog_ptr->reset(d_current_visual_layer);
}

