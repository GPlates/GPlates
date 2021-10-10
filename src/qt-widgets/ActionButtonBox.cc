/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include <QToolButton>
#include "ActionButtonBox.h"


GPlatesQtWidgets::ActionButtonBox::ActionButtonBox(
		int num_columns,
		int default_icon_size,
		QWidget *parent_):
	QWidget(parent_),
	d_num_columns(num_columns),
	d_default_icon_size(default_icon_size),
	d_layout_ptr(new QGridLayout(this)),
	d_next_row(0),
	d_next_col(0)
{
	// We are not using a Qt Designer .ui file, and do not need to call setupUi(this).
	// We will roll our layout by hand.
	// As we passed 'this' into the QGridLayout's constructor, we do not need to call
	// QWidget::setLayout().
	d_layout_ptr->setSpacing(2);
	d_layout_ptr->setContentsMargins(2, 2, 2, 2);
}


void
GPlatesQtWidgets::ActionButtonBox::add_action(
		QAction *action_ptr)
{
	// Make the QToolButton we'll assign to this action. Memory will be managed by Qt.
	QToolButton *tool_button = new QToolButton(this);
	tool_button->setIconSize(QSize(d_default_icon_size, d_default_icon_size));
	tool_button->setDefaultAction(action_ptr);
	
	// As soon as we call setDefaultAction, the QToolButton begins behaving identically
	// to the QAction assigned to it. This is advantageous for situations like automatic
	// enabling / disabling of permitted QActions, identical icons, tooltips etc, but it
	// has one downside - the QAction's 'shortcut', or mneumonic key.
	// The situation I found was that (although we are only displaying icons), pushing
	// Alt-E did not open the main &Edit menu as I expected, but instead keyboard-focused
	// the &Edit Feature action QToolButton in the Actions dock. Pushing Alt-E a second time
	// got me where I wanted to be.
	// Qt does this in automatically situations where the mneumonic is ambiguous, but under
	// normal circumstances, Alt-E should open the &Edit menu for us, and a tap of the E
	// key should then activate the &Edit Feature menu item that was revealed to us.
	// 
	// To counteract this situation, I am assuming that all Actions added to this
	// ActionButtonBox have already been added as a menu item somewhere appropriate,
	// and I'm automatically disabling any accelerator keys that have been automatically
	// set on the QToolButton.
	tool_button->setShortcut(QKeySequence());
	
	// Add to grid and increment the 'next grid cell pointer'.
	d_layout_ptr->addWidget(tool_button, d_next_row, d_next_col);
	next_cell();
}


void
GPlatesQtWidgets::ActionButtonBox::next_cell()
{
	++d_next_col;
	if (d_next_col >= d_num_columns) {
		d_next_col = 0;
		++d_next_row;
	}
}


