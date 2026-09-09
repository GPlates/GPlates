#####################################
# GPlates/pyGPlates release targets #
#####################################

#
# The release each develop line is heading towards.
#
# THIS IS THE ONLY HAND-EDITED PART OF THE VERSION. The development number that turns it into a
# full version (eg, '2.6.0' -> '2.6.0-47', '1.1.0' -> '1.1.0.dev46') is counted from git by
# 'VersionFromGit.cmake' - see the comments there, and 'Version.cmake' for the version grammars.
#
# There are exactly two times to edit this file:
#
# 1. When a release changes what the *next* release will be called. Immediately after tagging
#    'PyGPlates-1.1.0', for instance, the pyGPlates target becomes '1.2.0' - otherwise the next
#    development version would be '1.1.0.dev1', which sorts *below* the 1.1.0 just released
#    (the resolver refuses to produce that, rather than letting it reach a package).
#
#    Note: The pyGPlates version should typically be updated when the API changes (eg, a new
#          function or class) so users can then test for new functionality (even for internal
#          releases). That decision is made here, by choosing the next target.
#
# 2. On a release branch ('release/pygplates-<version>' etc), to name the candidate being
#    prepared - eg, set the target to '1.1.0rc1' on cutting the branch, to '1.1.0rc2' if a
#    second candidate is needed, and to '1.1.0' for the release itself. Development commits on
#    the release branch then carry '1.1.0rc1.dev3' and so on.
#
#    Nothing detects the branch by name: a target is a decision ("there will be another release
#    candidate"), and branch-name detection is unreliable anyway under detached HEAD, pull
#    request merge refs, worktrees and forks.
#
# A target must NOT carry a development suffix ('2.6.0-8', '1.1.0.dev10') - that part is what
# gets counted. The resolver rejects one that does.
#

# The GPlates release target - a restricted Semantic Version 'X.Y.Z' or 'X.Y.Z-{alpha|beta|rc}.N'.
set(GPLATES_RELEASE_VERSION 2.6.0)

# The pyGPlates release target - a restricted PEP 440 version 'X.Y.Z[{a|b|rc}N][.postN]'.
set(PYGPLATES_RELEASE_VERSION 1.1.0)
