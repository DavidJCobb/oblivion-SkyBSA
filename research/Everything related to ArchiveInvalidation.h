
// This file contains full translations, from x86 to almost-C++, for every single subroutine 
// in the game that is related to ArchiveInvalidation -- everything from the code to load each 
// individual BSA based on sArchiveList and Plugins.txt, to the code that actually performs 
// ArchiveInvalidation.
// 
// Think of this file as a "pidgin language" version of C++. Among other things, it's not 
// strict about types and variable name reuse; my preference was for the code to be readable 
// and recognizable to C++ developers, while still accurately indicating what registers are 
// used for what in any case where I could do so without compromising readability.
//
// For class/struct definitions, refer to Archive.h in NorthernUI's source code.

constexpr UInt32 ce_bsaSignatureBSwapped = '\0ASB'; // 'BSA\0' swapped

// All loaded Archive instances:
LinkedPointerList<Archive>* const g_archiveList = (LinkedPointerList<Archive>*) 0x00B338E0;

//
// These two lists are used for ArchiveInvalidation. As you can see below, the 
// ArchiveInvalidation.txt file is used to populate these lists and performs no 
// other function. All references to these lists have been translated and are 
// included in this file; there is no other program code in Oblivion.exe that 
// accesses these lists.
//
NiTArray<BSHash>* const g_archiveInvalidatedFilenames      = (NiTArray<BSHash>*) 0x00B33930;
NiTArray<BSHash>* const g_archiveInvalidatedDirectoryPaths = (NiTArray<BSHash>*) 0x00B33934;

void ReadArchiveInvalidationTXTFile(const char* filepath) {
   auto ebp = 0;
   BSFile* ebx = sub00431690(filepath, 0, 0x2800);
   if (!ebx)
      return;
   if (ebx->OpenFile(0, 0)) { // at 0x0042D8AC
      char line[MAX_PATH]; // esp1C
      eax = ebx->Unk_0A(&line, MAX_PATH, '\r'); // most likely "read until"
      if (eax) {
         do {
            edi = 1;
            char esp1B;
            esp14 = 1;
            (ebx->m_readProc)(ebx, &esp1B, 1, &esp14, 1); // are we skipping the '\n' expected to follow every '\r'?
            eax = strstr(&line, "\\");
            if (eax) { // if the path includes subfolders
               if (!*g_archiveInvalidatedDirectoryPaths) {
                  *g_archiveInvalidatedDirectoryPaths = new NiTArray<BSHash>; // inlined
               }
               esi = &line;
               if (line[0] == '\\')
                  esi = &line[1];
               auto esp14 = new BSHash(esi, edi);
               auto ecx = *g_archiveInvalidateDirectoryPaths;
               auto esi = ecx->unk0A;
               if (esi >= ecx->unk08)
                  ecx->SetCapacity(ecx->unk0E + esi);
               ecx->AddAtIndex(esi, &esp14);
            } else { // path is just a filename
               if (!*g_archiveInvalidatedFilenames) {
                  *g_archiveInvalidatedFilenames = new NiTArray<BSHash>; // inlined
               }
               auto esp14 = new BSHash(&line, ebp);
               auto ecx = *g_archiveInvalidatedFilenames;
               auto esi = ecx->unk0A;
               if (esi >= ecx->unk08)
                  ecx->SetCapacity(ecx->unk0E + esi);
               ecx->AddAtIndex(esi, &esp14);
            }
         } while (eax = ebx->Unk_0A(&line, MAX_PATH, '\r'));
      }
   }
   ebx->Dispose(true);
}

UInt32 Archive::InvalidateAgainstLooseFiles(const char* pathRoot, const char* pathDeep, FILETIME* thisArchiveLastModified) {
   auto ebx = this;
   char esp17C[MAX_PATH]; // stack 0x17C - 0x280
   esp20 = pathRoot;
   esp24 = pathDeep;
   esp1C = thisArchiveLastModified;
   UInt32 esp14 = 0; // number of files invalidated
   UInt32 esp18;     // BSA folder index
   strcpy(&esp17C, pathRoot); // inlined
   memcpy(&esp17C + strlen(esp17C), pathDeep, strlen(pathDeep)); // inlined string append
   memcpy(&esp17C + strlen(esp17C), "*.*", 4); // inlined string append
   WIN32_FIND_DATAA esp3C;
   auto esi   = FindFirstFileA(&esp17C, &esp3C); // at 0x0042D35F
   auto esp28 = esi;
   if (esi == -1)
      return 0;
   char esp280[MAX_PATH];
   strcpy(&esp280, pathDeep);
   auto eax = strlen(esp280);
   if (eax & 0xFF > 1) // at 0x0042D3A5
      esp280[(eax & 0xFF) - 1] = '\0';
   auto esp34 = BSHash(&esp280, 2);
   bool esp13 = false;
   //
   // esp13 == "does this exact subfolder path exist in our BSA?"
   //
   // Say we have a path "a/b/c/d/" and the function is currently being 
   // called on loose folder "a/b/c/". If we don't have any files in 
   // "a/b/c/", then we won't have a folder called "a/b/c". We still 
   // need to loop over the loose "a/b/c/" however, in order to catch 
   // its subfolders including the ".../d/" subfolder.
   //
   // esp13, then, signals to the loop code whether we should pay 
   // attention to files in the folder we are currently looping in.
   //
   if (this->ContainsFolder(&esp34, &esp18, 0))
      esp13 = true;
   UInt32 ebp = esp18;
   do { // at 0x0042D3F0
      if ((esp3C.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && esp3C.cFileName[0] != '.') {
         strcpy(esp17C, pathDeep);
         //auto eax = strlen(&esp3C.cFileName);
         memcpy(&esp17C + strlen(esp17C), esp3C.cFileName, strlen(esp3C.cFileName)); // inlined string append
         memcpy(&esp17C + strlen(esp17C), "\\", 1); // inlined string append
         esp14 += this->InvalidateAgainstLooseFiles(pathRoot, &esp17C, thisArchiveLastModified); // recurse
         esi = esp28;
         continue;
      }
      if (!esp13) // at 0x0042D49F
         continue;
      if (CompareFileTime(&esp3C.ftLastWriteTime, thisArchiveLastModified) <= 0) // call at 0x0042D4AB
         continue;
      auto esp2C = BSHash(&esp3C.cFileName, 0);
      if (!this->FolderContainsFile(ebp, &esp2C, &esp18, 0, 0)) // loose file date comparison is run in here
         continue;
      eax = esp18; // BSA file index in folder
      this->folders[ebp]->files[esp18]->offset &= 0x80000000; // or folders[esp18]->files[ebp]?
      esp14++;
   } while (FindNextFileA(esi, &esp3C)); // at 0x0042D50F
   FindClose(esi);
   return esp14;
}

UInt32 Archive::InvalidateOlderFiles() {
   auto edi   = this;
   auto esp0C = this;
   auto esp10 = 0;
   if (!*g_archiveInvalidatedDirectoryPaths || !*g_archiveInvalidatedFilenames)
      return 0;
   if (this->header.flags & kBSAFlag_HasFolderNames && this->header.flags & kBSAFlag_HasFileNames) {
      esp20 = 0;
      if (this->header.directoryCount <= 0)
         return 0;
      esp1C = 0; // == loop var * sizeof(BSAEntry)
      do {
         auto eax = *g_archiveInvalidatedDirectoryPaths;
         auto ebp = this->folders[esp20];
         auto esp14 = ebp;
         if (eax) {
            auto ebx = eax->unk0A;
            auto esi = 0;
            if (ebx > 0) {
               do {
                  if a hash matches {
                     // jumped to 0x0042E4A1
                     //
                     // Invalidate the whole folder:
                     //
                     // ecx == 0
                     if (ebp->count > 0) {
                        do {
                           ebp->files[edx].invalidate(); // offset &= 0x80000000
                        } while (++ecx < ebp->count);
                     }
                     esp10 += ebp->count;
                     goto lNextFolder;
                  }
               } while (++esi < ebx);
            }
         }
         // at 0x0042E3BA
         UInt64 esp40 = 'Data\\\0\0\0';
         const char* esi = this->LoadFolderNames(esp20); // load a single folder's name
         auto edi = &esp40;
         auto eax = strlen(esi);
         auto ecx = strlen("Data\\");
         memcpy(edi, esi, ecx); // inlined
         esi = 0;
         auto eax = _access(&esp40, 0); // file existence check
         if (eax == -1)
            continue;
         esp28 = 0;
         if (ebp->count <= 0)
            continue;
         // reminder: ebp == this->folders[esp20]
         do { // at 0x0042E440
            auto eax = *g_archiveInvalidatedFilenames;
            auto edx = ebp->files[esp28];
            if (!eax)
               continue;
            SInt32 esp2C = eax->unk08;
            if (esp2C <= 0)
               continue;
            do {
               if a hash matches {
                  // jumped to 0x0042E4D1
                  edx->invalidate(); // offset &= 0x80000000
                  esp10 += 1;
                  goto lNextFile; // implicit
               }
            } while (++edi < esp2C);
            continue;
            // ... the code for 0x0042E4A1 is here ...
            lNextFile: // at 0x0042E4DD
         } while (++esp28 < ebp->count);
         lNextFolder: // at 0x0042E4F6
      } while (++esp20 < this->header.directoryCount);
      return esp10;
   }
   // at 0x0042E51A
   //
   // Code below runs only for BSAs with no file/folder names retained:
   //
   Log004A7A60("bInvalidateOlderFiles is true in the INI file, but the archive doesn't have directory or file strings.  This is going to be really really slow.");/
   WIN32_FIND_DATAA esp38;
   auto eax = FindFirstFileA(&this->m_path, &esp38);
   if (eax == -1) {
      Log004A7A60("Could not find Archive %s to get file time.", &this->m_path);
   } else
      FindClose(eax);
   return this->InvalidateAgainstLooseFiles("Data\\", "", &esp38.ftLastWriteTime);
}

//
// This is the constructor for loaded BSA data. This is ONLY called by LoadBSAFile, 
// further below, which means that that function is the only one that loads an 
// archive into memory.
//
Archive::Archive(Arg1, UInt32 Arg2, Arg3, Arg4) {
   // constructor call for this->header at 0x0042EED4
   this->BSFile(Arg1, 0, Arg2, 0); // superclass constructor
   auto esp16C = 0; // unused var, probably a leftover from debugging; Oblivion has a lot like this
   this->unk200.Initialize(); // Bethesda wrapper around CRITICAL_SECTION
   esp16C = 1;
   this->unk18C = -1;
   this->unk190 = -1;
   this->folders = nullptr;
   this->folderNames = nullptr;
   this->folderNameOffsets = nullptr;
   this->fileNames = nullptr;
   this->fileNameOffsetsByFolder = nullptr;
   this->unk194 = 0;
   this->unk1A8 = 0;
   this->unk188 = 0;
   this->unk1AC = 0;
   if (!this->m_good) // at 0x0042EF5B
      return;
   if (Arg3) // the constructor is never called with a non-zero Arg3
      this->unk194 = 8;
   this->SetReadWriteFuncs(false);
   //
   // Read file header:
   //
   (this->m_readProc)(Arg1, &this->header, 0x24, &esp1C, 1);
   if (this->header.unk00 = ce_bsaSignatureBSwapped) {
      if (this->header.version <= 0x67) {
         if (!(this->unk194 & 8)) { // at 0x0042EFB5
            auto esp28 = GetTickCount();
            Log00404EE0("Loading archive %s", Arg1);
            auto esp20 = malloc(this->header.directoryCount * sizeof(BSAEntry));
            esp16C = 2;
            if (esp20)
               ArrayForEachOf(esp20, sizeof(BSAEntry), this->header.directoryCount, BSAEntry::BSAEntry);
            esp16C = 1;
            this->folders = esp20;
            this->Read(esp20, this->header.directoryCount * sizeof(BSAEntry));
            if (this->header.flags & kBSAFlag_HasFolderNames) {
               if (this->sub0042BD70()) { // at 0x0042F047
                  this->unk194 |= 0x10;
                  this->folderNames = malloc(this->header.totalFolderNameLength);
                  this->folderNameOffsets = malloc(this->header.directoryCount * 4);
               }
            }
            // at 0x0042F089
            if (this->header.flags & kBSAFlag_HasFileNames) {
               if (this->sub0042BD70()) { // at 0x0042F09E
                  this->unk194 |= 0x20;
                  this->fileNames = malloc(this->header.totalFileNameLength);
                  this->fileNameOffsetsByFolder = malloc(this->header.directoryCount * 4);
               }
            }
            // at 0x0042F0E0
            edi = 0;
            esp1C = 0;
            esp18 = 0;
            if (this->header.directoryCount > 0) {
               auto ebx = 0;
               do {
                  if (this->header.flags & kBSAFlag_HasFolderNames) {
                     esp20 = 1;
                     (this->m_readProc)(this, &esp17, 1, &esp20, 1);
                     if (this->sub0042BD70()) { // at 0x0042F135
                        this->Read(this->folderNames + edi, esp17);
                        this->folderNameOffsets[esp18] = edi;
                        edi += esp17;
                        esp1C = edi;
                     } else {
                        this->Read(&esp60, esp17);
                     }
                  }
                  // at 0x0042F179
                  auto ebp   = this->folders[ebx].count;
                  auto esp20 = malloc(ebp * sizeof(BSAEntry));
                  esp16C = 3;
                  if (esp20)
                     ArrayForEachOf(esp20, sizeof(BSAEntry), ebp, BSAEntry::BSAEntry);
                  auto edx = this->folders[ebx].count;
                  esp16C = 1;
                  esp20 = 1;
                  (this->m_readProc)(this, esp20, edx * 4, &esp20, 1);
                  this->folders[ebx].files = esp20;
               } while (++esp18 < this->header.directoryCount);
               ebx = esp24;
            }
            // at 0x0042F219
            if (this->header.flags & kBSAFlag_HasFileNames) {
               auto eax = this->m_pos;
               if (eax == -1)
                  eax = this->m_pos2;
               this->unk188 = eax;
               if (this->sub0042BD70()) {
                  this->Read(this->fileNames, this->header.totalFileNameLength);
                  auto ebx = 0;
                  auto esp18 = 0;
                  if (this->header.directoryCount > 0) {
                     auto ebp = 0;
                     do {
                        eax = this->folders[ebp].count;
                        this->fileNameOffsetsByFolder[ebp] = malloc(eax * sizeof(UInt32));
                        edx = this->folders[ebp];
                        ecx = 0;
                        if (edx->count <= 0)
                           continue;
                        do {
                           edx = this->fileNameOffsetsByFolder[esp18];
                           edx[ecx] = edi;
                           auto eax = this->fileNames + edi;
                           edi = edi + eax + 1;
                        } while (++ecx < this->folders[ebp].count);
                     } while (++esp18, ++ebp < this->header.directoryCount);
                  }
                  ebx = esp24; // at 0x0042F30F
               }
            }
            // at 0x0042F313
            auto eax = GetTickCount() - esp28;
            Log00404EE0("Finished loading archive %s containing %i directories and %i files in %f seconds", ebx, edx, ecx, (double)eax);
            ebp = 0;
         }
         // at 0x0042F359
         auto eax = sub00983754(&this->m_path, &esp30);
         if (eax == -1)
            Log004A7A60("Could not find Archive %s to get filetime.", &this->m_path);
         if (!(this->unk194 & 8)) {
            if (INI::Archive::bInvalidateOlderFiles->b) {
               auto edi = GetTickCount();
               Log00404EE0("Invalidating files in archive %s", ebx);
               auto ebp = this->InvalidateOlderFiles();
               Log00404EE0("Finished invalidating %i files in archive %s in %f seconds", ebp, ebx, (double)(GetTickCount() - edi));
               ebp = 0;
            }
            // at 0x0042F3FD
            if (Arg4) { // the constructor is never called with a non-zero Arg4
               esp1C = 0;
               if (this->header.directoryCount > 0) { // at 0x0042F413, and then redundantly at 0x0042F429
                  do {
                     ebx = esp1C << 4;
                     edi = 0; // == ebp * sizeof(BSAEntry)
                     do {
                        // at 0x0042F432
                        UInt32 ecx = this->header.fileFlags;
                        auto   edx = this->folders[esp1C]->files;
                        InvalidateFileInAllLoadedBSAs(&this->folders[esp1C]->hash, &edx[ebp]->hash, ecx);
                        //
                        // Every BSAEntry* is also a BSHash*, you see.
                        //
                     } while (++ebp < this->header.directoryCount);
                  } while (++esp1C < this->header.directoryCount);
               }
            }
         }
         // at 0x0042F473
         //
         // This next function call resizes the BSFile buffer for this archive i.e. 
         // the amount of file data we persist in RAM at any given moment. BSFile 
         // doesn't read the entire file into memory all at once; rather, it will 
         // keep a certain amount in RAM at any given time for quick access.
         //
         this->sub004303F0(0x2800);
         return;
      }
   }
   this->unk194 |= 1;
}

// only affects BSAs in g_archiveList, so Archives can use it while loading; 
// in practice, none should actually end up using it
void InvalidateFileInAllLoadedBSAs(BSHash* folder, BSHash* file, UInt16 filetypeFlags) {
   auto ebx = *g_archiveList;
   if (!ebx)
      return;
   auto ebp = file;
   UInt32 esp10; // folder index
   UInt32 esp14; // file index
   do {
      if (!ebx->data && !ebx->next)
         return;
      Archive* esi = ebx->data;
      UInt16 ax = filetypeFlags;
      if (!(esi->header.fileFlags & ax))
         continue;
      if (!esi->ContainsFolder(folder, &esp10, 0)) // at 0x0042EE35
         continue;
      if (!esi->FolderContainsFile(esp10, ebp, &esp14, 0, 0))
         continue;
      esi->folders[esp10]->files[esp14].invalidate();
   } while (ebx = ebx->next);
}

Archive* LoadBSAFile(const char* filepath, UInt16 /*always zero*/overrideFiletypeFlags, unknown_type /*always zero*/Arg3) {
   if (!INI::Archive::bUseArchive->b)
      return nullptr;
   auto ecx = *g_FileFinder;
   if (!ecx)
      return nullptr;
   // auto esi = &esp0C;
   auto eax = ecx->FindFile(filepath, &esp0C, 1, -1);
   if (!eax)
      return nullptr;
   auto esp08 = new Archive(&esp0C, 0x40000, 0, Arg3);
   auto esi   = esp08;
   if (!esi) // at 0x0042F57F
      return nullptr;
   if (esi->m_good && !(esi->unk194 & 1)) {
      auto ecx = *g_archiveList;
      if (!ecx) {
         *g_archiveList = new ...; // blah blah blah; inlined
      }
      ecx->Prepend(esi);
      if (overrideFiletypeFlags)
         esi->header.fileFlags = overrideFiletypeFlags;
   } else {
      esi->Dispose(true);
      esi = nullptr;
   }
   return esi;
}

void sub0042BE70() {
   //
   // This function tears down all BSA-related state INCLUDING the loaded BSA 
   // data itself. Its caller does things like saving all control settings and 
   // deleting the current OSInputGlobals instance, which means that this ONLY 
   // runs when Oblivion.exe is shutting down.
   //
   auto ecx = *g_archiveList;
   auto edi = 0;
   if (ecx) {
      for(auto esi = ecx; esi; esi = esi->next) {
         auto ecx = esi->data;
         if (ecx)
            ecx->Dispose(true);
      }
      (*g_archiveList)->~LinkedPointerList();
      delete *g_archiveList; // actually FormHeap_Free
      *g_archiveList = nullptr;
   }
   // at 0x0042BEB8
   auto ecx = *g_archiveInvalidatedFilenames;
   if (ecx) {
      for(UInt32 esi = 0; esi < (*g_archiveInvalidatedFilenames)->unk0A; ++esi) {
         auto ecx = *g_archiveInvalidatedFilenames[esi];
         delete ecx; // actually FormHeap_Free
      }
      (*g_archiveInvalidatedFilenames)->Dispose(true);
   }
   // at 0x0042BEF8
   auto ecx = *g_archiveInvalidatedDirectoryPaths;
   if (ecx) {
      for(UInt32 esi = 0; esi < (*g_archiveInvalidatedDirectoryPaths)->unk0A; ++esi) {
         auto ecx = *g_archiveInvalidatedDirectoryPaths[esi];
         delete ecx; // actually FormHeap_Free
      }
      (*g_archiveInvalidatedDirectoryPaths)->Dispose(true);
   }
   // at 0x0042BF00
   *(UInt32*)(0x00B338E8) = 0;
   *(UInt32*)(0x00B3390C) = 0;
   *(UInt32*)(0x00B338EC) = 0;
   *(UInt32*)(0x00B33910) = 0;
   *(UInt32*)(0x00B338F0) = 0;
   *(UInt32*)(0x00B33914) = 0;
   *(UInt32*)(0x00B338F4) = 0;
   *(UInt32*)(0x00B33918) = 0;
   *(UInt32*)(0x00B338F8) = 0;
   *(UInt32*)(0x00B3391C) = 0;
   *(UInt32*)(0x00B338FC) = 0;
   *(UInt32*)(0x00B33920) = 0;
   *(UInt32*)(0x00B33900) = 0;
   *(UInt32*)(0x00B33924) = 0;
   *(UInt32*)(0x00B33904) = 0;
   *(UInt32*)(0x00B33928) = 0;
   *(UInt32*)(0x00B33908) = 0;
   *(UInt32*)(0x00B3392C) = 0;
}

//
// And below, the only call to LoadBSAFile:
//

bool sub0042F610() {
   memset(0x00B338E8, 0, 0x48);
   bool esp03 = true;
   if (INI::Archive::bUseArchives!= 1)
      return true;
   ReadArchiveInvalidationTXTFile(INI::Archive::SInvalidationFile->s);
   auto edi = 0;
   auto esp1C = 0;
   auto esi;
   do {
      edi = esp1C;
      esi = esp1C + 1;
      if (esi >= 0x18)
         continue;
      ebx = edi * 8;
      do {
         //
         // ...
         //
      } while (++esi < 0x18); // at 0x0042F6BB
      if (esp1C != 0) {
         //
         // ...
         //
      }
      esi = esp14;
   } while ((esp1C = esi) < 0x17);
   auto esp18 = GetTickCount();
   Log00404EE0("Loading master archives");
   auto ebp = malloc(32768); // this will store a comma-separated list of ALL archives to load
   strcpy(ebp, INI::Archive::SArchiveList); // inlined
   FormatString(&esp350, "%sPlugins.txt", ".\\");
   FILE esi = fopen(&esp350, "r");
   auto esp14 = esi;
   if (esi) { // at 0x0042F782
      auto eax = sub0098474B(esi); // funcs at 0x0098xxxx are typically standard library stuff
      if (eax == 0) {
         do {
            char esp24C[MAX_PATH];
            getline(&esp24C, MAX_PATH, esi);
            char al = esp24C[0];
            if (al == '#')
               continue;
            if (al == '\0')
               continue;
            if (al == '\n')
               continue;
            char* eax = strstr(eax, ".esp");
            if (eax == 0)
               continue;
            strcpy(eax, "*.bsa");
            //
            // ...
            //
            auto ebx = sub009844EC(&esp148, &esp20); // at 0x0042F86C
            if (ebx == -1)
               continue;
            do {
               auto eax = strlen(ebp); // ebp == archive list
               if (ebp[eax - 2] == '\n') {
                  ebp[strlen(ebp) - 2] = '\0';
               }
               //
               // inlined: append ", " to the end of (ebp)
               //
               //
               // inlined: append (esp44) to (ebp)
               //
            } while (eax = sub0098461C(ebx, &esp20));
         } while (eax = sub0098474B(esp14));
      }
   }
   //
   // ebp is now a comma-separated list of all archives to load, with the 
   // ones in sArchiveList first and the ones indicated by Plugins.txt 
   // (i.e. every BSA associated with an ESP file) after that.
   //
   auto eax = strtok(ebp, ","); // string split? string find?
   if (eax) { // at 0x0042F943
      do {
         const char* ecx = eax;
         do {
            char al = *ecx;
            if (al == ' ')
               continue;
            if (al == '\t')
               continue;
            if (al != '\n')
               break;
         } while (++ecx, true);
         Archive* eax = LoadBSAFile(ecx, 0, 0);
         if (!eax) {
            esp13 = false;
            continue;
         }
         UInt32 ecx = 0;
         do {
            //
            // For each BSA filetype flag, remember the first BSA to load 
            // with that flag -- so, the first texture BSA, the first mesh 
            // BSA, and so on.
            //
            // I suspect that THIS is what makes "BSA Redirection" actually 
            // influence the game. At present, I still don't know what it 
            // actually DOES -- I don't know where this list of BSAs is 
            // used -- but I know that per all the above, it has absolutely 
            // nothing whatsoever to do with ArchiveInvalidation.
            //
            if (eax->header.fileFlags & (1 << ecx))
               if (!(0x00B338E8)[ecx])
                  (0x00B338E8)[ecx] = eax;
         } while (++ecx < 9);
      } while (eax = strtok(nullptr, ","));
   }
   delete ebp;
   eax = GetTickCount() - esp18;
   Log00404EE0("Finished loading master archives in %f seconds", (double)eax);
   return esp13;
}