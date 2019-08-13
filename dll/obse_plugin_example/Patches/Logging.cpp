#include "Logging.h"
#include "obse_common/SafeWrite.h"

namespace SkyBSAPatches {
   namespace Logging {
      void Log00404EE0(const char* format, ...) {
         va_list args;
         va_start(args, format);
         std::string formatPrefix("Log received from endpoint 0x00404EE0: ");
         formatPrefix += format;
         {  // Some of Bethesda's format messages are wrong.
            std::size_t found = formatPrefix.find("%S", 20);
            while (found != std::string::npos) {
               formatPrefix[found + 1] = 's';
               found = formatPrefix.find("%S", found);
            }
         }
         gLog.Log(IDebugLog::kLevel_Message, formatPrefix.c_str(), args);
         va_end(args);
      }

      void Apply() {
         WriteRelJump(0x00404EE0, (UInt32)&Log00404EE0);
      }
   }
}