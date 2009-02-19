/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008, 2009 The University of Sydney, Australia
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

#include <QLocale>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFrame>
#include <QLabel>
#include <QSizePolicy>

#include "ReconstructionViewWidget.h"
#include "ViewportWindow.h"
#include "GlobeCanvas.h"
#include "AnimateControlWidget.h"
#include "ZoomControlWidget.h"
#include "TimeControlWidget.h"
#include "ProjectionControlWidget.h"

#include "maths/PointOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "utils/FloatingPointComparisons.h"


namespace
{
	/**
	 * Wraps Qt widget up inside a frame suitably styled for ReconstructionViewWidget.
	 * Frame takes ownership of your widget, but you need to add the returned Frame
	 * to something so Qt can take ownership of the whole thing.
	 */
	QFrame *
	wrap_with_frame(
			QWidget *widget)
	{
		QFrame *frame = new QFrame();
		frame->setFrameShape(QFrame::StyledPanel);
		frame->setFrameShadow(QFrame::Sunken);
		QHBoxLayout *hbox = new QHBoxLayout(frame);
		hbox->setSpacing(2);
		hbox->setContentsMargins(0, 0, 0, 0);
		hbox->addWidget(widget);
		return frame;
	}

	/**
	 * Wraps Qt layout (or spacer) up inside a frame suitably styled for ReconstructionViewWidget.
	 * Frame takes ownership of your item, but you need to add the returned Frame
	 * to something so Qt can take ownership of the whole thing.
	 */
	QFrame *
	wrap_with_frame(
			QLayoutItem *item)
	{
		QFrame *frame = new QFrame();
		frame->setFrameShape(QFrame::StyledPanel);
		frame->setFrameShadow(QFrame::Sunken);
		QHBoxLayout *hbox = new QHBoxLayout(frame);
		hbox->setSpacing(2);
		hbox->setContentsMargins(0, 0, 0, 0);
		hbox->addItem(item);
		return frame;
	}
	
	/**
	 * This function is a bit of a hack, but we need this hack in enough places
	 * in our hybrid Designer/C++ laid-out ReconstructionViewWidget that it's worthwhile
	 * compressing it into an anonymous namespace function.
	 *
	 * The problem: We want to replace a 'placeholder' widget that we set up in the
	 * designer with a widget we created in code via new.
	 *
	 * The solution: make an 'invisible' layout inside the placeholder (@a outer_widget),
	 * then add the real widget (@a inner_widget) to that layout.
	 */
	void
	cram_widget_into_widget(
			QWidget *inner_widget,
			QWidget *outer_widget)
	{
		QHBoxLayout *invisible_layout = new QHBoxLayout(outer_widget);
		invisible_layout->setSpacing(0);
		invisible_layout->setContentsMargins(0, 0, 0, 0);
		invisible_layout->addWidget(inner_widget);
	}
	
	/**
	 * Slightly less awkward way to summon a horizontal spacer.
	 */
	QSpacerItem *
	new_horizontal_spacer()
	{
		return new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
	}
	
	/**
	 * Creates the label used for camera coordinate display.
	 */
	QLabel *
	new_camera_coords_label()
	{
		QLabel *label_ptr = new QLabel(QObject::tr("(lat: ---.-- ; lon: ---.-- )"));
		
		QSizePolicy size(QSizePolicy::Preferred, QSizePolicy::Preferred);
		size.setHorizontalStretch(0);
		size.setVerticalStretch(0);
		size.setHeightForWidth(label_ptr->sizePolicy().hasHeightForWidth());
		label_ptr->setSizePolicy(size);
		label_ptr->setMinimumSize(QSize(170, 0));
		
		return label_ptr;
	}

	/**
	 * Creates the label used for mouse coordinate display.
	 */
	QLabel *
	new_mouse_coords_label()
	{
		QLabel *label_ptr = new QLabel(QObject::tr("(lat: ---.-- ; lon: ---.-- ) (off globe)"));
		
		QSizePolicy size(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
		size.setHorizontalStretch(0);
		size.setVerticalStretch(0);
		size.setHeightForWidth(label_ptr->sizePolicy().hasHeightForWidth());
		label_ptr->setSizePolicy(size);
		label_ptr->setMinimumSize(QSize(231, 0));
		
		return label_ptr;
	}
}



GPlatesQtWidgets::ReconstructionViewWidget::ReconstructionViewWidget(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesGui::AnimationController &animation_controller,
		ViewportWindow &view_state,
		QWidget *parent_):
	QWidget(parent_),
	d_splitter_widget(new QSplitter(this))
{
	setupUi(this);

	// ensures that this widget accepts keyEvents, so that the keyPressEvent method is processed from start-up,
	// irrespective on which window (if any) the user has clicked. 
	setFocusPolicy(Qt::StrongFocus);

#if 0
	// Create the GlobeCanvas,
	d_canvas_ptr = new GlobeCanvas(view_state, this);
	// and add it to the grid layout in the ReconstructionViewWidget.
	// Note this is a bit of a hack, relying on the QGridLayout set up in the Designer.

	gridLayout->addWidget(d_canvas_ptr, 0, 0);
#endif
	// Create the GlobeCanvas.
	d_globe_canvas_ptr = new GlobeCanvas(rendered_geom_collection, view_state, this);
	// Create the ZoomSliderWidget for the right-hand side.
	d_zoom_slider_widget = new ZoomSliderWidget(d_globe_canvas_ptr->viewport_zoom(), this);


	// Construct the Awesome Bar. This used to go on top, but we want to push this
	// down so it goes to the left of the splitter, giving the TaskPanel some more
	// room.
	std::auto_ptr<QWidget> awesomebar_one = construct_awesomebar_one(animation_controller);

	// Construct the "View Bar" for the bottom.
	std::auto_ptr<QWidget> viewbar = construct_viewbar(d_globe_canvas_ptr->viewport_zoom());


	// With all our widgets constructed, on to the main canvas layout:-

	// Create a tiny invisible widget with a tiny invisible horizontal layout to
	// hold the "canvas" area (including the zoom slider). This layout will glue
	// the zoom slider to the right hand side of the canvas. We set a custom size
	// policy in an attempt to make sure that the GlobeCanvas+ZoomSlider eat as
	// much space as possible, leaving the TaskPanel to the default minimum.
	QWidget *canvas_widget = new QWidget(d_splitter_widget);
	QSizePolicy canvas_widget_size_policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	canvas_widget_size_policy.setHorizontalStretch(255);
	canvas_widget->setSizePolicy(canvas_widget_size_policy);
	// Another hack (but for stretchable-task-panel reasons I'm having to do things
	// this way for now), add two AwesomeBars to the top of this canvas_widget,
	// allowing the TaskPanel to consume more vertical space.
	QVBoxLayout *bars_plus_canvas_layout = new QVBoxLayout(canvas_widget);
	bars_plus_canvas_layout->setSpacing(2);
	bars_plus_canvas_layout->setContentsMargins(0, 0, 0, 0);
	bars_plus_canvas_layout->addWidget(awesomebar_one.release());
	// Globe and slider:
	QHBoxLayout *canvas_widget_layout = new QHBoxLayout();
	bars_plus_canvas_layout->addItem(canvas_widget_layout);
	canvas_widget_layout->setSpacing(2);
	canvas_widget_layout->setContentsMargins(2, 4, 0, 0);
	// Add GlobeCanvas and ZoomSliderWidget to this hand-made widget.
	// NOTE: If we had a MapCanvas, we'd add it here too.
	canvas_widget_layout->addWidget(d_globe_canvas_ptr);
	canvas_widget_layout->addWidget(d_zoom_slider_widget);
	
	// Then add that widget (globe (+ map) + zoom slider) to the QSplitter.
	d_splitter_widget->addWidget(canvas_widget);
	// The splitter should eat as much space as possible.
	QSizePolicy splitter_size(QSizePolicy::Expanding, QSizePolicy::Expanding);
	d_splitter_widget->setSizePolicy(splitter_size);
	
	// Add the QSplitter and the View Bar to the placeholder widget in the ReconstructionViewWidget.
	// Note this is a bit of a hack, relying on the canvas_taskpanel_place_holder widget
	// set up in the Designer.
	// FIXME: I am not yet replacing the use of canvas_taskpanel_place_holder with 'this',
	// as a bug emerges where the globe does not render properly. This can be fixed,
	// but the current method works and there are more urgent things to attack right now.
	QVBoxLayout *splitter_plus_viewbar_layout = new QVBoxLayout(canvas_taskpanel_place_holder);
	splitter_plus_viewbar_layout->setSpacing(2);
	splitter_plus_viewbar_layout->setContentsMargins(0, 0, 0, 0);
	splitter_plus_viewbar_layout->addWidget(d_splitter_widget);
	splitter_plus_viewbar_layout->addWidget(viewbar.release());



#if 0
<<<<<<< .working
	// Set up the Reconstruction Time slider 
	slider_reconstruction_time->setRange(0, 300); // FIXME : use the above values? 
	slider_reconstruction_time->setValue(0);
	slider_reconstruction_time->setInvertedAppearance( true );
	slider_reconstruction_time->setInvertedControls( true );

	// synchronize the slider and spinbox 
	QObject::connect(slider_reconstruction_time, SIGNAL(valueChanged(int)),
			this, SLOT(set_reconstruction_time_int(int)));


	// Listen for zoom events, everything is now handled through ViewportZoom.
	GPlatesGui::ViewportZoom &vzoom = d_globe_canvas_ptr->viewport_zoom();
	QObject::connect(&vzoom, SIGNAL(zoom_changed()),
			this, SLOT(handle_zoom_change()));

	// Zoom buttons.
	QObject::connect(button_zoom_in, SIGNAL(clicked()),
			&vzoom, SLOT(zoom_in()));
	QObject::connect(button_zoom_out, SIGNAL(clicked()),
			&vzoom, SLOT(zoom_out()));
	QObject::connect(button_zoom_reset, SIGNAL(clicked()),
			&vzoom, SLOT(reset_zoom()));

	// Zoom spinbox.
	QObject::connect(spinbox_zoom_percent, SIGNAL(editingFinished()),
			this, SLOT(propagate_zoom_percent()));
	QObject::connect(spinbox_zoom_percent, SIGNAL(editingFinished()),
			d_globe_canvas_ptr, SLOT(setFocus()));

	// Zoom slider.
	QObject::connect(d_zoom_slider_widget, SIGNAL(slider_moved(int)),
			&vzoom, SLOT(set_zoom_level(int)));

=======
#endif
	// Miscellaneous signal / slot connections that ReconstructionViewWidget deals with.
	
	QObject::connect(&(d_globe_canvas_ptr->globe().orientation()), SIGNAL(orientation_changed()),
			d_globe_canvas_ptr, SLOT(notify_of_orientation_change()));
	QObject::connect(&(d_globe_canvas_ptr->globe().orientation()), SIGNAL(orientation_changed()),
			this, SLOT(recalc_camera_position()));
	QObject::connect(&(d_globe_canvas_ptr->globe().orientation()), SIGNAL(orientation_changed()),
			d_globe_canvas_ptr, SLOT(force_mouse_pointer_pos_change()));

	recalc_camera_position();
}


std::auto_ptr<QWidget>
GPlatesQtWidgets::ReconstructionViewWidget::construct_awesomebar_one(
		GPlatesGui::AnimationController &animation_controller)
{
	// We create the bar widget in an auto_ptr, because it has no Qt parent yet.
	// auto_ptr will keep tabs on the memory for us until we can return it
	// and add it to the main ReconstructionViewWidget somewhere.
	std::auto_ptr<QWidget> awesomebar_widget(new QWidget);
	QHBoxLayout *awesomebar_layout = new QHBoxLayout(awesomebar_widget.get());
	awesomebar_layout->setSpacing(2);
	awesomebar_layout->setContentsMargins(0, 0, 0, 0);
	
	// Create the AnimateControlWidget.
	d_animate_control_widget_ptr = new AnimateControlWidget(animation_controller, awesomebar_widget.get());

	// Create the TimeControlWidget.
	d_time_control_widget_ptr = new TimeControlWidget(animation_controller, awesomebar_widget.get());
	QObject::connect(d_time_control_widget_ptr, SIGNAL(editing_finished()),
			d_globe_canvas_ptr, SLOT(setFocus()));
	
	// Insert Time and Animate controls.
	awesomebar_layout->addWidget(wrap_with_frame(d_time_control_widget_ptr));
	awesomebar_layout->addWidget(wrap_with_frame(d_animate_control_widget_ptr));

	return awesomebar_widget;
}


std::auto_ptr<QWidget>
GPlatesQtWidgets::ReconstructionViewWidget::construct_awesomebar_two(
		GPlatesGui::ViewportZoom &vzoom)
{
	// We create the bar widget in an auto_ptr, because it has no Qt parent yet.
	// auto_ptr will keep tabs on the memory for us until we can return it
	// and add it to the main ReconstructionViewWidget somewhere.
	std::auto_ptr<QWidget> awesomebar_widget(new QWidget);
	QHBoxLayout *awesomebar_layout = new QHBoxLayout(awesomebar_widget.get());
	awesomebar_layout->setSpacing(2);
	awesomebar_layout->setContentsMargins(0, 0, 0, 0);
		
	// Insert Zoom and Projection controls.
	awesomebar_layout->addWidget(wrap_with_frame(new ProjectionControlWidget(awesomebar_widget.get())));

	return awesomebar_widget;
}


std::auto_ptr<QWidget>
GPlatesQtWidgets::ReconstructionViewWidget::construct_viewbar(
		GPlatesGui::ViewportZoom &vzoom)
{
	// We create the bar widget in an auto_ptr, because it has no Qt parent yet.
	// auto_ptr will keep tabs on the memory for us until we can return it
	// and add it to the main ReconstructionViewWidget somewhere.
	std::auto_ptr<QWidget> viewbar_widget(new QWidget);
	QHBoxLayout *viewbar_layout = new QHBoxLayout(viewbar_widget.get());
	viewbar_layout->setSpacing(2);
	viewbar_layout->setContentsMargins(0, 0, 0, 0);
	
	// Create the Camera Coordinates label widget.
	d_label_camera_coords = new_camera_coords_label();
	QWidget *camera_coords_widget = new QWidget(viewbar_widget.get());
	QHBoxLayout *camera_coords_layout = new QHBoxLayout(camera_coords_widget);
	camera_coords_layout->setSpacing(2);
	camera_coords_layout->setContentsMargins(2, 2, 2, 2);
	camera_coords_layout->addWidget(new QLabel(tr("Camera:")));
	camera_coords_layout->addWidget(d_label_camera_coords);

	// Create the Mouse Coordinates label widget.
	d_label_mouse_coords = new_mouse_coords_label();
	QWidget *mouse_coords_widget = new QWidget(viewbar_widget.get());
	QHBoxLayout *mouse_coords_layout = new QHBoxLayout(mouse_coords_widget);
	mouse_coords_layout->setSpacing(2);
	mouse_coords_layout->setContentsMargins(2, 2, 2, 2);
	mouse_coords_layout->addWidget(new QLabel(tr("Mouse:")));
	mouse_coords_layout->addWidget(d_label_mouse_coords);

	// Create the ZoomControlWidget.
	d_zoom_control_widget_ptr = new ZoomControlWidget(vzoom, viewbar_widget.get());
	QObject::connect(d_zoom_control_widget_ptr, SIGNAL(editing_finished()),
			d_globe_canvas_ptr, SLOT(setFocus()));

	// Insert Zoom control and coordinate labels.
	viewbar_layout->addWidget(wrap_with_frame(d_zoom_control_widget_ptr));
	viewbar_layout->addWidget(wrap_with_frame(camera_coords_widget));
	viewbar_layout->addWidget(wrap_with_frame(mouse_coords_widget));

	return viewbar_widget;
}



void
GPlatesQtWidgets::ReconstructionViewWidget::insert_task_panel(
		std::auto_ptr<GPlatesQtWidgets::TaskPanel> task_panel)
{
	// Add the Task Panel to the right-hand edge of the QSplitter in
	// the middle of the ReconstructionViewWidget.
	d_splitter_widget->addWidget(task_panel.release());
}


void
GPlatesQtWidgets::ReconstructionViewWidget::activate_time_spinbox()
{
	d_time_control_widget_ptr->activate_time_spinbox();
}


void
GPlatesQtWidgets::ReconstructionViewWidget::recalc_camera_position()
{
	static const GPlatesMaths::PointOnSphere centre_of_canvas =
			GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(0, 0));

	GPlatesMaths::PointOnSphere oriented_centre = d_globe_canvas_ptr->globe().Orient(centre_of_canvas);
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(oriented_centre);

	QLocale locale_;
	QString lat = locale_.toString(llp.latitude(), 'f', 2);
	QString lon = locale_.toString(llp.longitude(), 'f', 2);
	QString position_as_string(QObject::tr("(lat: "));
	position_as_string.append(lat);
	position_as_string.append(QObject::tr(" ; lon: "));
	position_as_string.append(lon);
	position_as_string.append(QObject::tr(")"));

	d_label_camera_coords->setText(position_as_string);
}


void
GPlatesQtWidgets::ReconstructionViewWidget::update_mouse_pointer_position(
		const GPlatesMaths::PointOnSphere &new_virtual_pos,
		bool is_on_globe)
{
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(new_virtual_pos);

	QLocale locale_;
	QString lat = locale_.toString(llp.latitude(), 'f', 2);
	QString lon = locale_.toString(llp.longitude(), 'f', 2);
	QString position_as_string(QObject::tr("(lat: "));
	position_as_string.append(lat);
	position_as_string.append(QObject::tr(" ; lon: "));
	position_as_string.append(lon);
	position_as_string.append(QObject::tr(")"));
	if ( ! is_on_globe) {
		position_as_string.append(QObject::tr(" (off globe)"));
	}

	d_label_mouse_coords->setText(position_as_string);
}


void
GPlatesQtWidgets::ReconstructionViewWidget::activate_zoom_spinbox()
{
	d_zoom_control_widget_ptr->activate_zoom_spinbox();
}



