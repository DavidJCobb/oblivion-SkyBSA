#include "LooseFilesAlwaysOverride.h"
#include "obse_common/SafeWrite.h"

namespace SkyBSAPatches {
   namespace LooseFilesAlwaysOverride {
      //
      // The goal of this patch is to make loose files always override the contents 
      // of BSAs. We do this by simply patching the file timestamp check so that a 
      // loose file always appears "newer" than BSA files, invalidating the latter.
      //
      LONG FakeCompare(const FILETIME* looseFileTime, const FILETIME* archiveTime) {
         return 1;
      }
      void Apply() {
         WriteRelCall(0x0042D4AB, (UInt32)&FakeCompare);
         SafeWrite8  (0x0042D4AB + 5, 0x90); // NOP
      }
   }
}