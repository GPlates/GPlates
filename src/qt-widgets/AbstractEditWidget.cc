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


void
GPlatesQtWidgets::AbstractEditWidget::keyPressEvent(
		QKeyEvent *ev)
{
	if (ev->key() == Qt::Key_Enter) {
		// This is the big newline button on my num pad.
		emit commit_me();
		ev->accept();
	} else if (ev->key() == Qt::Key_Return) {
		// THIS is the big newline button labelled Enter on my keyboard proper.
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


