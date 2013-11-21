/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#include "AboutDialog.h"
#include "LicenseDialog.h"

#include "global/Constants.h"  // the copyright string macro
#include "global/SubversionInfo.h"

#include "gui/Dialogs.h"

#include "model/Gpgim.h"
#include "model/GpgimVersion.h"


GPlatesQtWidgets::AboutDialog::AboutDialog(
		GPlatesGui::Dialogs &dialogs,
		QWidget *parent_):
	GPlatesDialog(
			parent_,
			Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint),
	d_license_dialog_ptr(NULL)
{
	setupUi(this);

	QObject::connect(button_License, SIGNAL(clicked()),
			&dialogs, SLOT(pop_up_license_dialog()));

	// Set GPlates version label text.
	QString version(QObject::tr(GPlatesGlobal::VersionString));
	label_GPlates->setText(version);

	// Set Subversion info label text.
	QString subversion_version_number = GPlatesGlobal::SubversionInfo::get_working_copy_version_number();
	QString subversion_branch_name = GPlatesGlobal::SubversionInfo::get_working_copy_branch_name();
	if (subversion_version_number.isEmpty())
	{
		if (subversion_branch_name.isEmpty())
		{
			label_subversion_info->hide();
		}
		else
		{
			label_subversion_info->setText("(" + subversion_branch_name + ")");
		}
	}
	else
	{
		QString subversion_info = "Build: " + subversion_version_number;
		if (!subversion_branch_name.isEmpty())
		{
			if (subversion_branch_name == "trunk")
			{
				subversion_info.append(" (trunk)");
			}
			else
			{
				subversion_info.append(" (").append(subversion_branch_name).append(" branch)");
			}
		}
		label_subversion_info->setText(subversion_info);
	}

	// Set the GPGIM version label.
	const QString gpgim_version_string = GPlatesModel::Gpgim::instance().get_version().version_string();
	label_GPGIM->setText(QString("GPlates Geological Information Model: %1").arg(gpgim_version_string));

	// Set contents of copyright box.
	QString copyright(QObject::tr(GPlatesGlobal::HtmlCopyrightString));
	text_Copyright->setHtml(copyright);
}
