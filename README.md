# Tangerine Stream

Tangerine Stream adds Navidrome and OpenSubsonic browsing to the WACUP Media Library. It supports cover-art browsing, multiple server profiles, playback and enqueueing, offline music, and durable play reports. The plugin targets 32-bit WACUP on Windows 10 or later.

## Build and install

The build defaults to the Winamp 5.666 headers in `SDKs/Winamp-5.666`. Set `WACUP_SDK_HOME` in the build environment to select another SDK directory. An explicit `-DWINAMP_SDK=...` takes precedence. To clear an SDK path left in an older build directory, configure once with `cmake -S . -B build-mingw32 -UWINAMP_SDK`. [SDK sources and versions](SDKs/README.md)

Double-click **build.bat** to build with 32-bit MSYS2. The build produces `ml_tangerinestream.dll` and `in_tangerinestream.dll` in `build-mingw32`. The script uses `C:\msys64` by default. Set the `MSYS2_ROOT` environment variable if MSYS2 is installed elsewhere. CMake and Ninja can come from MSYS2 or from your Windows PATH.

If you need the tools, install them from the MSYS2 **MINGW32** shell.

~~~sh
pacman -S --needed mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-i686-ninja
~~~

Close WACUP before running **build-and-install.bat**. It builds and copies both DLLs into the detected WACUP `Plugins` folder. Use **Run as administrator** if the folder requires elevated write access. Reopen WACUP and select **Tangerine Stream** in the Media Library.

From Command Prompt, pass `vs` to use Visual Studio's 32-bit C++ tools and write the build to `build-vs`. Pass `--no-pause` when running either script unattended. For a portable WACUP installation, set `WACUP_PLUGINS_DIR` to its actual plugin directory.

~~~bat
build.bat vs
set "WACUP_PLUGINS_DIR=D:\Portable\WACUP\Plugins"
build-and-install.bat --no-pause
~~~

With the toolchain environment set up, `cmake --build build-mingw32 --target install` also builds and installs both DLLs. To set the destination when configuring CMake, pass `-DWACUP_PLUGINS_DIR="D:\Portable\WACUP\Plugins"`. The MinGW build links its compiler runtime statically and uses Windows-provided DLLs. Enable Tangerine Stream under both **Plug-ins > Media Library** and **Plug-ins > Input**. Both use the server profiles configured in the library.

Declare each new plug-in with `add_wacup_plugin(target sources...)` in CMake. This adds its DLL to the `wacup_plugins` build target and the standard `install` target used by the two batch files.

## Connect and play

1. Select **Add server…** and enter the server base URL, including its installation subpath if it has one, plus the account username and password. Use the HTTPS URL when the server provides one.
2. Use the default OpenSubsonic browser for Navidrome or another compatible server. Enable **Use Navidrome’s native library browser** for Navidrome’s album, artist, playlist, and track metadata, including play counts and ratings. Playback, favorites, and reporting use OpenSubsonic. [Navidrome compatibility](https://www.navidrome.org/docs/developers/subsonic-api/) · [Feishin’s Navidrome controller](https://github.com/jeffvli/feishin/blob/development/src/renderer/api/navidrome/navidrome-controller.ts)
3. Expand **Tangerine Stream** and choose **Albums**, **Genres**, **Album Artists**, **Artists**, **Tracks**, **Folders**, **Playlists**, **Favorites**, or **Offline Cache**. Album browsing uses the WACUP Media Library's skinned list-view and loads server artwork as you scroll.
4. Click a tile to select it. Ctrl-click to toggle items, Shift-click to select a range, or drag from empty space to select a rectangle. The bottom bar shows the selection and its combined playing time when available. Drag selected albums onto WACUP's Playlist Editor to enqueue them in order. Offline, the drop includes their downloaded tracks.
   Double-click a tile to open it and use **Back** to return. Type in **Search** to filter the current view. Open the sort button and choose **Text list** to browse the same results in rows.
5. Double-click a track to play it. Select albums or tracks and use **Play** or **Enqueue** in the bottom bar.

Open **Preferences > Plug-ins > Media Library**, select **Tangerine Stream**, and click **Configure** to add or edit server profiles. **Unify Libraries** puts the categories directly under Tangerine Stream. Clear it to give each server its own branch. A single server always uses the unified tree.

The server filter beside Search lets you choose **<All>** or a named profile in either tree layout. Switching servers keeps the current category. Each profile keeps its own track IDs, cached music, and pending reports.

The adjacent button shows the current sort, such as **Name** or **Recently added**. Its choices depend on the category. Select **Descending** to reverse the order or select **Random** again to reshuffle. Each category remembers its sort. Album and playlist tracks initially keep their original sequence. Sorting combines the results from the selected servers. Entries with missing metadata follow entries with known values. Cached entries pick up additional sort metadata when you browse online.

Playlist entries use the server's artist and track title. Use WACUP's **Album Art** and **Lyrics** windows for track covers and lyrics. Tangerine Stream supplies cover images through the player's album art service and plain lyrics through track metadata. The cache keeps OpenSubsonic line and word timing for future synchronized display. [OpenSubsonic song lyrics](https://opensubsonic.github.io/docs/endpoints/getlyricsbysongid/)

## Streaming quality

In **Configure**, select a server and choose a format and maximum bitrate for each connection. Windows supplies the cost of the preferred Internet connection. Roaming, a data-limit warning, and unknown cost all use the metered setting. A connection change takes effect on the next track. An already cached track plays from disk on either connection.

| Connection | Default | Behavior |
| --- | --- | --- |
| Unmetered | Automatic, 0 kbps | Let the server choose direct play or transcoding with no client bitrate ceiling. |
| Metered | MP3, 128 kbps | Request MP3 at up to 128 kbps. Skip the separate rolling-cache download. |

Choose **Original** for the server's original file, or select MP3, Opus, AAC, Vorbis, or FLAC. A bitrate of **0** leaves the rate to the server. In Automatic or codec-specific mode, the plug-in sends its playback capabilities and bitrate ceiling to servers advertising the OpenSubsonic transcoding extension. If the server declines the requested quality, playback reports the failure in the library. Older servers receive Subsonic stream parameters and determine which encoder settings they can honor. For those servers, Automatic requests the original file with no ceiling and MP3 with a ceiling. [OpenSubsonic transcoding](https://opensubsonic.github.io/docs/extensions/transcoding/)

Permanent downloads use the active connection's quality. With automatic favorite caching enabled, **Upgrade offline favorites to original quality on unmetered connections** can replace a transcoded favorite with its server original. The existing copy stays playable until the replacement finishes. A failed or interrupted upgrade keeps the existing copy for the next attempt.

## Offline playback and reports

- Choose **Work offline** from the sort menu to browse and search cached albums, genres, artists, folders, playlists, favorites, and tracks with network requests paused. The view also switches to its cache when a server cannot be reached and retries the connection periodically. Pages and relationships are saved as you browse. Open a playlist or folder while connected to make its contents available offline.
- The track list shows which songs have audio cached, and those songs can play offline. Artwork and lyrics fetched while online remain available in the cache. The artwork database holds up to 128 MiB. Tangerine Stream also keeps up to 24 MiB of recent covers beside playlist entries so WACUP's Album Art window can read them as soon as you enqueue or select a track.
- The rolling cache is shared by all profiles and defaults to 1,024 MiB. A complete stream is saved locally when it fits. The least recently used rolling entries are evicted as space is needed. A budget of `0` disables rolling storage.
- Select tracks and choose **Cache > Keep offline** to download them permanently. **Cache > Unpin** returns a track to the rolling cache if it has no automatic favorite pin. **Offline Cache** shows downloaded tracks and pending offline downloads.
- Enable automatic favorite caching per profile to download starred songs and tracks from starred albums and artists. Favorite downloads stay offline while starred. Permanent and favorite downloads use disk space outside the rolling budget.
- A play report is saved after listening to half a track or four minutes, whichever comes first. The plugin retries queued batches after connection failures and across WACUP restarts. If a server processes a batch and the acknowledgement is lost, it may receive the batch again. [OpenSubsonic scrobble](https://opensubsonic.github.io/docs/endpoints/scrobble/)

The database and audio cache live under `Plugins/Tangerine Stream` in the WACUP settings directory. Saved passwords use Windows DPAPI for the current user. Each new playlist entry points to a small local `.tstream` reference in that profile. The input plug-in supplies the title, duration and lyrics for that reference, then uses the SDK's `IPC_CANPLAY` to select WACUP's installed input for the cached file or a freshly authenticated server stream. WACUP's selected output plug-in plays the decoded audio. Server credentials stay out of the playlist. The plug-ins communicate inside the WACUP process without a localhost listener.

With rolling caching enabled, playing an uncached track on an unmetered connection also schedules a separate download using that connection?s quality settings. Setting the rolling budget to zero disables that download. Previous releases' downloaded audio is migrated in place. For saved playlists containing old localhost links, re-enqueue the tracks if WACUP routes those links to its HTTP input plug-in.

Provider integrations implement the interface in [src/server.hpp](src/server.hpp). Playback, caching, and report storage use provider-independent track and profile IDs.
