##########################################################
# Tests for the version resolver's pure helper functions #
##########################################################

#
# Run with:
#
#     cmake -P cmake/modules/VersionFromGitTest.cmake
#
# or as the CTest test 'version-resolver-test', which the root 'CMakeLists.txt' registers.
#
# This covers mainly the parts of 'VersionFromGit.cmake' that are pure functions of their
# arguments - splitting and joining a version, ordering two of them, checking the hand-edited
# release target against the nearest release, checking an anchor tag against the release target
# at its own commit, checking a frozen version, and reading a series cut record. The parts that
# consult git are not covered
# case by case: what they compute is mostly a commit count rather than a decision. The
# exception is a set of scenarios on one synthetic repository, with a release series branch per
# product, for failures that come from the shape of the history rather than from any one
# decision. The whole resolver is then run once, at the end, against the repository this file
# is in.
#
# That split is deliberate rather than a shortcut. The decisions this file tests are the ones
# that abort a configure, and each is a rule somebody has to be able to change with confidence
# - particularly the no-skip policy, which is a convention rather than a correctness
# requirement, and is meant to be loosenable.
#

cmake_minimum_required(VERSION 3.22)

include("${CMAKE_CURRENT_LIST_DIR}/VersionFromGit.cmake")

set(_failures 0)

# The message may be given in pieces, as the assertions below do to stay within the line width.
function(_fail)
	math(EXPR _n "${_failures} + 1")
	set(_failures ${_n} PARENT_SCOPE)
	string(JOIN "" _message ${ARGV})
	message(SEND_ERROR "${_message}")
endfunction()

# The version splits into the expected base and development number.
function(expect_split product version expected_base expected_dev)
	gplates_split_version(${product} "${version}" _ok _base _dev)
	if (NOT _ok)
		_fail("${product} '${version}': expected it to parse, but it did not.")
	elseif (NOT _base STREQUAL expected_base OR NOT _dev EQUAL expected_dev)
		_fail("${product} '${version}': split to base '${_base}' dev ${_dev}, "
				"expected base '${expected_base}' dev ${expected_dev}.")
	endif()
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

# The version does not parse, so it is skipped rather than counted from.
function(expect_no_split product version)
	gplates_split_version(${product} "${version}" _ok _base _dev)
	if (_ok)
		_fail("${product} '${version}': expected it not to parse, but it gave base '${_base}'.")
	endif()
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

function(expect_join product base dev expected)
	gplates_join_version(${product} "${base}" ${dev} _version)
	if (NOT _version STREQUAL expected)
		_fail("${product} base '${base}' + dev ${dev}: joined to '${_version}', expected '${expected}'.")
	endif()
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

# 'a' sorts strictly below 'b', and 'b' strictly above 'a'.
function(expect_below product a b)
	gplates_compare_versions(${product} "${a}" "${b}" _cmp)
	if (NOT _cmp STREQUAL "-1")
		_fail("${product}: expected '${a}' to sort below '${b}', got '${_cmp}'.")
	endif()
	gplates_compare_versions(${product} "${b}" "${a}" _reverse)
	if (NOT _reverse STREQUAL "1")
		_fail("${product}: expected '${b}' to sort above '${a}', got '${_reverse}' - the "
				"comparison is not antisymmetric.")
	endif()
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

function(expect_equal_versions product a b)
	gplates_compare_versions(${product} "${a}" "${b}" _cmp)
	if (NOT _cmp STREQUAL "0")
		_fail("${product}: expected '${a}' and '${b}' to compare equal, got '${_cmp}'.")
	endif()
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

# Neither side can be ranked, so the caller is told so rather than given a wrong order.
function(expect_incomparable product a b)
	gplates_compare_versions(${product} "${a}" "${b}" _cmp)
	if (NOT _cmp STREQUAL "")
		_fail("${product}: expected '${a}' vs '${b}' to be incomparable, got '${_cmp}'.")
	endif()
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

# The release target is acceptable against this reference release.
# 'on_line' is whether the reference tag is on HEAD's first-parent line (a release series
# branch) or off it (the development branch, seen from which every series tag is off the line).
# An empty 'expected_phrase' means the target is expected to be accepted; otherwise it is
# expected to be rejected, with a message mentioning the phrase, so that the right one of the
# refusals is doing the rejecting.
function(_expect_target product target reference on_line expected_phrase)
	gplates_check_release_target(${product} "${target}" "${reference}" "tag-${reference}" 3
			${on_line} _error)
	if (on_line)
		set(_seen "after release '${reference}'")
	else()
		set(_seen "with '${reference}' released on another branch")
	endif()
	if (expected_phrase STREQUAL "")
		if (NOT _error STREQUAL "")
			_fail("${product} target '${target}' ${_seen}: expected it to be accepted, but got:"
					"\n  ${_error}")
		endif()
	elseif (_error STREQUAL "")
		_fail("${product} target '${target}' ${_seen}: expected it to be rejected, but it was "
				"accepted.")
	elseif (NOT _error MATCHES "${expected_phrase}")
		_fail("${product} target '${target}' ${_seen}: rejected, but for the wrong reason - "
				"expected a message matching '${expected_phrase}', got:\n  ${_error}")
	endif()
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

function(expect_target_ok product target reference)
	_expect_target(${product} "${target}" "${reference}" TRUE "")
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

function(expect_target_rejected product target reference expected_phrase)
	_expect_target(${product} "${target}" "${reference}" TRUE "${expected_phrase}")
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

function(expect_target_ok_off_line product target reference)
	_expect_target(${product} "${target}" "${reference}" FALSE "")
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

function(expect_target_rejected_off_line product target reference expected_phrase)
	_expect_target(${product} "${target}" "${reference}" FALSE "${expected_phrase}")
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

# An empty 'expected_tag' means the anchor is expected to be accepted; otherwise it is expected
# to be rejected, with a message naming that tag as the one the commit should carry instead.
# 'release_file' stands in for 'VersionRelease.cmake' as it was at the anchor's commit.
function(_expect_anchor product prefix version release_file expected_tag)
	gplates_check_anchor_tag(${product} "${prefix}" "${version}" "${release_file}" _error)
	if (expected_tag STREQUAL "")
		if (NOT _error STREQUAL "")
			_fail("${product} anchor '${prefix}${version}': expected it to be accepted, but got:"
					"\n  ${_error}")
		endif()
	elseif (_error STREQUAL "")
		_fail("${product} anchor '${prefix}${version}': expected it to be rejected, but it was "
				"accepted.")
	elseif (NOT _error MATCHES "'${expected_tag}'")
		_fail("${product} anchor '${prefix}${version}': rejected, but without naming "
				"'${expected_tag}' as the tag to use instead, got:\n  ${_error}")
	endif()
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

function(expect_anchor_ok product prefix version release_file)
	_expect_anchor(${product} "${prefix}" "${version}" "${release_file}" "")
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

function(expect_anchor_rejected product prefix version release_file expected_tag)
	_expect_anchor(${product} "${prefix}" "${version}" "${release_file}" "${expected_tag}")
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

# The release's target on a commit not tagged as it. An empty 'expected_phrase' means the target
# is expected to be accepted.
function(_expect_untagged product prefix target reference distance on_line expected_phrase)
	gplates_check_untagged_release(${product} "${prefix}" "${target}" "${reference}"
			"${prefix}${reference}" ${distance} ${on_line} _error)
	set(_seen "${product} target '${target}', ${distance} commit(s) after '${reference}'")
	if (expected_phrase STREQUAL "")
		if (NOT _error STREQUAL "")
			_fail("${_seen}, untagged: expected it to be accepted, but got:\n  ${_error}")
		endif()
	elseif (_error STREQUAL "")
		_fail("${_seen}, untagged: expected it to be rejected, but it was accepted.")
	elseif (NOT _error MATCHES "${expected_phrase}")
		_fail("${_seen}, untagged: rejected, but for the wrong reason - expected a message "
				"matching '${expected_phrase}', got:\n  ${_error}")
	endif()
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

function(expect_untagged_ok product prefix target reference distance on_line)
	_expect_untagged(${product} "${prefix}" "${target}" "${reference}" ${distance} ${on_line} "")
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

function(expect_untagged_rejected product prefix target reference distance on_line
		expected_phrase)
	_expect_untagged(${product} "${prefix}" "${target}" "${reference}" ${distance} ${on_line}
			"${expected_phrase}")
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

# 'tags_at_head' stands in for the tags on the commit being built. An empty 'expected_phrase'
# means the frozen version is expected to be accepted.
function(_expect_frozen product prefix version tags_at_head expected_phrase)
	gplates_check_frozen_version(${product} "${prefix}" "${version}" "${tags_at_head}" _error)
	if (expected_phrase STREQUAL "")
		if (NOT _error STREQUAL "")
			_fail("${product} frozen at '${version}', tags '${tags_at_head}': expected it to be "
					"accepted, but got:\n  ${_error}")
		endif()
	elseif (_error STREQUAL "")
		_fail("${product} frozen at '${version}', tags '${tags_at_head}': expected it to be "
				"rejected, but it was accepted.")
	elseif (NOT _error MATCHES "${expected_phrase}")
		_fail("${product} frozen at '${version}', tags '${tags_at_head}': rejected, but for "
				"the wrong reason - expected a message matching '${expected_phrase}', got:\n"
				"  ${_error}")
	endif()
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

function(expect_frozen_ok product prefix version tags_at_head)
	_expect_frozen(${product} "${prefix}" "${version}" "${tags_at_head}" "")
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

function(expect_frozen_rejected product prefix version tags_at_head expected_phrase)
	_expect_frozen(${product} "${prefix}" "${version}" "${tags_at_head}" "${expected_phrase}")
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

# The release file records the given cut ('' for no record).
function(expect_series_cut release_file expected)
	gplates_read_series_cut("${release_file}" _cut)
	if (NOT _cut STREQUAL expected)
		_fail("series cut read from '${release_file}': got '${_cut}', expected '${expected}'.")
	endif()
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

# The release file freezes a product (TRUE) or not (FALSE).
function(expect_freezes release_file expected)
	gplates_release_file_freezes("${release_file}" _freezes)
	if (NOT _freezes STREQUAL expected)
		_fail("'${release_file}': freezes is ${_freezes}, expected ${expected}.")
	endif()
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

# The release file could be a development branch commit's (an empty phrase), or the reason it
# could not matches the phrase.
function(expect_not_development release_file expected_phrase)
	gplates_release_file_not_development("${release_file}" _why)
	if (expected_phrase STREQUAL "")
		if (NOT _why STREQUAL "")
			_fail("'${release_file}': expected a development branch file, got: ${_why}")
		endif()
	elseif (NOT _why MATCHES "${expected_phrase}")
		_fail("'${release_file}': expected a reason matching '${expected_phrase}', got '${_why}'.")
	endif()
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()

# The advice added to an off-line refusal matches the phrase, or is empty for an empty phrase.
function(expect_off_line_advice product target reference series_line expected_phrase)
	gplates_off_line_refusal_advice(${product} "${target}" "${reference}" "Tag-${reference}"
			${series_line} _advice)
	if (expected_phrase STREQUAL "")
		if (NOT _advice STREQUAL "")
			_fail("${product} target '${target}', off-line reference '${reference}': expected no "
					"advice, got:\n  ${_advice}")
		endif()
	elseif (NOT _advice MATCHES "${expected_phrase}")
		_fail("${product} target '${target}', off-line reference '${reference}': expected advice "
				"matching '${expected_phrase}', got:\n  ${_advice}")
	endif()
	set(_failures ${_failures} PARENT_SCOPE)
endfunction()


#
# Splitting.
#
expect_split(gplates "2.6.0" "2.6.0" 0)
expect_split(gplates "2.6.0-8" "2.6.0" 8)
expect_split(gplates "2.6.0-rc.1" "2.6.0-rc.1" 0)
expect_split(gplates "2.6.0-rc.1.8" "2.6.0-rc.1" 8)
# Two-component release tags are normalised, which is what lets 'GPlates-2.5' be counted from.
expect_split(gplates "2.5" "2.5.0" 0)
expect_split(pygplates "1.1.0" "1.1.0" 0)
expect_split(pygplates "1.1.0.dev10" "1.1.0" 10)
expect_split(pygplates "1.1.0rc1" "1.1.0rc1" 0)
expect_split(pygplates "1.1.0rc1.dev5" "1.1.0rc1" 5)
expect_split(pygplates "0.36" "0.36.0" 0)

# The tags in this repository that must be skipped rather than counted from.
expect_no_split(gplates "1.5+hellinger-testing")
expect_no_split(gplates "0.9.10.1")

#
# Joining. A development number is appended with '-' to a plain GPlates version and with '.' to
# one that already carries a pre-release suffix, which keeps both the SemVer and the Debian
# orderings intact.
#
expect_join(gplates "2.6.0" 0 "2.6.0")
expect_join(gplates "2.6.0" 8 "2.6.0-8")
expect_join(gplates "2.6.0-rc.1" 8 "2.6.0-rc.1.8")
expect_join(pygplates "1.1.0" 0 "1.1.0")
expect_join(pygplates "1.1.0" 10 "1.1.0.dev10")
expect_join(pygplates "1.1.0rc1" 5 "1.1.0rc1.dev5")

#
# Ordering. The case that matters most is a candidate against its release: CMake's own
# VERSION_LESS reads '1.1.0rc1' and '1.1.0' as equal, so the resolver cannot use it.
#
expect_below(pygplates "1.1.0rc1" "1.1.0")
expect_below(pygplates "1.1.0a1" "1.1.0b1")
expect_below(pygplates "1.1.0b1" "1.1.0rc1")
expect_below(pygplates "1.1.0rc1" "1.1.0rc2")
expect_below(pygplates "1.1.0" "1.1.0.post1")
expect_below(pygplates "1.0.0" "1.1.0")
expect_below(pygplates "1.9.0" "1.10.0")
expect_equal_versions(pygplates "1.1.0" "1.1.0")

expect_below(gplates "2.6.0-alpha.1" "2.6.0-beta.1")
expect_below(gplates "2.6.0-beta.1" "2.6.0-rc.1")
expect_below(gplates "2.6.0-rc.1" "2.6.0")
expect_below(gplates "2.5.0" "2.6.0")

# An unrankable base is reported as such, so that a tag nobody anticipated cannot silently
# order itself first.
expect_incomparable(gplates "2.6.0" "not-a-version")

#
# The release target, checked against the nearest release. The table below is the one in
# 'gplates_check_release_target'.
#
# After a plain release, the next patch, minor or major is allowed - and a candidate of any of
# them - while anything further ahead is a skipped release.
#
expect_target_ok(pygplates "1.0.1" "1.0.0")
expect_target_ok(pygplates "1.1.0" "1.0.0")
expect_target_ok(pygplates "2.0.0" "1.0.0")
expect_target_ok(pygplates "1.0.1rc1" "1.0.0")
expect_target_ok(pygplates "1.1.0rc1" "1.0.0")
# A post-release republishes the same version, so it keeps the head it follows.
expect_target_ok(pygplates "1.0.0.post1" "1.0.0")

expect_target_rejected(pygplates "1.0.0" "1.0.0" "already been released")
expect_target_rejected(pygplates "0.9.0" "1.0.0" "sorts below")
expect_target_rejected(pygplates "1.0.0rc2" "1.0.0" "sorts below")
expect_target_rejected(pygplates "1.0.2" "1.0.0" "skips past")
expect_target_rejected(pygplates "1.2.0" "1.0.0" "skips past")
expect_target_rejected(pygplates "3.0.0" "1.0.0" "skips past")

# After a candidate on this line - the release series branch - only the same release line may
# follow: another candidate, or the release it was a candidate for. This is the step that must
# not fire during a real release. (Where the release's target may stand is a separate check,
# gplates_check_untagged_release, tested below.)
expect_target_ok(pygplates "1.1.0rc2" "1.1.0rc1")
expect_target_ok(pygplates "1.1.0" "1.1.0rc1")
expect_target_rejected(pygplates "1.1.0rc1" "1.1.0rc1" "already been released")
expect_target_rejected(pygplates "1.1.0a1" "1.1.0rc1" "sorts below")
expect_target_rejected(pygplates "1.1.1" "1.1.0rc1" "not been finished")
expect_target_rejected(pygplates "1.2.0" "1.1.0rc1" "not been finished")

# A candidate on another branch - what the development branch sees once the series branch cut
# from it has its first candidate - binds nothing about its own line. Its head counts as
# released for the no-skip rule, and staying on that head is refused, because the numbers are
# being minted over there and this line's count has restarted. The first cut of the candidate
# rule made no such distinction, and would have stopped every configure on the development
# branch until the release was final.
expect_target_ok_off_line(pygplates "1.1.1" "1.1.0rc1")
expect_target_ok_off_line(pygplates "1.2.0" "1.1.0rc1")
expect_target_ok_off_line(pygplates "2.0.0" "1.1.0rc1")
expect_target_ok_off_line(pygplates "1.2.0rc1" "1.1.0rc1")
expect_target_rejected_off_line(pygplates "1.1.0" "1.1.0rc1" "another branch")
expect_target_rejected_off_line(pygplates "1.1.0rc2" "1.1.0rc1" "another branch")
expect_target_rejected_off_line(pygplates "1.1.0rc1" "1.1.0rc1" "already been released")
expect_target_rejected_off_line(pygplates "1.0.0" "1.1.0rc1" "sorts below")
expect_target_rejected_off_line(pygplates "1.3.0" "1.1.0rc1" "skips past")
# A patch candidate on a maintenance series, seen from the development branch, which has long
# since moved on to the next minor.
expect_target_ok_off_line(pygplates "1.2.0" "1.1.1rc1")
expect_target_ok_off_line(gplates "2.7.0" "2.6.1-rc.1")
# A final release on another branch is treated exactly as one on this line.
expect_target_ok_off_line(pygplates "1.2.0" "1.1.0")
expect_target_ok_off_line(pygplates "1.1.1" "1.1.0")
expect_target_rejected_off_line(pygplates "1.1.0" "1.1.0" "already been released")
expect_target_rejected_off_line(pygplates "1.3.0" "1.1.0" "skips past")

expect_target_ok(gplates "2.6.0" "2.5.0")
expect_target_ok(gplates "2.5.1" "2.5.0")
expect_target_ok(gplates "3.0.0" "2.5.0")
expect_target_ok(gplates "2.6.0-rc.1" "2.5.0")
expect_target_rejected(gplates "2.5.0" "2.5.0" "already been released")
expect_target_rejected(gplates "2.4.0" "2.5.0" "sorts below")
expect_target_rejected(gplates "2.7.0" "2.5.0" "skips past")
expect_target_ok(gplates "2.6.0-rc.2" "2.6.0-rc.1")
expect_target_ok(gplates "2.6.0" "2.6.0-rc.1")
expect_target_rejected(gplates "2.6.1" "2.6.0-rc.1" "not been finished")

# The hole this check was added to close: verified by hand before it existed, a target of
# '0.9.0' with 'PyGPlates-1.0.0' long released resolved to '0.9.0.dev49' and exited 0.
expect_target_rejected(pygplates "0.9.0" "1.0.0" "sorts below")

#
# The release's target on a commit not tagged as it, after a candidate on this line: anywhere but
# its tag, '1.1.0' mints '1.1.0.devN', which sorts below '1.1.0rc1'. Directly on the candidate the
# commit may be the release awaiting its tag; further on it has changes the candidate lacked, and
# only the next candidate is suggested. The suggestion keeps each product's own spelling.
#
expect_untagged_rejected(pygplates "PyGPlates-" "1.1.0" "1.1.0rc1" 1 TRUE
		"not tagged 'PyGPlates-1.1.0'.*tag it 'PyGPlates-1.1.0'.*'1.1.0rc2'")
expect_untagged_rejected(pygplates "PyGPlates-" "1.1.0" "1.1.0rc1" 2 TRUE
		"not directly on the candidate.*'1.1.0rc2'")
expect_untagged_rejected(pygplates "PyGPlates-" "1.1.0.post1" "1.1.0rc1" 1 TRUE "not tagged")
expect_untagged_rejected(pygplates "PyGPlates-" "1.1.0" "1.1.0b2" 1 TRUE "'1.1.0b3'")
expect_untagged_rejected(gplates "GPlates-" "2.6.0" "2.6.0-rc.1" 1 TRUE
		"not tagged 'GPlates-2.6.0'.*'2.6.0-rc.2'")
expect_untagged_rejected(gplates "GPlates-" "2.6.0" "2.6.0-rc.9" 3 TRUE "'2.6.0-rc.10'")
# Another candidate is fine, and so is anything not following a candidate on this line.
expect_untagged_ok(pygplates "PyGPlates-" "1.1.0rc2" "1.1.0rc1" 1 TRUE)
expect_untagged_ok(pygplates "PyGPlates-" "1.1.1" "1.1.0" 1 TRUE)
expect_untagged_ok(pygplates "PyGPlates-" "1.2.0" "1.1.0rc1" 1 FALSE)
expect_untagged_ok(pygplates "PyGPlates-" "1.1.0" "1.0.0" 1 TRUE)

#
# An anchor tag's base must be the release target at its commit - the version that commit
# resolves to - so that the tag reads as a version rather than as a label. The message names
# the tag to use instead, which keeps the number and so changes nothing else.
#
set(_release_file "set(GPLATES_RELEASE_VERSION 2.6.0)\nset(PYGPLATES_RELEASE_VERSION 1.1.0)\n")
expect_anchor_ok(gplates "GPlates-" "2.6.0-2000" "${_release_file}")
expect_anchor_ok(pygplates "PyGPlates-" "1.1.0.dev59" "${_release_file}")
expect_anchor_rejected(gplates "GPlates-" "2.5.0-2000" "${_release_file}" "GPlates-2.6.0-2000")
expect_anchor_rejected(gplates "GPlates-" "2.7.0-2000" "${_release_file}" "GPlates-2.6.0-2000")
expect_anchor_rejected(pygplates "PyGPlates-" "1.0.0.dev59" "${_release_file}"
		"PyGPlates-1.1.0.dev59")
# Each product is checked against its own target only.
expect_anchor_ok(gplates "GPlates-" "2.6.0-2000" "set(PYGPLATES_RELEASE_VERSION 9.9.9)\n")
# The target may be quoted, and a candidate's development builds anchor like any other.
expect_anchor_ok(pygplates "PyGPlates-" "1.1.0rc1.dev8"
		"set(PYGPLATES_RELEASE_VERSION \"1.1.0rc1\")")
expect_anchor_ok(gplates "GPlates-" "2.6.0-rc.1.8" "set(GPLATES_RELEASE_VERSION 2.6.0-rc.1)")
expect_anchor_rejected(gplates "GPlates-" "2.6.0-8" "set(GPLATES_RELEASE_VERSION 2.6.0-rc.1)"
		"GPlates-2.6.0-rc.1.8")
# The target is read at a line start, whatever the case of 'set': a copy of the old line left in
# a comment above the new one must not be read instead.
expect_anchor_ok(pygplates "PyGPlates-" "1.2.0.dev5"
		"# set(PYGPLATES_RELEASE_VERSION 1.1.0)\nset(PYGPLATES_RELEASE_VERSION 1.2.0)\n")
expect_anchor_rejected(pygplates "PyGPlates-" "1.1.0.dev5"
		"# set(PYGPLATES_RELEASE_VERSION 1.1.0)\nset(PYGPLATES_RELEASE_VERSION 1.2.0)\n"
		"PyGPlates-1.2.0.dev5")
expect_anchor_rejected(pygplates "PyGPlates-" "1.2.0.dev5"
		"  SET( PYGPLATES_RELEASE_VERSION  1.1.0 )\n" "PyGPlates-1.1.0.dev5")
# A release tag is not an anchor, and a commit from before the release file set this product's
# target cannot be checked: both pass.
expect_anchor_ok(gplates "GPlates-" "2.5.0" "${_release_file}")
expect_anchor_ok(gplates "GPlates-" "2.5.0-2000" "set(PYGPLATES_RELEASE_VERSION 1.1.0)\n")
expect_anchor_ok(gplates "GPlates-" "2.5.0-2000" "")

#
# A frozen version - the other product's, on a release series branch - is used as it is. It must
# be a development version in full, since that is what the development branch resolves to, and
# the commit must not be tagged as a release of the frozen product; a development tag recording
# it is harmless, and a tag of the other product is not this check's business.
#
expect_frozen_ok(gplates "GPlates-" "2.6.0-150" "")
expect_frozen_ok(gplates "GPlates-" "2.7.0-rc.1.4" "")
expect_frozen_ok(pygplates "PyGPlates-" "1.2.0.dev30" "PyGPlates-1.2.0.dev30")
expect_frozen_ok(gplates "GPlates-" "2.6.0-150" "GPlates-2.6.0-150;GPlates-1.5+hellinger-testing")
expect_frozen_rejected(gplates "GPlates-" "2.6.0-150" "GPlates-2.6.0" "not released from")
expect_frozen_rejected(pygplates "PyGPlates-" "1.2.0.dev3" "PyGPlates-1.2.0rc1" "not released from")
expect_frozen_rejected(gplates "GPlates-" "2.6.0.dev150" "" "not a development version")
# A release is not a frozen development version, and a two-component version would be used as it
# is, unnormalised.
expect_frozen_rejected(gplates "GPlates-" "2.6.0" "" "not a development version")
expect_frozen_rejected(gplates "GPlates-" "2.6.0-rc.1" "" "not a development version")
expect_frozen_rejected(pygplates "PyGPlates-" "1.2.0" "" "not a development version")
expect_frozen_rejected(gplates "GPlates-" "2.6-150" "" "not a development version")

#
# A release series branch records the development branch commit it was cut from. The record is
# read at a line start, whatever the case of 'set', and may be quoted; a copy in a comment is not
# a record. (Whether a record is valid needs git, so the scenarios below check that.)
#
set(_cut "0123456789abcdef0123456789abcdef01234567")
expect_series_cut("set(RELEASE_SERIES_CUT_FROM ${_cut})\n" "${_cut}")
expect_series_cut("  SET( RELEASE_SERIES_CUT_FROM \"${_cut}\" )\n" "${_cut}")
expect_series_cut("set(GPLATES_FROZEN_VERSION 2.6.0-2)\nset(RELEASE_SERIES_CUT_FROM ${_cut})"
		"${_cut}")
expect_series_cut("#    set(RELEASE_SERIES_CUT_FROM ${_cut})\n" "")
# Of several, the last, as CMake itself would take it.
set(_two_records "set(RELEASE_SERIES_CUT_FROM 1111111111111111111111111111111111111111)\n")
string(APPEND _two_records "set(RELEASE_SERIES_CUT_FROM ${_cut})\n")
expect_series_cut("${_two_records}" "${_cut}")
expect_series_cut("set(PYGPLATES_RELEASE_VERSION 1.2.0)\n" "")
expect_series_cut("" "")
# A series commit freezes the product it does not release; a development branch commit freezes
# nothing, and neither does a commented-out freeze (the file's own header has one).
expect_freezes("set(GPLATES_FROZEN_VERSION 2.6.0-2)\nset(PYGPLATES_RELEASE_VERSION 1.1.0rc1)" TRUE)
expect_freezes("set(GPLATES_RELEASE_VERSION 2.6.0-rc.1)\nset(PYGPLATES_FROZEN_VERSION 1.2.0.dev2)"
		TRUE)
expect_freezes("set(GPLATES_RELEASE_VERSION 2.6.0)\nset(PYGPLATES_RELEASE_VERSION 1.2.0)" FALSE)
expect_freezes("#        set(GPLATES_FROZEN_VERSION 2.6.0-150)\nset(GPLATES_RELEASE_VERSION 2.6.0)"
		FALSE)
# What a recorded cut must look like: a development branch commit's file, with no freeze and no
# pre-release target. A series branch's first commit sets its first candidate, so recording it
# by mistake - with the freeze forgotten there, and added later - is caught by its target.
expect_not_development("set(GPLATES_RELEASE_VERSION 2.6.0)\nset(PYGPLATES_RELEASE_VERSION 1.2.0)"
		"")
expect_not_development("set(GPLATES_FROZEN_VERSION 2.6.0-2)\nset(PYGPLATES_RELEASE_VERSION 1.1.0)"
		"freezes a version")
expect_not_development("set(GPLATES_RELEASE_VERSION 2.6.0)\nset(PYGPLATES_RELEASE_VERSION 1.1.0rc1)"
		"pygplates target '1.1.0rc1' is a pre-release")
expect_not_development("set(GPLATES_RELEASE_VERSION 2.6.0-beta.2)\n"
		"gplates target '2.6.0-beta.2' is a pre-release")
set(_commented "# set(PYGPLATES_RELEASE_VERSION 1.1.0rc1)\n")
string(APPEND _commented "set(PYGPLATES_RELEASE_VERSION 1.1.0)\n")
expect_not_development("${_commented}" "")
# Of several 'set's of a target, the last, as CMake itself would take it.
set(_two_targets "set(PYGPLATES_RELEASE_VERSION 1.1.0rc1)\n")
string(APPEND _two_targets "set(PYGPLATES_RELEASE_VERSION 1.1.0)\n")
expect_not_development("${_two_targets}" "")
# A post-release is not a pre-release (nor would the development branch set one).
expect_not_development("set(PYGPLATES_RELEASE_VERSION 1.1.0.post1)" "")

#
# An off-line refusal also names the way out for a branch made from the development branch, or
# from a series branch, before its target moved on - except for a target that skips ahead, which
# is a different mistake.
#
expect_off_line_advice(pygplates "1.1.0" "1.1.0rc1" FALSE "merge the development branch in")
expect_off_line_advice(pygplates "1.1.0" "1.1.0" FALSE "PYGPLATES_PEP440_VERSION")
expect_off_line_advice(pygplates "1.0.0" "1.1.0rc1" FALSE "moved on past '1.1.0'")
expect_off_line_advice(gplates "2.6.0" "2.6.0-rc.1" FALSE "GPLATES_SEMANTIC_VERSION")
expect_off_line_advice(pygplates "1.1.0rc1" "1.1.0" TRUE
		"before 'Tag-1.1.0' was tagged on it, rebase onto the series branch")
expect_off_line_advice(pygplates "1.3.0" "1.1.0rc1" FALSE "")

#
# Scenarios on a synthetic repository: two release series branches, one per product, cut from the
# development branch one after the other. They are here because what they check is not a pure
# function: before versions were frozen, every commit on the older series branch - its own release
# tags included - stopped configuring the moment the other product was tagged on the newer one.
# That depends on the shape of the history, not on any one decision.
#
# Skipped without git (the resolver itself then falls back to the recorded version, below), and
# without a temporary directory: the source tree is no place for a scratch repository, which a
# run that fails part way would leave behind.
#
find_program(_scenario_git_executable NAMES git)
# The loop variable is restored when the loop ends, hence the copy.
set(_temporary_dir "")
foreach (_candidate IN ITEMS "$ENV{TMPDIR}" "$ENV{TEMP}" "$ENV{TMP}" "/tmp")
	if (NOT _candidate STREQUAL "" AND IS_DIRECTORY "${_candidate}")
		set(_temporary_dir "${_candidate}")
		break()
	endif()
endforeach()
if (NOT _scenario_git_executable)
	message(STATUS "git not found: skipping the scenarios on a synthetic repository.")
elseif (_temporary_dir STREQUAL "")
	message(STATUS "No temporary directory: skipping the scenarios on a synthetic repository.")
else()
	string(RANDOM LENGTH 8 _random)
	set(_scenario_dir "${_temporary_dir}/version-resolver-test-${_random}")
	file(MAKE_DIRECTORY "${_scenario_dir}")
	message(STATUS "Scenarios in ${_scenario_dir}")

	# Every git and resolver process below runs without the variables that point git at a
	# repository. Git sets them for a hook it runs (GIT_DIR in a linked worktree, GIT_INDEX_FILE for
	# pre-commit), so a test run from a hook would otherwise commit these files into the real one.
	# And without the version variables that short-cut the resolver (the Linux build container sets
	# them).
	set(_scenario_env "${CMAKE_COMMAND}" -E env
			--unset=GIT_DIR --unset=GIT_WORK_TREE --unset=GIT_INDEX_FILE --unset=GIT_COMMON_DIR
			--unset=GIT_OBJECT_DIRECTORY --unset=GIT_ALTERNATE_OBJECT_DIRECTORIES
			--unset=GIT_NAMESPACE --unset=GPLATES_SEMANTIC_VERSION --unset=PYGPLATES_PEP440_VERSION)

	# Runs git in the synthetic repository, independent of the user's configuration: no identity
	# needed, no signing, no hooks, no line-ending conversion.
	function(_scenario_git)
		execute_process(
				COMMAND ${_scenario_env} "${_scenario_git_executable}" -C "${_scenario_dir}"
					-c user.name=version-resolver-test -c user.email=test@invalid
					-c commit.gpgsign=false -c tag.gpgsign=false -c core.autocrlf=false
					-c core.hooksPath=no-hooks ${ARGN}
				RESULT_VARIABLE _result
				OUTPUT_VARIABLE _output
				ERROR_VARIABLE _error)
		if (NOT _result EQUAL 0)
			message(FATAL_ERROR
					"Setting up the scenario repository: 'git ${ARGN}' failed:\n${_error}")
		endif()
	endfunction()

	# The full hash of a commit in the synthetic repository.
	function(_scenario_rev out_var rev)
		execute_process(
				COMMAND ${_scenario_env} "${_scenario_git_executable}" -C "${_scenario_dir}"
					rev-parse --verify "${rev}^{commit}"
				RESULT_VARIABLE _result
				OUTPUT_VARIABLE _output
				OUTPUT_STRIP_TRAILING_WHITESPACE)
		if (NOT _result EQUAL 0)
			message(FATAL_ERROR "Setting up the scenario repository: no commit '${rev}'.")
		endif()
		set(${out_var} "${_output}" PARENT_SCOPE)
	endfunction()

	# A commit whose 'VersionRelease.cmake' holds the two given lines, and any more given after
	# them (a series cut record).
	set(_scenario_count 0)
	function(_scenario_commit message gplates_line pygplates_line)
		math(EXPR _n "${_scenario_count} + 1")
		set(_scenario_count ${_n} PARENT_SCOPE)
		string(JOIN "\n" _more ${ARGN})
		file(WRITE "${_scenario_dir}/cmake/modules/VersionRelease.cmake"
				"${gplates_line}\n${pygplates_line}\n${_more}\n")
		file(WRITE "${_scenario_dir}/file" "${_n}\n")
		_scenario_git(add -A)
		_scenario_git(commit -q -m "${message}")
	endfunction()

	# The version the resolver gives here, or 'FATAL:<phrase>' for a refusal mentioning the phrase.
	# An optional fourth argument is a phrase a warning must mention. Run as a separate process,
	# as the build workflows run it.
	function(_scenario_expect label product expected)
		execute_process(
				COMMAND ${_scenario_env}
					"${CMAKE_COMMAND}" -P "${_scenario_dir}/cmake/modules/VersionFromGit.cmake"
					${product}
				RESULT_VARIABLE _result
				OUTPUT_VARIABLE _output
				ERROR_VARIABLE _error
				OUTPUT_STRIP_TRAILING_WHITESPACE)
		# CMake wraps a message at spaces, so compare with the whitespace collapsed.
		string(REGEX REPLACE "[ \t\r\n]+" " " _error "${_error}")
		if (expected MATCHES "^FATAL:(.*)$")
			set(_phrase "${CMAKE_MATCH_1}")
			if (_result EQUAL 0)
				_fail("${label}: expected ${product} to be refused (\"${_phrase}\"), but it "
						"resolved to '${_output}'.")
			elseif (NOT _error MATCHES "${_phrase}")
				_fail("${label}: ${product} was refused, but not with \"${_phrase}\":\n  ${_error}")
			endif()
		elseif (NOT _result EQUAL 0)
			_fail("${label}: expected ${product} '${expected}', but it was refused:\n  ${_error}")
		elseif (NOT _output STREQUAL expected)
			_fail("${label}: expected ${product} '${expected}', got '${_output}'.")
		elseif (ARGC GREATER 3 AND NOT _error MATCHES "${ARGV3}")
			_fail("${label}: expected a warning mentioning \"${ARGV3}\", got:\n  ${_error}")
		endif()
		set(_failures ${_failures} PARENT_SCOPE)
	endfunction()

	_scenario_git(init -q)
	_scenario_git(symbolic-ref HEAD refs/heads/main)
	# From here on, every path is the one git reports, so that it and the resolver agree on where
	# the repository is (a temporary directory can be reached by more than one spelling).
	execute_process(
			COMMAND ${_scenario_env}
				"${_scenario_git_executable}" -C "${_scenario_dir}" rev-parse --show-toplevel
			OUTPUT_VARIABLE _scenario_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
	file(COPY "${CMAKE_CURRENT_LIST_DIR}/VersionFromGit.cmake"
			DESTINATION "${_scenario_dir}/cmake/modules")

	set(_g "set(GPLATES_RELEASE_VERSION")
	set(_p "set(PYGPLATES_RELEASE_VERSION")

	# The development branch, two commits past the last releases of both products.
	_scenario_commit("base" "${_g} 2.5.0)" "${_p} 1.0.0)")

	# No tags yet, as in a fork made on GitHub: refused, naming the tags, even with a stale
	# recorded version in the tree (every 'pip install .' writes one, and it is gitignored).
	_scenario_expect("no tags" gplates "FATAL:has no 'GPlates-")
	file(WRITE "${_scenario_dir}/cmake/modules/VersionRecorded.cmake"
			"set(GPLATES_SEMANTIC_VERSION 2.6.0-999)\nset(PYGPLATES_PEP440_VERSION 1.0.0.dev999)\n")
	_scenario_expect("no tags, a stale recorded version" gplates "FATAL:has no 'GPlates-")
	file(REMOVE "${_scenario_dir}/cmake/modules/VersionRecorded.cmake")

	_scenario_git(tag GPlates-2.5)
	_scenario_git(tag PyGPlates-1.0.0)
	_scenario_commit("m1" "${_g} 2.6.0)" "${_p} 1.1.0)")
	_scenario_rev(_m1 HEAD)
	_scenario_commit("m2" "${_g} 2.6.0)" "${_p} 1.1.0)")
	_scenario_expect("development branch" gplates "2.6.0-2")
	_scenario_expect("development branch" pygplates "1.1.0.dev2")

	# 'release/pygplates-1.1' cut, freezing GPlates at what the development branch gave and
	# recording the commit it was cut from, and its first commit tagged as the first candidate.
	# The development branch moves its target on.
	_scenario_rev(_m2 HEAD)
	set(_cut_pyg11 "set(RELEASE_SERIES_CUT_FROM ${_m2})")
	_scenario_git(switch -q -c release/pygplates-1.1)
	_scenario_commit("pyg cut" "set(GPLATES_FROZEN_VERSION 2.6.0-2)" "${_p} 1.1.0rc1)"
			"${_cut_pyg11}")
	_scenario_git(tag PyGPlates-1.1.0rc1)
	_scenario_expect("on PyGPlates-1.1.0rc1" gplates "2.6.0-2")
	_scenario_expect("on PyGPlates-1.1.0rc1" pygplates "1.1.0rc1")
	_scenario_git(switch -q main)
	_scenario_commit("bump 1.2" "${_g} 2.6.0)" "${_p} 1.2.0)")
	_scenario_commit("m3" "${_g} 2.6.0)" "${_p} 1.2.0)")
	_scenario_expect("development branch after the pyGPlates cut" gplates "2.6.0-4")
	_scenario_expect("development branch after the pyGPlates cut" pygplates "1.2.0.dev2")

	# 'release/gplates-2.6' cut later, freezing pyGPlates, and tagged as the first GPlates
	# candidate. This is the tag that used to stop every configure on 'release/pygplates-1.1'.
	_scenario_rev(_m3 HEAD)
	set(_cut_gpl26 "set(RELEASE_SERIES_CUT_FROM ${_m3})")
	_scenario_git(switch -q -c release/gplates-2.6)
	_scenario_commit("gpl cut" "${_g} 2.6.0-rc.1)" "set(PYGPLATES_FROZEN_VERSION 1.2.0.dev2)"
			"${_cut_gpl26}")
	_scenario_git(tag GPlates-2.6.0-rc.1)
	_scenario_expect("on GPlates-2.6.0-rc.1" gplates "2.6.0-rc.1")
	_scenario_expect("on GPlates-2.6.0-rc.1" pygplates "1.2.0.dev2")
	_scenario_git(switch -q main)
	_scenario_commit("bump 2.7" "${_g} 2.7.0)" "${_p} 1.2.0)")
	_scenario_commit("m4" "${_g} 2.7.0)" "${_p} 1.2.0)")
	_scenario_expect("development branch after both cuts" gplates "2.7.0-2")
	_scenario_expect("development branch after both cuts" pygplates "1.2.0.dev4")

	# Back on 'release/pygplates-1.1': the release. Its target is refused until the commit is
	# tagged, since '1.1.0.dev1' would sort below the candidate; then it is exactly '1.1.0'. The
	# GPlates candidate on the other series branch does not touch the frozen GPlates version.
	_scenario_git(switch -q release/pygplates-1.1)
	_scenario_commit("pyg release" "set(GPLATES_FROZEN_VERSION 2.6.0-2)" "${_p} 1.1.0)"
			"${_cut_pyg11}")
	_scenario_expect("release commit, untagged" pygplates
			"FATAL:not tagged 'PyGPlates-1.1.0'.*tag it 'PyGPlates-1.1.0'")
	_scenario_expect("release commit, untagged" gplates "2.6.0-2")
	_scenario_git(tag PyGPlates-1.1.0)
	_scenario_expect("on PyGPlates-1.1.0" pygplates "1.1.0")
	_scenario_expect("on PyGPlates-1.1.0" gplates "2.6.0-2")
	_scenario_commit("pyg bump 1.1.1" "set(GPLATES_FROZEN_VERSION 2.6.0-2)" "${_p} 1.1.1)"
			"${_cut_pyg11}")
	_scenario_commit("pyg fix" "set(GPLATES_FROZEN_VERSION 2.6.0-2)" "${_p} 1.1.1)"
			"${_cut_pyg11}")
	_scenario_expect("pyGPlates series, patch line" pygplates "1.1.1.dev2")
	_scenario_expect("pyGPlates series, patch line" gplates "2.6.0-2")

	# GPlates is not released from a pyGPlates series branch.
	_scenario_git(tag GPlates-2.6.5)
	_scenario_expect("GPlates release tag on a pyGPlates series" gplates "FATAL:not released from")
	_scenario_git(tag -d GPlates-2.6.5)

	# The mirror image: a later pyGPlates series leaves the GPlates candidate's tag buildable.
	_scenario_git(switch -q main)
	_scenario_rev(_m4 HEAD)
	_scenario_git(switch -q -c release/pygplates-1.2)
	_scenario_commit("pyg 1.2 cut" "set(GPLATES_FROZEN_VERSION 2.7.0-2)" "${_p} 1.2.0rc1)"
			"set(RELEASE_SERIES_CUT_FROM ${_m4})")
	_scenario_git(tag PyGPlates-1.2.0rc1)
	_scenario_git(switch -q main)
	_scenario_commit("bump 1.3" "${_g} 2.7.0)" "${_p} 1.3.0)")
	_scenario_expect("development branch after the second pyGPlates cut" pygplates "1.3.0.dev1")
	# A series branch given the development branch's target by mistake still skips a version:
	# that a later series is skipped from here does not make its successor this line's.
	_scenario_git(switch -q -c release/pygplates-1.1-mistake release/pygplates-1.1)
	_scenario_commit("development target taken" "set(GPLATES_FROZEN_VERSION 2.6.0-2)"
			"${_p} 1.3.0)" "${_cut_pyg11}")
	_scenario_expect("series branch given the development branch's target" pygplates
			"FATAL:skips past '1.1.0'")
	_scenario_git(switch -q main)
	_scenario_git(switch -q --detach GPlates-2.6.0-rc.1)
	_scenario_expect("on GPlates-2.6.0-rc.1, later" gplates "2.6.0-rc.1")
	_scenario_expect("on GPlates-2.6.0-rc.1, later" pygplates "1.2.0.dev2")
	_scenario_git(switch -q --detach PyGPlates-1.1.0)
	_scenario_expect("on PyGPlates-1.1.0, later" pygplates "1.1.0")
	_scenario_expect("on PyGPlates-1.1.0, later" gplates "2.6.0-2")

	# A release commit with a fix on top, the fix then tagged as the release (as the publishing
	# run's override allows). Before the tag, the fix is refused, with only the next candidate
	# offered. After it, the release commit below it is history: warned about, not refused, so
	# that it stays buildable.
	_scenario_git(switch -q release/gplates-2.6)
	_scenario_commit("gpl release" "${_g} 2.6.0)" "set(PYGPLATES_FROZEN_VERSION 1.2.0.dev2)"
			"${_cut_gpl26}")
	_scenario_commit("gpl late fix" "${_g} 2.6.0)" "set(PYGPLATES_FROZEN_VERSION 1.2.0.dev2)"
			"${_cut_gpl26}")
	_scenario_expect("late fix on the release commit, untagged" gplates
			"FATAL:not directly on the candidate.*'2.6.0-rc.2'")
	_scenario_git(tag GPlates-2.6.0)
	_scenario_expect("late fix, tagged as the release" gplates "2.6.0")
	_scenario_git(switch -q --detach HEAD~1)
	_scenario_expect("release commit below a later release tag" gplates "2.6.0-1"
			"already in the history of 'GPlates-2.6.0'")

	# A release tagged on a branch that does not freeze the other product: refused at the tag,
	# since the first release of the other product on a series branch cut later would stop it
	# configuring anyway, by which time the tag could not be repaired.
	_scenario_git(switch -q -c release/gplates-2.7 main)
	_scenario_commit("gpl 2.7 cut, unfrozen" "${_g} 2.7.0-rc.1)" "${_p} 1.3.0)")
	_scenario_git(tag GPlates-2.7.0-rc.1)
	_scenario_expect("GPlates release tag, pyGPlates not frozen" gplates
			"FATAL:does not freeze the pygplates version")
	_scenario_git(tag -d GPlates-2.7.0-rc.1)

	# A candidate tagged on the development branch by mistake. Refused at the tag (it freezes
	# nothing), and the commit after it is bound to that release as a series branch would be, since
	# git cannot tell the lines apart - so that refusal says the tag may be the mistake.
	_scenario_git(switch -q main)
	_scenario_commit("pyg candidate on main" "${_g} 2.7.0)" "${_p} 1.3.0rc1)")
	_scenario_git(tag PyGPlates-1.3.0rc1)
	_scenario_expect("pyGPlates candidate on the development branch" pygplates
			"FATAL:does not freeze the gplates version")
	_scenario_commit("bump 1.4" "${_g} 2.7.0)" "${_p} 1.4.0)")
	_scenario_expect("after a candidate on the development branch" pygplates
			"FATAL:not been finished.*put on it by mistake")
	_scenario_git(tag -d PyGPlates-1.3.0rc1)

	# Two anchors on one commit: the larger number wins, on the commit and after it, so the count
	# does not go backwards between the two.
	_scenario_git(tag GPlates-2.7.0-100)
	_scenario_git(tag GPlates-2.7.0-2000)
	_scenario_expect("two anchors on HEAD" gplates "2.7.0-2000")
	_scenario_commit("after two anchors" "${_g} 2.7.0)" "${_p} 1.3.0)")
	_scenario_expect("one commit after two anchors" gplates "2.7.0-2001")

	# A branch made from the development branch before either series branch was cut, its targets
	# still '2.6.0' and '1.1.0'. Every release of those series is skipped, since the cut each
	# records is not in its history, so it counts from the releases it had when it was made.
	# Without the records it was refused: the newest of the series' tags, all tied at its fork
	# point, became its reference.
	_scenario_git(switch -q -c feature/old "${_m1}")
	_scenario_commit("old feature" "${_g} 2.6.0)" "${_p} 1.1.0)")
	_scenario_expect("branch made before both cuts" pygplates "1.1.0.dev2")
	_scenario_expect("branch made before both cuts" gplates "2.6.0-2")
	# The same branch given the development branch's later target without its history (a squash
	# merge, say): '1.2.0' is the release after the skipped 1.1 series, so it skips nothing, but
	# '1.4.0' still skips 1.3 after the skipped '1.2.0rc1'.
	_scenario_git(switch -q -c feature/old-squashed)
	_scenario_commit("squash of the development branch" "${_g} 2.7.0)" "${_p} 1.2.0)")
	_scenario_expect("branch made before the cuts, given a later target" pygplates "1.2.0.dev3")
	_scenario_commit("target too far" "${_g} 2.7.0)" "${_p} 1.4.0)")
	_scenario_expect("branch made before the cuts, given a skipping target" pygplates
			"FATAL:skips past.*the release after one of them would also be accepted")
	# A candidate on the branch's own line binds it as on any line ("not been finished"), even
	# with a later series skipped whose successor the target would be.
	_scenario_git(switch -q -c feature/old-candidate feature/old)
	_scenario_commit("candidate on the branch" "${_g} 2.6.0)" "${_p} 1.1.0rc5)")
	_scenario_git(tag PyGPlates-1.1.0rc5)
	_scenario_commit("past the candidate" "${_g} 2.6.0)" "${_p} 1.2.0)")
	_scenario_expect("branch made before the cuts, its own candidate unfinished" pygplates
			"FATAL:not been finished")
	_scenario_git(tag -d PyGPlates-1.1.0rc5)
	_scenario_git(switch -q feature/old)

	# A branch made at the very commit 'release/pygplates-1.1' was cut from has that cut in its
	# history, so as far as anything can tell it is the development branch with its target left
	# behind: refused, naming the way out. Its GPlates version is unaffected, since
	# 'release/gplates-2.6' was cut later.
	_scenario_git(switch -q -c feature/at-cut "${_m2}")
	_scenario_commit("feature at the cut" "${_g} 2.6.0)" "${_p} 1.1.0)")
	_scenario_expect("branch made at the pyGPlates cut" pygplates
			"FATAL:already been released.*merge the development branch in")
	_scenario_expect("branch made at the pyGPlates cut" gplates "2.6.0-3")
	# Built as it is, with the version set explicitly as a define, which may come before '-P'
	# when the resolver is run as a script.
	execute_process(
			COMMAND ${_scenario_env} "${CMAKE_COMMAND}" -DPYGPLATES_PEP440_VERSION=1.1.0.dev7
				-P "${_scenario_dir}/cmake/modules/VersionFromGit.cmake" pygplates
			RESULT_VARIABLE _result
			OUTPUT_VARIABLE _output
			ERROR_VARIABLE _error
			OUTPUT_STRIP_TRAILING_WHITESPACE)
	if (NOT _result EQUAL 0 OR NOT _output STREQUAL "1.1.0.dev7")
		_fail("a define before '-P': expected '1.1.0.dev7', got '${_output}':
  ${_error}")
	endif()

	# A patch branch made from 'release/pygplates-1.1' at its first candidate, then overtaken by
	# the release: refused, naming the series branch to rebase onto.
	_scenario_git(switch -q -c fix/pyg-patch PyGPlates-1.1.0rc1)
	_scenario_commit("pyg patch" "set(GPLATES_FROZEN_VERSION 2.6.0-2)" "${_p} 1.1.0rc2)"
			"${_cut_pyg11}")
	_scenario_expect("patch branch overtaken by its series' release" pygplates
			"FATAL:sorts below '1.1.0'.*rebase onto the series branch")

	# A release tag whose commit has no valid record is refused at the tag, where it can still be
	# repaired. Each attempt is a new commit on one series branch, tagged and then untagged.
	_scenario_git(switch -q main)
	_scenario_rev(_m5 HEAD)
	_scenario_git(switch -q -c release/pygplates-1.3)
	set(_pyg13 "set(GPLATES_FROZEN_VERSION 2.7.0-2001)" "${_p} 1.3.0rc1)")
	_scenario_commit("pyg 1.3 cut, no record" ${_pyg13})
	_scenario_git(tag PyGPlates-1.3.0rc1)
	_scenario_expect("release tag without a cut record" pygplates
			"FATAL:does not record the commit it was cut from")
	# Seen from elsewhere, a release without a record is treated as before records existed: the
	# branch made before every cut is refused again, against that release.
	_scenario_git(switch -q feature/old)
	_scenario_expect("branch made before a cut that records nothing" pygplates
			"FATAL:sorts below '1.3.0rc1'")
	_scenario_git(switch -q release/pygplates-1.3)
	_scenario_git(tag -d PyGPlates-1.3.0rc1)
	# The likeliest slips: a series commit (here the one before), and an abbreviated hash.
	_scenario_rev(_series_commit HEAD)
	_scenario_commit("pyg 1.3, a series commit recorded" ${_pyg13}
			"set(RELEASE_SERIES_CUT_FROM ${_series_commit})")
	_scenario_git(tag PyGPlates-1.3.0rc1)
	_scenario_expect("cut record naming a series commit" pygplates
			"FATAL:looks like a commit on a release series branch")
	_scenario_git(tag -d PyGPlates-1.3.0rc1)
	string(SUBSTRING "${_m5}" 0 12 _short)
	_scenario_commit("pyg 1.3, abbreviated record" ${_pyg13}
			"set(RELEASE_SERIES_CUT_FROM ${_short})")
	_scenario_git(tag PyGPlates-1.3.0rc1)
	_scenario_expect("cut record abbreviated" pygplates "FATAL:not a full")
	_scenario_git(tag -d PyGPlates-1.3.0rc1)
	# A commit off this branch's history, and one not in the repository at all.
	_scenario_rev(_old feature/old)
	_scenario_commit("pyg 1.3, record off the history" ${_pyg13}
			"set(RELEASE_SERIES_CUT_FROM ${_old})")
	_scenario_git(tag PyGPlates-1.3.0rc1)
	_scenario_expect("cut record not in the history" pygplates "FATAL:not in the history")
	_scenario_git(tag -d PyGPlates-1.3.0rc1)
	_scenario_commit("pyg 1.3, record not a commit" ${_pyg13}
			"set(RELEASE_SERIES_CUT_FROM 0123456789abcdef0123456789abcdef01234567)")
	_scenario_git(tag PyGPlates-1.3.0rc1)
	_scenario_expect("cut record not a commit" pygplates "FATAL:not a commit in this repository")
	_scenario_git(tag -d PyGPlates-1.3.0rc1)
	# The freeze forgotten in a commit that sets the first candidate, then added in the next
	# with a record naming the one before: that commit freezes nothing, but its candidate target
	# shows it is a series commit. Recorded, it would have switched off the development branch's
	# check that it moves its target on.
	_scenario_commit("pyg 1.3, freeze forgotten" "${_g} 2.7.0)" "${_p} 1.3.0rc1)")
	_scenario_rev(_unfrozen HEAD)
	_scenario_commit("pyg 1.3, frozen late" ${_pyg13} "set(RELEASE_SERIES_CUT_FROM ${_unfrozen})")
	_scenario_git(tag PyGPlates-1.3.0rc1)
	_scenario_expect("cut record naming an unfrozen series commit" pygplates
			"FATAL:target '1.3.0rc1' is a pre-release")
	_scenario_git(tag -d PyGPlates-1.3.0rc1)
	# Recorded correctly: the tag resolves, the branch made before every cut skips it again, and
	# the development branch, which has the cut in its history, is still held to moving its
	# target on from its next commit. (At the cut itself the tag is in HEAD's future, which is
	# skipped as it always was.)
	_scenario_commit("pyg 1.3, recorded" ${_pyg13} "set(RELEASE_SERIES_CUT_FROM ${_m5})")
	# Annotated, as real release tags are.
	_scenario_git(tag -a -m "pyGPlates 1.3.0rc1" PyGPlates-1.3.0rc1)
	_scenario_expect("on PyGPlates-1.3.0rc1" pygplates "1.3.0rc1")
	_scenario_git(switch -q feature/old)
	_scenario_expect("branch made before a cut that records it" pygplates "1.1.0.dev2")
	# Without the one release it can count from, everything left is skipped: refused, rather
	# than passed on to a stale recorded version.
	_scenario_rev(_base PyGPlates-1.0.0)
	_scenario_git(tag -d PyGPlates-1.0.0)
	file(WRITE "${_scenario_dir}/cmake/modules/VersionRecorded.cmake"
			"set(GPLATES_SEMANTIC_VERSION 2.6.0-999)\nset(PYGPLATES_PEP440_VERSION 1.0.0.dev999)\n")
	_scenario_expect("branch whose only countable tags are skipped" pygplates
			"FATAL:the only 'PyGPlates-")
	file(REMOVE "${_scenario_dir}/cmake/modules/VersionRecorded.cmake")
	_scenario_git(tag PyGPlates-1.0.0 "${_base}")
	_scenario_git(switch -q main)
	_scenario_commit("m6, target not moved on" "${_g} 2.7.0)" "${_p} 1.3.0)")
	_scenario_expect("development branch, target not moved on past a recorded cut" pygplates
			"FATAL:being released on another branch")

	# A fork made from the development branch before the cuts, with its own anchor (the design
	# document's section 9): it counts on from the anchor, and its target is checked against the
	# releases it had when it was made, not against 'GPlates-2.6.0' at its fork point.
	_scenario_git(switch -q feature/old)
	_scenario_git(tag GPlates-2.6.0-2000)
	_scenario_commit("fork commit" "${_g} 2.6.0)" "${_p} 1.1.0)")
	_scenario_expect("fork made before the cuts, past its anchor" gplates "2.6.0-2001")

	file(REMOVE_RECURSE "${_scenario_dir}")
endif()

#
# The whole resolver, once, against the repository this file is in: the targets in
# 'VersionRelease.cmake' checked against the real tags, and the git path exercised end to end.
# A failure aborts with the resolver's own message. (An earlier version of this test wrote the
# expected nearest releases down as literals, which would have gone red at the first release
# after it was written; the resolver finds them itself.) In a tree with no usable repository
# the resolver falls back to the recorded version, so this still passes.
#
gplates_resolve_version(gplates _resolved)
message(STATUS "gplates resolves to ${_resolved}")
gplates_resolve_version(pygplates _resolved)
message(STATUS "pygplates resolves to ${_resolved}")


if (_failures GREATER 0)
	message(FATAL_ERROR "${_failures} version-resolver test(s) failed.")
endif()
message(STATUS "All version-resolver tests passed.")
