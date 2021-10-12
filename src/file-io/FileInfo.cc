/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009 The University of Sydney, Australia
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

#include "FileInfo.h"


const QString
GPlatesFileIO::FileInfo::get_display_name(
		bool use_absolute_path_name) const
{
	// Check there is a FeatureCollection on this FileInfo.
	if ( ! get_feature_collection()) {
		return QObject::tr("< No Feature Collection >");
	}
	GPlatesModel::FeatureCollectionHandle::const_weak_ref collection = *get_feature_collection();
	
	// Double-check it is valid.
	if ( ! collection.is_valid()) {
		return QObject::tr("< Invalid Feature Collection >");
	}
	
	// See if we have a file associated with this FileInfo yet;
	// we might be a 'dummy' FileInfo used in the creation of
	// a new FeatureCollection, for example at the end of the
	// digitisation step.
	if ( ! d_file_info.exists()) {
		return QObject::tr("New Feature Collection");
	}
	
	if (use_absolute_path_name) {
		return d_file_info.absoluteFilePath();
	} else {
		return d_file_info.fileName();
	}
}


GPlatesFileIO::FileInfo::~FileInfo()
{
}


bool
GPlatesFileIO::FileInfo::unload_feature_collection()
{
	if ( ! d_feature_collection) {
		// No associated feature-collection.
		return false;
	}
	const GPlatesModel::FeatureCollectionHandle::weak_ref &fc_weak_ref = *d_feature_collection;
	if ( ! fc_weak_ref.is_valid()) {
		// The weak-ref of the associated feature-collection is invalid.  (Perhaps the
		// feature-collection has already been unloaded?)
		return false;
	}

	// OK, we can finally unload the feature-collection.
	fc_weak_ref->unload();
	return true;
}


bool
GPlatesFileIO::is_writable(
		const QFileInfo &file_info)
{
	QFileInfo dir(file_info.path());
	return dir.permission(QFile::WriteUser);
}
