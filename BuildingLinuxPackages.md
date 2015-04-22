# Ubuntu #
I'm only going to support the latest versions of Ubuntu & LTS.
Currently that's
  * LTS: 10.04 Lucid Lynx
  * Standard: 11.10 Oneiric Ocelot

## Setting up a build server ##
We're assuming a 64-bit version of the latest Ubuntu Server
We can use pbuilder & pbuilder-dist to do builds for us for different platforms. In order to prepare the server, we have to do the following:

```
sudo apt-get install build-essential mercurial pbuilder debootstrap devscripts debhelper ubuntu-dev-tools
```

The following take a while, so you probably want to do the following to speed them up: (as root)
```
echo "force-unsafe-io" > /etc/dpkg/dpkg.cfg.d/02apt-speedup
```

Once those are installed, we have to create the build environments:
```
pbuilder-dist lucid i386 create
pbuilder-dist lucid amd64 create
pbuilder-dist oneiric i386 create
pbuilder-dist oneiric amd64 create
```

Setup a source directory using hg clone:
```
hg clone https://code.google.com/p/dwarftherapist/
```

Building:
```
cd dwarftherapist
scripts/make-srctar lucid
cd ..
```


## References ##
  * [PbuilderHowto](https://wiki.ubuntu.com/PbuilderHowto)
  * [PackagingGuide](https://wiki.ubuntu.com/PackagingGuide/Complete)