#include "Logging.h"
#include "obse_common/SafeWrite.h"

namespace SkyBSAPatches {
   namespace Logging {
      namespace LogLazyLokups {
         void _stdcall Inner(const char* path) {
            if (path)
               _MESSAGE("Detected lazy lookup to: %s", path);
            else
               _MESSAGE("Detected lazy lookup, but no path was given.");
         }
         __declspec(naked) void Outer() {
            _asm {
               mov  edx, dword ptr [esp + 0x10];
               push edx;
               call Inner; // stdcall
               mov  edx, dword ptr [esp + 4];       // reproduce patched-over instructions
               mov  eax, 0x00B338E8;                //
               mov  ecx, dword ptr [edx * 4 + eax]; //
               mov  eax, 0x0042DB1B;
               jmp  eax;
            };
         }
         void Apply() {
            WriteRelJump(0x0042DB10, (UInt32)&Outer);
         }
      }

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
         LogLazyLokups::Apply();
      }
   }
}