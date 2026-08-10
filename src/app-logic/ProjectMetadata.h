/*
 * Copyright (C) 2026 The GPlates developers
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 */

#ifndef GPLATES_APP_LOGIC_PROJECTMETADATA_H
#define GPLATES_APP_LOGIC_PROJECTMETADATA_H

#include <boost/optional.hpp>
#include <QString>


namespace GPlatesAppLogic
{
	/**
	 * The typed GPlates metadata read from a Primary Project Document.
	 */
	struct ProjectMetadata
	{
		ProjectMetadata();

		bool has_front_matter;
		bool is_valid;
		boost::optional<double> planet_radius_metres;
		QString diagnostic;
	};


	/**
	 * Parses the deliberately small YAML subset supported in Markdown front matter.
	 *
	 * The parser is read-only. Unknown keys are accepted and ignored, while YAML
	 * constructs outside nested mappings and scalar values produce a diagnostic.
	 */
	class ProjectMetadataParser
	{
	public:
		static
		ProjectMetadata
		parse(
				const QString &markdown);
	};
}

#endif // GPLATES_APP_LOGIC_PROJECTMETADATA_H
