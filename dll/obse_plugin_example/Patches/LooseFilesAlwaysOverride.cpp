#include "LooseFilesAlwaysOverride.h"
#include "obse_common/SafeWrite.h"
#include "ReverseEngineered/Files/Archive.h"
#include <string>
#include <io.h>

constexpr bool ce_shimTheCompareCall = false;

namespace SkyBSAPatches {
   namespace LooseFilesAlwaysOverride {
      //
      // The goal of this patch is to make loose files always override the contents 
      // of BSAs. We do this by simply patching the file timestamp check so that a 
      // loose file always appears "newer" than BSA files, invalidating the latter.
      //
      // --------------------------------------------------------------------------
      //
      // If the archive DOES NOT retain file and folder names, then the game will 
      // iterate over every loose file and folder when loading the archive, and 
      // invalidate any files in the archive that are older than their loose file 
      // counterparts. This is the only circumstance under which the game calls 
      // Archive::InvalidateAgainstLooseFiles, which carries out the process.
      //
      // If the archive DOES retain file and folder names, then the game will only 
      // apply ArchiveInvalidation when loading the BSA. Subsequent attempts to 
      // look up a file from the BSA will run a check for newer loose files that 
      // should override that file. This is done in Archive::CheckFileIsOverridden, 
      // which is in turn called by Archive::FolderContainsFile.
      //
      void Apply() {
         //
         // Fix loose file handling for BSAs with no retained names:
         //
         SafeWrite16(0x0042D4B3, 0x9090); // Archive::InvalidateAgainstLooseFiles+0x213
         //
         // Fix loose file handling on-demand:
         //
         WriteRelJump(0x0042C283, 0x0042C295); // Archive::CheckFileIsOverridden+0xB3
      }

      // other known CompareFileTime calls:
      // 0044A4B2 -- probably enforces timestamp effects on load order, not BSAs specifically
   }
}