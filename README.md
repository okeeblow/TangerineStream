# Tangerine Stream

Tangerine Stream adds Navidrome and OpenSubsonic browsing to the WACUP Media Library. It supports cover-art browsing, multiple server profiles, playback and enqueueing, offline music, and durable play reports. The plugin targets 32-bit WACUP on Windows 10 or later.

## Build and install

The build defaults to the Winamp 5.666 headers in `SDKs/Winamp-5.666`. Set `WACUP_SDK_HOME` in the build environment to select another SDK directory. The CMake cache option `WINAMP_SDK` takes precedence. [SDK sources and versions](SDKs/README.md)

Double-click **build.bat** to build with 32-bit MSYS2. The DLL goes to `build-mingw32/ml_tangerinestream.dll`. The script uses `C:\msys64` by default. Set the `MSYS2_ROOT` environment variable if MSYS2 is installed elsewhere. CMake and Ninja can come from MSYS2 or from your Windows PATH.

If you need the tools, install them from the MSYS2 **MINGW32** shell.

~~~sh
pacman -S --needed mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-i686-ninja
~~~

Close WACUP before running **build-and-install.bat**. It builds and copies the DLL into the detected WACUP `Plugins` folder. Use **Run as administrator** if the folder requires elevated write access. Reopen WACUP and select **Tangerine Stream** in the Media Library.

From Command Prompt, pass `vs` to use Visual Studio's 32-bit C++ tools and write the build to `build-vs`. Pass `--no-pause` when running either script unattended. For a portable WACUP installation, set `WACUP_PLUGINS_DIR` to its actual plugin directory.

~~~bat
build.bat vs
set "WACUP_PLUGINS_DIR=D:\Portable\WACUP\Plugins"
build-and-install.bat --no-pause
~~~

With the toolchain environment set up, `cmake --build build-mingw32 --target install` also builds and installs the DLL. To set the destination when configuring CMake, pass `-DWACUP_PLUGINS_DIR="D:\Portable\WACUP\Plugins"`. The MinGW build links its compiler runtime statically and uses Windows-provided DLLs.

## Connect and play

1. Select **Add server…** and enter the server base URL, including its installation subpath if it has one, plus the account username and password. Use the HTTPS URL when the server provides one.
2. Use the default OpenSubsonic browser for Navidrome or another compatible server. Enable **Use Navidrome’s native song browser** to use Navidrome’s native login and song list with OpenSubsonic playback, favorites, and reporting. [Navidrome compatibility](https://www.navidrome.org/docs/developers/subsonic-api/) · [Feishin’s Navidrome controller](https://github.com/jeffvli/feishin/blob/development/src/renderer/api/navidrome/navidrome-controller.ts)
3. Expand **Tangerine Stream** and choose **Albums**, **Genres**, **Album Artists**, **Artists**, **Tracks**, **Folders**, **Playlists**, **Favorites**, or **Offline Cache**. Album browsing uses the WACUP Media Library's skinned list-view and loads server artwork as you scroll.
4. Double-click a tile to open it and use **Back** to return to your previous selection. Use **Text list** to browse the same results in rows. Double-click a track to play it, or select several tracks and use **Play** or **Enqueue**.

Add profiles for other servers or accounts and switch between them using the server selector. **Settings** lets you update the profile name, password, browsing mode, and favorite download preference. Each profile keeps its own track IDs, cached music, and pending reports.

## Offline playback and reports

- The rolling cache is shared by all profiles and defaults to 1,024 MiB. A complete stream is saved locally when it fits. The least recently used rolling entries are evicted as space is needed. A budget of `0` disables rolling storage.
- Select tracks and use **Keep offline** to download them permanently. **Unpin** returns a track to the rolling cache if it has no automatic favorite pin. **Offline Cache** shows downloaded tracks and pending offline downloads.
- Enable automatic favorite caching per profile to download starred songs and tracks from starred albums and artists. Favorite downloads stay offline while starred. Permanent and favorite downloads use disk space outside the rolling budget.
- A play report is saved after listening to half a track or four minutes, whichever comes first. The plugin retries queued batches after connection failures and across WACUP restarts. If a server processes a batch and the acknowledgement is lost, it may receive the batch again. [OpenSubsonic scrobble](https://opensubsonic.github.io/docs/endpoints/scrobble/)

The database and audio cache live under `Plugins/Tangerine Stream` in the WACUP settings directory. Saved passwords use Windows DPAPI for the current user. Playlist entries use a local playback URL so they can find the offline copy or stream from the server without storing server credentials in the playlist.

Provider integrations implement the interface in [src/server.hpp](src/server.hpp). Playback, caching, and report storage use provider-independent track and profile IDs.
