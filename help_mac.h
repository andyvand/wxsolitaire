#ifndef WXSOL_HELP_MAC_H
#define WXSOL_HELP_MAC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Open the macOS Help Book located at helpBundlePath (absolute path to a
 * .help bundle). Registers the book with the AppleHelp framework and
 * navigates Help Viewer to its default page. Returns true on success. */
bool OpenMacHelpBook(const char* helpBundlePath);

#ifdef __cplusplus
}
#endif

#endif /* WXSOL_HELP_MAC_H */
