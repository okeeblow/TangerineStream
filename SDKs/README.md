# SDKs

| Directory | Contents |
| --- | --- |
| `Winamp-5.55` | The original extracted Nullsoft 5.55 SDK. [Archived Winamp plug-in developer page](https://web.archive.org/web/20251116194911/http://wiki.winamp.com/wiki/Plug-in_Developer) |
| `Winamp-5.666` | The Winamp 5.666 header collection published by WACUP and the default for building Tangerine Stream. [Header collection](https://getwacup.com/sdk/) |
| `Winamp-5.9` | The SDK payload from the Winamp SA 5.9.0.0 installer. [Installer in the archived Winamp source snapshot](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/installer/SDK/WA5.9_SDK.exe) |
| `WACUP` | A Windows shortcut to the dedicated SDK announcement on the [WACUP homepage](https://getwacup.com/). |

The Winamp SDK files retain their original notices and line endings. The SHA-256 of the Winamp 5.9 SDK installer is `1f14e0c73251ea899ac10754e40e219ebbe0a2f5827f8de1b88cc698749598c5`.

Set `WACUP_SDK_HOME` to select a Winamp SDK directory for a build. The Media Library plug-in uses the legacy Media Library ABI. The build includes the bundled 5.55 Wasabi and Agave service headers because the 5.666 package omits them.
