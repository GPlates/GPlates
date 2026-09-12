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
# This covers only the parts of 'VersionFromGit.cmake' that are pure functions of their
# arguments - splitting and joining a version, ordering two of them, and checking the
# hand-edited release target against the nearest release. The parts that consult git are not
# covered case by case: they would need a synthetic repository per case, and what they compute
# is a commit count rather than a decision. The whole resolver is run once, at the end, against
# the repository this file is in.
#
# That split is deliberate rather than a shortcut. The decisions this file tests are the ones
# that abort a configure, and each is a rule somebody has to be able to change with confidence
# - particularly the no-skip policy, which is a convention rather than a correctness
# requirement, and is meant to be loosenable.
#

cmake_minimum_required(VERSION 3.22)

include("${CMAKE_CURRENT_LIST_DIR}/VersionFromGit.cmake")

set(_failures 0)

function(_fail message)
	math(EXPR _n "${_failures} + 1")
	set(_failures ${_n} PARENT_SCOPE)
	message(SEND_ERROR "${message}")
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
# not fire during a real release.
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
