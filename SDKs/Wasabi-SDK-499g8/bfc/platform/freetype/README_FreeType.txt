How to integrate FreeType font rendering into your Wasabi.

1)  Go to http://www.freetype.org/ for overviews on freetype
2)  Go to http://sourceforge.net/project/showfiles.php?group_id=3157 for
	binary downloads.  Download the tar.gz of the latest 2.1.x series.
	THIS DOCUMENT WAS CREATED USING 2.1.3 AS A GUIDE.
3)  If you downloaded the 2.1.3.zip distro instead of the tar.gz, curse
	loudly about the fact that the .zip distro is broken.
4)  If you are attempting to integrate freetype on Windows, you'll have 
	to download the Gnu "make" util.  You're on your own, kid.
5)  After building the distro, you should wind up with freetype.lib in
	a folder called "objs" which is a sibling of "include"
6)  Make a folder named "Freetype2" in this folder, place the "objs" and
	"include" folders in it.

The "fontmaker_freetype" and "freetypefont" modules may then be compiled into the svc_%platform%.wac projects.