/**
 * \file
 * $Revision: 8672 $
 * $Date: 2010-06-10 07:00:38 +0200 (to, 10 jun 2010) $
 *
 * Copyright (C) 2010 Geological Survey of Norway
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

#include "SymbolManagerDialog.h"

GPlatesQtWidgets::SymbolManagerDialog::SymbolManagerDialog(
    QWidget *parent_) :
    QDialog(parent_)
{
    setupUi(this);

    set_up_connections();
}

void
GPlatesQtWidgets::SymbolManagerDialog::set_up_connections()
{
    QObject::connect(
		button_close,
		SIGNAL(clicked()),
		this,
		SLOT(handle_close()));
}

void
GPlatesQtWidgets::SymbolManagerDialog::handle_close()
{
    reject();
}
