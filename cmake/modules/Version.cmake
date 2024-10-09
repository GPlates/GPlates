#############################
# GPlates/pyGPlates Version #
#############################

#
# Whether to build (and install, package, etc) GPlates or pyGPlates.
#
# We no longer support building *both* since, for example, CPack's Debian packages don't support different versions for different components
# (eg, we used to have a 'gplates' component and a 'pygplates' component using the COMPONENT variable of the 'install()' command).
# Now we build *either* GPlates or pyGPlates. If you need to build both then create two separate out-of-place CMake builds
# (out-of-place means the binary artifacts are created in a directory separate from the source code).
#
# NOTE: THIS IS CURRENTLY THE PYGPLATES BRANCH (SO 'GPLATES_BUILD_GPLATES' DEFAULTS TO 'FALSE').
#       YOU SHOULD ONLY BUILD 'pygplates'. YOU SHOULDN'T BUILD 'gplates' UNTIL THIS BRANCH IS FULLY MERGED TO TRUNK
#       (WHICH CAN ONLY HAPPEN ONCE WE'VE COMPLETELY UPDATED THE INTERNAL MODEL).
#
option(GPLATES_BUILD_GPLATES "True to build GPlates (false to build pyGPlates)." false)


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
#
set(GPLATES_SEMANTIC_VERSION 2.5.0)


#
# The pyGPlates version.
#
# This is a *restricted* form of Python PEP 440 versioning.
# For the *unrestricted* form see https://peps.python.org/pep-0440/.
#
# NOTE: The restrictions are:
#       The first part of the version should be three dot-separated numbers (MAJOR.MINOR.PATCH).
#       The optional pre-release suffix of the version should be one of:
#       - '.dev' followed by a number for development pre-releases (eg, .dev1, .dev2, etc),
#       - 'a' followed by a number for alpha pre-releases (eg, a1, a2, etc),
#       - 'b' followed by a number for beta pre-releases (eg, b1, b2, etc),
#       - 'rc' followed by a number for pre-release candidates (eg, rc1, rc2, etc).
#
# For example (in order of precedence):
#
#   0.44.0a1
#   0.44.0b1
#   0.44.0rc1
#   0.44.0
#   0.45.0.dev1
#   0.45.0.rc1
#   0.45.0
#   1.0.0rc1
#   1.0.0rc2
#   1.0.0
#   1.0.1
#
set(PYGPLATES_PEP440_VERSION 0.48.0)


##################
# Implementation #
##################


#
# A note about pre-release version suffixes (such as GPLATES_VERSION_PRERELEASE_SUFFIX and PYGPLATES_VERSION_PRERELEASE_SUFFIX)...
#
# These are:
# - empty if not a pre-release,
# - a number for development pre-releases (eg, 1, 2, etc),
# - 'alpha' followed by '.' followed by a number for alpha pre-releases (eg, alpha.1, alpha.2, etc),
# - 'beta' followed by '.' followed by a number for beta pre-releases (eg, beta.1, beta.2, etc),
# - 'rc' followed by '.' followed by a number for pre-release candidates (eg, rc.1, rc.2, etc).
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
# PYGPLATES_VERSION                           - Major.Minor.Patch version (without optional pre-release suffix).
#
# PYGPLATES_VERSION_PRERELEASE_SUFFIX         - Optional pre-release suffix (in Semantic Versioning format).
# PYGPLATES_VERSION_PRERELEASE_SUFFIX_USER    - Human-readable pre-release suffix that inserts 'dev' for development pre-releases.
#
# PYGPLATES_VERSION_PRERELEASE                - Version dictated by Semantic Versioning.
#                                               Used when need correct version precendence (eg, '1' < 'alpha.1').
# PYGPLATES_VERSION_PRERELEASE_USER           - Human-readable version that inserts 'dev' for development pre-releases.
#                                               Useful for any string the user might see.
#                                               Does not maintain correct version precedence (eg, 'dev1' > 'alpha.1' whereas '1' < 'alpha.1').
#

# Extract version information from PYGPLATES_PEP440_VERSION.
if (NOT PYGPLATES_PEP440_VERSION MATCHES [[^([0-9]+)\.([0-9]+)\.([0-9]+)((\.dev|a|b|rc)[0-9]+)?$]])
	message(FATAL_ERROR "${PYGPLATES_PEP440_VERSION} should be X.Y.Z or a pre-release X.Y.Z.devN, X.Y.ZaN, X.Y.ZbN or X.Y.ZrcN")
endif()
set(PYGPLATES_VERSION_MAJOR ${CMAKE_MATCH_1})
set(PYGPLATES_VERSION_MINOR ${CMAKE_MATCH_2})
set(PYGPLATES_VERSION_PATCH ${CMAKE_MATCH_3})
# The pyGPlates version without the pre-release suffix
# (matches the version generated by 'project()' which does not support pre-release suffixes).
set(PYGPLATES_VERSION ${PYGPLATES_VERSION_MAJOR}.${PYGPLATES_VERSION_MINOR}.${PYGPLATES_VERSION_PATCH})
# If a pre-release suffix was specified.
if (CMAKE_MATCH_COUNT EQUAL 5)
	set(PYGPLATES_VERSION_PRERELEASE_SUFFIX ${CMAKE_MATCH_4})
	# For development releases remove the '.dev' part (and just leave the number).
	# For alpha, beta and release candidates change, eg, a1 to alpha.1, b1 to beta.1, rc1 to rc.1).
	if (PYGPLATES_VERSION_PRERELEASE_SUFFIX MATCHES [[^\.dev([0-9]+)$]])
		set(PYGPLATES_VERSION_PRERELEASE_SUFFIX ${CMAKE_MATCH_1})
	elseif (PYGPLATES_VERSION_PRERELEASE_SUFFIX MATCHES [[^a([0-9]+)$]])
		set(PYGPLATES_VERSION_PRERELEASE_SUFFIX alpha.${CMAKE_MATCH_1})
	elseif (PYGPLATES_VERSION_PRERELEASE_SUFFIX MATCHES [[^b([0-9]+)$]])
		set(PYGPLATES_VERSION_PRERELEASE_SUFFIX beta.${CMAKE_MATCH_1})
	elseif (PYGPLATES_VERSION_PRERELEASE_SUFFIX MATCHES [[^rc([0-9]+)$]])
		set(PYGPLATES_VERSION_PRERELEASE_SUFFIX rc.${CMAKE_MATCH_1})
	else()  # shouldn't be able to get here
		message(FATAL_ERROR "${PYGPLATES_VERSION_PRERELEASE_SUFFIX} should be .devN, aN, bN or rcN")
	endif()
	set(PYGPLATES_VERSION_PRERELEASE ${PYGPLATES_VERSION}-${PYGPLATES_VERSION_PRERELEASE_SUFFIX})
	# A human-readable pre-release version (unset/empty if not a pre-release).
	#
	# If a development release (ie, if pre-release version is just a number) then insert 'dev' into the version *name* to make it more obvious to users.
	# Note: We don't insert 'dev' into the version itself because that would give it a higher version ordering precedence than 'alpha' and 'beta' (since a < b < d).
	#       Keeping only the development number in the actual version works because digits have lower precedence than non-digits (according to Semantic and Debian versioning).
	if (PYGPLATES_VERSION_PRERELEASE_SUFFIX MATCHES [[^[0-9]+$]])
		set(PYGPLATES_VERSION_PRERELEASE_SUFFIX_USER dev${PYGPLATES_VERSION_PRERELEASE_SUFFIX})
	else()
		set(PYGPLATES_VERSION_PRERELEASE_SUFFIX_USER ${PYGPLATES_VERSION_PRERELEASE_SUFFIX})
	endif()
	set(PYGPLATES_VERSION_PRERELEASE_USER ${PYGPLATES_VERSION}-${PYGPLATES_VERSION_PRERELEASE_SUFFIX_USER})
else()
	set(PYGPLATES_VERSION_PRERELEASE_SUFFIX "")
	set(PYGPLATES_VERSION_PRERELEASE_SUFFIX_USER "")
	set(PYGPLATES_VERSION_PRERELEASE ${PYGPLATES_VERSION})
	set(PYGPLATES_VERSION_PRERELEASE_USER ${PYGPLATES_VERSION})
endif()
