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
      // If the archive retains file and folder names, then the game will only use 
      // ArchiveInvalidation.txt to invalidate files. If the archive doesn't retain 
      // names, then the game will not use ArchiveInvalidation.txt on it and will 
      // instead loop over all loose files, matching them against the files in the 
      // BSA data and invalidating files in the BSA that are outdated compared to 
      // a loose file.
      //
      UInt32 _invalidateOlderFiles(RE::Archive* bsa) {
         //
         // TODO: UNFINISHED
         //
         // This function loops over all known files and folders in all loaded 
         // BSAs. If any given folder path exists, then we loop over all files 
         // that the BSA has in that folder and check if matching loose files 
         // exist.
         //
         UInt32 invalidated = 0;
         UInt32 di = 0;
         do {
            auto& directory = bsa->folders[di];
            auto  name = CALL_MEMBER_FN(bsa, LoadFolderNames)(di); // this may actually be "read single folder name"
            if (name) {
               std::string directoryPath = "Data\\";
               directoryPath += name;
               if (_access(directoryPath.c_str(), 0) == -1)
                  continue;
               for (UInt32 fi = 0; fi < directory.count; fi++) {
                  auto& file  = directory.files[fi];
                  auto  names = bsa->fileNameOffsetsByFolder;
                  if (names) {
                     auto fnames = names[di];
                     if (fnames) {
                        auto offset = fnames[fi];
                        const char* filename = &bsa->fileNames[offset];
                        std::string filePath = directoryPath;
                        filePath += "\\";
                        filePath += filename;
                        if (_access(directoryPath.c_str(), 0) == -1)
                           continue;
                        file.invalidate();
                     }
                  }

               }
            }
         } while (++di < bsa->header.directoryCount);
         return invalidated;
      }
      //
      void Apply() {
         //
         // Force BSAs to always scan local files even if they retain names:
         //
         SafeWrite8  (0x0042E300, 0xEB); // JNZ SHORT -> JMP SHORT
         WriteRelJump(0x0042E322, 0x0042E51A);
         //
         // Fix loose file handling for BSAs with no retained names:
         //
         SafeWrite16(0x0042D4B3, 0x9090); // alternatively, NOP out the jump based on the comparison
      }

      // other known CompareFileTime calls:
      // 0044A4B2 -- probably enforces timestamp effects on load order, not BSAs specifically
   }
}