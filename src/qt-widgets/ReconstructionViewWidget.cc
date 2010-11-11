/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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
#include <QToolButton>
#include <QMenu>

#include "ReconstructionViewWidget.h"

#include "ViewportWindow.h"
#include "GlobeCanvas.h"
#include "AnimateControlWidget.h"
#include "ZoomControlWidget.h"
#include "TimeControlWidget.h"
#include "ProjectionControlWidget.h"
#include "MapCanvas.h"
#include "MapView.h"
#include "LeaveFullScreenButton.h"
#include "GlobeAndMapWidget.h"

#include "gui/MapProjection.h"
#include "gui/MapTransform.h"
#include "gui/ViewportProjection.h"

#include "maths/PointOnSphere.h"
#include "maths/LatLonPoint.h"

#include "presentation/ViewState.h"

#include "qt-widgets/TaskPanel.h"


namespace
{
	const QString
	DEFAULT_MOUSE_COORDS_LABEL_TEXT_FOR_GLOBE = QObject::tr("(lat: ---.-- ; lon: ---.-- ) (off globe)");

	const QString
	DEFAULT_MOUSE_COORDS_LABEL_TEXT_FOR_MAP = QObject::tr("(lat: ---.-- ; lon: ---.-- ) (off map)");

	const QString
	DEFAULT_CAMERA_COORDS_LABEL_TEXT = QObject::tr("(lat: ---.-- ; lon: ---.-- )");

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
		QLabel *label_ptr = new QLabel(DEFAULT_CAMERA_COORDS_LABEL_TEXT);
		
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
		QLabel *label_ptr = new QLabel(DEFAULT_MOUSE_COORDS_LABEL_TEXT_FOR_GLOBE);
		
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
		GPlatesGui::AnimationController &animation_controller,
		ViewportWindow &viewport_window,
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_):
	QWidget(parent_),
	d_view_state(view_state),
	d_splitter_widget(new QSplitter(this))
{
	setupUi(this);

	d_globe_and_map_widget_ptr = new GlobeAndMapWidget(view_state, this);
	
	// Construct the Awesome Bar. This used to go on top, but we want to push this
	// down so it goes to the left of the splitter, giving the TaskPanel some more
	// room.
	std::auto_ptr<QWidget> awesomebar_one = construct_awesomebar_one(
			animation_controller, viewport_window);

	// Construct the "View Bar" for the bottom.
	std::auto_ptr<QWidget> viewbar = 
		construct_viewbar_with_projections(view_state.get_viewport_zoom(),
										   view_state.get_viewport_projection());

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

	// Globe/Map + slider
	QHBoxLayout *canvas_widget_layout = new QHBoxLayout();
	bars_plus_canvas_layout->addItem(canvas_widget_layout);
	canvas_widget_layout->setSpacing(2);
	canvas_widget_layout->setContentsMargins(0, 0, 0, 0);

	// Add GlobeCanvas, MapView to this hand-made widget.
	d_globe_and_map_widget_ptr->setParent(canvas_widget);
	canvas_widget_layout->addWidget(d_globe_and_map_widget_ptr);

	// Add ZoomSliderWidget to this hand-made widget too.
	d_zoom_slider_widget = new ZoomSliderWidget(view_state.get_viewport_zoom(), canvas_widget);
	canvas_widget_layout->addWidget(d_zoom_slider_widget);

	// Sometimes if the user moves off the splitter too quickly, the mouse pointer stays as the splitter cursor
	d_zoom_slider_widget->setCursor(Qt::ArrowCursor);
	
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

	// Handle signals to recalculate the camera position
	QObject::connect(
			&(d_globe_and_map_widget_ptr->get_globe_canvas().globe().orientation()),
			SIGNAL(orientation_changed()),
			this,
			SLOT(recalc_camera_position()));
	QObject::connect(
			&(view_state.get_map_transform()),
			SIGNAL(transform_changed(const GPlatesGui::MapTransform &)),
			this,
			SLOT(recalc_camera_position()));

	// Handle signals to update mouse pointer position
	QObject::connect(
			&(d_globe_and_map_widget_ptr->get_globe_canvas()),
			SIGNAL(mouse_pointer_position_changed(const GPlatesMaths::PointOnSphere &, bool)),
			this,
			SLOT(update_mouse_pointer_position(const GPlatesMaths::PointOnSphere &, bool)));
	QObject::connect(
			&(d_globe_and_map_widget_ptr->get_map_view()),
			SIGNAL(mouse_pointer_position_changed(const boost::optional<GPlatesMaths::LatLonPoint> &, bool)),
			this,
			SLOT(update_mouse_pointer_position(const boost::optional<GPlatesMaths::LatLonPoint> &, bool)));
	QObject::connect(
			&(view_state.get_viewport_projection()),
			SIGNAL(projection_type_changed(const GPlatesGui::ViewportProjection &)),
			this,
			SLOT(handle_projection_type_changed(const GPlatesGui::ViewportProjection &)));

	// Propagate messages up to ViewportWindow
	QObject::connect(
			d_globe_and_map_widget_ptr,
			SIGNAL(update_tools_and_status_message()),
			this,
			SLOT(handle_update_tools_and_status_message()));
	QObject::connect(
			this,
			SIGNAL(update_tools_and_status_message()),
			&viewport_window,
			SLOT(update_tools_and_status_message()));

	// Our GlobeAndMapWidget is the main viewport for ViewportWindow
	QObject::connect(
			d_globe_and_map_widget_ptr,
			SIGNAL(resized(int, int)),
			this,
			SLOT(handle_globe_and_map_widget_resized(int, int)));

	recalc_camera_position();
	// Focus anything, ANYTHING, other than the time spinbox as it eats hotkeys.
	globe_canvas().setFocus(Qt::ActiveWindowFocusReason);
}


void
GPlatesQtWidgets::ReconstructionViewWidget::handle_globe_and_map_widget_resized(
		int new_width, int new_height)
{
	d_view_state.set_main_viewport_dimensions(std::make_pair(new_width, new_height));
}


void
GPlatesQtWidgets::ReconstructionViewWidget::handle_projection_type_changed(
		const GPlatesGui::ViewportProjection &viewport_projection)
{
	// Reset the mouse coords label if projection changed.
	if (viewport_projection.get_projection_type() == GPlatesGui::ORTHOGRAPHIC)
	{
		d_label_mouse_coords->setText(DEFAULT_MOUSE_COORDS_LABEL_TEXT_FOR_GLOBE);
	}
	else
	{
		d_label_mouse_coords->setText(DEFAULT_MOUSE_COORDS_LABEL_TEXT_FOR_MAP);
	}
}


GPlatesQtWidgets::GlobeCanvas &
GPlatesQtWidgets::ReconstructionViewWidget::globe_canvas()
{
	return d_globe_and_map_widget_ptr->get_globe_canvas();
}


const GPlatesQtWidgets::GlobeCanvas &
GPlatesQtWidgets::ReconstructionViewWidget::globe_canvas() const
{
	return d_globe_and_map_widget_ptr->get_globe_canvas();
}


GPlatesQtWidgets::MapView &
GPlatesQtWidgets::ReconstructionViewWidget::map_view()
{
	return d_globe_and_map_widget_ptr->get_map_view();
}


const GPlatesQtWidgets::MapView &
GPlatesQtWidgets::ReconstructionViewWidget::map_view() const
{
	return d_globe_and_map_widget_ptr->get_map_view();
}


GPlatesQtWidgets::SceneView &
GPlatesQtWidgets::ReconstructionViewWidget::active_view()
{
	return d_globe_and_map_widget_ptr->get_active_view();
}


const GPlatesQtWidgets::SceneView &
GPlatesQtWidgets::ReconstructionViewWidget::active_view() const
{
	return d_globe_and_map_widget_ptr->get_active_view();
}


GPlatesQtWidgets::GlobeAndMapWidget &
GPlatesQtWidgets::ReconstructionViewWidget::globe_and_map_widget()
{
	return *d_globe_and_map_widget_ptr;
}


const GPlatesQtWidgets::GlobeAndMapWidget &
GPlatesQtWidgets::ReconstructionViewWidget::globe_and_map_widget() const
{
	return *d_globe_and_map_widget_ptr;
}


void
GPlatesQtWidgets::ReconstructionViewWidget::handle_update_tools_and_status_message()
{
	recalc_camera_position();
	emit update_tools_and_status_message();
}


std::auto_ptr<QWidget>
GPlatesQtWidgets::ReconstructionViewWidget::construct_awesomebar_one(
		GPlatesGui::AnimationController &animation_controller,
		GPlatesQtWidgets::ViewportWindow &main_window)
{
	// We create the bar widget in an auto_ptr, because it has no Qt parent yet.
	// auto_ptr will keep tabs on the memory for us until we can return it
	// and add it to the main ReconstructionViewWidget somewhere.
	std::auto_ptr<QWidget> awesomebar_widget(new QWidget);
	awesomebar_widget->setObjectName("AwesomeBar_1");
	QHBoxLayout *awesomebar_layout = new QHBoxLayout(awesomebar_widget.get());
	awesomebar_layout->setSpacing(2);
	awesomebar_layout->setContentsMargins(0, 0, 0, 0);
	
	// Create the GMenu Button (which is hidden until full-screen mode activates).
	d_gmenu_button_ptr = new GMenuButton(main_window, awesomebar_widget.get());
	
	// Create the Leave Full Screen Mode Button (which is hidden until full-screen mode activates).
	LeaveFullScreenButton *leave_full_screen_button_ptr = new LeaveFullScreenButton(awesomebar_widget.get());

	// Create the AnimateControlWidget.
	d_animate_control_widget_ptr = new AnimateControlWidget(animation_controller, awesomebar_widget.get());

	// Create the TimeControlWidget.
	d_time_control_widget_ptr = new TimeControlWidget(animation_controller, awesomebar_widget.get());
	QObject::connect(
			d_time_control_widget_ptr,
			SIGNAL(editing_finished()),
			&(d_globe_and_map_widget_ptr->get_globe_canvas()),
			SLOT(setFocus()));
	
	// Insert Time and Animate controls.
	awesomebar_layout->addWidget(d_gmenu_button_ptr);
	awesomebar_layout->addWidget(wrap_with_frame(d_time_control_widget_ptr));
	awesomebar_layout->addWidget(wrap_with_frame(d_animate_control_widget_ptr));
	awesomebar_layout->addWidget(leave_full_screen_button_ptr);

	return awesomebar_widget;
}


/**
 * NOTE! Awesomebar two is not currently used. We played with the idea of a double-bar
 * at the top for a while, but in the end found other ways to save space.
 */
std::auto_ptr<QWidget>
GPlatesQtWidgets::ReconstructionViewWidget::construct_awesomebar_two(
		GPlatesGui::ViewportZoom &vzoom,
		GPlatesGui::ViewportProjection &vprojection)
{
	// We create the bar widget in an auto_ptr, because it has no Qt parent yet.
	// auto_ptr will keep tabs on the memory for us until we can return it
	// and add it to the main ReconstructionViewWidget somewhere.
	std::auto_ptr<QWidget> awesomebar_widget(new QWidget);
	awesomebar_widget->setObjectName("AwesomeBar_2");
	QHBoxLayout *awesomebar_layout = new QHBoxLayout(awesomebar_widget.get());
	awesomebar_layout->setSpacing(2);
	awesomebar_layout->setContentsMargins(0, 0, 0, 0);
		
	// Insert Zoom and Projection controls.
	awesomebar_layout->addWidget(wrap_with_frame(new ProjectionControlWidget(
			vprojection,
			awesomebar_widget.get())));

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
	viewbar_widget->setObjectName("ViewBar");
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
	QObject::connect(
			d_zoom_control_widget_ptr,
			SIGNAL(editing_finished()),
			&(d_globe_and_map_widget_ptr->get_globe_canvas()),
			SLOT(setFocus()));

	// Insert Zoom control and coordinate labels.
	viewbar_layout->addWidget(wrap_with_frame(d_zoom_control_widget_ptr));
	viewbar_layout->addWidget(wrap_with_frame(camera_coords_widget));
	viewbar_layout->addWidget(wrap_with_frame(mouse_coords_widget));

	return viewbar_widget;
}

std::auto_ptr<QWidget>
GPlatesQtWidgets::ReconstructionViewWidget::construct_viewbar_with_projections(
	GPlatesGui::ViewportZoom &vzoom,
	GPlatesGui::ViewportProjection &vprojection)
{
	// We create the bar widget in an auto_ptr, because it has no Qt parent yet.
	// auto_ptr will keep tabs on the memory for us until we can return it
	// and add it to the main ReconstructionViewWidget somewhere.
	std::auto_ptr<QWidget> viewbar_widget(new QWidget);
	viewbar_widget->setObjectName("ViewBar");
	QHBoxLayout *viewbar_layout = new QHBoxLayout(viewbar_widget.get());
	viewbar_layout->setSpacing(2);
	viewbar_layout->setContentsMargins(0, 0, 0, 0);

	// Create the Camera Coordinates label widget.
	d_label_camera_coords = new_camera_coords_label();
	QWidget *camera_coords_widget = new QWidget(viewbar_widget.get());
	QHBoxLayout *camera_coords_layout = new QHBoxLayout(camera_coords_widget);
	camera_coords_layout->setSpacing(2);
	camera_coords_layout->setContentsMargins(2, 2, 2, 2);
	// Try without the Camera label. 
	//camera_coords_layout->addWidget(new QLabel(tr("Camera:")));
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
	QObject::connect(
			d_zoom_control_widget_ptr,
			SIGNAL(editing_finished()),
			&(d_globe_and_map_widget_ptr->get_globe_canvas()),
			SLOT(setFocus()));

	// Create the ProjectionControlWidget
	d_projection_control_widget_ptr = new ProjectionControlWidget(vprojection,viewbar_widget.get());

	// Create a view-controls widget to house the View label, Projection box, Zoom spinbox, and Camera. 
	QWidget *view_controls_widget = new QWidget(viewbar_widget.get());
	QHBoxLayout *view_controls_layout = new QHBoxLayout(view_controls_widget);

	view_controls_layout->setSpacing(2);
	view_controls_layout->setContentsMargins(2,2,2,2);		
	view_controls_layout->addWidget(new QLabel(tr("View:")));
	view_controls_layout->addWidget(d_projection_control_widget_ptr);
	view_controls_layout->addWidget(d_zoom_control_widget_ptr);
	view_controls_layout->addWidget(camera_coords_widget);

	// Insert view and mouse widgets to the viewbar.
	viewbar_layout->addWidget(wrap_with_frame(view_controls_widget));	
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


boost::optional<GPlatesMaths::LatLonPoint>
GPlatesQtWidgets::ReconstructionViewWidget::camera_llp()
{
	return d_globe_and_map_widget_ptr->get_camera_llp();
}


void
GPlatesQtWidgets::ReconstructionViewWidget::recalc_camera_position()
{
	// FIXME: This is a bit convoluted.
	boost::optional<GPlatesMaths::LatLonPoint> llp = d_globe_and_map_widget_ptr->get_camera_llp();

	QString lat_label(QObject::tr("(lat: "));
	QString lon_label(QObject::tr(" ; lon: "));
	QString position_as_string;

	if (llp)
	{
		QLocale locale_;
		QString lat = locale_.toString((*llp).latitude(), 'f', 2);
		QString lon = locale_.toString((*llp).longitude(), 'f', 2);
		position_as_string.append(lat_label);
		position_as_string.append(lat);
		position_as_string.append(lon_label);
		position_as_string.append(lon);
		position_as_string.append(QObject::tr(")"));
	}
	else
	{
		position_as_string.append(lat_label);
		position_as_string.append(lon_label);
		position_as_string.append(QObject::tr(")"));
		position_as_string.append(QObject::tr(" (off map)"));
	}

	d_label_camera_coords->setText(position_as_string);
}


void
GPlatesQtWidgets::ReconstructionViewWidget::update_mouse_pointer_position(
		const GPlatesMaths::PointOnSphere &new_virtual_pos,
		bool is_on_globe)
{
	//std::cerr << "Updating pos pointer" << std::endl;
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
GPlatesQtWidgets::ReconstructionViewWidget::update_mouse_pointer_position(
		const boost::optional<GPlatesMaths::LatLonPoint> &llp,
		bool is_on_map)
{
	//std::cerr << "Updating llp pointer" << std::endl;
	QString lat_label(QObject::tr("(lat: "));
	QString lon_label(QObject::tr(" ; lon: "));
	QString position_as_string;
	if (llp)
	{
		QLocale locale_;
		QString lat = locale_.toString((*llp).latitude(), 'f', 2);
		QString lon = locale_.toString((*llp).longitude(), 'f', 2);
		position_as_string.append(lat_label);
		position_as_string.append(lat);
		position_as_string.append(lon_label);
		position_as_string.append(lon);
		position_as_string.append(QObject::tr(")"));
	}
	else
	{
		position_as_string.append(lat_label);
		position_as_string.append(lon_label);
		position_as_string.append(QObject::tr(")"));
		position_as_string.append(QObject::tr(" (off map)"));

	}

	d_label_mouse_coords->setText(position_as_string);
}


void
GPlatesQtWidgets::ReconstructionViewWidget::activate_zoom_spinbox()
{
	d_zoom_control_widget_ptr->activate_zoom_spinbox();
}


bool
GPlatesQtWidgets::ReconstructionViewWidget::globe_is_active()
{
	return d_globe_and_map_widget_ptr->get_globe_canvas().isVisible();
}

bool
GPlatesQtWidgets::ReconstructionViewWidget::map_is_active()
{
	return d_globe_and_map_widget_ptr->get_map_view().isVisible();
}

