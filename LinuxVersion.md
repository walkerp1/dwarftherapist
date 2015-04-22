#everything about the linux version of DwarfTherapist

# Linux Package Repositories #
As of version 0.6.6 of DwarfTherapist, I've started creating a selection of Debian packages.  Currently the following repositories are available and host 32-bit & 64bit binaries of DwarfTherapist:

  * **Ubuntu 10.04 (Lucid)** - deb http://dwarftherapist.com/apt lucid universe
  * **Ubuntu 11.10 (Oneiric)** - deb http://dwarftherapist.com/apt oneiric universe

If you don't see your distro here and would like it added, please let me know; I'll do what I can.  I would still consider DwarfTherapist for linux to be in a late alpha/very early beta phase however, so please do report issues as you see them.

Finally, I'm also working on a distro-independent package. If you happen to be a linux expert and have any tips, please let me know!

### Reporting Results ###
If this works please comment on this wiki page and tell me the following information:
  * What Distro of Linux you're running
  * What kernel version you're running (uname -a)
  * What version of glibc you have
  * If you saw any other weirdness
If it doesn't work please give me the same info as above as well as what follows
  * the **log/run.log** logfile in DT's folder
  * the output of **ldd DwarfTherapist**

To post long text output, please feel free to open a new ticket here: http://code.google.com/p/dwarftherapist/issues/entry This will allow better tracking as things are fixed. Please make sure you reference the linux build by revision number (Rxxx) for better tracking as well.

THANK YOU!