#pragma once
#include "obse_common/SafeWrite.h"
#include "ReverseEngineered/Files/Archive.h"

namespace SkyBSAPatches {
   namespace BSARedirection {
      //
      // BSA Redirection disables an incompletely-understood optimization inside of the 
      // engine that the community previously believed was part of ArchiveInvalidation. 
      // This optimization keeps track of the first BSA to load for every filetype; 
      // there are specific parts of the game engine that will always/only/????? check 
      // that specific BSA. BSA Redirection attempts to ensure that the first BSA to 
      // load for textures is empty, such that all lookups from that BSA fail.
      //
      // Forcing the "first BSA to load for this file type" to nullptr should have the 
      // same effect.
      //
      // NOTE: When testing with a version of this code that disabled this optimization 
      // for ALL file types, I experienced a significant performance degradation when 
      // traveling through the game world -- no doubt the result of LOD lookups being 
      // slowed down. Current code does this only for textures, consistent with the 
      // "ArchiveInvalidation Invalidated" BSA Redirection used by many.
      //
      void _stdcall Inner(RE::Archive* loaded) {
         UInt32 type = 0;
         do {
            if (type == 1) // BSA-redirect textures
               continue;
            if (loaded->header.flags & (1 << type))
               if (!RE::g_firstLoadedArchivesByType[type])
                  RE::g_firstLoadedArchivesByType[type] = loaded;
         } while (++type < 9);
      }
      __declspec(naked) void Outer() {
         _asm {
            xor  ecx, ecx; // reproduce patched-over instruction, even though we don't need it
            push eax;
            call Inner; // stdcall
            mov  eax, 0x0042F999;
            jmp  eax;
         }
      }
      void Apply() {
         WriteRelJump(0x0042F96B, (UInt32)&Outer);
      }
   }
}