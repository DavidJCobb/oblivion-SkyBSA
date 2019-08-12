# oblivion-SkyBSA
 An attempt at a DLL that can force Oblivion to handle BSAs similarly to Skyrim.

## Oblivion BSA-handling behavior

Loose files only override BSA contents if the loose files have a newer timestamp.

BSAs are not intended to override each other, and no formal behavior is defined for the case where multiple BSAs define the same file. In practice, the result is unpredictable: when Oblivion wants to look up a file from the loaded BSAs, it first checks the last BSA that provided a requested file (out of order); if that BSA doesn't have the newly-requested file, then Oblivion loops over all loaded BSAs except for it, in order of earliest-loaded to latest-loaded.

## Skyrim BSA-handling behavior

Loose files always override BSA contents.

BSAs can override each other. BSAs that load late have priority over BSAs that load early.

## Oblivion vanilla implementation details

Loose file overrides occur by having the loose file "invalidate" the corresponding file inside of all BSAs. This doesn't rely on the same mechanisms as ArchiveInvalidation (i.e. the hash-maps, with their [well-documented problems](http://devakm.urikslargda.com/2006/05/archiveinvalidation-explained.html)), and should be considered reliable. Loose files are handled this way because when a file is requested, the game actually scans all loaded BSAs first, before scanning loose files.

BSA "overrides" are accidental and occur as a consequence of the (inconsistent) order in which the game scans BSAs for a requested file.

For further information, refer to the NorthernUI repo, where I keep my reverse-engineered findings for Oblivion.