#include "Benchmarking.h"
#include "obse_common/SafeWrite.h"
#include "ReverseEngineered\Files\Archive.h"

namespace SkyBSAPatches {
   namespace Exploratory {
      namespace Benchmarking {
         constexpr char* ce_looseTexture = "textures\\menus\\northernui\\title_loading_screen.dds";
         constexpr UInt32 ce_lookupCount = 100000;
         
         void Inner() {
            UInt32 start = GetTickCount();
            for (UInt32 i = 0; i < ce_lookupCount; i++) {
               RE::FindBSAThatContainsFile(ce_looseTexture, RE::kBSAFiletypeFlag_Textures);
            }
            UInt32 end = GetTickCount() - start;
            _MESSAGE("BENCHMARK: %s\n%d thorough lookups took %f seconds.", ce_looseTexture, ce_lookupCount, (double)end / 1000.0);
            //
            start = GetTickCount();
            for (UInt32 i = 0; i < ce_lookupCount; i++) {
               RE::BSHash file;
               RE::BSHash folder;
               RE::HashFilePath(ce_looseTexture, folder, file);
               RE::LazyFileLookup(RE::kBSAFiletype_Textures, folder, file, ce_looseTexture);
            }
            end = GetTickCount() - start;
            _MESSAGE("BENCHMARK: %s\n%d lazy lookups took %f seconds.", ce_looseTexture, ce_lookupCount, (double)end / 1000.0);
            //
            RE::BSHash file;
            RE::BSHash folder;
            RE::HashFilePath(ce_looseTexture, folder, file);
            start = GetTickCount();
            for (UInt32 i = 0; i < ce_lookupCount; i++) {
               RE::LazyFileLookup(RE::kBSAFiletype_Textures, folder, file, ce_looseTexture);
            }
            end = GetTickCount() - start;
            _MESSAGE("BENCHMARK: %s\n%d lazy lookups took %f seconds with hashes pregenerated.", ce_looseTexture, ce_lookupCount, (double)end / 1000.0);
         }
         __declspec(naked) void Outer() {
            _asm {
               mov  eax, 0x00404EE0; // reproduce patched-over call to a logging function
               call eax;             //
               call Inner;
               mov  eax, 0x0042F9F0;
               jmp  eax;
            };
         }
         void Apply() {
            WriteRelJump(0x0042F9EB, (UInt32)&Outer);
         }
      }
   }
}