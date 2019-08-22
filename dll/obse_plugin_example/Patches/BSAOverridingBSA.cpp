#include "BSAOverridingBSA.h"
#include "obse_common/SafeWrite.h"
#include "ReverseEngineered/Files/Archive.h"

namespace SkyBSAPatches {
   namespace BSAOverridingBSA {
      //
      // In Oblivion, BSAs cannot override each other. In fact, it appears that if 
      // multiple BSAs define the same file, it's impossible to predict which BSA 
      // will supply the file at any given time:
      //
      //  - When Oblivion wishes to look up a file from the loaded BSAs, it loops 
      //    over the BSAs in load-order order until it finds the file. However, if 
      //    the lookup succeeds, then Oblivion remembers the BSA that contained 
      //    the file... and the next lookup will check that BSA first, out of order.
      //
      // This patch is intended to make it so that BSAs that are loaded last will 
      // always override files in BSAs that are loaded earlier.
      //
      namespace NoCaching {
         //
         // Prevent Oblivion from remembering which BSA contained the last file 
         // that was successfully looked up from the loaded BSAs. This way, BSAs 
         // will always be iterated in order.
         //
         void Apply() {
            {  // FindBSAThatContainsFile+0x8B
               SafeWrite8(0x0042EAEB + 0, 0x31); // XOR ESI, ESI
               SafeWrite8(0x0042EAEB + 1, 0xF6);
               SafeWrite32(0x0042EAEB + 2, 0x90909090); // NOP; NOP; NOP; NOP
            }
            {  // FindFileInBSA+0x8B
               SafeWrite8(0x0042E96B + 0, 0x31); // XOR EBX, EBX
               SafeWrite8(0x0042E96B + 1, 0xDB);
               SafeWrite32(0x0042E96B + 2, 0x90909090); // NOP; NOP; NOP; NOP
            }
            {  // Subroutine 0x0042DDF0: static bool ArchiveManager::GetRandomFileNameForDirectory(...)
               SafeWrite8(0x0042DE89 + 0, 0x31); // XOR EBX, EBX
               SafeWrite8(0x0042DE89 + 1, 0xDB);
               SafeWrite32(0x0042DE89 + 2, 0x90909090); // NOP; NOP; NOP; NOP
            }
         }
      }
      namespace ReverseListOrder {
         //
         // Reverse the order of the BSA list, so that the last-loaded archives are 
         // at the front of the list instead of the end. This is needed because we 
         // can't patch the file lookup to reverse-iterate the list: it's a singly-
         // linked list, so you can only go forward.
         //
         void Apply() {
            WriteRelCall(0x0042F5C2, 0x00446CB0); // replace call to LinkedPointerList::Append with call to LinkedPointerList::Prepend
         }
      }
      //
      void Apply() {
         NoCaching::Apply();
         ReverseListOrder::Apply();
      }
   }
}