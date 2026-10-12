# The dependency version pins for the pyGPlates wheel builds - the single source of truth
# shared by the Linux image ('manylinux_2_28.dockerfile' copies this file in and sources it
# in each build step) and the macOS and Windows dependency builds ('build_macos_deps.sh' and
# 'build_windows_deps.sh' source it directly). Bumping a version here rebuilds that dependency
# everywhere: a push rebuilds the Linux images (see '.github/workflows/build-wheel-images.yml'),
# and the macOS and Windows deps cache keys include this file's hash (see
# '.github/workflows/build-wheels.yml').
#
# Not everything is pinned here: the libraries vcpkg supplies to the Windows build are pinned
# by the vcpkg baseline in 'vcpkg.json' instead. For the libraries both systems pin - PROJ and
# GDAL - the versions here are kept equal to what that baseline supplies, so all three
# platforms' wheels ship the same geospatial behaviour. Bump the baseline and these two
# together, as one act, with vcpkg setting the cadence (it is the one of the two whose available
# versions cannot be picked freely); 'build_windows_deps.sh' fails if they drift apart.
#
# Each downloaded source archive has its SHA-256 pinned beside its version, and the download
# fails unless the archive matches (see 'download.sh'), so bumping a version means moving its
# pin too. Compute the new one with 'curl -fsSL <url> | sha256sum', and compare it with the
# checksum the project publishes, where it publishes one. Two downloads are checked by something
# else: aqtinstall checks the Qt binaries macOS and Windows install against Qt's own checksums
# (though aqtinstall itself comes from PyPI by version range, as do the Python build tools), and
# vcpkg checks each port's download against the SHA-512 in the port.
#
# Keep this file to plain NAME=VALUE assignments and comments - it is sourced by /bin/sh in
# the Docker build steps.

# The Python versions to build Boost.Python for, in the Linux image - which is the only thing
# that reads this. Must match the 'build' list in '[tool.cibuildwheel]' in 'pyproject.toml' (the
# authoritative list of wheel Python versions); the sdist job in
# '.github/workflows/build-wheels.yml' fails if the two ever disagree. macOS and Windows need no
# such list: 'build_boost_python.sh' builds Boost.Python for whatever Python version each wheel
# build brings.
PYTHON_VERSIONS="3.9 3.10 3.11 3.12 3.13 3.14 3.15"

# (The SHA-256 is of the 'qtbase' source archive, which only the Linux image downloads.)
QT_VERSION=6.7.3
QT_SHA256=8ccbb9ab055205ac76632c9eeddd1ed6fc66936fc56afc2ed0fd5d9e23da3097

BOOST_VERSION=1.91.0
BOOST_SHA256=de5e6b0e4913395c6bdfa90537febd9028ea4c0735d2cdb0cd9b45d5f51264f5

PROJ_VERSION=9.8.1
PROJ_SHA256=af5b731c145c1d13c4e3b4eeb7d167e94e845e440f71e3496b4ed8dae0291960

GDAL_VERSION=3.12.4
GDAL_SHA256=68844ae29557b7efae4292c3b4cb3a3b8a79d14b765b89c5a7b17cbae7fa715a

CGAL_VERSION=6.0.3
CGAL_SHA256=b83fdf5de2cd8b3de3b159d222d3a76add91dec0ff2b0d6b3d433e6197195c47

# sccache is a prebuilt binary, so there is one archive, and one pin, per platform.
SCCACHE_VERSION=0.17.0
SCCACHE_SHA256_LINUX_X86_64=67c4a96dd237c1f518f6b36083f270f9976d516f1e57fce891755ea782e50006
SCCACHE_SHA256_LINUX_AARCH64=821a86343191aa1cbab74bd42f9e93c9a63bf85e4742945f40d3ae84193c1c77
SCCACHE_SHA256_MACOS_X86_64=c2144cafbfe3d22e34ae637f9974ce53613543ac19477fdb287df22ea3668261
SCCACHE_SHA256_MACOS_ARM64=0c560bfba31aef5bdfb4fb3d2677f6e61d71c5c00952f2a83344f47aa31f00f1
SCCACHE_SHA256_WINDOWS_X86_64=caf1932d76a909c909b7a2e41443cdfe3c79a49a380da1a22fa422e1d00d3ca7

# macOS only (the Linux image uses EL8's gmp-devel/mpfr-devel packages, and Windows takes
# both from vcpkg - neither has an MSVC build system).
GMP_VERSION=6.3.0
GMP_SHA256=a3c2b80201b89e68616f4ad30bc66aee4927c3ce50e33929ca819d5c43538898
MPFR_VERSION=4.2.1
MPFR_SHA256=277807353a6726978996945af13e52829e3abd7a9a5b7fb2793894e18f1fcbb2

# Linux only (macOS uses the SDK's SQLite). The year is part of the sqlite.org download path.
SQLITE3_VERSION=3460000
SQLITE3_YEAR=2024
SQLITE3_SHA256=6f8e6a7b335273748816f9b3b62bbdc372a889de8782d7f048c653a447417a7d
