# Tangerine Stream

Play music from Navidrome and other OpenSubsonic servers in the Media Library of 32-bit WACUP on Windows 10 or later. Browse by cover art, drag albums onto the Playlist Editor, and follow along in WACUP’s Album Art and Lyrics windows.

## Install

With WACUP closed, copy both `ml_tangerinestream.dll` and `in_tangerinestream.dll` into its `Plugins` folder. The first adds the library browser. The second handles playback.

To build from source, install the tools in the MSYS2 **MINGW32** shell.

```sh
pacman -S --needed mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-i686-ninja
```

Double-click [build-and-install.bat](build-and-install.bat) to build and copy both DLLs. For portable WACUP, set `WACUP_PLUGINS_DIR` to its `Plugins` folder first. For Visual Studio’s x86 C++ tools, run `build-and-install.bat vs` from Command Prompt.

## Connect a server

Open **Tangerine Stream** in the Media Library and click **Add server**. Enter your server’s base URL, including any subpath, and your username and password. Navidrome works with the default OpenSubsonic browser. Check **Use Navidrome’s native library browser** for additional metadata such as play counts and ratings.

To add another server or change streaming quality, open **Preferences → Plug-ins → Media Library**, select **Tangerine Stream**, and click **Configure**. **Unify Libraries** combines servers in one library tree. Each server has its own quality settings. Metered connections default to MP3 at up to 192 kbps. Unmetered connections let the server choose.

## Listen offline

Select tracks and choose **Cache → Keep offline**, or enable **Automatically keep server favorites offline** in Configure. Pinned tracks and downloaded favorites use space outside the shared rolling cache, which defaults to 1 GiB.

Choose **Work offline** from the sort menu to browse what you’ve already visited. Downloaded tracks play offline, and Tangerine Stream saves your scrobbles across restarts until it can send them to the server.
