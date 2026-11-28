# SDKs

| Directory | Contents |
| --- | --- |
| `Winamp-2.90` | Nullsoft's Media Library headers and example plug-in from the [Winamp 2.90 SDK forum release](https://forums.winamp.com/forum/developer-center/winamp-development/107264-winamp-2-90-sdk). |
| `Winamp3-build488` | The 597 files in the required Wasabi SDK component of [Nullsoft's Winamp3 build 488 installer](https://web.archive.org/web/20031220201328/http://download.nullsoft.com/winamp/client/wasabi-sdk_build488.exe). |
| `Wasabi-SDK-499g8` | The full Wasabi Development SDK for component ABI 7, including the optional example components, Media Library, and Playlist Editor sources. [499g8 release](https://sourceforge.net/projects/wasabidev/files/wasabidev/Wasabi%20SDK%20499g8/) |
| `Winamp-5.55` | The original extracted Nullsoft 5.55 SDK. [Archived Winamp plug-in developer page](https://web.archive.org/web/20251116194911/http://wiki.winamp.com/wiki/Plug-in_Developer) |
| `Winamp-5.666` | The Winamp 5.666 header collection published by WACUP and the default for building Tangerine Stream. [Header collection](https://getwacup.com/sdk/) |
| `Winamp-5.9` | The SDK payload from the Winamp SA 5.9.0.0 installer. [Installer in the archived Winamp source snapshot](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/installer/SDK/WA5.9_SDK.exe) |
| `WACUP` | A Windows shortcut to the dedicated SDK announcement on the [WACUP homepage](https://getwacup.com/). |

The Winamp SDK files retain their original notices and line endings. The SHA-256 of the Winamp 5.9 SDK installer is `1f14e0c73251ea899ac10754e40e219ebbe0a2f5827f8de1b88cc698749598c5`.

The SHA-256 of `winamp290-mlsdk.zip` is `2f8cc4fc43071b06ee3533d014336bf748ccd48ad1ac7c701a2469fbb832f058`.

Tangerine Stream supplies the [2.90 navigation compatibility header](Winamp-2.90/gen_ml/ml_ipc_0313.h) alongside the original SDK. It maps a subset of the later C++ navigation API onto the tree messages in 2.90's `ml.h`. A plug-in can append rows, look up their IDs, read or change the selection, and delete its branches. Use the helpers on the Media Library UI thread for the lifetime of those rows. The old tree displays titles in the Windows ANSI code page and has no storage for invariant names. For list skinning, use 2.90's `ML_IPC_SKIN_LISTVIEW` and `ML_IPC_UNSKIN_LISTVIEW`.

The SHA-256 of `wasabi-sdk_build488.exe` is `4117051c3271cbe3600aeeb239e94b7a772e99a1c61ddcd352cec9ca616893e2`. A copy of the [Mozilla Public License 1.1](https://www.mozilla.org/MPL/1.1/) accompanies the SDK's Expat source.

Set `WACUP_SDK_HOME` to select a Winamp SDK directory for a build. The Media Library plug-in uses the legacy Media Library ABI. The build includes the bundled 5.55 Wasabi and Agave service headers because the 5.666 package omits them.

The SHA-256 of `wasabi-sdk499g8.exe` is `26028ea533c6b2dfd80587d80b1b3a629f2aec91da1b8caf3bf130b39e1c90f8`. The installer writes `docs/codingstyle.txt` twice. `installer-alternates` preserves the earlier copy.
