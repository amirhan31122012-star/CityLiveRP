/* Run under the oldest supported i386 loader; never call server callbacks here. */
#include <dlfcn.h>
#include <stdio.h>
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    void *plugin = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!plugin) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
    const char *symbols[] = {"Supports", "Load", "Unload", "ProcessTick", "OnGameModeInit", "OnGameModeExit", "OnDialogResponse"};
    for (unsigned i = 0; i < sizeof symbols / sizeof symbols[0]; ++i) {
        if (!dlsym(plugin, symbols[i])) { fprintf(stderr, "Missing callback: %s\n", symbols[i]); dlclose(plugin); return 1; }
    }
    if (dlclose(plugin)) { fprintf(stderr, "dlclose: %s\n", dlerror()); return 1; }
    puts("PASS: plugin relocates, exports callbacks and closes on this runtime");
    return 0;
}
