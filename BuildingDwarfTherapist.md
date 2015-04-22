# Tools #

Dwarf Therapist source is managed via Mercurial repository and is built using the Qt SDK.  You can find the command line version of Mercurial at the [Mercurial SCM page](http://mercurial.selenic.com/).  You can find the Qt SDK at Nokia's [Qt Downloads page](http://qt.nokia.com/downloads).  The free LPGL version is recommended.  Currently Dwarf Therapist is built using Qt 4.7.0.  The Qt SDK comes with the Qt Creator software, which is used to manage the Dwarf Therapist project.

# Building #

The instructions provided have been tested on windows.  Your mileage may vary on other operating systems.

Once you've checked out the project following the instructions on the [Source](http://code.google.com/p/dwarftherapist/source/checkout) page, Open Qt Creator, select File > Open and open dwarftherapist.pro (found in the root level of the source directory.)

Once the project is open, Qt Creator is similar to a number of other IDEs with the exception that a large amount of build options are obscured from the developer.  The buttons in the bottom left let you control the type of build (release or debug.)

# Ubuntu 10.04 #
I've tested the above instructions on Ubuntu 10.04.  I only had to install the following packages using apt:
```
sudo apt-get install build-essential libglib2.0-dev libSM-dev libxrender-dev libfontconfig1-dev libxext-dev
```

Alternatively, if you want to build from the command line, the following should work (Thanks to Doug who commented on the LinuxVersion wiki page):
```
$ sudo apt-get install qt4-qmake qt4-dev-tools

$ hg clone https://dwarftherapist.googlecode.com/hg/ dwarftherapist

$ cd dwarftherapist

$ qmake

$ make
```

# Debugging #

The build output directory for dwarf therapist is parallel to the root of the source directory and named _dwarftherapist-build-desktop_.  In order to run dwarf therapist from Qt creator you should create an empty log directory under the release or debug directory, and copy the etc folder to the same location. You should end up with a directory structure such as this:

  * bin
    * debug
      * etc
      * log
      * DwarfTherapist.exe
    * release
      * etc
      * log
      * DwarfTherapist.exe

Once you have this structure, you should be able to launch DwarfTherapist from within Qt creator and execute step-through debugging.