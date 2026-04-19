#include "../types.h"
#include <AppKit/AppKit.h>

result_t files_open(char *filepath, size_t maxlen) {
  static bool opening = false;
  if (opening)
    return ERR_FILES_REOPEN;
  opening = true;

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  NSWindow *keywin = [[NSApplication sharedApplication] keyWindow];
  NSOpenPanel *dialog = [NSOpenPanel openPanel];
  [dialog setAllowsMultipleSelection:NO];
  [dialog setAllowedFileTypes:@[ @"mp3" ]];
  [dialog setContentSize:NSMakeSize(800, 500)];

  result_t result = ERR_FILES_CANCEL;
  if ([dialog runModal] == NSModalResponseOK) {
    NSURL *url = [dialog URL];
    const char *path = [[url path] UTF8String];
    size_t len = strlen(path);

    if (!len || !path) {
      result = ERR_FILES;
      goto done;
    }

    snprintf(filepath, maxlen, "%s", path);
    result = OK;
  }

done:
  [pool release];
  [keywin makeKeyAndOrderFront:nil];
  opening = false;
  return result;
}
