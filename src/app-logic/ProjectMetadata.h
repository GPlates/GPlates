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
#include <vector>


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

		/**
		 * Whether @a planet_radius_metres reflects a well-formed document. Tracked separately
		 * from @a is_valid so a malformed required_timestamps_ma cannot be mistaken for a bad
		 * radius, or vice versa - the two are validated independently.
		 */
		bool planet_radius_is_valid;
		QString planet_radius_diagnostic;

		/**
		 * The schedule of reconstruction times, in Ma, that the project intends to be worked
		 * through in order, oldest first. Optional - absent means no schedule is active and
		 * ordinary timeline stepping is unaffected.
		 *
		 * Validated independently of the planet radius above: a malformed schedule does not
		 * invalidate an otherwise-good radius, and a missing radius (which is mandatory and
		 * fails the whole document) never reaches this field at all.
		 */
		boost::optional< std::vector<double> > required_timestamps_ma;
		bool required_timestamps_are_valid;
		QString required_timestamps_diagnostic;

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
