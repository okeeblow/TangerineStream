EXAMPLE EXECUTABLE
------------------

The latest SDKs provide you the ability to create your own wasabi executable, so if you want to make your own Wasabi program (instead of "just" winamp3 components), you're able to.  

To make your own executable WACloader, simply add the three modules in $/studio/wasabi to a standard "Win32 Application" project, and change the settings to link through the "Multithreaded" runtime library.  If you look at $/studio/wasabi/winmain.cpp you'll see it's honestly and truly the Winamp3 winmain.  You'll want to make your own version of that module.

To help you in this process, ExampleStudio.dsp has been altered to build Example.exe into the $/studio/studio folder, based off of $/studio/Examples/ExampleStudio.winmain.cpp 

This currently requires you to install a version of Winamp3 build #491 or greater.  If you do not do this and try to run the executable, you'll get the following error messagebox:

---------------------------
OS error while loading Wasabi
---------------------------
The filename or extension is too long.
---------------------------

You should also copy common.dll from your winamp install (with the same build number!) to your SDK install.  We currently omit this file for download size issues, we hope for this to be only a temporary requirement.

If you get an assertion in std.cpp on startup, this is a known bug currently within the config system.  Try copying studio.xnf over to your program folder and this may fix the problem.

We're still putting it all together, but we thought you would like a peek at it.

