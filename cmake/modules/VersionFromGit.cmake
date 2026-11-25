##################################################
# Derive the GPlates/pyGPlates version from git  #
##################################################

#
# A version is a hand-edited *release target* (see 'VersionRelease.cmake') plus a *development
# number* counted from git:
#
#   N = <development number of the nearest release tag> + <first-parent commits since that tag>
#
#   GPlates target '2.6.0'  + N -> '2.6.0-N'      (and target '2.6.0-rc.1' + N -> '2.6.0-rc.1.N')
#   pyGPlates target '1.1.0' + N -> '1.1.0.devN'  (and target '1.1.0rc1'   + N -> '1.1.0rc1.devN')
#
# At a release tag the development number vanishes and the version is exactly the target.
#
# WHY DERIVE IT: hand-incrementing the development number requires knowing the order in which
# branches will *merge*, which is not known while editing the file. Two pull requests opened
# from the same base both take the next number, and whichever merges second is wrong. That has
# already happened here (7c6c6243f jumped dev7 -> dev9 because c967acd21 had taken dev8 on
# another line of history the same day) and the two products' counters have drifted apart.
# Git already knows the merge order.
#
# WHY --first-parent: it counts the number of times the branch tip has advanced - one per merged
# pull request or direct push - rather than every commit on every branch that was merged in. It
# does not require that every commit be a merge; both forms advance the tip exactly once.
#
# WHY 'rev-list --count' AND NOT 'git describe': 'git describe --first-parent' requires the tag
# to sit *on* the first-parent line and fails outright here ("No tags can describe"). Release tags
# live on the release series branches ('release/<product>-<major>.<minor>'), never on the develop
# branch, so no release tag is ever on the develop branch's first-parent line. (Series branches
# are not merged back: a fix wanted on both lines goes to the develop branch and is cherry-picked
# to the series, and a merge would carry the series branch's release target with it. Were one
# ever merged, it would have to be with '--no-ff' - a fast-forward puts the tags on the line and
# hands the develop branch the series' target, with no guard firing. A release tag put on the
# develop branch by mistake does fire one, and a misleading one: git cannot tell the lines apart,
# so a candidate there binds the develop branch as it would its series branch, and the next
# target is refused as "not finished".)
# 'rev-list --count --first-parent <tag>..HEAD' needs no such ancestry - it counts the
# first-parent commits of HEAD that are not reachable from the tag - so the tags work unchanged.
# Counting from a tag on a series branch gives the number of commits since that series was cut,
# which is the more meaningful quantity anyway.
#
# ANCHOR TAGS: because the *nearest* tag wins, a tag placed on a branch takes over the numbering
# from that point. That is how a downstream fork keeps its own development numbers without
# colliding with upstream's (tag its branch 'GPlates-2.6.0-2000', and its own commits count on
# from 2000), and how upstream can re-anchor a counter that has grown unwieldy. Give an anchor
# tag a non-zero development number, so it stays distinguishable from the release itself, and
# the base the commit resolves to - the release target in force at its commit - so that
# 'GPlates-2.6.0-2000' reads as "the 2.6.0 line, 2000 commits in", rather than as a label
# somebody chose. The resolver draws no line between an anchor and a development tag that merely
# records a build's version (the design document, section 10): every tag carrying a development
# number is treated the same way. It supplies its number, its base must be the target at its
# commit (gplates_check_anchor_tag), and it counts only on the first-parent line it was placed
# on - a fork's anchor, or a development tag on a merged feature branch, is ignored from the
# development branch (the loop below says why). The number itself is never checked: no rule
# constrains it, so a tag whose number equals the count already in force changes nothing, and an
# anchor is one whose number is larger.
#
# GPlates depends on two right now. 'GPlates-2.6.0-47' is on the old gplates line, which runs
# back through the 2013 'python-api' branch with no newer GPlates release tag on it - so without
# the anchor the count ran from 2013 and gave 2.6.0-1206 instead of 2.6.0-47. 'GPlates-2.6.0-56'
# is on the merge that unified the development branches: that merge was made on the old
# pygplates line, which is the line main follows now, so the older anchor is off it and does
# not count for it - the merge's own number, re-anchored where the line is. Each is load-bearing
# for its stretch: the older for the old gplates commits a 'git bisect' walks, the newer for
# everything since. Once 'release/gplates-2.6' is cut and 'GPlates-2.6.0' tagged on it, the tip
# counts from that tag instead - but every commit between the anchors and the branch point still
# counts from the anchors (a release tag scores 0 there and is skipped, see below). Never delete
# either.
#
# FROZEN VERSIONS: a release series branch releases one product, and the other product's version
# is frozen there - set in 'VersionRelease.cmake' when the branch is cut, and used as it is, with
# none of the above. Counted from git, it would be judged against the other product's releases,
# and once one of those is made on a series branch cut later, every configure here would fail
# (gplates_check_frozen_version says why).
#
# SERIES CUT RECORDS: the same commit that cuts a release series branch also records the
# development branch commit it was cut from, 'set(RELEASE_SERIES_CUT_FROM <commit>)'. Without
# it, a series' branch point can only be taken to be where HEAD's history and the series' tags
# part - which, from a branch made off the development branch *before* the cut, is that branch's
# own fork point. Such a branch looks exactly like the development branch after a cut whose
# target was never moved on, and from the series' first tag it was refused. With the record, a
# release tag whose recorded cut is not in HEAD's history is skipped: HEAD left the development
# branch before that series existed, so its releases are not HEAD's business. The record is read
# at the tag, so the answer depends only on the commit and the tags, like everything else here
# (_gplates_version_series_cut has the rules).
#

if (CMAKE_SCRIPT_MODE_FILE)
	# Only when run as 'cmake -P'; when included, the project has already set this.
	cmake_minimum_required(VERSION 3.22)
endif()

# This file lives in '<source>/cmake/modules'.
get_filename_component(GPLATES_VERSION_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# The hand-edited release targets.
include("${CMAKE_CURRENT_LIST_DIR}/VersionRelease.cmake")


#
# Split a product version into its base version and its development number.
#
#   GPlates      2.6.0         -> base '2.6.0'         dev 0
#                2.6.0-8       -> base '2.6.0'         dev 8
#                2.6.0-rc.1    -> base '2.6.0-rc.1'    dev 0
#                2.6.0-rc.1.8  -> base '2.6.0-rc.1'    dev 8
#   pyGPlates    1.1.0         -> base '1.1.0'         dev 0
#                1.1.0.dev10   -> base '1.1.0'         dev 10
#                1.1.0rc1      -> base '1.1.0rc1'      dev 0
#                1.1.0rc1.dev5 -> base '1.1.0rc1'      dev 5
#
# A two-component 'X.Y' version is accepted and normalised to 'X.Y.0'. Existing release tags use
# that form ('GPlates-2.5', 'PyGPlates-0.36'), so this is what lets them be counted from without
# retagging. Only *tags* are expected to need it - 'Version.cmake' still requires the full
# 'X.Y.Z' of the resolved version.
#
# Sets <ok_var> FALSE (and nothing else meaningful) if the version does not parse; unparseable
# tags are simply skipped, which is what excludes 'GPlates-1.5+hellinger-testing',
# 'GPlates-0.9.10.1' and the couple of hundred 'svn-migration/*' tags.
#
function(gplates_split_version product version ok_var base_var dev_var)
	set(${ok_var} FALSE PARENT_SCOPE)
	set(${base_var} "" PARENT_SCOPE)
	set(${dev_var} 0 PARENT_SCOPE)

	set(_dev 0)

	if (product STREQUAL "gplates")
		# Peel an optional development number off the end, then require what is left to be
		# 'X.Y[.Z]' with an optional '-{alpha|beta|rc}.N' pre-release suffix.
		#
		# The pre-release forms are tested first so that each branch holds exactly one MATCHES:
		# a second MATCHES in the same condition would overwrite the captures of the first.
		if ("${version}" MATCHES [[^(.+)-(alpha|beta|rc)\.([0-9]+)\.([0-9]+)$]])
			# 'X.Y.Z-{alpha|beta|rc}.M.N' - a development pre-release of a pre-release.
			set(_head "${CMAKE_MATCH_1}")
			set(_suffix "-${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
			set(_dev "${CMAKE_MATCH_4}")
		elseif ("${version}" MATCHES [[^(.+)-(alpha|beta|rc)\.([0-9]+)$]])
			# 'X.Y.Z-{alpha|beta|rc}.N' - a pre-release.
			set(_head "${CMAKE_MATCH_1}")
			set(_suffix "-${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
		elseif ("${version}" MATCHES [[^(.+)-([0-9]+)$]])
			# 'X.Y.Z-N' - a development pre-release.
			set(_head "${CMAKE_MATCH_1}")
			set(_dev "${CMAKE_MATCH_2}")
			set(_suffix "")
		else()
			set(_head "${version}")
			set(_suffix "")
		endif()
	elseif (product STREQUAL "pygplates")
		if ("${version}" MATCHES [[^(.+)\.dev([0-9]+)$]])
			set(_rest "${CMAKE_MATCH_1}")
			set(_dev "${CMAKE_MATCH_2}")
		else()
			set(_rest "${version}")
		endif()
		# Peel the optional '[{a|b|rc}N][.postN]' release suffixes off the numeric head.
		if (NOT "${_rest}" MATCHES [[^([0-9]+(\.[0-9]+)*)((a|b|rc)[0-9]+)?(\.post[0-9]+)?$]])
			return()
		endif()
		set(_head "${CMAKE_MATCH_1}")
		set(_suffix "${CMAKE_MATCH_3}${CMAKE_MATCH_5}")
	else()
		message(FATAL_ERROR "Unknown product '${product}' - expected 'gplates' or 'pygplates'.")
	endif()

	# Normalise the numeric head to 'X.Y.Z'.
	if (_head MATCHES [[^([0-9]+)\.([0-9]+)$]])
		set(_head "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.0")
	elseif (NOT _head MATCHES [[^[0-9]+\.[0-9]+\.[0-9]+$]])
		return()
	endif()

	set(${ok_var} TRUE PARENT_SCOPE)
	set(${base_var} "${_head}${_suffix}" PARENT_SCOPE)
	set(${dev_var} ${_dev} PARENT_SCOPE)
endfunction()


#
# Combine a base version and a development number into a full product version.
#
# GPlates spells the development number as a bare number so that it orders correctly under
# *both* Semantic Versioning and Debian rules - see the long note in 'Version.cmake'. It is
# appended with '-' to a plain 'X.Y.Z' and with '.' to a version that already carries a
# pre-release suffix ('2.6.0-rc.1' -> '2.6.0-rc.1.8'), which keeps both orderings intact.
#
function(gplates_join_version product base dev out_var)
	if (dev EQUAL 0)
		set(${out_var} "${base}" PARENT_SCOPE)
	elseif (product STREQUAL "gplates")
		if (base MATCHES [[^[0-9]+\.[0-9]+\.[0-9]+$]])
			set(${out_var} "${base}-${dev}" PARENT_SCOPE)
		else()
			set(${out_var} "${base}.${dev}" PARENT_SCOPE)
		endif()
	else()
		set(${out_var} "${base}.dev${dev}" PARENT_SCOPE)
	endif()
endfunction()


#
# Split a base version - one with no development number - into a numeric head and a comparable
# pre-release rank, so that two bases can be ordered.
#
# The rank orders the suffixes each product allows, later being greater:
#
#   GPlates      2.6.0-alpha.N  <  2.6.0-beta.N  <  2.6.0-rc.N  <  2.6.0
#   pyGPlates    1.1.0aN        <  1.1.0bN       <  1.1.0rcN    <  1.1.0  <  1.1.0.postN
#
# Neither ordering is invented here: both are what Semantic Versioning and PEP 440 respectively
# already require, and 'Version.cmake' relies on them.
#
# Sets <ok_var> FALSE if the base does not parse. That is not an error in itself - an unparseable
# tag is skipped everywhere else too - it only means the base cannot be compared.
#
function(_gplates_version_base_rank product base ok_var head_var rank_var n_var post_var)
	set(${ok_var} FALSE PARENT_SCOPE)
	set(${head_var} "" PARENT_SCOPE)
	set(${rank_var} 0 PARENT_SCOPE)
	set(${n_var} 0 PARENT_SCOPE)
	set(${post_var} 0 PARENT_SCOPE)

	set(_n 0)
	set(_post 0)

	if (product STREQUAL "gplates")
		if (base MATCHES [[^([0-9]+\.[0-9]+\.[0-9]+)-(alpha|beta|rc)\.([0-9]+)$]])
			set(_head "${CMAKE_MATCH_1}")
			set(_kind "${CMAKE_MATCH_2}")
			set(_n "${CMAKE_MATCH_3}")
		elseif (base MATCHES [[^([0-9]+\.[0-9]+\.[0-9]+)$]])
			set(_head "${CMAKE_MATCH_1}")
			set(_kind "")
		else()
			return()
		endif()
	elseif (product STREQUAL "pygplates")
		if (NOT base MATCHES [[^([0-9]+\.[0-9]+\.[0-9]+)((a|b|rc)([0-9]+))?(\.post([0-9]+))?$]])
			return()
		endif()
		set(_head "${CMAKE_MATCH_1}")
		set(_kind "${CMAKE_MATCH_3}")
		if (NOT _kind STREQUAL "")
			set(_n "${CMAKE_MATCH_4}")
		endif()
		if (NOT "${CMAKE_MATCH_6}" STREQUAL "")
			set(_post "${CMAKE_MATCH_6}")
		endif()
	else()
		message(FATAL_ERROR "Unknown product '${product}' - expected 'gplates' or 'pygplates'.")
	endif()

	# 4 is "no pre-release suffix", so a plain release outranks every candidate of it. A
	# post-release sorts above that again, and is ranked by <post_var>; only pyGPlates can
	# express one.
	if (_kind STREQUAL "alpha" OR _kind STREQUAL "a")
		set(_rank 1)
	elseif (_kind STREQUAL "beta" OR _kind STREQUAL "b")
		set(_rank 2)
	elseif (_kind STREQUAL "rc")
		set(_rank 3)
	else()
		set(_rank 4)
	endif()

	set(${ok_var} TRUE PARENT_SCOPE)
	set(${head_var} "${_head}" PARENT_SCOPE)
	set(${rank_var} ${_rank} PARENT_SCOPE)
	set(${n_var} ${_n} PARENT_SCOPE)
	set(${post_var} ${_post} PARENT_SCOPE)
endfunction()


#
# Order two base versions of the same product: sets <out_var> to -1, 0 or 1 as 'a' sorts below,
# equal to, or above 'b' - or to the empty string if either of them does not parse.
#
# CMake's own VERSION_LESS cannot do this alone: it compares dotted numbers and ignores the rest,
# so it reads '1.1.0rc1' and '1.1.0' as equal - the one comparison that matters most here.
#
function(gplates_compare_versions product a b out_var)
	_gplates_version_base_rank(${product} "${a}" _ok_a _head_a _rank_a _n_a _post_a)
	_gplates_version_base_rank(${product} "${b}" _ok_b _head_b _rank_b _n_b _post_b)
	if (NOT _ok_a OR NOT _ok_b)
		set(${out_var} "" PARENT_SCOPE)
		return()
	endif()

	if (_head_a VERSION_GREATER _head_b)
		set(${out_var} 1 PARENT_SCOPE)
		return()
	elseif (_head_a VERSION_LESS _head_b)
		set(${out_var} -1 PARENT_SCOPE)
		return()
	endif()

	# Equal heads: the pre-release rank decides, then its number, then the post-release number.
	set(_a_parts "${_rank_a}" "${_n_a}" "${_post_a}")
	set(_b_parts "${_rank_b}" "${_n_b}" "${_post_b}")
	foreach (_i RANGE 2)
		list(GET _a_parts ${_i} _x)
		list(GET _b_parts ${_i} _y)
		if (_x GREATER _y)
			set(${out_var} 1 PARENT_SCOPE)
			return()
		elseif (_x LESS _y)
			set(${out_var} -1 PARENT_SCOPE)
			return()
		endif()
	endforeach()

	set(${out_var} 0 PARENT_SCOPE)
endfunction()


#
# Check the hand-edited release target against the nearest release, setting <error_var> to a
# message to abort with, or to the empty string if the target is acceptable.
#
# 'reference' is the base version of the nearest tag that is a *release* rather than an anchor,
# and 'reference_tag' names it. Anchor tags are deliberately excluded: an anchor says nothing
# about what has been released, and a fork standing on 'GPlates-2.6.0-2000' with the target
# '2.6.0' is the normal configuration, not an error.
#
# CHECK 1 (correctness): the target must sort strictly above the reference. Without it, a target
# left behind - or set backwards by a typo - silently mints versions sorting below a release that
# is already published: '1.1.0.dev3' after '1.1.0', or '0.9.0.dev49' after '1.0.0'. Equality is
# the common case and gets its own message.
#
# CHECK 2 (policy): the target must not skip a version. Check 1 accepts '1.2.0' after a released
# '1.0.0', which is what an accidentally skipped '1.1' looks like. The GPlates release line has no
# skips - the one apparent gap, 0.9.5 to 0.9.7.1, is a release whose tag went missing; the 0.9.x
# line numbered itself as if 0.9 were the major version, 0.9.10 a minor and 0.9.10.1 its patch,
# until 1.0.0 declared the product mature; and the big early pyGPlates jumps are from an era that
# used the SVN revision as the minor version - so this matches the project's actual practice. It
# is policy rather than correctness, and a future
# maintainer wanting a deliberate jump can loosen it without breaking anything.
#
# A candidate binds the line it is on, and only that line. 'on_line' says whether the reference
# tag sits on HEAD's first-parent line:
#
#   - On the line (a release series branch, or a patch branch off one): only the same numeric
#     head may follow a candidate, so '1.1.0rc1' can be followed by '1.1.0rc2' or '1.1.0' but
#     not by '1.1.1'. There is no such thing as skipping past a release that never happened.
#     (And '1.1.0' only on the commit tagged as it - gplates_check_untagged_release.)
#   - Off the line (the development branch, once the series branch cut from it has its first
#     candidate; or a branch made from it before its target moved on - one made before the cut
#     sees no candidate at all, if the series records where it was cut, since a release of a
#     series cut after HEAD's history left the development branch is skipped; see SERIES CUT
#     RECORDS at the top of this file): the candidate's head counts as released
#     for the no-skip rule, and *staying* on that head is refused. The 1.1.0 line has moved to
#     the series branch, which is minting '1.1.0rc1.devN' there, and this line's nearest tag is
#     now that candidate at the branch point - so its own count has restarted, and left on
#     '1.1.0' it would re-issue numbers it has already used. The target has to move on to
#     '1.2.0' (or '1.1.1', or '2.0.0') at that moment: the same bump that check 1 forces after
#     a final release, brought forward to the first candidate.
#
# Without that distinction, the first candidate on any series branch stopped every configure on
# the development branch until the release was final - and, since both products' versions are
# always resolved, a GPlates candidate stopped every pyGPlates build.
#
# Honest limit: neither check catches a typo landing one step *forwards*. '1.10' normalises to
# '1.10.0', which is a legitimate next minor from '1.9.0'.
#
function(gplates_check_release_target product target reference reference_tag distance on_line
		error_var)
	set(${error_var} "" PARENT_SCOPE)

	# 'Version.cmake' is what validates the target's grammar; an unparseable one is its error to
	# report, and there is nothing to compare here.
	gplates_split_version(${product} "${target}" _ok _target_base _target_dev)
	if (NOT _ok)
		return()
	endif()

	gplates_compare_versions(${product} "${_target_base}" "${reference}" _cmp)
	if (_cmp STREQUAL "")
		return()
	endif()

	set(_where "'cmake/modules/VersionRelease.cmake'")

	if (_cmp EQUAL 0)
		string(CONCAT _msg
				"The ${product} release target is '${target}', but '${reference_tag}' has already "
				"been released ${distance} commit(s) back. Every commit from here would carry a "
				"development version of something already published, sorting below it. Set the "
				"next target in ${_where}."
		)
		set(${error_var} "${_msg}" PARENT_SCOPE)
		return()
	endif()

	if (_cmp LESS 0)
		string(CONCAT _msg
				"The ${product} release target is '${target}', which sorts below '${reference}' - "
				"released ${distance} commit(s) back as '${reference_tag}'. Every version derived "
				"from here would sort below a release that is already published. Set the target in "
				"${_where} to a version above '${reference}'."
		)
		set(${error_var} "${_msg}" PARENT_SCOPE)
		return()
	endif()

	# Check 2. The heads are what the no-skip rule is about; the suffixes were settled by check 1.
	_gplates_version_base_rank(${product} "${_target_base}" _ok_t _target_head _target_rank _t_n _t_post)
	_gplates_version_base_rank(${product} "${reference}" _ok_r _ref_head _ref_rank _r_n _r_post)
	if (NOT _ok_t OR NOT _ok_r)
		return()
	endif()

	if (_ref_rank LESS 4)
		# The reference is a candidate.
		if (on_line)
			if (_target_head VERSION_EQUAL _ref_head)
				# Another candidate, or the release itself (gplates_check_untagged_release has
				# more to say about the release).
				return()
			endif()
			string(CONCAT _msg
					"The ${product} release target is '${target}', but the release line it follows has "
					"not been finished: '${reference_tag}' is a candidate for '${_ref_head}', "
					"${distance} commit(s) back. Set the target in ${_where} to another candidate for "
					"'${_ref_head}', or to '${_ref_head}' itself. (Unless this is the development "
					"branch, and the tag was put on it by mistake: release tags belong on release "
					"series branches. Then delete the tag, locally and wherever it was pushed.)"
			)
			set(${error_var} "${_msg}" PARENT_SCOPE)
			return()
		endif()
		if (_target_head VERSION_EQUAL _ref_head)
			string(CONCAT _msg
					"The ${product} release target is '${target}', but '${_ref_head}' is being released "
					"on another branch: '${reference_tag}' is a candidate for it, on a release series "
					"branch cut from this line ${distance} commit(s) back. This line's development "
					"number now counts from that branch point, so a target of '${_ref_head}' would "
					"re-issue versions it has already used. Set the target in ${_where} to the release "
					"after '${_ref_head}'."
			)
			set(${error_var} "${_msg}" PARENT_SCOPE)
			return()
		endif()
		# Any other head: the candidate's head counts as released, and the no-skip rule applies.
	elseif (_target_head VERSION_EQUAL _ref_head)
		# A post-release republishes the same version, so it keeps the head it follows.
		return()
	endif()

	if (NOT _ref_head MATCHES [[^([0-9]+)\.([0-9]+)\.([0-9]+)$]])
		return()
	endif()
	set(_major "${CMAKE_MATCH_1}")
	set(_minor "${CMAKE_MATCH_2}")
	set(_patch "${CMAKE_MATCH_3}")
	math(EXPR _next_major "${_major} + 1")
	math(EXPR _next_minor "${_minor} + 1")
	math(EXPR _next_patch "${_patch} + 1")
	set(_allowed
			"${_major}.${_minor}.${_next_patch}"
			"${_major}.${_next_minor}.0"
			"${_next_major}.0.0")

	if (NOT _target_head IN_LIST _allowed)
		string(REPLACE ";" "', '" _allowed_text "${_allowed}")
		string(CONCAT _msg
				"The ${product} release target is '${target}', which skips past '${reference}' "
				"('${reference_tag}', ${distance} commit(s) back). The next release after it is "
				"'${_allowed_text}', optionally as a candidate of one of those. If this checkout "
				"may be missing tags - a fork, or a clone that fetched one branch, since release "
				"tags live on the release series branches - run 'git fetch --tags <upstream>' "
				"first. Otherwise set the target in ${_where} - or, if the jump is deliberate, "
				"loosen this check in 'gplates_check_release_target' (it is policy, not "
				"correctness)."
		)
		set(${error_var} "${_msg}" PARENT_SCOPE)
	endif()
endfunction()


#
# The ways out of a gplates_check_release_target refusal whose reference is off HEAD's first-parent
# line, to append to its message: <out_var> is a paragraph starting with a newline, or the empty
# string if there is nothing to add. The arguments are gplates_check_release_target's, plus
# 'series_line', which says whether HEAD is on a release series line (its release file freezes the
# other product).
#
# That refusal says what to do on the line it was written for - the development branch, or a
# release series branch - but the same refusal reaches a branch made from either of them before
# its target moved on past the reference: one made from the development branch at the commit a
# series branch was cut from (a series cut record cannot help it, since the cut is in its
# history), or a patch branch overtaken by the next release on its series branch. The first's
# way out is to merge the development branch in. The second's is to rebase onto the series
# branch: merged in, a candidate stays off the patch branch's first-parent line, where it binds
# the target exactly as before. And any of them can be built as it is by setting the version.
#
# A target that skips past the reference has nothing to do with any of this, so gets nothing.
#
function(gplates_off_line_refusal_advice product target reference reference_tag series_line out_var)
	set(${out_var} "" PARENT_SCOPE)

	gplates_split_version(${product} "${target}" _ok _target_base _target_dev)
	if (NOT _ok)
		return()
	endif()
	_gplates_version_base_rank(${product} "${_target_base}" _ok_t _target_head _t_rank _t_n _t_post)
	_gplates_version_base_rank(${product} "${reference}" _ok_r _ref_head _r_rank _r_n _r_post)
	if (NOT _ok_t OR NOT _ok_r OR _target_head VERSION_GREATER _ref_head)
		return()
	endif()

	if (product STREQUAL "gplates")
		set(_variable_name GPLATES_SEMANTIC_VERSION)
	else()
		set(_variable_name PYGPLATES_PEP440_VERSION)
	endif()
	if (series_line)
		string(CONCAT _advice
				"\nOn a branch made from a release series branch before '${reference_tag}' was "
				"tagged on it, rebase onto the series branch instead (merged in, a candidate stays "
				"off this branch's line, refused the same way).")
	else()
		string(CONCAT _advice
				"\nOn a branch made from the development branch before its target moved on past "
				"'${_ref_head}', merge the development branch in instead: that brings the target "
				"it has moved on to.")
	endif()
	string(APPEND _advice
			" To build this commit as it is, set the ${product} version explicitly in the "
			"environment variable ${_variable_name} (a '-D${_variable_name}=<version>' define "
			"works for the build tree it is given to, but not for the 'version-resolver-test' "
			"registered there)."
	)
	set(${out_var} "${_advice}" PARENT_SCOPE)
endfunction()


#
# Check a release's target on a commit that is not tagged as that release, setting <error_var> to
# a message to abort with, or to the empty string if the target is acceptable. The arguments are
# gplates_check_release_target's, plus the product's tag prefix; the caller does not ask about a
# commit standing on a release tag.
#
# After a candidate on this line, the target '1.1.0' would mint '1.1.0.devN', and a development
# release of '1.1.0' sorts *below* its candidates under PEP 440 ('2.6.0-5' sorts below
# '2.6.0-rc.1' under SemVer, likewise). Wheels built from such a commit - by a dispatch of
# 'build-wheels.yml', say - lose to the candidate whenever pip chooses between them. So the
# release's target belongs only on the commit tagged as it, and the commit after a candidate
# either sets the next candidate or is the release, tagged as soon as it is made. This is a
# correctness check, unlike the rule that the release is its last candidate unchanged, which the
# publishing run checks instead ('pygplates/wheel/check_release_commit.py').
#
# The advice depends on where the commit is. Directly on the candidate, it may be the release
# before its tag exists. Further on, it has something the candidate did not, and tagging it would
# release that untested - the publishing run refuses it - so the way forward is the next candidate.
#
function(gplates_check_untagged_release product tag_prefix target reference reference_tag distance
		on_line error_var)
	set(${error_var} "" PARENT_SCOPE)

	if (NOT on_line)
		return()
	endif()
	gplates_split_version(${product} "${target}" _ok _target_base _target_dev)
	if (NOT _ok)
		return()
	endif()
	_gplates_version_base_rank(${product} "${_target_base}"
			_ok_t _target_head _target_rank _t_n _t_post)
	_gplates_version_base_rank(${product} "${reference}" _ok_r _ref_head _ref_rank _r_n _r_post)
	if (NOT _ok_t OR NOT _ok_r)
		return()
	endif()
	# Only a release (not another candidate) following a candidate for the same version.
	if (_ref_rank EQUAL 4 OR _target_rank LESS 4 OR NOT _target_head VERSION_EQUAL _ref_head)
		return()
	endif()

	# The next candidate is the reference with its candidate number incremented, which keeps
	# each product's own spelling ('1.1.0rc2', '2.6.0-rc.2') without restating it here.
	string(REGEX REPLACE "[0-9]+(\\.post[0-9]+)?$" "" _stem "${reference}")
	math(EXPR _next_n "${_r_n} + 1")
	set(_next_candidate "${_stem}${_next_n}")

	set(_where "'cmake/modules/VersionRelease.cmake'")
	string(CONCAT _msg
			"The ${product} release target is '${target}', but this commit is not tagged "
			"'${tag_prefix}${target}'. It follows the candidate '${reference_tag}' (${distance} "
			"commit(s) back), so it would resolve to a development version of '${target}', which "
			"sorts below that candidate. Only the commit released as '${target}' may carry that "
			"target. "
	)
	if (distance EQUAL 1)
		string(APPEND _msg
				"If this commit is the release - the candidate plus only the change of target "
				"and changelog heading - tag it '${tag_prefix}${target}' (a tag publishes "
				"nothing until it is pushed). Otherwise set the target in ${_where} to the next "
				"candidate, '${_next_candidate}'."
		)
	else()
		string(APPEND _msg
				"This commit is not directly on the candidate, so it has changes the candidate was "
				"never tested with: set the target in ${_where} to the next candidate, "
				"'${_next_candidate}'."
		)
	endif()
	set(${error_var} "${_msg}" PARENT_SCOPE)
endfunction()


#
# Check an anchor tag against the release target at the commit it stands on, setting <error_var>
# to a message to abort with, or to the empty string if the anchor is acceptable.
#
# Every tag carrying a development number comes through here: the resolver does not know, or
# need to know, whether the number merely records the count in force at the commit or raises it.
# Either way the base is the version the commit it names resolves to - the release target in
# force there - which is what lets 'GPlates-2.6.0-2000' be read as "the 2.6.0 line, 2000 commits
# in" rather than as a label somebody chose. The resolver takes only the number from the tag, so
# a wrong base would never show in a resolved version; this is the one place it is looked at.
# The number is not checked, and cannot be: nothing constrains what number a tag may carry.
#
# 'anchor_version' is the tag with 'tag_prefix' removed, and 'release_file' is the content of
# 'cmake/modules/VersionRelease.cmake' at the anchor's commit. A commit from before that file
# existed (upstream's own 'GPlates-2.6.0-47' is one) cannot be checked, and passes; so does one
# whose file does not set this product's target.
#
function(gplates_check_anchor_tag product tag_prefix anchor_version release_file error_var)
	set(${error_var} "" PARENT_SCOPE)

	# At a line start, so that a copy of the line left in a comment above the real one is not
	# read instead (a leading newline is prepended because CMake's regex engine has no multiline
	# '^'), and in any case, since CMake's commands are case-insensitive.
	string(TOUPPER "${product}" _product_upper)
	set(_ws "[ \t]*")
	set(_name "${_product_upper}_RELEASE_VERSION")
	set(_value "\"?([^\" \t\r\n)]+)\"?")
	set(_target_set
			"[\r\n]${_ws}[sS][eE][tT]${_ws}\\(${_ws}${_name}[ \t]+${_value}${_ws}\\)")
	string(PREPEND release_file "\n")
	if (NOT release_file MATCHES "${_target_set}")
		return()
	endif()
	set(_target_there "${CMAKE_MATCH_1}")

	gplates_split_version(${product} "${anchor_version}" _ok _anchor_base _anchor_dev)
	if (NOT _ok OR _anchor_dev EQUAL 0)
		return()
	endif()
	gplates_split_version(${product} "${_target_there}" _ok _target_base _target_dev)
	if (NOT _ok)
		return()
	endif()

	gplates_compare_versions(${product} "${_anchor_base}" "${_target_base}" _cmp)
	if (_cmp STREQUAL "" OR _cmp EQUAL 0)
		return()
	endif()

	gplates_join_version(${product} "${_target_base}" ${_anchor_dev} _retag_version)
	string(CONCAT _msg
			"'${tag_prefix}${anchor_version}' is an anchor tag naming the base '${_anchor_base}', "
			"but the ${product} release target at the commit it stands on is '${_target_base}'. "
			"A tag carrying a development number is read as a version of the commit it names, "
			"not as a label, so its base is the release target in force there. Delete the "
			"tag ('git tag -d ${tag_prefix}${anchor_version}', and 'git push <remote> "
			":refs/tags/${tag_prefix}${anchor_version}' wherever it was pushed) and tag the commit "
			"'${tag_prefix}${_retag_version}' instead: the number stays, so nothing else changes. "
			"Or just delete it, if it was not meant as an anchor."
	)
	set(${error_var} "${_msg}" PARENT_SCOPE)
endfunction()


#
# Check a frozen version (see 'VersionRelease.cmake'), setting <error_var> to a message to abort
# with, or to the empty string if it is acceptable.
#
# A release series branch releases one product. The other product's version is frozen there, at
# what it resolved to on the development branch when the series branch was cut. It is not counted
# from git, and not checked against the tags, which is the point: a release of that product made
# later, on its own series branch, is off this line, and seen from here it is indistinguishable
# from a series branch cut from this one (both lines leave the same commit, and nothing in git
# says which of them is the development branch). Counted as one, it made the target on this
# branch look stale, and every configure here - this branch's own release tags included - failed
# from the moment it was tagged.
#
# The frozen version is what the development branch resolved to, so it is a full development
# version: 'X.Y.Z' spelt out (it is used as it is, and not normalised the way a tag is), with a
# development number. The development branch never resolves to development number 0 - release
# tags are never on it - and a frozen '2.6.0' would claim to be a release this branch does not
# make.
#
# 'tags_at_head' lists the tags with this product's prefix on the commit being built. None of them
# may be a release: a product whose version is frozen is not released from this branch.
#
function(gplates_check_frozen_version product tag_prefix frozen_version tags_at_head error_var)
	set(${error_var} "" PARENT_SCOPE)

	gplates_split_version(${product} "${frozen_version}" _ok _base _dev)
	if (NOT _ok OR NOT frozen_version MATCHES [[^[0-9]+\.[0-9]+\.[0-9]+]] OR _dev EQUAL 0)
		string(CONCAT _msg
				"The frozen ${product} version '${frozen_version}' is not a development version "
				"in full ('X.Y.Z' with a development number, such as '2.6.0-150' or "
				"'1.2.0.dev30'). Set it to what 'cmake -P cmake/modules/VersionFromGit.cmake "
				"${product}' gives on the development branch commit this branch was cut from."
		)
		set(${error_var} "${_msg}" PARENT_SCOPE)
		return()
	endif()

	foreach (_tag IN LISTS tags_at_head)
		string(REGEX REPLACE "^${tag_prefix}" "" _tag_version "${_tag}")
		gplates_split_version(${product} "${_tag_version}" _ok _tag_base _tag_dev)
		if (_ok AND _tag_dev EQUAL 0)
			string(CONCAT _msg
					"This commit is tagged '${_tag}', but the ${product} version is frozen on "
					"this branch at '${frozen_version}' ('cmake/modules/VersionRelease.cmake'): "
					"this is a release series branch of the other product, and ${product} is not "
					"released from it. Delete the tag, and tag the release on a ${product} release "
					"series branch."
			)
			set(${error_var} "${_msg}" PARENT_SCOPE)
			return()
		endif()
	endforeach()
endfunction()


#
# Read a release series branch's record of the development branch commit it was cut from,
# 'set(RELEASE_SERIES_CUT_FROM <commit>)', out of the content of a 'VersionRelease.cmake'. Sets
# <out_var> to the commit as written, or to the empty string if there is no record. Only a 'set'
# at a line start counts, as in gplates_check_anchor_tag, so that a copy of the line in a comment
# is not read instead; and of several, the last, as CMake itself would take it.
#
function(gplates_read_series_cut release_file out_var)
	set(${out_var} "" PARENT_SCOPE)
	set(_ws "[ \t]*")
	set(_name "RELEASE_SERIES_CUT_FROM")
	set(_value "\"?([^\" \t\r\n);]+)\"?")
	set(_record_set "[\r\n]${_ws}[sS][eE][tT]${_ws}\\(${_ws}${_name}[ \t]+${_value}${_ws}\\)")
	string(PREPEND release_file "\n")
	string(REGEX MATCHALL "${_record_set}" _records "${release_file}")
	if (_records STREQUAL "")
		return()
	endif()
	list(GET _records -1 _record)
	if (_record MATCHES "${_record_set}")
		set(${out_var} "${CMAKE_MATCH_1}" PARENT_SCOPE)
	endif()
endfunction()


#
# Set <out_var> TRUE if the content of a 'VersionRelease.cmake' freezes either product's version
# (a 'set' of GPLATES_FROZEN_VERSION or PYGPLATES_FROZEN_VERSION at a line start), and FALSE if not.
# Every commit on a release series branch freezes the product it does not release, and no commit
# on the development branch freezes anything - which is what tells the two apart.
#
function(gplates_release_file_freezes release_file out_var)
	set(_ws "[ \t]*")
	set(_frozen_set "[\r\n]${_ws}[sS][eE][tT]${_ws}\\(${_ws}(PY)?GPLATES_FROZEN_VERSION[ \t]")
	string(PREPEND release_file "\n")
	if (release_file MATCHES "${_frozen_set}")
		set(${out_var} TRUE PARENT_SCOPE)
	else()
		set(${out_var} FALSE PARENT_SCOPE)
	endif()
endfunction()


#
# Whether the content of a 'VersionRelease.cmake' could be a development branch commit's, which is
# what a series cut record must name. Sets <out_var> to the empty string if it could, and otherwise
# to why not, for a message: it freezes a version, which every commit of a series branch does from
# the one that cuts it on; or a target is a pre-release (alpha, beta or candidate), which a series
# branch sets - the first commit of one usually sets its first candidate - and the development
# branch is not expected to. Nothing refuses a candidate target on the development branch, though
# (only a candidate *tag* there), so the message says what to do if it did carry one: record the
# last development branch commit before it, an older commit than the cut, which is harmless.
# Of several 'set's of a target, the last is read, as CMake itself would take it.
#
function(gplates_release_file_not_development release_file out_var)
	set(${out_var} "" PARENT_SCOPE)

	gplates_release_file_freezes("${release_file}" _freezes)
	if (_freezes)
		set(${out_var} "it freezes a version" PARENT_SCOPE)
		return()
	endif()

	set(_ws "[ \t]*")
	set(_value "\"?([^\" \t\r\n);]+)\"?")
	string(PREPEND release_file "\n")
	foreach (_product IN ITEMS gplates pygplates)
		string(TOUPPER "${_product}_RELEASE_VERSION" _name)
		set(_target_set "[\r\n]${_ws}[sS][eE][tT]${_ws}\\(${_ws}${_name}[ \t]+${_value}${_ws}\\)")
		# Of several, the last, as CMake itself would take it.
		string(REGEX MATCHALL "${_target_set}" _sets "${release_file}")
		if (_sets STREQUAL "")
			continue()
		endif()
		list(GET _sets -1 _set)
		if (NOT _set MATCHES "${_target_set}")
			continue()
		endif()
		set(_target "${CMAKE_MATCH_1}")
		_gplates_version_base_rank(${_product} "${_target}" _ok _head _rank _n _post)
		if (_ok AND _rank LESS 4)
			string(CONCAT _why
					"its ${_product} target '${_target}' is a pre-release, which the development "
					"branch is not expected to carry - if it did carry it, record the last "
					"development branch commit before that target was set")
			set(${out_var} "${_why}" PARENT_SCOPE)
			return()
		endif()
	endforeach()
endfunction()


# Run git in the source tree, returning its exit code and stripped standard output.
function(_gplates_version_git result_var output_var)
	execute_process(
			COMMAND "${GPLATES_VERSION_GIT_EXECUTABLE}" -C "${GPLATES_VERSION_SOURCE_DIR}" ${ARGN}
			RESULT_VARIABLE _result
			OUTPUT_VARIABLE _output
			ERROR_VARIABLE _error
			OUTPUT_STRIP_TRAILING_WHITESPACE)
	set(${result_var} ${_result} PARENT_SCOPE)
	set(${output_var} "${_output}" PARENT_SCOPE)
endfunction()


#
# Check the anchor tag the count runs from against the release target at its own commit
# (gplates_check_anchor_tag), aborting the configure on a mismatch. Reaching every build that
# counts from the anchor, not only one standing on it, is what catches an older commit tagged
# wrongly: the next build after it fails, naming the tag the commit should carry.
#
function(_gplates_version_check_anchor product tag_prefix anchor_tag)
	# 'git show' peels an annotated tag to its commit itself.
	_gplates_version_git(_result _release_file
			show "${anchor_tag}:cmake/modules/VersionRelease.cmake")
	if (NOT _result EQUAL 0)
		# The commit predates the release file, so there is no target there to check against.
		return()
	endif()
	string(REGEX REPLACE "^${tag_prefix}" "" _anchor_version "${anchor_tag}")
	gplates_check_anchor_tag(${product} "${tag_prefix}" "${_anchor_version}" "${_release_file}"
			_error)
	if (NOT _error STREQUAL "")
		message(FATAL_ERROR "${_error}")
	endif()
endfunction()


#
# The development branch commit a release series branch was cut from, as recorded in
# 'VersionRelease.cmake' at <commit> on it (see SERIES CUT RECORDS at the top of this file). Sets
# <cut_var> to the full commit hash if the record is valid, and otherwise to the empty string,
# with <problem_var> saying what is wrong, for a message.
#
# A record is valid if it is:
#
#   - a full commit hash. An abbreviated one can become ambiguous as the repository grows, and a
#     record that stopped resolving would change what an old commit resolves to;
#   - in <commit>'s history;
#   - a commit that could be the development branch's (gplates_release_file_not_development):
#     one that freezes nothing and has no pre-release target. A series branch freezes the other
#     product from the commit that cuts it on, so the likeliest slip - recording that commit
#     itself instead of its parent - is caught.
#
# What remains is a commit of the development branch at or before the cut. One before it is
# wrong but harmless: branches made from the development branch between it and the cut are
# refused as they were without a record, and the development branch itself still has it in its
# history, so it still gets every check it got before.
#
# The other slip is not harmless. If the freeze is forgotten in the commit that cuts the series
# branch and added in a later one, the series commits before it freeze nothing either, and one of
# them recorded skips the series' tags from the development branch, which then loses the check
# that makes it move its target on. The pre-release test is what catches it, as long as that
# commit set the series' first candidate, as the first commit of one usually does. A series
# commit with a plain target that freezes nothing gets through - a series cut straight at a final
# release with its freeze forgotten, or preparation commits made on the series branch before the
# one that freezes - since nothing in the history tells it from a real cut. So guards 7 and 8 (in
# _gplates_version_from_git) both say which commit to record - the one the series branch was
# made from, not the parent of a later commit.
#
# The parts that depend only on the recorded commit (what its release file says, and whether it
# exists) are remembered for the rest of the run: every release of a series records the same
# commit.
#
function(_gplates_version_series_cut commit cut_var problem_var)
	set(${cut_var} "" PARENT_SCOPE)
	set(${problem_var} "" PARENT_SCOPE)

	_gplates_version_git(_result _release_file show "${commit}:cmake/modules/VersionRelease.cmake")
	if (NOT _result EQUAL 0)
		set(_release_file "")
	endif()
	gplates_read_series_cut("${_release_file}" _cut)
	if (_cut STREQUAL "")
		set(${problem_var} "does not record the commit it was cut from" PARENT_SCOPE)
		return()
	endif()

	set(_recorded "records the cut as '${_cut}'")
	string(LENGTH "${_cut}" _length)
	if (NOT _cut MATCHES "^[0-9a-fA-F]+$" OR NOT _length EQUAL 40)
		set(${problem_var} "${_recorded}, which is not a full (40-digit) commit hash" PARENT_SCOPE)
		return()
	endif()
	string(TOLOWER "${_cut}" _cut)

	# What the recorded commit itself says - '' if it is fine, else the problem - worked out once.
	get_property(_known GLOBAL PROPERTY "GPLATES_VERSION_SERIES_CUT_${_cut}" SET)
	if (_known)
		get_property(_cut_problem GLOBAL PROPERTY "GPLATES_VERSION_SERIES_CUT_${_cut}")
	else()
		set(_cut_problem "")
		_gplates_version_git(_result _type cat-file -t "${_cut}")
		if (NOT _result EQUAL 0 OR NOT _type STREQUAL "commit")
			set(_cut_problem "${_recorded}, which is not a commit in this repository")
		else()
			# A commit from before 'VersionRelease.cmake' existed has no file to show, and
			# freezes nothing.
			_gplates_version_git(_result _cut_release_file
					show "${_cut}:cmake/modules/VersionRelease.cmake")
			if (_result EQUAL 0)
				gplates_release_file_not_development("${_cut_release_file}" _why)
				if (NOT _why STREQUAL "")
					string(CONCAT _cut_problem
							"${_recorded}, which looks like a commit on a release series branch "
							"(${_why}), not the development branch commit the series was cut from")
				endif()
			endif()
		endif()
		set_property(GLOBAL PROPERTY "GPLATES_VERSION_SERIES_CUT_${_cut}" "${_cut_problem}")
	endif()
	if (NOT _cut_problem STREQUAL "")
		set(${problem_var} "${_cut_problem}" PARENT_SCOPE)
		return()
	endif()

	# Exit code 1 is 'not an ancestor'; anything else means git could not answer.
	_gplates_version_git(_result _ignored merge-base --is-ancestor "${_cut}" "${commit}")
	if (NOT _result EQUAL 0)
		set(${problem_var} "${_recorded}, which is not in the history of this commit" PARENT_SCOPE)
		return()
	endif()

	set(${cut_var} "${_cut}" PARENT_SCOPE)
endfunction()


#
# Make CMake reconfigure when HEAD moves.
#
# The version is now a configure-time value that no source file records, so merging, pulling
# or switching branch changes it without touching anything CMake watches. Without this, a
# 'git pull' followed by 'cmake --build' would quietly produce a module still carrying the
# version resolved by the previous configure.
#
function(_gplates_version_watch_head)
	if (CMAKE_SCRIPT_MODE_FILE)
		# No directory properties in script mode, and nothing to reconfigure.
		return()
	endif()

	# In a worktree, HEAD lives in the per-worktree directory while the refs live in the
	# common one; outside a worktree the two are the same directory.
	_gplates_version_git(_result _git_dir rev-parse --absolute-git-dir)
	if (NOT _result EQUAL 0)
		return()
	endif()
	_gplates_version_git(_result _common_dir rev-parse --path-format=absolute --git-common-dir)
	if (NOT _result EQUAL 0)
		set(_common_dir "${_git_dir}")
	endif()

	set(_watch "${_git_dir}/HEAD" "${_common_dir}/packed-refs")
	# The loose ref file for the branch HEAD is on, if it has one (it is absent when the ref is
	# packed, and on a detached HEAD there is no branch at all).
	_gplates_version_git(_result _head_ref symbolic-ref --quiet HEAD)
	if (_result EQUAL 0 AND NOT _head_ref STREQUAL "")
		list(APPEND _watch "${_common_dir}/${_head_ref}")
	endif()

	foreach (_file IN LISTS _watch)
		# Only files that exist: CMake re-runs whenever a watched path changes, but listing one
		# that was never there just adds noise.
		if (EXISTS "${_file}")
			set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_file}")
		endif()
	endforeach()
endfunction()


#
# Set <ok_var> TRUE if git can be run on this source tree's own repository, and FALSE if there is
# no usable one (an unpacked sdist, an exported tarball, or a build container without git).
#
function(_gplates_version_repository ok_var)
	set(${ok_var} FALSE PARENT_SCOPE)

	find_program(GPLATES_VERSION_GIT_EXECUTABLE NAMES git DOC "git, used to derive the version")
	mark_as_advanced(GPLATES_VERSION_GIT_EXECUTABLE)
	if (NOT GPLATES_VERSION_GIT_EXECUTABLE)
		return()
	endif()

	# The repository must be *this* source tree's, not one it happens to sit inside (a
	# distributor unpacking a tarball into their own checkout would otherwise be versioned
	# from that checkout's tags).
	_gplates_version_git(_result _top_level rev-parse --show-toplevel)
	if (NOT _result EQUAL 0)
		return()
	endif()
	get_filename_component(_top_level "${_top_level}" REALPATH)
	get_filename_component(_source_dir "${GPLATES_VERSION_SOURCE_DIR}" REALPATH)
	if (NOT _top_level STREQUAL _source_dir)
		# Windows paths differ in case without being different paths.
		string(TOLOWER "${_top_level}" _top_level_lower)
		string(TOLOWER "${_source_dir}" _source_dir_lower)
		if (NOT (CMAKE_HOST_WIN32 AND _top_level_lower STREQUAL _source_dir_lower))
			return()
		endif()
	endif()

	set(${ok_var} TRUE PARENT_SCOPE)
endfunction()


#
# Check a frozen version against the commit being built (gplates_check_frozen_version), aborting
# the configure if it is unacceptable. The tags are consulted only if there is a usable
# repository; without one, the version is still checked. HEAD is watched as on the counted path,
# so that the check is made again when HEAD moves - onto a commit carrying a release tag, say.
#
function(_gplates_version_check_frozen product tag_prefix frozen_version)
	set(_tags_at_head "")
	_gplates_version_repository(_have_repository)
	if (_have_repository)
		_gplates_version_watch_head()
		_gplates_version_git(_result _tags_at_head tag --points-at HEAD --list "${tag_prefix}*")
		if (NOT _result EQUAL 0)
			set(_tags_at_head "")
		endif()
		string(REGEX REPLACE "\r?\n" ";" _tags_at_head "${_tags_at_head}")
	endif()
	gplates_check_frozen_version(${product} "${tag_prefix}" "${frozen_version}"
			"${_tags_at_head}" _error)
	if (NOT _error STREQUAL "")
		message(FATAL_ERROR "${_error}")
	endif()
endfunction()


#
# Derive the version from git, or leave <out_var> empty if this source tree has no usable
# repository (an unpacked sdist, an exported tarball, or a build container without git).
#
function(_gplates_version_from_git product tag_prefix target out_var)
	set(${out_var} "" PARENT_SCOPE)

	_gplates_version_repository(_have_repository)
	if (NOT _have_repository)
		return()
	endif()
	# Watched from here rather than at the end, which a commit standing on a tag never reached:
	# a tree configured on a tag did not reconfigure when HEAD moved off it.
	_gplates_version_watch_head()

	# A shallow clone truncates the history at its graft point, so 'rev-list --count' returns a
	# smaller number that still looks perfectly plausible. Refuse rather than emit a wrong
	# version - this is why the workflows check out with 'fetch-depth: 0'.
	_gplates_version_git(_result _is_shallow rev-parse --is-shallow-repository)
	if (_result EQUAL 0 AND _is_shallow STREQUAL "true")
		message(FATAL_ERROR
				"Cannot derive the ${product} version: this is a shallow clone, and the development "
				"number counted from it would be silently wrong.\n"
				"Run 'git fetch --unshallow --tags', or check out with 'fetch-depth: 0', or set the "
				"version explicitly (see the message at the end of VersionFromGit.cmake).")
	endif()

	_gplates_version_git(_result _tags tag --list "${tag_prefix}*")
	if (NOT _result EQUAL 0)
		return()
	endif()
	if (_tags STREQUAL "")
		# A source tree that carries the sdist's PKG-INFO is an sdist somebody has put under git -
		# imported into a packaging repository, say - and its recorded version answers for it
		# (see gplates_resolve_version). Otherwise a working repository with none of the tags is
		# almost always a fork: GitHub copies no tags into a fork, and fetching a single branch
		# brings only the tags on it, while release tags live on the release series branches.
		# Falling through to the file fallbacks would then end in a message blaming the missing
		# repository, which is the wrong diagnosis - or, worse, in a stale recorded version.
		# The recorded file itself is no sign of an sdist: it is gitignored, and every
		# 'pip install .' writes one into the source tree.
		if (EXISTS "${GPLATES_VERSION_SOURCE_DIR}/PKG-INFO")
			return()
		endif()
		message(FATAL_ERROR
				"Cannot derive the ${product} version: this repository has no '${tag_prefix}*' tags "
				"to count from. A fork made on GitHub carries none of the upstream tags, and a fetch "
				"of a single branch brings only the tags on it. Fetch them from the upstream "
				"repository:\n"
				"    git fetch --tags <upstream>\n"
				"or just the release tags:\n"
				"    git fetch <upstream> 'refs/tags/GPlates-*:refs/tags/GPlates-*' "
				"'refs/tags/PyGPlates-*:refs/tags/PyGPlates-*'\n"
				"and push them to the fork ('git push <fork> --tags') so that its clones and its CI "
				"have them too. Or set the version explicitly (see the message at the end of "
				"VersionFromGit.cmake).")
	endif()
	string(REGEX REPLACE "\r?\n" ";" _tags "${_tags}")

	_gplates_version_git(_result _head_commit rev-list -n 1 HEAD)
	if (NOT _result EQUAL 0)
		return()
	endif()

	set(_parsed_any FALSE)
	set(_best_distance -1)
	set(_best_offset 0)
	set(_best_tag "")
	# The nearest tag that is a *release* rather than an anchor, tracked apart from the tag that
	# supplies the development number: an anchor tag can be nearer, and supply the number, while
	# it is a release the target has to be checked against. Among releases tied at the same
	# distance the highest base wins - seen from the develop branch 'GPlates-2.6.0' and
	# 'GPlates-2.6.1' tie, both being off its first-parent line, and comparing against the older
	# one would make a legitimate '2.6.2' look like a skipped release.
	set(_ref_base "")
	set(_ref_tag "")
	set(_ref_commit "")
	set(_ref_distance 0)
	set(_at_tag_base "")
	set(_at_tag_name "")
	set(_at_tag_dev 0)

	# The tags HEAD does not have in its history, which are the only ones a series cut record can
	# skip (see the loop), found once rather than asked of each tag.
	_gplates_version_git(_result _not_merged
			tag --no-merged "${_head_commit}" --list "${tag_prefix}*")
	if (NOT _result EQUAL 0)
		set(_not_merged "")
	endif()
	string(REGEX REPLACE "\r?\n" ";" _not_merged "${_not_merged}")
	set(_skipped "")
	set(_skipped_bases "")

	foreach (_tag IN LISTS _tags)
		string(REGEX REPLACE "^${tag_prefix}" "" _tag_version "${_tag}")
		gplates_split_version(${product} "${_tag_version}" _ok _base _dev)
		if (NOT _ok)
			continue()
		endif()
		set(_parsed_any TRUE)

		# 'rev-list -n 1' peels an annotated tag to its commit.
		_gplates_version_git(_result _tag_commit rev-list -n 1 "${_tag}")
		if (NOT _result EQUAL 0)
			continue()
		endif()
		if (_tag_commit STREQUAL _head_commit)
			# More than one tag can name this commit (a release tag and an anchor tag, say). One
			# matching the release target always wins. Otherwise the rule is the one below for
			# tags at the same distance, so that the tag giving this commit its number is the tag
			# the commits after it count from: a release beats an anchor, and among anchors the
			# largest number wins.
			gplates_join_version(${product} "${_base}" ${_dev} _this_tag_version)
			if (_at_tag_base STREQUAL "" OR _this_tag_version STREQUAL target
					OR (NOT _at_tag_base STREQUAL target AND NOT _at_tag_dev EQUAL 0
						AND (_dev EQUAL 0 OR _dev GREATER _at_tag_dev)))
				set(_at_tag_base "${_this_tag_version}")
				set(_at_tag_name "${_tag}")
				set(_at_tag_dev ${_dev})
			endif()
		endif()

		_gplates_version_git(_result _distance rev-list --count --first-parent "${_tag}..HEAD")
		if (NOT _result EQUAL 0)
			continue()
		endif()

		# A tag HEAD is an ancestor of has nothing to count from - it names a *later* commit, so
		# every first-parent commit of HEAD is already reachable from it and the distance is 0.
		# Skipping such tags is what lets an older commit be built at all: without it, checking
		# out anything below a tag (a 'git bisect' step, or just reproducing a bug at HEAD~10)
		# scored 0, won the minimum, and aborted the configure. Being *at* the tag is different,
		# and was handled above.
		if (_distance EQUAL 0 AND NOT _tag_commit STREQUAL _head_commit)
			continue()
		endif()

		# An anchor counts only if it is on HEAD's first-parent line. The distance above gives an
		# off-line tag a count too - from the branch point, which is what a series branch's
		# release tag needs - but a number recorded on another line means nothing on this one:
		# a fork's anchor, fetched by tag auto-follow when its branch is, or a development tag
		# on a feature branch since merged, would otherwise take over this line's count. 'HEAD~n'
		# follows first parents, so the commit that many steps back is the tag's own if it is on
		# the line.
		if (NOT _dev EQUAL 0 AND NOT _tag_commit STREQUAL _head_commit)
			_gplates_version_git(_result _line_commit rev-parse --verify "HEAD~${_distance}")
			if (NOT _result EQUAL 0 OR NOT _line_commit STREQUAL _tag_commit)
				continue()
			endif()
		endif()

		# A release of a series cut from the development branch after HEAD's history left it is
		# skipped (SERIES CUT RECORDS at the top of this file). The distance above would count
		# from HEAD's own fork point instead, and make this line look like the development branch
		# with its target left behind. Only a tag HEAD does not have in its history can be one; a
		# tag without a valid record is treated as it always was.
		if (_dev EQUAL 0 AND _tag IN_LIST _not_merged)
			_gplates_version_series_cut("${_tag_commit}" _cut _ignored)
			if (NOT _cut STREQUAL "")
				# Once per recorded commit, since a series' releases all record the same one.
				# Exit code 1 only: anything else means git could not answer.
				if (NOT DEFINED _cut_outside_head_${_cut})
					_gplates_version_git(_result _ignored
							merge-base --is-ancestor "${_cut}" "${_head_commit}")
					if (_result EQUAL 1)
						set(_cut_outside_head_${_cut} TRUE)
					else()
						set(_cut_outside_head_${_cut} FALSE)
					endif()
				endif()
				if (_cut_outside_head_${_cut})
					list(APPEND _skipped "${_tag}")
					list(APPEND _skipped_bases "${_base}")
					continue()
				endif()
			endif()
		endif()

		# Track the nearest release, which the release-target checks below compare against.
		if (_dev EQUAL 0)
			set(_is_nearer FALSE)
			if (_ref_base STREQUAL "" OR _distance LESS _ref_distance)
				set(_is_nearer TRUE)
			elseif (_distance EQUAL _ref_distance)
				gplates_compare_versions(${product} "${_base}" "${_ref_base}" _ref_cmp)
				if (NOT _ref_cmp STREQUAL "" AND _ref_cmp GREATER 0)
					set(_is_nearer TRUE)
				endif()
			endif()
			if (_is_nearer)
				set(_ref_base "${_base}")
				set(_ref_tag "${_tag}")
				set(_ref_commit "${_tag_commit}")
				set(_ref_distance ${_distance})
			endif()
		endif()

		# Nearest wins. At the same distance a release beats an anchor - a release resets the
		# count by design, and a development tag left beside it on the commit must not carry the
		# old count past it - and among anchors the largest number wins, so the count never goes
		# backwards.
		set(_wins FALSE)
		if (_best_distance LESS 0 OR _distance LESS _best_distance)
			set(_wins TRUE)
		elseif (_distance EQUAL _best_distance AND NOT _best_offset EQUAL 0)
			if (_dev EQUAL 0 OR _dev GREATER _best_offset)
				set(_wins TRUE)
			endif()
		endif()
		if (_wins)
			set(_best_distance ${_distance})
			set(_best_offset ${_dev})
			set(_best_tag "${_tag}")
		endif()
	endforeach()

	if (NOT _parsed_any)
		message(WARNING
				"Found git tags matching '${tag_prefix}*' but none of them parse as a ${product} "
				"version, so the development number cannot be counted from a release.")
		return()
	endif()
	if (_best_distance LESS 0)
		# Nothing to count from. Without a skipped tag, that is a commit older than every tag,
		# and the file fallbacks have always answered for it. With one, those fallbacks could
		# only answer with a stale recorded version (every 'pip install .' writes one into the
		# source tree), so refuse instead.
		if (NOT _skipped STREQUAL "")
			string(REPLACE ";" "', '" _skipped_text "${_skipped}")
			message(FATAL_ERROR
					"Cannot derive the ${product} version: the only '${tag_prefix}*' tags it "
					"could be counted from ('${_skipped_text}') are releases of series cut from "
					"the development branch after this commit's history left it, which are not "
					"counted from here. This repository may be missing the tags of earlier "
					"releases: fetch them from the upstream repository ('git fetch --tags "
					"<upstream>'). Or set the version explicitly (see the message at the end of "
					"VersionFromGit.cmake).")
		endif()
		return()
	endif()

	# The tag the count runs from, if it is an anchor (a tag at HEAD is always the nearest), is
	# checked against the release target at its own commit: an anchor's base is the version its
	# commit resolves to.
	if (NOT _best_offset EQUAL 0)
		_gplates_version_check_anchor(${product} "${tag_prefix}" "${_best_tag}")
	endif()

	# Check the hand-edited release target against the nearest release.
	#
	# Skipped when HEAD is standing on a release tag: there the target must *equal* that tag's
	# base, which the block below enforces, and check 1 rejects exactly that equality. An anchor
	# tag at HEAD is not skipped - the reference excludes anchors, so there is still a real
	# release behind it to check against, and a fork's target has to clear it like anyone else's.
	set(_at_release_tag FALSE)
	if (NOT _at_tag_base STREQUAL "" AND _at_tag_dev EQUAL 0)
		set(_at_release_tag TRUE)
	endif()
	if (NOT _at_release_tag AND NOT _ref_base STREQUAL "")
		# Whether the reference is on HEAD's first-parent line, which decides how a candidate is
		# treated. 'HEAD~n' follows first parents, so the commit that many steps back is the
		# reference's own commit if it is on the line, and otherwise the commit where the two
		# histories part - the one the release series branch was cut from.
		set(_ref_on_line FALSE)
		_gplates_version_git(_result _line_commit rev-parse --verify "HEAD~${_ref_distance}")
		if (_result EQUAL 0 AND _line_commit STREQUAL _ref_commit)
			set(_ref_on_line TRUE)
		endif()
		gplates_check_release_target(${product} "${target}" "${_ref_base}" "${_ref_tag}"
				${_ref_distance} ${_ref_on_line} _target_error)

		# Whether HEAD is on a release series line: its release file freezes the other product.
		set(_series_line FALSE)
		if (DEFINED GPLATES_FROZEN_VERSION OR DEFINED PYGPLATES_FROZEN_VERSION)
			set(_series_line TRUE)
		endif()

		# A target that skips past the reference may still be the release after a series
		# skipped above: a branch made from the development branch before that series was cut,
		# which then took the development branch's later target without its history (a squash
		# merge, or a cherry-picked target change), is not skipping a version - that series
		# released it. So for such a branch the no-skip policy accepts the release after any
		# skipped series too. Only that policy: a target at or below a skipped series' head is
		# not refused for it, which is the point of skipping the series.
		#
		# Not on a release series line, where every later series is skipped, and a target past
		# this series' head is the development branch's, taken by mistake. Nor against a
		# candidate on HEAD's first-parent line, whose refusal is a different rule ("not been
		# finished").
		set(_skip_relaxable FALSE)
		if (NOT _target_error STREQUAL "" AND NOT _skipped_bases STREQUAL "" AND NOT _series_line)
			gplates_split_version(${product} "${target}" _ok _target_base _target_dev)
			_gplates_version_base_rank(${product} "${_target_base}"
					_ok_t _target_head _t_rank _t_n _t_post)
			_gplates_version_base_rank(${product} "${_ref_base}"
					_ok_r _ref_head _r_rank _r_n _r_post)
			if (_ok AND _ok_t AND _ok_r AND _target_head VERSION_GREATER _ref_head
					AND NOT (_ref_on_line AND _r_rank LESS 4))
				set(_skip_relaxable TRUE)
				foreach (_skipped_base IN LISTS _skipped_bases)
					gplates_check_release_target(${product} "${target}" "${_skipped_base}" ""
							0 FALSE _skipped_error)
					if (_skipped_error STREQUAL "")
						set(_target_error "")
						break()
					endif()
				endforeach()
			endif()
		endif()

		if (NOT _target_error STREQUAL "")
			if (_skip_relaxable)
				string(REPLACE ";" "', '" _skipped_text "${_skipped}")
				string(APPEND _target_error
						"\nThe releases of series cut after this commit's history left the "
						"development branch ('${_skipped_text}') are not counted from here, but "
						"the release after one of them would also be accepted.")
			endif()
			if (NOT _ref_on_line)
				gplates_off_line_refusal_advice(${product} "${target}" "${_ref_base}" "${_ref_tag}"
						${_series_line} _advice)
				string(APPEND _target_error "${_advice}")
			endif()
			message(FATAL_ERROR "${_target_error}")
		endif()

		gplates_check_untagged_release(${product} "${tag_prefix}" "${target}" "${_ref_base}"
				"${_ref_tag}" ${_ref_distance} ${_ref_on_line} _untagged_error)
		if (NOT _untagged_error STREQUAL "")
			# A commit already in the history of that release is history, not a build anyone
			# will publish: the release commit of a patch branch whose merge was tagged, say, or
			# one with a fix tagged on top of it under the publishing run's override. Refusing
			# it would leave it unbuildable for good, 'git bisect' included, so it is only warned
			# about.
			_gplates_version_git(_result _later_release
					tag --list "${tag_prefix}${target}" --contains HEAD)
			if (_result EQUAL 0 AND NOT _later_release STREQUAL "")
				message(WARNING
						"${_untagged_error}\nAllowed, because this commit is already in the "
						"history of '${_later_release}': building it is looking back, not "
						"preparing a release.")
			else()
				message(FATAL_ERROR "${_untagged_error}")
			endif()
		endif()
	endif()

	# Standing on a tag.
	if (NOT _at_tag_base STREQUAL "")
		if (NOT _at_tag_dev EQUAL 0)
			# An anchor tag - it exists to carry a development number, so take that number and
			# leave the base version to the release target (which its own base was checked
			# against above).
			gplates_join_version(${product} "${target}" ${_at_tag_dev} _version)
			set(${out_var} "${_version}" PARENT_SCOPE)
			return()
		endif()
		# A release tag: the version is exactly the target, with no development number.
		if (NOT _at_tag_base STREQUAL target)
			message(FATAL_ERROR
					"HEAD is at tag '${_at_tag_name}', but the ${product} release target is "
					"'${target}'. Set the target in 'cmake/modules/VersionRelease.cmake' to "
					"'${_at_tag_base}', or tag the release to match the target.")
		endif()
		# A release is tagged on a release series branch of its product, which freezes the other
		# product's version. Forgotten, nothing goes wrong until the other product is released on
		# a series branch cut later, and from then on this commit stops configuring anyway
		# (gplates_check_frozen_version says why) - by which time its tag cannot be repaired. So
		# refusing it now loses nothing. The answer depends only on this commit and its own tag,
		# so it can never change afterwards.
		if (product STREQUAL "gplates")
			set(_other pygplates)
		else()
			set(_other gplates)
		endif()
		string(TOUPPER "${_other}" _other_upper)
		if (NOT DEFINED ${_other_upper}_FROZEN_VERSION)
			message(FATAL_ERROR
					"This commit is tagged '${_at_tag_name}', but 'cmake/modules/VersionRelease.cmake' "
					"does not freeze the ${_other} version. A ${product} release is tagged on a "
					"${product} release series branch, and the commit that cuts it replaces "
					"'set(${_other_upper}_RELEASE_VERSION ...)' with "
					"'set(${_other_upper}_FROZEN_VERSION <version>)', the version that "
					"'cmake -P cmake/modules/VersionFromGit.cmake ${_other}' gives on the "
					"development branch commit the series branch was cut from. Without it, the first "
					"${_other} release made on a series branch cut later stops every configure here, "
					"this tag included. Delete the tag, freeze ${_other} in a new commit, and tag "
					"that. If the series branch does not yet record the development branch commit "
					"it was made from, record it in the same new commit: "
					"'set(RELEASE_SERIES_CUT_FROM <commit>)', with <commit> the parent of the "
					"series branch's first commit - not the parent of the new commit.")
		endif()
		# The same commit that cuts the series branch records where it was cut from (SERIES CUT
		# RECORDS at the top of this file). Forgotten, every branch made from the development
		# branch before the cut would stop configuring once this tag exists, and a tag cannot be
		# repaired afterwards - so, like the freeze, it is checked at the tag. Read from the
		# commit rather than the working tree, since the commit is what other clones will read.
		_gplates_version_series_cut("${_head_commit}" _cut _problem)
		if (_cut STREQUAL "")
			message(FATAL_ERROR
					"This commit is tagged '${_at_tag_name}', but its "
					"'cmake/modules/VersionRelease.cmake' ${_problem}. A release series branch "
					"records, beside the freeze, the development branch commit it was made "
					"from, in full: 'set(RELEASE_SERIES_CUT_FROM <commit>)', with <commit> the "
					"parent of the series branch's first commit (or 'git merge-base HEAD "
					"<development branch>') - the parent of this commit only if this is that "
					"first commit. Without the record, every branch made from the development "
					"branch before the cut stops configuring once this tag exists; and a record "
					"naming a series commit instead would switch off the development branch's "
					"check that it moves its target on. Delete the tag, record the cut in a new "
					"commit, and tag that.")
		endif()
		set(${out_var} "${target}" PARENT_SCOPE)
		return()
	endif()

	math(EXPR _dev_number "${_best_offset} + ${_best_distance}")
	gplates_join_version(${product} "${target}" ${_dev_number} _version)
	set(${out_var} "${_version}" PARENT_SCOPE)
endfunction()


#
# Resolve the full version of 'gplates' or 'pygplates' into <out_var>.
#
# In order of precedence:
#
#   1. The CMake variable itself, if already set (eg, '-DPYGPLATES_PEP440_VERSION=1.2.0.dev3').
#   2. The environment variable of the same name. This is how the version reaches a build that
#      cannot run git: cibuildwheel copies the project into a Linux container that has no git
#      (and a shallow checkout anyway), and conda-build sets it from the recipe version.
#   3. The frozen version, on a release series branch of the other product (see
#      'VersionRelease.cmake' and gplates_check_frozen_version).
#   4. Derived from git, as described at the top of this file.
#   5. 'cmake/modules/VersionRecorded.cmake', which 'cmake/version.py' writes into the pyGPlates
#      sdist: an unpacked sdist, or the conda-forge feedstock building from one. (The sdist's
#      PKG-INFO has only the pyGPlates version, and both are needed.)
#
# Git is tried ahead of the recorded file, and a repository with none of the product's tags - or
# none left to count from once releases of series cut after this commit's history left the
# development branch are skipped - is refused rather than passed on to it, so that a stale
# recorded file does not win in a working
# checkout: the file is gitignored, but every 'pip install .' writes one into the source tree.
#
function(gplates_resolve_version product out_var)
	if (product STREQUAL "gplates")
		set(_variable_name GPLATES_SEMANTIC_VERSION)
		set(_target "${GPLATES_RELEASE_VERSION}")
		set(_frozen_name GPLATES_FROZEN_VERSION)
		set(_other_frozen_name PYGPLATES_FROZEN_VERSION)
		set(_tag_prefix "GPlates-")
	elseif (product STREQUAL "pygplates")
		set(_variable_name PYGPLATES_PEP440_VERSION)
		set(_target "${PYGPLATES_RELEASE_VERSION}")
		set(_frozen_name PYGPLATES_FROZEN_VERSION)
		set(_other_frozen_name GPLATES_FROZEN_VERSION)
		set(_tag_prefix "PyGPlates-")
	else()
		message(FATAL_ERROR "Unknown product '${product}' - expected 'gplates' or 'pygplates'.")
	endif()

	# 1. Already set.
	if (DEFINED ${_variable_name})
		set(${out_var} "${${_variable_name}}" PARENT_SCOPE)
		return()
	endif()

	# 2. Environment.
	set(_environment_version "$ENV{${_variable_name}}")
	if (NOT _environment_version STREQUAL "")
		set(${out_var} "${_environment_version}" PARENT_SCOPE)
		return()
	endif()

	# 3. Frozen.
	if (DEFINED ${_frozen_name})
		if (NOT _target STREQUAL "")
			message(FATAL_ERROR
					"'cmake/modules/VersionRelease.cmake' sets both a release target and a frozen "
					"version for ${product}. A release series branch of the other product replaces "
					"the ${product} target with the frozen version; set one or the other.")
		endif()
		if (DEFINED ${_other_frozen_name})
			message(FATAL_ERROR
					"'cmake/modules/VersionRelease.cmake' freezes the versions of both products, "
					"so nothing could be released from this branch. A release series branch "
					"freezes only the product it does not release.")
		endif()
		_gplates_version_check_frozen(${product} "${_tag_prefix}" "${${_frozen_name}}")
		set(${out_var} "${${_frozen_name}}" PARENT_SCOPE)
		return()
	endif()

	# The release target has to be a target, not a full version - the development number is what
	# gets counted, so a target carrying one would be doubled.
	if (_target STREQUAL "")
		message(FATAL_ERROR "No release target set for ${product} in 'cmake/modules/VersionRelease.cmake'.")
	endif()
	gplates_split_version(${product} "${_target}" _target_ok _target_base _target_dev)
	if (NOT _target_ok)
		message(FATAL_ERROR "The ${product} release target '${_target}' is not a valid version.")
	endif()
	if (NOT _target_dev EQUAL 0)
		message(FATAL_ERROR
				"The ${product} release target '${_target}' carries a development number. That part "
				"is counted from git - set the target to '${_target_base}' instead.")
	endif()

	# 4. Git.
	_gplates_version_from_git(${product} "${_tag_prefix}" "${_target}" _version)
	if (NOT _version STREQUAL "")
		set(${out_var} "${_version}" PARENT_SCOPE)
		return()
	endif()

	# 5. The recorded version, for an unpacked sdist.
	if (EXISTS "${GPLATES_VERSION_SOURCE_DIR}/cmake/modules/VersionRecorded.cmake")
		include("${GPLATES_VERSION_SOURCE_DIR}/cmake/modules/VersionRecorded.cmake")
		if (DEFINED ${_variable_name})
			set(${out_var} "${${_variable_name}}" PARENT_SCOPE)
			return()
		endif()
	endif()

	# A development version of the target, spelt for the product, for the message.
	if (product STREQUAL "pygplates")
		set(_example "${_target}.dev<n>")
	elseif (_target MATCHES [[^[0-9]+\.[0-9]+\.[0-9]+$]])
		set(_example "${_target}-<n>")
	else()
		set(_example "${_target}.<n>")
	endif()
	message(FATAL_ERROR
			"Cannot determine the ${product} version.\n"
			"It is normally counted from this repository's git tags, but there is no usable git "
			"repository here (no git executable, no '.git', or a source tree extracted from an "
			"archive).\n"
			"Set it explicitly, either as a CMake define:\n"
			"    -D${_variable_name}=${_example}\n"
			"or as an environment variable of the same name - and likewise the other product's "
			"version (GPLATES_SEMANTIC_VERSION or PYGPLATES_PEP440_VERSION), since a build of "
			"either product resolves both. A packager building from an archive of a release "
			"should use that release's version.")
endfunction()


#
# Standalone use: 'cmake -P cmake/modules/VersionFromGit.cmake <gplates|pygplates>' prints the
# resolved version on standard output, which is how the build workflows and the scikit-build-core
# metadata plugin ('cmake/version.py') get it without configuring a build.
#
if (CMAKE_SCRIPT_MODE_FILE AND CMAKE_CURRENT_LIST_FILE STREQUAL CMAKE_SCRIPT_MODE_FILE)
	# The product is the argument after this script, which follows '-P'. Not a fixed position:
	# '-D' options may come before '-P' ('cmake -DPYGPLATES_PEP440_VERSION=... -P ...').
	set(_product "")
	math(EXPR _last "${CMAKE_ARGC} - 1")
	foreach (_i RANGE 1 ${_last})
		if (CMAKE_ARGV${_i} STREQUAL "-P")
			math(EXPR _product_index "${_i} + 2")
			if (DEFINED CMAKE_ARGV${_product_index})
				set(_product "${CMAKE_ARGV${_product_index}}")
			endif()
			break()
		endif()
	endforeach()
	if (_product STREQUAL "")
		message(FATAL_ERROR "usage: cmake -P cmake/modules/VersionFromGit.cmake <gplates|pygplates>")
	endif()
	gplates_resolve_version("${_product}" GPLATES_RESOLVED_VERSION)
	# 'message()' writes to standard error in script mode, so echo it instead.
	execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${GPLATES_RESOLVED_VERSION}")
endif()
