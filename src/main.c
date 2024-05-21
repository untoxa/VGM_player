#include <gbdk/platform.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "systemdetect.h"
#include "module_vwf.h"
#include "screen.h"

#include "filebrowser.h"

void main(void) {
    detect_system();
    setup_system();

    INIT_module_screen();
    INIT_module_vwf();
    INIT_module_misc_assets();

    while (true) {
        file_browser_execute();
    }
}