#############################
# GPlates/pyGPlates Version #
#############################

#
# Whether to build (and install, package, etc) GPlates or pyGPlates.
#
# You can build *either* GPlates or pyGPlates. If you need to build both then create two separate out-of-place CMake builds
# (out-of-place means the binary artifacts are created in a directory separate from the source code).
# Note: We used to allow building *both*. But it turned out that CPack's Debian packages don't support different versions for different components
#       (eg, we used to have a 'gplates' component and a 'pygplates' component using the COMPONENT variable of the 'install()' command).
#       Update: We no longer support building Debian packages for pyGPlates (instead using conda and pip).
#               However we still only support *either* GPlates or pyGPlates (even though we could potentially go back to supporting both).
#
# NOTE: THIS BRANCH BUILDS BOTH GPLATES AND PYGPLATES (ONE PRODUCT PER CMAKE BUILD).
#       'GPLATES_BUILD_GPLATES' DEFAULTS TO 'TRUE' (BUILDS 'gplates'). SET IT TO 'FALSE' TO BUILD 'pygplates'.
#
option(GPLATES_BUILD_GPLATES "True to build GPlates (false to build pyGPlates)." true)


#
# The GPlates version.
#
# This is a *restricted* form of Semantic Versioning.
# For the *unrestricted* form see https://semver.org/spec/v2.0.0.html.
#
# NOTE: The restrictions are:
#       The first part of the version should be three dot-separated numbers (MAJOR.MINOR.PATCH).
#       The optional pre-release suffix of the version (the part after the '-') should be one of:
#       - a number for development pre-releases (eg, 1, 2, etc),
#       - 'alpha' followed by '.' followed by a number for alpha pre-releases (eg, alpha.1, alpha.2, etc),
#       - 'beta' followed by '.' followed by a number for beta pre-releases (eg, beta.1, beta.2, etc),
#       - 'rc' followed by '.' followed by a number for pre-release candidates (eg, rc.1, rc.2, etc).
#
# For example (in order of precedence):
#
#   2.5.0-alpha.1
#   2.5.0-beta.1
#   2.5.0-rc.1
#   2.5.0
#   2.5.1
#   2.6.0-1
#   2.6.0-2
#   2.6.0-rc1
#   2.6.0-rc2
#   2.6.0
#   2.6.1
#
set(GPLATES_SEMANTIC_VERSION 2.6.0-8)


#
# The pyGPlates version.
#
# This is a *restricted* form of Python PEP 440 versioning.
# For the *unrestricted* form see https://peps.python.org/pep-0440/.
#
# NOTE: The restrictions are:
#       The first part of the version should be three dot-separated numbers MAJOR.MINOR.PATCH (ie, no epoch "N!" segment).
#       It must be a *public* version identifier (ie, no "+<local version label>" appended).
#
# For example (in order of precedence):
#
#   0.44.0a1
#   0.44.0b1
#   0.44.0rc1.dev1
#   0.44.0rc1
#   0.44.0rc1.post1.dev1
#   0.44.0rc1.post1
#   0.44.0
#   0.44.0.post1
#   0.45.0.dev1
#   0.45.0rc1
#   0.45.0
#   1.0.0rc1
#   1.0.0rc2
#   1.0.0
#   1.0.1
#
set(PYGPLATES_PEP440_VERSION 1.1.0.dev10)


##################
# Implementation #
##################


###########
# GPlates #
###########

#
# Various GPlates version variables (obtained from GPLATES_SEMANTIC_VERSION).
#
# GPLATES_VERSION_MAJOR                     - Major version number.
# GPLATES_VERSION_MINOR                     - Minor version number.
# GPLATES_VERSION_PATCH                     - Patch version number.
#
# GPLATES_VERSION                           - Major.Minor.Patch version (without optional pre-release suffix).
#
# GPLATES_VERSION_PRERELEASE_SUFFIX         - Optional pre-release suffix (in Semantic Versioning format).
# GPLATES_VERSION_PRERELEASE_SUFFIX_USER    - Human-readable pre-release suffix that inserts 'dev' for development pre-releases.
#
# GPLATES_VERSION_PRERELEASE                - Version dictated by Semantic Versioning.
#                                             Used when need correct version precendence (eg, '1' < 'alpha.1').
# GPLATES_VERSION_PRERELEASE_USER           - Human-readable version that inserts 'dev' for development pre-releases.
#                                             Useful for any string the user might see.
#                                             Does not maintain correct version precedence (eg, 'dev1' > 'alpha.1' whereas '1' < 'alpha.1').
#

#
# A note about the GPlates pre-release version suffix (GPLATES_VERSION_PRERELEASE_SUFFIX)...
#
# It is:
# - empty if not a pre-release,
# - a number for a development pre-release (eg, 1, 2, etc),
# - 'alpha' followed by '.' followed by a number for an alpha pre-release (eg, alpha.1, alpha.2, etc),
# - 'beta' followed by '.' followed by a number for a beta pre-release (eg, beta.1, beta.2, etc),
# - 'rc' followed by '.' followed by a number for a pre-release candidate (eg, rc.1, rc.2, etc).
#
# The reason for the above rules is they support the correct version ordering precedence for both Semantic Versioning and Debian versioning
# (even though Semantic and Debian versioning have slightly different precedence rules).
#
# Semantic version precedence separates identifiers between dots and compares each identifier.
# According to https://semver.org/spec/v2.0.0.html ...
# - digit-only identifiers are compared numerically,
# - identifiers with letters are compared lexically in ASCII order,
# - numeric identifiers have lower precedence than non-numeric identifiers.
# ...and so '1' < 'beta.1' because '1' < 'beta', and 'beta.1' < 'beta.2' because 'beta' == 'beta' but '1' < '2'.
#
# Debian version precedence separates identifiers into alternating non-digit and digit identifiers.
# According to https://www.debian.org/doc/debian-policy/ch-controlfields.html#version ...
# - find initial part consisting only of non-digits and compare lexically in ASCII order (modified so letters sort earlier than non-letters, and '~' earliest of all),
# - find next part consisting only of digits and compare numerically,
# - repeat the above two steps until a difference is found.
# ...and so '1' < 'beta.1' because '' < 'beta.', and 'beta.1' < 'beta.2' because 'beta.' == 'beta.' but '1' < '2'.
#
# For example, for a major.minor.patch verson of 2.3.0:
# For Semantic Versioning: 2.3.0-1 < 2.3.0-alpha.1 < 2.3.0-beta.1 < 2.3.0-rc.1 < 2.3.0.
# For Debian versioning:   2.3.0~1 < 2.3.0~alpha.1 < 2.3.0~beta.1 < 2.3.0~rc.1 < 2.3.0.
#

# Extract version information from GPLATES_SEMANTIC_VERSION.
if (NOT GPLATES_SEMANTIC_VERSION MATCHES [[^([0-9]+)\.([0-9]+)\.([0-9]+)([-]([0-9]+|alpha\.[0-9]+|beta\.[0-9]+|rc\.[0-9]+))?$]])
	message(FATAL_ERROR "${GPLATES_SEMANTIC_VERSION} should be X.Y.Z or a pre-release X.Y.Z-N, X.Y.Z-alpha.N, X.Y.Z-beta.N or X.Y.Z-rc.N")
endif()
set(GPLATES_VERSION_MAJOR ${CMAKE_MATCH_1})
set(GPLATES_VERSION_MINOR ${CMAKE_MATCH_2})
set(GPLATES_VERSION_PATCH ${CMAKE_MATCH_3})
# The GPlates version without the pre-release suffix
# (matches the version generated by 'project()' which does not support pre-release suffixes).
set(GPLATES_VERSION ${GPLATES_VERSION_MAJOR}.${GPLATES_VERSION_MINOR}.${GPLATES_VERSION_PATCH})
# If a pre-release suffix was specified.
if (CMAKE_MATCH_COUNT EQUAL 5)
	set(GPLATES_VERSION_PRERELEASE_SUFFIX ${CMAKE_MATCH_5})
	set(GPLATES_VERSION_PRERELEASE ${GPLATES_VERSION}-${GPLATES_VERSION_PRERELEASE_SUFFIX})
	# A human-readable pre-release version (unset/empty if not a pre-release).
	#
	# If a development release (ie, if pre-release version is just a number) then insert 'dev' into the version *name* to make it more obvious to users.
	# Note: We don't insert 'dev' into the version itself because that would give it a higher version ordering precedence than 'alpha' and 'beta' (since a < b < d).
	#       Keeping only the development number in the actual version works because digits have lower precedence than non-digits (according to Semantic and Debian versioning).
	if (GPLATES_VERSION_PRERELEASE_SUFFIX MATCHES [[^[0-9]+$]])
		set(GPLATES_VERSION_PRERELEASE_SUFFIX_USER dev${GPLATES_VERSION_PRERELEASE_SUFFIX})
	else()
		set(GPLATES_VERSION_PRERELEASE_SUFFIX_USER ${GPLATES_VERSION_PRERELEASE_SUFFIX})
	endif()
	set(GPLATES_VERSION_PRERELEASE_USER ${GPLATES_VERSION}-${GPLATES_VERSION_PRERELEASE_SUFFIX_USER})
else()
	set(GPLATES_VERSION_PRERELEASE_SUFFIX "")
	set(GPLATES_VERSION_PRERELEASE_SUFFIX_USER "")
	set(GPLATES_VERSION_PRERELEASE ${GPLATES_VERSION})
	set(GPLATES_VERSION_PRERELEASE_USER ${GPLATES_VERSION})
endif()


#############
# PyGPlates #
#############
#
# The pyGPlates version should typically be updated when the API changes (eg, a new function or class)
# so users can then test for new functionality (even for internal releases).
#

#
# Various pyGPlates version variables (obtained from PYGPLATES_PEP440_VERSION).
#
# PYGPLATES_VERSION_MAJOR                     - Major version number.
# PYGPLATES_VERSION_MINOR                     - Minor version number.
# PYGPLATES_VERSION_PATCH                     - Patch version number.
#
# PYGPLATES_VERSION                           - Major.Minor.Patch version (WITHOUT optional release suffix).
#
# PYGPLATES_VERSION_PRE_RELEASE_SUFFIX        - Optional pre-release suffix "{a|b|rc}N".
# PYGPLATES_VERSION_POST_RELEASE_SUFFIX       - Optional post-release suffix ".postN".
# PYGPLATES_VERSION_DEV_RELEASE_SUFFIX        - Optional development-release suffix ".devN".
#
# PYGPLATES_VERSION_RELEASE_SUFFIX            - Optional release suffix (combination of pre/post/dev release suffixes "[{a|b|rc}N][.postN][.devN]").
#
# PYGPLATES_VERSION_RELEASE                   - The full Python PEP 440 version.
#

# Extract version information from PYGPLATES_PEP440_VERSION.
#
# NOTE: The pre/post/dev *number* subexpressions use a plain "[0-9]+" rather than a capturing
#       "(0|[1-9][0-9]*)". This keeps the total number of capture groups down to 7, because CMake's
#       regex engine (cmsys) supports at most 9 capture groups (CMAKE_MATCH_1 to CMAKE_MATCH_9), and
#       older CMake versions (eg, 3.22) fail to even *compile* a regex with more than 9 capture groups
#       ("Too many parentheses"). Newer CMake (eg, 4.4) raised that limit, which is why it accepts the
#       previous 10-capture-group form.
#
#       Unlike "(0|[1-9][0-9]*)", the plainer "[0-9]+" also accepts leading zeros (eg, ".dev0010"), but
#       that is harmless: PEP 440 integer normalization treats numeric segments as integers and drops any
#       leading zeros (eg, ".dev0010" normalizes to ".dev10"), so such a version is still valid PEP 440.
#       (Major/Minor/Patch keep the stricter "(0|[1-9][0-9]*)" form; they must be capture groups anyway.)
if (NOT PYGPLATES_PEP440_VERSION MATCHES [[^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)((a|b|rc)[0-9]+)?(\.post[0-9]+)?(\.dev[0-9]+)?$]])
	message(FATAL_ERROR "${PYGPLATES_PEP440_VERSION} should match X.Y.Z[{a|b|rc}N][.postN][.devN]")
endif()

# Copy capture groups in a list.
#
# NOTE: Our regular expression has 7 capture groups (excluding CMAKE_MATCH_0 which captures the entire match):
#       1, 2, 3 are Major, Minor, Patch; 4 is the pre-release suffix "{a|b|rc}N" (with a nested group 5 for "{a|b|rc}");
#       6 is the post-release suffix ".postN"; 7 is the dev-release suffix ".devN".
#       For example, "1.0.0rc1.post1.dev1" captures 1, 0, 0, rc1, rc, .post1, .dev1.
#       Major/Minor/Patch are read by index below; the pre/post/dev segments are then classified in the loop further down.
set(_VERSION_SEGMENT_LIST)
set(_VERSION_SEGMENT_INDEX 1)
while (_VERSION_SEGMENT_INDEX LESS_EQUAL ${CMAKE_MATCH_COUNT})
	list(APPEND _VERSION_SEGMENT_LIST ${CMAKE_MATCH_${_VERSION_SEGMENT_INDEX}})
	math(EXPR _VERSION_SEGMENT_INDEX "${_VERSION_SEGMENT_INDEX} + 1")
endwhile()
list(LENGTH _VERSION_SEGMENT_LIST _NUM_VERSION_SEGMENTS)

# Get Major, Minor, Patch version numbers.
list(GET _VERSION_SEGMENT_LIST 0 PYGPLATES_VERSION_MAJOR)
list(GET _VERSION_SEGMENT_LIST 1 PYGPLATES_VERSION_MINOR)
list(GET _VERSION_SEGMENT_LIST 2 PYGPLATES_VERSION_PATCH)

# Default values for pre/post/dev version segments.
set(PYGPLATES_VERSION_PRE_RELEASE_SUFFIX "")
set(PYGPLATES_VERSION_POST_RELEASE_SUFFIX "")
set(PYGPLATES_VERSION_DEV_RELEASE_SUFFIX "")

# Iterate over the pre/post/dev version segments (if any).
set(_VERSION_SEGMENT_INDEX 3)
while (_VERSION_SEGMENT_INDEX LESS ${_NUM_VERSION_SEGMENTS})
	list(GET _VERSION_SEGMENT_LIST ${_VERSION_SEGMENT_INDEX} _VERSION_SEGMENT)
	
	if (_VERSION_SEGMENT MATCHES [[^(a|b|rc).+$]])  # note the '+' to ensure correct capture group (eg, 'rc1' instead of just 'rc')
		set(PYGPLATES_VERSION_PRE_RELEASE_SUFFIX ${_VERSION_SEGMENT})
	elseif(_VERSION_SEGMENT MATCHES [[^\.post.+$]])
		set(PYGPLATES_VERSION_POST_RELEASE_SUFFIX ${_VERSION_SEGMENT})
	elseif(_VERSION_SEGMENT MATCHES [[^\.dev.+$]])
		set(PYGPLATES_VERSION_DEV_RELEASE_SUFFIX ${_VERSION_SEGMENT})
	# ...else it's one of the *nested* capture groups that we ignore.
	endif()

	math(EXPR _VERSION_SEGMENT_INDEX "${_VERSION_SEGMENT_INDEX} + 1")
endwhile()

# The pyGPlates version without the release suffix
# (matches the version generated by 'project()' which does not support release suffixes).
set(PYGPLATES_VERSION ${PYGPLATES_VERSION_MAJOR}.${PYGPLATES_VERSION_MINOR}.${PYGPLATES_VERSION_PATCH})

# Combine the pre/post/dev version segments into the release suffix.
set(PYGPLATES_VERSION_RELEASE_SUFFIX ${PYGPLATES_VERSION_PRE_RELEASE_SUFFIX}${PYGPLATES_VERSION_POST_RELEASE_SUFFIX}${PYGPLATES_VERSION_DEV_RELEASE_SUFFIX})

# The full Python PEP 440 version (same as 'PYGPLATES_PEP440_VERSION' actually).
set(PYGPLATES_VERSION_RELEASE ${PYGPLATES_VERSION}${PYGPLATES_VERSION_RELEASE_SUFFIX})
