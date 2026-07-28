/*
 * Copyright (C) 2026 The GPlates development team
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 */

#ifndef GPLATES_QTWIDGETS_FEATURETYPEDISPLAYPREFERENCES_H
#define GPLATES_QTWIDGETS_FEATURETYPEDISPLAYPREFERENCES_H

#include <QString>


namespace GPlatesQtWidgets
{
	namespace FeatureTypeDisplayPreferences
	{
		/**
		 * A QStringList of qualified feature type names hidden from feature type choosers.
		 *
		 * Storing the exceptions instead of the visible types means new GPGIM feature
		 * types are visible by default when a newer version of GPlates is installed.
		 */
		inline
		QString
		hidden_feature_types_key()
		{
			return QString("feature_type_display/hidden_feature_types");
		}
	}
}

#endif  // GPLATES_QTWIDGETS_FEATURETYPEDISPLAYPREFERENCES_H
