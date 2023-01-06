# MW_SPI_Mods
This directory contains files that have been modified in order to enable both SPI modules on any of the chip select pins for the Raspberry Pi being accessed with the Simulink SPI Master Transfer block. Pre-compiled \*.p files should be placed in the same directory alongside of the identically-named \*.m files.

Each of the files contains a comment in the header that gives the location of the file it should replace. Once these files have been replaced, any Simulink model with the Raspberry Pi SPI Master Transfer block should show the new functionality. To force any blocks in models already open to update, use Ctrl-D.
