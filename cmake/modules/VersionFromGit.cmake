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
# hands the develop branch the series' target, with no guard firing.)
# 'rev-list --count --first-parent <tag>..HEAD' needs no such ancestry - it counts the
# first-parent commits of HEAD that are not reachable from the tag - so the tags work unchanged.
# Counting from a tag on a series branch gives the number of commits since that series was cut,
# which is the more meaningful quantity anyway.
#
# ANCHOR TAGS: because the *nearest* tag wins, a tag placed on a branch takes over the numbering
# from that point. That is how a downstream fork keeps its own development numbers without
# colliding with upstream's (tag its branch 'GPlates-2.6.0-2000', and its own commits count on
# from 2000), and how upstream can re-anchor a counter that has grown unwieldy. Give an anchor
# tag a non-zero development number, so it stays distinguishable from the release itself.
#
# GPlates depends on one right now: 'GPlates-2.6.0-47'. The gplates branch's first-parent line
# runs back through the 2013 'python-api' branch, and no ancestor of any GPlates release tag
# newer than that sits on it - so without the anchor the count runs from 2013 and gives
# 2.6.0-1206 instead of 2.6.0-47. Deleting the tag would silently restore the larger number.
# Once 'release/gplates-2.6' is cut and 'GPlates-2.6.0' tagged on it, the tip counts from that
# tag instead - but every commit between the anchor and the branch point still counts from the
# anchor (a release tag scores 0 there and is skipped, see below), and those are the commits
# 'git bisect' walks. So the anchor stays load-bearing for its stretch of history: never delete
# it.
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
# skips - the one apparent gap, 0.9.5 to 0.9.7.1, is a release whose tag went missing, and the big
# early pyGPlates jumps are from an era that used the SVN revision as the minor version - so this
# matches the project's actual practice. It is policy rather than correctness, and a future
# maintainer wanting a deliberate jump can loosen it without breaking anything.
#
# A candidate binds the line it is on, and only that line. 'on_line' says whether the reference
# tag sits on HEAD's first-parent line:
#
#   - On the line (a release series branch, or a patch branch off one): only the same numeric
#     head may follow a candidate, so '1.1.0rc1' can be followed by '1.1.0rc2' or '1.1.0' but
#     not by '1.1.1'. There is no such thing as skipping past a release that never happened.
#   - Off the line (the development branch, once the series branch cut from it has its first
#     candidate; or a feature branch cut before then): the candidate's head counts as released
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
				return()
			endif()
			string(CONCAT _msg
					"The ${product} release target is '${target}', but the release line it follows has "
					"not been finished: '${reference_tag}' is a candidate for '${_ref_head}', "
					"${distance} commit(s) back. Set the target in ${_where} to another candidate for "
					"'${_ref_head}', or to '${_ref_head}' itself."
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
					"after '${_ref_head}' - or, on a branch cut before the series branch was, merge the "
					"development branch in, where the target has already moved on."
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
# Derive the version from git, or leave <out_var> empty if this source tree has no usable
# repository (an unpacked sdist, an exported tarball, or a build container without git).
#
function(_gplates_version_from_git product tag_prefix target out_var)
	set(${out_var} "" PARENT_SCOPE)

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
	if (NOT _result EQUAL 0 OR _tags STREQUAL "")
		return()
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
			# matching the release target always wins; otherwise the first listed does, which keeps
			# the choice deterministic ('git tag --list' sorts its output).
			gplates_join_version(${product} "${_base}" ${_dev} _this_tag_version)
			if (_at_tag_base STREQUAL "" OR _this_tag_version STREQUAL target)
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

		# Nearest wins; among equally near tags the largest development number wins, so the
		# count never goes backwards.
		if (_best_distance LESS 0 OR _distance LESS _best_distance)
			set(_best_distance ${_distance})
			set(_best_offset ${_dev})
			set(_best_tag "${_tag}")
		elseif (_distance EQUAL _best_distance)
			if (_dev GREATER _best_offset)
				set(_best_offset ${_dev})
				set(_best_tag "${_tag}")
			endif()
		endif()
	endforeach()

	if (NOT _parsed_any)
		message(WARNING
				"Found git tags matching '${tag_prefix}*' but none of them parse as a ${product} "
				"version, so the development number cannot be counted from a release.")
		return()
	endif()
	if (_best_distance LESS 0)
		return()
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
		if (NOT _target_error STREQUAL "")
			message(FATAL_ERROR "${_target_error}")
		endif()
	endif()

	# Standing on a tag.
	if (NOT _at_tag_base STREQUAL "")
		if (NOT _at_tag_dev EQUAL 0)
			# An anchor tag - it exists to carry a development number, so take that number and
			# leave the base version to the release target (a fork's anchor may well name an
			# older release than the one being worked towards).
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
		set(${out_var} "${target}" PARENT_SCOPE)
		return()
	endif()

	math(EXPR _dev_number "${_best_offset} + ${_best_distance}")
	gplates_join_version(${product} "${target}" ${_dev_number} _version)
	set(${out_var} "${_version}" PARENT_SCOPE)
	_gplates_version_watch_head()
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
#   3. Derived from git, as described at the top of this file.
#   4. 'Version:' from a PKG-INFO beside this source tree - an unpacked pyGPlates sdist, whose
#      version was resolved when the sdist was built.
#   5. 'cmake/modules/VersionRecorded.cmake', for a source archive made without a repository.
#
# Git is tried ahead of the file fallbacks so that a stale recorded file can never win in a
# working checkout.
#
function(gplates_resolve_version product out_var)
	if (product STREQUAL "gplates")
		set(_variable_name GPLATES_SEMANTIC_VERSION)
		set(_target "${GPLATES_RELEASE_VERSION}")
		set(_tag_prefix "GPlates-")
	elseif (product STREQUAL "pygplates")
		set(_variable_name PYGPLATES_PEP440_VERSION)
		set(_target "${PYGPLATES_RELEASE_VERSION}")
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

	# 3. Git.
	_gplates_version_from_git(${product} "${_tag_prefix}" "${_target}" _version)
	if (NOT _version STREQUAL "")
		set(${out_var} "${_version}" PARENT_SCOPE)
		return()
	endif()

	# 4. An unpacked sdist (pyGPlates only - GPlates has no sdist, and a PKG-INFO found beside it
	#    would be describing something else).
	if (product STREQUAL "pygplates" AND EXISTS "${GPLATES_VERSION_SOURCE_DIR}/PKG-INFO")
		file(READ "${GPLATES_VERSION_SOURCE_DIR}/PKG-INFO" _pkg_info)
		# A leading newline, then required, so this matches the 'Version:' header rather than the
		# tail of 'Metadata-Version:' - which is the first header in the file. CMake's regex engine
		# has no multiline '^' to anchor with.
		string(PREPEND _pkg_info "\n")
		if (_pkg_info MATCHES "[\r\n]Version:[ \t]*([^\r\n]+)")
			string(STRIP "${CMAKE_MATCH_1}" _version)
			if (NOT _version STREQUAL "")
				set(${out_var} "${_version}" PARENT_SCOPE)
				return()
			endif()
		endif()
	endif()

	# 5. A recorded version, for a source archive made without a repository.
	if (EXISTS "${GPLATES_VERSION_SOURCE_DIR}/cmake/modules/VersionRecorded.cmake")
		include("${GPLATES_VERSION_SOURCE_DIR}/cmake/modules/VersionRecorded.cmake")
		if (DEFINED ${_variable_name})
			set(${out_var} "${${_variable_name}}" PARENT_SCOPE)
			return()
		endif()
	endif()

	message(FATAL_ERROR
			"Cannot determine the ${product} version.\n"
			"It is normally counted from this repository's git tags, but there is no usable git "
			"repository here (no git executable, no '.git', or a source tree extracted from an "
			"archive).\n"
			"Set it explicitly, either as a CMake define:\n"
			"    -D${_variable_name}=${_target}-<n>\n"
			"or as an environment variable of the same name. A packager building from an archive "
			"should use the version that archive was made from.")
endfunction()


#
# Standalone use: 'cmake -P cmake/modules/VersionFromGit.cmake <gplates|pygplates>' prints the
# resolved version on standard output, which is how the build workflows and the scikit-build-core
# metadata plugin ('cmake/version.py') get it without configuring a build.
#
if (CMAKE_SCRIPT_MODE_FILE AND CMAKE_CURRENT_LIST_FILE STREQUAL CMAKE_SCRIPT_MODE_FILE)
	if (NOT DEFINED CMAKE_ARGV3)
		message(FATAL_ERROR "usage: cmake -P cmake/modules/VersionFromGit.cmake <gplates|pygplates>")
	endif()
	gplates_resolve_version("${CMAKE_ARGV3}" GPLATES_RESOLVED_VERSION)
	# 'message()' writes to standard error in script mode, so echo it instead.
	execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${GPLATES_RESOLVED_VERSION}")
endif()
