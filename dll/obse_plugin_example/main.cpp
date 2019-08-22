#include "obse/PluginAPI.h"
#include "obse/CommandTable.h"

#include "Patches/LooseFilesAlwaysOverride.h"
#include "Patches/BSAOverridingBSA.h"
#include "Patches/BSARedirection.h"
#include "Patches/Exploratory.h"
#include "Patches/Logging.h"

IDebugLog    gLog("Data\\OBSE\\Plugins\\SkyBSA.log");
PluginHandle g_pluginHandle = kPluginHandle_Invalid;

extern "C" {
   bool OBSEPlugin_Query(const OBSEInterface* obse, PluginInfo* info) {
      // fill out the info structure
      info->infoVersion = PluginInfo::kInfoVersion;
      info->name        = "SkyBSA";
      info->version     = 0x01010000; // major, minor, patch, build // REMINDER: Update SkyBSA.rc, which controls what people see if they right-click the DLL and go to Details

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
         _MESSAGE("We're loading in the game, not the editor.");
         SkyBSAPatches::LooseFilesAlwaysOverride::Apply();
         SkyBSAPatches::BSAOverridingBSA::Apply();
         SkyBSAPatches::BSARedirection::Apply();
         //SkyBSAPatches::Logging::Apply();
         SkyBSAPatches::Exploratory::Apply();
      }
      _MESSAGE("DLL successfully loaded.");
      return true;
   }
};
