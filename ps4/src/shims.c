/*
 * Ship of Harkinian (Vulkan) for the PS4 - libc behaviour a title needs that the console lacks.
 * From damiantw/PS5_Shipwright's ps5/src/shims.c (MIT): there the names are bound by the PS5 link; here they are
 * the linker's --wrap (ps4/cmake/ps4-game.cmake).
 *
 * SPDX-License-Identifier: MIT
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "platform.h"

#ifndef PS4_DATA_DIR
#define PS4_DATA_DIR "/data/sohvk"
#endif

/* lstat: a title's folders hold no symbolic links; stat answers the same (the PS5 found lstat unreliable for a
 * title, and std::filesystem::symlink_status is built on it). */
int __wrap_lstat(const char *path, struct stat *out)
{
   return stat(path, out);
}

/* No /tmp for a title: a scratch file in its data folder, gone when it is closed. */
FILE *__wrap_tmpfile(void)
{
   static int serial;
   char path[300];
   snprintf(path, sizeof path, "%s/.tmp/scratch-%d", ps5_data_dir(), __atomic_fetch_add(&serial, 1, __ATOMIC_RELAXED));
   FILE *file = fopen(path, "w+");
   if (file)
      unlink(path);
   return file;
}

/* getcwd fails for a PS4 title (the Wind Waker HD port's finding), and std::filesystem's absolute, canonical and
 * relative are built on it: the kernel's answer when it has one, else the data folder (port/ps4_platform.cpp enters
 * it at start-up). */
char *__real_getcwd(char *buffer, size_t size);

static const char *
working_dir(void)
{
   static char cwd[512];
   static int known;
   if (!known) {
      if (!__real_getcwd(cwd, sizeof cwd) || cwd[0] != '/')
         snprintf(cwd, sizeof cwd, "%s", PS4_DATA_DIR);
      known = 1;
   }
   return cwd;
}

char *__wrap_getcwd(char *buffer, size_t size)
{
   const char *dir = working_dir();
   const size_t need = strlen(dir) + 1;
   if (!buffer) {
      if (size == 0)
         size = need > 1024 ? need : 1024;
      buffer = malloc(size);
      if (!buffer)
         return NULL;
   }
   if (size < need) {
      errno = ERANGE;
      return NULL;
   }
   memcpy(buffer, dir, need);
   return buffer;
}

/* realpath, by the path's text: "." dropped, ".." takes the component before it, slashes once each (no symbolic
 * links to follow). */
char *__wrap_realpath(const char *path, char *resolved)
{
   char full[1024], out[1024];
   struct stat status;
   size_t length = 0;

   if (!path || !*path) {
      errno = path ? ENOENT : EINVAL;
      return NULL;
   }
   if ((size_t)snprintf(full, sizeof full, "%s%s%s", path[0] == '/' ? "" : working_dir(), path[0] == '/' ? "" : "/",
                        path) >= sizeof full) {
      errno = ENAMETOOLONG;
      return NULL;
   }
   for (const char *part = full; *part;) {
      size_t size;
      while (*part == '/')
         part++;
      size = strcspn(part, "/");
      if (size == 2 && part[0] == '.' && part[1] == '.') {
         while (length > 0 && out[--length] != '/')
            ;
      } else if (size > 0 && !(size == 1 && part[0] == '.')) {
         out[length++] = '/';
         memcpy(out + length, part, size);
         length += size;
      }
      part += size;
   }
   if (length == 0)
      out[length++] = '/';
   out[length] = 0;
   if (stat(out, &status) != 0) {
      errno = ENOENT;
      return NULL;
   }
   if (!resolved) {
      resolved = malloc(1024);
      if (!resolved)
         return NULL;
   }
   memcpy(resolved, out, length + 1);
   return resolved;
}
