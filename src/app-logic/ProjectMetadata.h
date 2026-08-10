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
#include <QMap>
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

		/**
		 * Intended resolution in kilometres - the distance a user means to work at when
		 * digitising, expressed as the longest segment they want a line to have.
		 *
		 * Optional. Absent means the project has not expressed an opinion, and consumers
		 * should fall back to their own default rather than inventing a number here.
		 */
		boost::optional<double> default_resolution_km;

		/**
		 * Per-feature-type overrides of @a default_resolution_km, keyed by qualified feature
		 * type ("gpml:MidOceanRidge").
		 *
		 * The document is written with bare names ("MidOceanRidge:") because a colon inside a
		 * YAML key needs quoting, which is a trap for anyone hand-editing the file. The
		 * "gpml:" prefix is added here so callers can look up by the qualified name they
		 * already hold.
		 *
		 * A coastline wants finer detail than an ocean-floor isochron; this is how a project
		 * says so once rather than the user remembering it every time.
		 */
		QMap<QString, double> resolution_km_by_feature_type;

		/**
		 * The step, in My, the project intends to evolve the world by. Optional.
		 *
		 * A statement of how finely the user means to work, not a limit. It matters to anything
		 * that takes a known amount of time to happen: at a 1 My step, subduction initiation is
		 * something watched over ten of them; at 10 My it arrives as a single event. A tool
		 * modelling such a process reads this to know which of the two it is giving the user.
		 */
		boost::optional<double> granularity_my;

		/**
		 * How quickly subduction spreads once it exists. All optional, and independent of one
		 * another - a project may pin down one of these and leave the rest to the consumer.
		 *
		 * These are rules of thumb rather than constants. The template's numbers are Earth's,
		 * and a world with hotter mantle or weaker lithosphere has every right to different
		 * ones. They live in the project so that "how long should this take?" has an answer
		 * written down rather than guessed at afresh each time.
		 */
		boost::optional<double> subduction_initiation_my;
		boost::optional<double> subduction_propagation_km_per_my;
		boost::optional<double> subduction_reversal_my;
		boost::optional<double> subduction_breakoff_my;

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
