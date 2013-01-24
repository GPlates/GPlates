/* $Id$ */

/**
 * \file A Widget class to accompany GPlatesGui::ConfigValueDelegate.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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


#include <QHBoxLayout>
#include <QToolButton>
#include <QIcon>
#include <QLineEdit>

#include "ConfigValueEditorWidget.h"


GPlatesQtWidgets::ConfigValueEditorWidget::ConfigValueEditorWidget(
		QWidget *parent_) :
	QWidget(parent_),
	d_wants_reset(false)
{
	static const QIcon reset_icon(":/tango_undo_16.png");

	// Our outermost UI area is simply a layout that wraps around the traditional
	// edit widget (in this case, a QLineEdit) and includes a "Reset to default" button.
	QHBoxLayout *hbox = new QHBoxLayout(this);
	hbox->setContentsMargins(0, 0, 0, 0);
	hbox->setSpacing(1);
	setLayout(hbox);
	
	// Create the *actual* edit widget(s) that deals with the user-input.
	// Setting the ObjectName property is important, so that the ConfigValueDelegate can find it.
	QLineEdit *line_edit = new QLineEdit(this);		// FIXME: Other types of edit widget.
	line_edit->setObjectName("editor");
	hbox->addWidget(line_edit);
	
	QToolButton *reset_button = new QToolButton(this);		// FIXME: keep as a member variable?
	reset_button->setObjectName("reset");
	reset_button->setIcon(reset_icon);
	reset_button->setIconSize(QSize(16, 16));
	reset_button->setToolTip(tr("Reset to default value"));
	connect(reset_button, SIGNAL(clicked()), this, SLOT(handle_reset()));
	hbox->addWidget(reset_button);
	
	// When the ConfigValueEditorWidget gets focus, it is imperative that focus gets
	// handed off to the actual main editing widget, or the UI will feel very clunky.
	setFocusProxy(line_edit);
}


void
GPlatesQtWidgets::ConfigValueEditorWidget::handle_reset()
{
	d_wants_reset = true;
	Q_EMIT reset_requested(this);
}


