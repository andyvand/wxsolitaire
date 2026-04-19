// wxSolitaire macOS Help Book launcher.
//
// Opens the bundled .help book (passed as an absolute path) in Help Viewer.
// Preferred path: AHRegisterHelpBookWithURL + AHGotoPage so the book is
// indexed by helpd regardless of signing / origin. Falls back through
// NSHelpManager's private -registerBooksInBundle:error: and finally to
// `open -b com.apple.helpviewer <access-url>` if everything else fails.

// Work around a Homebrew e2fsprogs-libs conflict: /usr/local/include/uuid/uuid.h
// (shipped by Homebrew's libuuid) defines the _UUID_UUID_H guard but does
// NOT define Apple's uuid_string_t. When the macOS SDK's hfs_format.h
// (pulled in via AppKit -> CoreServices -> CarbonCore -> HFSVolumes.h)
// then does #include <uuid/uuid.h>, the guard short-circuits Apple's
// real header and uuid_string_t stays undefined, breaking the build.
#include <sys/_types.h>
#ifndef _UUID_STRING_T
#define _UUID_STRING_T
typedef __darwin_uuid_string_t uuid_string_t;
#endif

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <Carbon/Carbon.h>

#include "help_mac.h"

// NSHelpManager -registerBooksInBundle:error: is available at runtime on
// 10.6+ but not declared in the public SDK headers. Forward-declare so the
// compiler knows the signature; guard actual calls with respondsToSelector:.
@interface NSHelpManager (wxSolitairePrivate)
- (BOOL)registerBooksInBundle:(NSBundle *)bundle error:(NSError **)outError;
@end

// Resolve the landing page inside a .help bundle, following preferred
// localisations and the development region.
static NSURL *LocalAccessURL(NSBundle *helpBundle)
{
    if (!helpBundle) return nil;

    NSString *access = [helpBundle objectForInfoDictionaryKey:@"HPDBookAccessPath"];
    if (!access.length) access = @"index.html";

    NSURL *resURL = [helpBundle resourceURL];
    NSMutableArray<NSString *> *candidates = [NSMutableArray array];
    for (NSString *loc in [NSBundle preferredLocalizationsFromArray:[helpBundle localizations]]) {
        [candidates addObject:[loc stringByAppendingString:@".lproj"]];
    }
    NSString *dev = [helpBundle objectForInfoDictionaryKey:@"CFBundleDevelopmentRegion"];
    if (dev.length) [candidates addObject:[dev stringByAppendingString:@".lproj"]];
    // Legacy English.lproj fallback for bundles whose dev region is "en".
    [candidates addObject:@"English.lproj"];

    for (NSString *lproj in candidates) {
        NSURL *url = [[resURL URLByAppendingPathComponent:lproj]
                      URLByAppendingPathComponent:access];
        if ([url checkResourceIsReachableAndReturnError:NULL]) return url;
    }
    NSURL *flat = [resURL URLByAppendingPathComponent:access];
    return [flat checkResourceIsReachableAndReturnError:NULL] ? flat : nil;
}

// Determine the book name helpd uses to key the book. Prefer Info.plist
// keys; fall back to scanning the landing page for <meta name="AppleTitle">.
static NSString *BookNameFromHelpBundle(NSBundle *helpBundle)
{
    if (!helpBundle) return nil;
    NSDictionary *info = [helpBundle infoDictionary];
    NSString *name = info[@"CFBundleHelpBookName"];
    if (name.length) return name;
    name = info[@"HPDBookTitle"];
    if (name.length) return name;

    NSURL *landing = LocalAccessURL(helpBundle);
    if (!landing) return nil;

    NSError *err = nil;
    NSString *html = [NSString stringWithContentsOfURL:landing
                                              encoding:NSUTF8StringEncoding
                                                 error:&err];
    if (!html) return nil;

    NSRegularExpression *re =
        [NSRegularExpression regularExpressionWithPattern:
            @"<meta[^>]*name=\"AppleTitle\"[^>]*content=\"([^\"]+)\""
                                                  options:NSRegularExpressionCaseInsensitive
                                                    error:NULL];
    NSTextCheckingResult *m = [re firstMatchInString:html
                                             options:0
                                               range:NSMakeRange(0, html.length)];
    if (m && m.numberOfRanges > 1) {
        return [html substringWithRange:[m rangeAtIndex:1]];
    }
    return nil;
}

bool OpenMacHelpBook(const char *helpBundlePath)
{
    if (!helpBundlePath || !*helpBundlePath) return false;

    @autoreleasepool {
        NSString *path = [NSString stringWithUTF8String:helpBundlePath];
        BOOL isDir = NO;
        if (![[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDir] || !isDir) {
            NSLog(@"wxSolitaire: help bundle not found at %@", path);
            return false;
        }

        NSURL *bundleURL = [NSURL fileURLWithPath:path isDirectory:YES];
        NSBundle *helpBundle = [NSBundle bundleWithURL:bundleURL];
        NSString *book = BookNameFromHelpBundle(helpBundle);

        // Register the book with helpd. Prefer the direct URL API (10.13+),
        // fall back to -registerBooksInBundle:error: on the main app bundle
        // (which in turn requires CFBundleHelpBookFolder in the app's
        // Info.plist).
        OSStatus regStatus = paramErr;
        regStatus = AHRegisterHelpBookWithURL((__bridge CFURLRef)bundleURL);
        if (regStatus != noErr) {
            NSHelpManager *mgr = [NSHelpManager sharedHelpManager];
            if ([mgr respondsToSelector:@selector(registerBooksInBundle:error:)]) {
                NSError *err = nil;
                if ([mgr registerBooksInBundle:[NSBundle mainBundle] error:&err]) {
                    regStatus = noErr;
                } else {
                    NSLog(@"wxSolitaire: registerBooksInBundle failed: %@", err);
                }
            }
        }

        // Preferred navigation: hand helpd the book name and let it open its
        // default page (HPDBookAccessPath) in Help Viewer.
        if (regStatus == noErr && book.length) {
            OSStatus goStatus = AHGotoPage((__bridge CFStringRef)book, NULL, NULL);
            if (goStatus == noErr) return true;
            NSLog(@"wxSolitaire: AHGotoPage(%@) failed with %d", book, (int)goStatus);
        }

        // Fallback: force-open the access page in Help Viewer itself via
        // `open -b com.apple.helpviewer`. Avoids Safari or helpd registry misses.
        NSURL *accessURL = LocalAccessURL(helpBundle);
        if (accessURL) {
            NSTask *task = [[NSTask alloc] init];
            task.launchPath = @"/usr/bin/open";
            task.arguments  = @[ @"-b", @"com.apple.helpviewer", accessURL.path ];
            @try {
                [task launch];
                return true;
            } @catch (NSException *e) {
                NSLog(@"wxSolitaire: launching Help Viewer failed: %@", e);
            }
        }

        // Last resort: let Launch Services open the bundle.
        return [[NSWorkspace sharedWorkspace] openURL:bundleURL];
    }
}
