# GPlates distribution files

Platform files that GPlates' executable, application bundle and installers are built from. Only
GPlates builds use them (not pyGPlates). CMake finds this directory through
`GPLATES_SOURCE_DISTRIBUTION_DIR`, set in `cmake/modules/ConfigDefault.cmake`.

| File | What it is | Used by |
|---|---|---|
| `gplates_desktop_icon.ico` | Windows application icon, several sizes in one file | `gplates_desktop_icon.rc`, and the NSIS installer and uninstaller icons (`cmake/modules/Package.cmake`) |
| `gplates_desktop_icon.rc` | Windows resource script that embeds the `.ico` into `gplates.exe` | Added as a source of the `gplates` target (`src/CMakeLists.txt`). It names the `.ico` by a relative path, so the two files must stay side by side. |
| `gplates_desktop_icon.icns` | macOS application icon | Copied into the bundle's `Resources` (`src/CMakeLists.txt`), and the DragNDrop volume icon (`cmake/modules/PackageGeneratorOverrides.cmake.in`) |
| `MacOSXBundleInfo.plist.in` | Template for the bundle's `Info.plist`. CMake fills in the `${MACOSX_BUNDLE_*}` values; the template adds the keys for Retina (high-resolution) support. | `MACOSX_BUNDLE_INFO_PLIST` in `src/CMakeLists.txt` |

## Regenerating the icons

Both icons are made from `src/qt-resources/GPlates-icon-160x160-opaque.png`.

### Windows (`gplates_desktop_icon.ico`)

The icon holds 128, 64, 48, 32 and 16 pixel images. With [ImageMagick](https://imagemagick.org/):

```
magick src/qt-resources/GPlates-icon-160x160-opaque.png -define icon:auto-resize=128,64,48,32,16 cmake/distribution/gplates_desktop_icon.ico
```

(The committed icon was made with the convertico.com website, choosing the same sizes.)

Windows caches icons, so a rebuilt `gplates.exe` may keep showing the old one in Explorer until
Windows is restarted, even though the executable contains the new icon.

### macOS (`gplates_desktop_icon.icns`)

Following Apple's
[high-resolution guidelines](https://developer.apple.com/library/archive/documentation/GraphicsAnimation/Conceptual/HighResolutionOSX/Optimizing/Optimizing.html):

1. Create a directory `gplates_desktop_icon.iconset` containing these PNG files:

   | File | Pixels |
   |---|---|
   | `icon_16x16.png` | 16 |
   | `icon_16x16@2x.png` | 32 |
   | `icon_32x32.png` | 32 |
   | `icon_32x32@2x.png` | 64 |
   | `icon_128x128.png` | 128 |
   | `icon_128x128@2x.png` | 256 |

   An `@2x` image is for Retina displays and has twice the pixels of its namesake. Since each
   image is just the source downsampled, `icon_16x16@2x.png` holds the same image as
   `icon_32x32.png`. (The 160-pixel source has to be *up*sampled for the 256-pixel image.)
   Any image editor will do, or `sips` on the command line, for example:

   ```
   sips -z 32 32 src/qt-resources/GPlates-icon-160x160-opaque.png --out gplates_desktop_icon.iconset/icon_16x16@2x.png
   ```

2. Build the `.icns` from the iconset:

   ```
   iconutil -c icns -o cmake/distribution/gplates_desktop_icon.icns gplates_desktop_icon.iconset
   ```

Apple's old Icon Composer application is no longer available; `iconutil` replaces it.
