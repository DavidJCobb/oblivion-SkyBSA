#include "obse/PluginAPI.h"
#include "obse/CommandTable.h"

#include "Patches/LooseFilesAlwaysOverride.h"
#include "Patches/BSAOverridingBSA.h"
#include "Patches/Logging.h"

IDebugLog    gLog("Data\\OBSE\\Plugins\\SkyBSA.log");
PluginHandle g_pluginHandle = kPluginHandle_Invalid;

extern "C" {
   bool OBSEPlugin_Query(const OBSEInterface* obse, PluginInfo* info) {
      // fill out the info structure
      info->infoVersion = PluginInfo::kInfoVersion;
      info->name        = "SkyBSA";
      info->version     = 0x01000000; // major, minor, patch, build

      {  // log our version number -- be helpful!
         auto v = info->version;
         UInt8 major = v >> 0x18;
         UInt8 minor = (v >> 0x10) & 0xFF;
         UInt8 patch = (v >> 0x08) & 0xFF;
         UInt8 build = v & 0xFF;
         _MESSAGE("SkyBSA Version %d.%d.%d, build %d.", major, minor, patch, build);
      }

      // version checks
      if(!obse->isEditor) {
         if(obse->obseVersion < OBSE_VERSION_INTEGER && obse->obseVersion < 21) {
            _ERROR("OBSE version too old (got %08X; expected at least %08X).", obse->obseVersion, OBSE_VERSION_INTEGER);
            return false;
         }
         #if OBLIVION
            if(obse->oblivionVersion != OBLIVION_VERSION) {
               _ERROR("incorrect Oblivion version (got %08X; need %08X).", obse->oblivionVersion, OBLIVION_VERSION);
               return false;
            }
         #endif
      } else {
         // no version checks needed for editor
      }
      // version checks pass
      return true;
   }

   bool OBSEPlugin_Load(const OBSEInterface* obse) {
      g_pluginHandle = obse->GetPluginHandle();
      //
      if (!obse->isEditor) {
         SkyBSAPatches::LooseFilesAlwaysOverride::Apply();
         SkyBSAPatches::BSAOverridingBSA::Apply();
         SkyBSAPatches::Logging::Apply();
         //
         // GOALS:
         //
         //  - Loose files should always override BSAs, regardless of timestamps.
         //
         //     - Needs testing.
         //
         //  - BSAs later in the load order should always override BSAs earlier in the load order.
         //
         //     - In vanilla, BSAs cannot invalidate files in other BSAs. However, if a loose file 
         //       invalidates one BSA (due to its timestamp), a file in another BSA can then take 
         //       priority over the loose file.
         //
         //     - In vanilla, when the game wants to look up files from the loaded BSAs, it loops 
         //       over the BSAs in load-order order to find the file. However, if the game finds 
         //       the file in a BSA, then that BSA is remembered: the next file lookup will check 
         //       that BSA first, out of order. Subroutine 0042EA60 does this search.
         //
         //     - We address this by patching LoadBSAFile to reverse the order of the BSA list in 
         //       memory (so last-loaded files are checked first), and patching 0042EA60 to avoid 
         //       remembering what BSA had the last requested file.
         //
         //     - Needs testing.
         //
      }
      return true;
   }
};
