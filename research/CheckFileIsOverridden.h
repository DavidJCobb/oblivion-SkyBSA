bool Archive::CheckFileIsOverridden(BSAEntry& file, const char* looseFilePath) {
   if (!INI:Archive:bInvalidateOlderFiles)
      return false;
   if (file.offset & 0x80000000) // high bit is used as an "already checked" flag
      return false;
   if (looseFilePath) {
      char[MAX_PATH] finalPath = "Data\\"; // at esp40
      auto eax = strlen(looseFilePath); // inlined
      // Prepend "Data\\" to the loose file path:
      memcpy(finalPath + strlen(finalPath), looseFilePath, eax);
      //
      struct _stat esp10;
      auto eax = _stat(finalPath, &esp10);
      if (eax != -1) {
         //
         // The (_stat) call managed to find a matching loose file 
         // and retrieved some basic information about it, including 
         // its timestamp.
         //
         // this->myDateModified is stored in registers edi and esi
         //
         // esp10.st_mtime is checked one register at a time, using ecx
         //
         // timestamp type is __time64_t
         //
         if (esp10.st_mtime > this->myDateModified) {
            file.offset &= 0x80000000; // a file with a zero offset is "invalid"
            return true;
         }
      }
   }
   file.offset |= 0x80000000; // mark the file as already checked so we can skip the above next time
   return false;
}