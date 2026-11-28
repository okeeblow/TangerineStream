# Tangerine Stream

Play music from Navidrome and other OpenSubsonic servers in the Media Library of 32-bit WACUP. Browse by cover art, drag albums onto the Playlist Editor, and follow along in WACUP’s Album Art and Lyrics windows.

## Install

With WACUP closed, copy both `ml_tangerinestream.dll` and `in_tangerinestream.dll` into its `Plugins` folder. The first adds the library browser. The second handles playback.

To build from source, install the tools in the MSYS2 **MINGW32** shell.

```sh
pacman -S --needed mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-i686-ninja
```

Double-click [build-and-install.bat](build-and-install.bat) to build and copy both DLLs. For portable WACUP, set `WACUP_PLUGINS_DIR` to its `Plugins` folder first. To build with Visual Studio, install the **MSVC v143 14.44 x64/x86 build tools** and run `build-and-install.bat vs` from Command Prompt.

## Connect a server

Open **Tangerine Stream** in the Media Library and click **Add server**. Enter your server’s base URL, including any subpath, and your username and password. Navidrome works with the default OpenSubsonic browser. Check **Use Navidrome’s native library browser** for additional metadata such as play counts and ratings.

To add another server or change streaming quality, open **Preferences → Plug-ins → Media Library**, select **Tangerine Stream**, and click **Configure**. **Unify Libraries** combines servers in one library tree. Each server has its own quality settings. Metered connections default to MP3 at up to 192 kbps. Unmetered connections let the server choose.

Cached tracks start immediately. FLAC and MP3 can start during the download when the enabled WACUP decoder supports Extended Read. If playback catches up with the download, it waits for more audio and resumes at the same frame. Other decoders open the completed file. The same download supplies the cache. Skipping cancels a transfer still in progress. Temporary copies are deleted after playback.

## Change the library view

Open **≡** at the right of the search bar, or press **F2** to edit the current view. Choose a simple list, album covers, or up to three filters such as Genre, Artist and Album. The menu also lets you choose columns, set the cover size and show the selected album’s tracks below or beside its art. Tangerine Stream remembers the layout for each category.

## Listen offline

Select tracks and choose **Cache → Keep offline**, or enable **Automatically keep server favorites offline** in Configure. Pinned tracks and downloaded favorites use space outside the shared rolling cache, which defaults to 1 GiB.

Choose **≡ → Work offline** to browse what you’ve already visited. Downloaded tracks play offline. Tangerine Stream saves your listens across restarts and submits them when your server is reachable.
