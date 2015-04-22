#how to add support for new versions of DF to DT

# Mapping New Versions #
If you're quite brave and willing to get your hands a bit dirty you can try this method to add support for a new version of DF.

## In Dwarf Fortress ##
  1. Launch the new version of DF, generate a new world, and embark somewhere (you don't need to pick a good location or even wait for all the years of history, just get to fortress mode embark ASAP
  1. Prepare for the journey carefully
  1. customize the top dwarf in your list to have the nickname **0123456789abcdef**
  1. give him/her level 10 in mining and teaching
  1. embark, and pause the game

## In Dwarf Therapist ##
  1. Have DT connect to the new version of DF, it will say something about "I don't know how to talk to this version"
  1. Go read the DT logfile (`log/run.log`) and look for a line that says something about not finding a memory layout for checksum `0xXXXXXXXX`. That number is the checksum for the version you're trying to map.
  1. Copy the `etc/memory_layouts/windows/v<LATEST_OLD_VERSION>.ini` to `etc/memory_layouts/windows/v<NEWVERSION>.ini`
  1. Update the checksum in the new layout file to match what was found in the log in step 2
  1. under the `[info]` section of the new file you made, add the line `complete = false`
  1. Update each address in the `[addresses]` section to be `0x0`
  1. save the memory layout file
  1. Relaunch DT and see if it connects now. Note: You need to be in dwarf fortress mode, this won't work while you're in the main menu.
  1. If you connected ok, hit the "Scan Memory" button in DT's toolbar. You can try the buttons "Find Translation Vector" and "Find Dwarf Race Index" "Find Creature Vector" etc... This may or may not work, but it's what needs to be done. As you find new vectors, try putting them into the new layout file you made and then relaunch DT after each change to see if it works.
  1. If everything appears to be working, you will notice that Dwarf lastnames will not be showing correctly. This is because we still have `complete = false` in our new memory map. Remove that line, or set it to `true` to begin reading last names correctly.

It's a long tedious process, and it helps to understand how C++ allocates memory, but I've been working a bit on making this process easier with that scanner tool.