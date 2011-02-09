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
#include <QLocale> 

#include "ExportCoRegistrationAnimationStrategy.h"

#include "data-mining/DataSelector.h"
#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"
#include "gui/CsvExport.h"

const QString GPlatesGui::ExportCoRegistrationAnimationStrategy::DEFAULT_FILENAME_TEMPLATE
		="co_registration_data_%0.2f.csv";

const QString GPlatesGui::ExportCoRegistrationAnimationStrategy::FILENAME_TEMPLATE_DESC
		=FORMAT_CODE_DESC;

const QString GPlatesGui::ExportCoRegistrationAnimationStrategy::CO_REGISTRATION_DESC =
		"co-registration data for data-mining\n";

const GPlatesGui::ExportCoRegistrationAnimationStrategy::non_null_ptr_type
GPlatesGui::ExportCoRegistrationAnimationStrategy::create(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const ExportAnimationStrategy::Configuration& cfg)
{
	ExportCoRegistrationAnimationStrategy* ptr=
		new ExportCoRegistrationAnimationStrategy(
				export_animation_context,
				cfg.filename_template());
	
	ptr->d_delimiter = ',';
	return non_null_ptr_type(
			ptr,
			GPlatesUtils::NullIntrusivePointerHandler());
}

GPlatesGui::ExportCoRegistrationAnimationStrategy::ExportCoRegistrationAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const QString &filename_template):
	ExportAnimationStrategy(export_animation_context)
{
	set_template_filename(filename_template);
}

bool
GPlatesGui::ExportCoRegistrationAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{	
	if(!check_filename_sequence())
	{
		return false;
	}
	GPlatesUtils::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	CsvExport::ExportOptions option;
	option.delimiter=d_delimiter;
	
	QString basename = *filename_it;
	QString full_filename = d_export_animation_context_ptr->target_dir().absoluteFilePath(basename);

	GPlatesDataMining::DataSelector::get_data_table().export_as_CSV(full_filename);
	filename_it++;

	// Normal exit, all good, ask the Context process the next iteration please.
	return true;
}

const QString&
GPlatesGui::ExportCoRegistrationAnimationStrategy::get_default_filename_template()
{
	return DEFAULT_FILENAME_TEMPLATE;
}

const QString&
GPlatesGui::ExportCoRegistrationAnimationStrategy::get_filename_template_desc()
{
	return FILENAME_TEMPLATE_DESC;
}


