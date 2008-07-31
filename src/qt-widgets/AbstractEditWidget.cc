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
 
#include <QKeyEvent>
#include "AbstractEditWidget.h"


QLabel *
GPlatesQtWidgets::AbstractEditWidget::label()
{
	return d_default_label_ptr;
}


void
GPlatesQtWidgets::AbstractEditWidget::keyPressEvent(
		QKeyEvent *ev)
{
	if ( ! d_handle_enter_key) {
		// The owner of this edit widget does not want us processing the Enter key.
		ev->ignore();
	}
	
	if (ev->key() == Qt::Key_Enter) {
		// This is the big newline button on my num pad.
		emit enter_pressed();
		emit commit_me();
		ev->accept();
	} else if (ev->key() == Qt::Key_Return) {
		// THIS is the big newline button labelled Enter on my keyboard proper.
		emit enter_pressed();
		emit commit_me();
		ev->accept();
	} else {
		ev->ignore();
	}
}


bool
GPlatesQtWidgets::AbstractEditWidget::is_dirty()
{
	return d_dirty;
}

void
GPlatesQtWidgets::AbstractEditWidget::set_dirty()
{
	d_dirty = true;
}

void
GPlatesQtWidgets::AbstractEditWidget::set_clean()
{
	d_dirty = false;
}


bool
GPlatesQtWidgets::AbstractEditWidget::will_handle_enter_key()
{
	return d_handle_enter_key;
}

void
GPlatesQtWidgets::AbstractEditWidget::set_handle_enter_key(
		bool should_handle)
{
	d_handle_enter_key = should_handle;
}


void
GPlatesQtWidgets::AbstractEditWidget::declare_default_label(
		QLabel *label_)
{
	d_default_label_ptr = label_;
}

