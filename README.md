# oblivion-SkyBSA
 A DLL that forces Oblivion to handle BSAs versus loose files in the same manner as Skyrim.

## Oblivion BSA-handling behavior

Loose files only override BSA contents if the loose files have a newer timestamp.

BSAs are not intended to override each other, and no formal behavior is defined for the case where multiple BSAs define the same file. In practice, the result is unpredictable: when Oblivion wants to look up a file from the loaded BSAs, it first checks the last BSA that provided a requested file (out of order); if that BSA doesn't have the newly-requested file, then Oblivion loops over all loaded BSAs except for it, in order of earliest-loaded to latest-loaded.

## Skyrim BSA-handling behavior

Loose files always override BSA contents.

BSAs can override each other. BSAs that load late have priority over BSAs that load early.

## External links

The mod can be downloaded [from NexusMods](https://www.nexusmods.com/oblivion/mods/49568).

## Technical explanation

The game's internal class for BSA files is Archive, a subclass of BSFile. Archives don't store the full contents of all packed files in memory; instead, they store entries indicating the 64-bit hash of the folder/file name, and: for files, the offset of the file's contents inside of the BSA, and the file's size in bytes; for folders, a pointer to an array of the file entries in that folder. Folders are not nested in a BSA: given a path like "foo\bar\baz.dds," the Archive doesn't consider "bar" a folder inside of "foo;" rather, the Archive has two folders, one called "foo" and the other called "foo\bar."

Those aware of the BSA file format will know that every archive has flags specifying how the names of its files and folders should be handled. If an archive is flagged as having folder and file names, then when it loads, ArchiveInvalidation will be performed on it. If the archive is not flagged as having folder and file names, then the game will not perform ArchiveInvalidation on it; instead, the game will invalidate any files inside of the archive that have matching loose files with more recent Date Modified values (looping over every loose folder and file to run the appropriate checks).

This begs the question of how the game handles loose file behavior for BSAs that have file and folder names. The relevant checks are run on demand. Whenever the game must search a BSA for a specific file, it will use Archive::ContainsFolder to find the relevant folder entry, and then Archive::FolderContainsFile to find the relevant file entry. The latter function does a quick check for a matching loose file with a more recent Date Modified value, and if one is found, it invalidates the BSA-side file at that moment before then claiming that that file does not exist.

"Invalidation" here means that the BSA simply pretends that the relevant file doesn't exist. It does this by setting the file entry's offset to zero.

When the game wishes to load a file, it first checks all loaded BSAs for the file. Typically it checks BSAs in order from the earliest-loaded archive to the last-loaded archive, stopping when the file is found; if the file is found, then the game remembers the Archive that provided it; the next search will check that Archive first, out of order. If the file is not found in any Archives, then the game searches for a loose file.

### SkyBSA fixes

We patch the Date Modified check (CompareFileTime) used for BSAs that have no folder or file names embedded. We also patch the Date Modified check (_stat) used when looking files up from a BSA. In both cases we force the game to act as if the loose file is always newer.

We patch the function that searches all BSAs for a given file, to avoid preferring the last BSA that successfully supplied a file; all BSAs are now checked in whatever order they occupy in memory. Speaking of which...

The game maintains a singly-linked list of all Archive instances in memory, which is normally in the order the Archives are loaded (INI-specified files first; then BSAs paired with ESPs, matching the load order). Because this is a singly-linked list, it is impossible to iterate it in reverse order (i.e. latest-loaded first); as such, we simply reverse the order of the list so that the latest-loaded archives are at the front. Because those archives are now checked first, files that exist in multiple BSAs will be pulled from whichever BSA loaded last.

### The tests used to verify proper functionality are as follows:

Create a BSA that overrides all vanilla sky textures with a checkerboard pattern. Load this BSA last. If SkyBSA is working and the test BSA is successfully overriding the vanilla texture BSA, then the sky will appear as a checkerboard.

Create a set of loose files that override all vanilla sky textures with a checkerboard pattern, and tamper with them so that their Date Modified is midnight on January 1st, 1970, well before the Date Modified on the vanilla BSAs. If SkyBSA is working, then these files will override the vanilla texture BSA despite their dates, and the sky will appear as a checkerboard.

### Research on ArchiveInvalidation

On startup, the game will open ArchiveInvalidation.txt if it exists, and generate two lists of 64-bit hashes based on the file's contents: one list is for folder hashes, and the other is for file hashes. If a BSA performs ArchiveInvalidation during load, then it will invalidate all files or folders whose hashes are found within the respective list. This is the only circumstance under which ArchiveInvalidation is performed.

ArchiveInvalidation was originally recommended as a means to force loose files to override BSA-side files, but it was found to have numerous bugs. I haven't investigated the specific bugs that have been reported with ArchiveInvalidation -- namely, issues where invalidating, say, "textures/armor/glass/shield.dds" in an attempt to force a Glass Shield loose file to load would also invalidate every "shield.dds" file for every armor. These bugs are the reason for a more recent backlash against ArchiveInvalidation, with multiple guides advising people to avoid it or to prefer ArchiveInvalidation Invalidated instead. My research does establish that the ArchiveInvalidation logic only applies to files and folders listed in the ArchiveInvalidation text file; this contradicts some claims that have been made about ArchiveInvalidation bugs in the past based on black-box testing.

For the record: as of this writing, SkyBSA does not tamper with ArchiveInvalidation. Just turn ArchiveInvalidation off. If you have that text file, delete it.

[This file contains pidgin translations, from x86 to C++, of every single piece of code in the game that deals with ArchiveInvalidation.](https://github.com/DavidJCobb/oblivion-SkyBSA/blob/master/research/Everything%20related%20to%20ArchiveInvalidation.h)

### Research on BSA redirection

I have conclusively established that BSA Redirection has absolutely nothing to do with ArchiveInvalidation. One of the most popular BSA Redirection fixes is called "ArchiveInvalidation Invalidated" and its description states that BSA Redirection works by preventing the game from applying ArchiveInvalidation to all BSAs after the first to load; this explanation is categorically false as can be seen in the code link above.

BSA redirection solves two bugs that can cause issues with file handling. The first bug causes the game to always prefer vanilla BSAs (for a different reason than described above), whereas the second bug causes the game to ignore loose texture files even when they should override.

The game keeps track of the first Archive to load for each file type that Archives are meant to contain, e.g. the first texture BSA, the first mesh BSA, and so on. Again, this list is not used for ArchiveInvalidation. Under specific yet-to-be-determined circumstances, the game attempts to look up a file from these BSAs exclusively (this behavior shall hereafter be referred to as lazy lookups, and the relevant BSAs shall be referred to as lazy BSAs); if there is no lazy BSA for the desired filetype or if the lazy BSA does not contain the file, then the lazy lookup fails. Evidently the game responds to a lazy lookup failure by falling back to a normal lookup method; if that weren't the case, BSA redirection would cause missing texture errors basically everywhere.

(Lazy lookups are performed as part of a two-stage process. The game creates a "queued file" object e.g. QueuedFileEntry or QueuedTexture, which uses 64-bit hashes of the folder and filenames to find the metadata (offset and size) for a file in the relevant lazy BSA; later on, the archived file is loaded from the BSA using this metadata.)

Before I continue describing lazy lookups, there's something you need to know about file lookups in general. Generally speaking, Oblivion's code will ask an Archive instance whether it contains an entry that matches a 64-bit folder name hash and a 64-bit filename hash; however, Oblivion will also supply the original file path as a string, alongside these hashes, so that the Archive can perform loose file checks. Lazy lookups behave similarly: there is a central lazy lookup function that takes a filetype number (to specify the lazy BSA to use), a folder name hash, a filename hash, and the original file path, and it supplies the latter three parameters to the appropriate lazy BSA. This means that *if the game does a lazy lookup properly*, then lazy lookups should only cause problems in the case of two BSAs having different versions of the same file (because the lazy BSA, which will inevitably be a vanilla one if you don't use BSA redirection, will be preferred); BSA redirection will solve this problem, but to no effect, since a different bug causes the exact same thing to happen anyway (see top of article).

Moving on: most lazy lookups are performed properly, specifying the folder hash, file hash, and full file path as a string. However, there are exceptions. Some forms can have MODT, MO2T, MO3T, MO4T, or NIFT subrecords, which specify folder and file hashes for textures, but not file paths. When these subrecords load, they perform a lazy lookup using only the hashes, which means that the game cannot and does not check for loose files. This is the bug that BSA redirection visibly solves, by causing all lazy texture lookups to fail.

I have been referred to [Strand Magic](https://www.nexusmods.com/oblivion/mods/48460) as a reproducible test case for BSA redirection, and I can confirm that without BSA redirection or an equivalent, this mod's textures fail to load.

At this point, I'm capable of recreating the effect of BSA redirection entirely programmatically, without the need for dummy BSA files or changes to sArchiveList. However, I don't know whether I should do it for texture files only or for all file types. Typical BSA redirection setups only apply to texture files, which fixes loose file handling but doesn't fix conflicts between BSAs; applying redirection to all filetypes would fix conflicts between BSAs, but could worsen performance slightly.

### Miscellaneous information

BSA file headers contain a list of flags indicating the file types that a BSA contains. These flags appear to serve no other purpose than to speed up file lookups: if for example the game is searching through all BSAs in search of a texture file, the very first thing it does is check whether a given BSA is flagged as containing textures; if not, then the BSA can be skipped. 
