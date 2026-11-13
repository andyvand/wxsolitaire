/* Prints the window number of an emulator window, for screencapture -l.
   winid [pid]: with a pid, only that process's windows are considered;
   otherwise the first Basilisk II or SheepShaver window. macOS only. */
#include <CoreGraphics/CoreGraphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int want = argc > 1 ? atoi(argv[1]) : 0;
    CFArrayRef list = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly,
                                                 kCGNullWindowID);
    for (CFIndex i = 0; i < CFArrayGetCount(list); i++) {
        CFDictionaryRef w = CFArrayGetValueAtIndex(list, i);
        char owner[256] = "";
        CFStringRef name = CFDictionaryGetValue(w, kCGWindowOwnerName);
        if (name)
            CFStringGetCString(name, owner, sizeof owner, kCFStringEncodingUTF8);
        int layer = 0, pid = 0, n = 0;
        CFNumberGetValue(CFDictionaryGetValue(w, kCGWindowLayer), kCFNumberIntType, &layer);
        CFNumberGetValue(CFDictionaryGetValue(w, kCGWindowOwnerPID), kCFNumberIntType, &pid);
        if (layer != 0)
            continue;
        if (want ? pid == want
                 : (!strncmp(owner, "BasiliskII", 10) || !strncmp(owner, "SheepShaver", 11))) {
            CFNumberGetValue(CFDictionaryGetValue(w, kCGWindowNumber), kCFNumberIntType, &n);
            printf("%d\n", n);
            return 0;
        }
    }
    return 1;
}
