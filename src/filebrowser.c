#pragma bank 255

#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "systemdetect.h"
#include "systemhelpers.h"
#include "pff.h"

#include "vgm_player.h"
#include "joy.h"

#include "globals.h"
#include "vwf.h"
#include "module_vwf.h"
#include "screen.h"
#include "menus.h"
#include "menu_msgbox.h"
#include "menu_codes.h"

// #define SD_READ_EMULATION

const uint8_t * const VGM_ERRORS[N_VGM_RESULTS] = {"Ok!", "Read error", "VGM format error", "Unsupported chip", "Version error", "Wrong VGM command" };
VGM_RESULT play_error;

#define MAX_DIR_FILES 192

bool fs_inited = false;

uint8_t current_path[256] = "";
FILINFO files_list[MAX_DIR_FILES];
FILINFO * files[MAX_DIR_FILES];
uint8_t files_loaded;

uint8_t onTranslateKeyFileBrowser(const struct menu_t * menu, const struct menu_item_t * self, uint8_t value);
uint8_t onTranslateSubResultFileBrowser(const struct menu_t * menu, const struct menu_item_t * self, uint8_t value);
uint8_t onHelpFileBrowser(const struct menu_t * menu, const struct menu_item_t * selection);
uint8_t onShowFileBrowser(const menu_t * self, uint8_t * param);
uint8_t onIdleFileBrowser(const struct menu_t * menu, const struct menu_item_t * selection);
uint8_t * onFileBrowserMenuItemPaint(const struct menu_t * menu, const struct menu_item_t * self);
uint8_t onFileBrowserMenuItemProps(const struct menu_t * menu, const struct menu_item_t * self);

const uint8_t NO_FILES[] = "NO FILES";

#if defined(MASTERSYSTEM)
#define MAX_FILES_ON_PAGE 48
#define DISPLAY_COLUMNS   3
#else
#define MAX_FILES_ON_PAGE 32
#define DISPLAY_COLUMNS   2
#endif
menu_item_t FileBrowserMenuItems[MAX_FILES_ON_PAGE];
menu_t FileBrowserMenu = {
#if defined(MASTERSYSTEM)
    .x = 1, .y = ((DEVICE_SCREEN_HEIGHT - (MAX_FILES_ON_PAGE / DISPLAY_COLUMNS)) / 2), .width = DEVICE_SCREEN_WIDTH - 1, .height = 16,
#else
    .x = 0, .y = 1, .width = DEVICE_SCREEN_WIDTH, .height = 16,
#endif
    .cancel_mask = J_START, .cancel_result = ACTION_INITIALIZE,
    .flags = MENU_FLAGS_INVERSE,
    .items = FileBrowserMenuItems, .last_item = FileBrowserMenuItems,
    .onShow = onShowFileBrowser, .onIdle = onIdleFileBrowser, .onHelpContext = onHelpFileBrowser,
    .onTranslateKey = onTranslateKeyFileBrowser, .onTranslateSubResult = onTranslateSubResultFileBrowser
};
uint8_t browser_current_page, browser_max_pages;
uint8_t browser_menu_result;
const menu_item_t * browser_last_selection;

void load_browser_page(uint8_t page) {
    menu_item_t * current_item = FileBrowserMenuItems;
    current_item->caption = NO_FILES;
    current_item->result = ACTION_READ_DIRECTORY;
    FileBrowserMenu.last_item = current_item;
    for (uint8_t i = page * MAX_FILES_ON_PAGE, j = MIN((page * MAX_FILES_ON_PAGE + MAX_FILES_ON_PAGE), files_loaded); (i < j); i++, current_item++) {
        current_item->caption = files[i]->fname;
        current_item->result = (files[i]->fattrib & AM_DIR) ? ACTION_EXECUTE_DIRECTORY : ACTION_EXECUTE_FILE;
        FileBrowserMenu.last_item = current_item;
    }
}

uint8_t onTranslateKeyFileBrowser(const struct menu_t * menu, const struct menu_item_t * self, uint8_t value) {
    menu; self;
    // swap J_UP/J_DOWN with J_LEFT/J_RIGHT buttons, because our menus are horizontal
    if (value & J_LEFT) {
        if (browser_current_page) {
            browser_current_page--;
            browser_menu_result = ACTION_PREV_PAGE;
        }
    } else if (value & J_RIGHT) {
        if (browser_current_page < (browser_max_pages - 1)) {
            browser_current_page++;
            browser_menu_result = ACTION_NEXT_PAGE;
        }
    }
    return value;
}
uint8_t onTranslateSubResultFileBrowser(const struct menu_t * menu, const struct menu_item_t * self, uint8_t value) {
    self; menu;
    return value;
}
uint8_t onHelpFileBrowser(const struct menu_t * menu, const struct menu_item_t * selection) {
    menu; selection;
    return 0;
}
uint8_t onShowFileBrowser(const menu_t * self, uint8_t * param) {
    self; param;
    screen_clear_rect(self->x, self->y, self->width, self->height, WHITE_ON_BLACK);
    strcpy(text_buffer, "[/");
    uint8_t len = strlen(current_path);
    if (len > 25) {
        strcat(text_buffer, "...");
        strcat(text_buffer, current_path + (len - 25));
    } else {
        strcat(text_buffer, current_path);
    }
    strcat(text_buffer, "]");
    menu_text_out(0, 0, DEVICE_SCREEN_WIDTH, WHITE_ON_BLACK, ITEM_TEXT_CENTERED, text_buffer);
    browser_menu_result = MENU_RESULT_NONE;
    return MENU_PROP_NO_FRAME;
}
uint8_t onIdleFileBrowser(const struct menu_t * menu, const struct menu_item_t * selection) {
    menu;
    browser_last_selection = selection;
    vsync();
    return browser_menu_result;
}
uint8_t * onFileBrowserMenuItemPaint(const struct menu_t * menu, const struct menu_item_t * self) {
    menu; self;
    if (self->result == ACTION_EXECUTE_DIRECTORY) {
        strcpy(text_buffer, " [");
        strcat(text_buffer, self->caption);
        strcat(text_buffer, "]");
    } else {
        strcpy(text_buffer, " ");
        strcat(text_buffer + 1, self->caption);
    }
    return text_buffer;
}
uint8_t onFileBrowserMenuItemProps(const struct menu_t * menu, const struct menu_item_t * self) {
    menu; self;
    return ITEM_DEFAULT;
}

bool check_ext(const uint8_t * substr, const uint8_t * str) {
    for ( ;(*str); str++) {
        if (*str == '.') break;
    }
    return (strcmp(substr, str) == 0);
}
void cut_path(uint8_t * str) {
    uint8_t * slash = str;
    for ( ;(*str); str++) {
        if (*str == '/') slash = str;
    }
    *slash = 0;
}

uint8_t read_directory(uint8_t * path) {
    static FATFS fs;
    static DIR dir;
    static FILINFO * fn;

    files_loaded = 0;

#ifndef SD_READ_EMULATION
    // mount FS if not mounted yet
    if (!fs_inited) {
        if (fs_inited = (pf_mount(&fs) != FR_OK)) return files_loaded;
    }
    // open the current directory
    if (pf_opendir(&dir, path) != FR_OK) return files_loaded;
#endif
    // add ".." if not root directory
    if (strlen(path)) {
        fn = files[files_loaded] = files_list + files_loaded;
        fn->fattrib = AM_DIR;
        strcpy(fn->fname, "..");
        files_loaded++;
    }
#ifdef SD_READ_EMULATION
    while (files_loaded < 89) {
        fn = files[files_loaded] = files_list + files_loaded;
        if ((files_loaded & 7) == 0) {
            strcpy(fn->fname, "somedir");
            fn->fattrib = AM_DIR;
        } else {
            strcpy(fn->fname, "test.vgm");
            fn->fattrib = 0;
        }
        files_loaded++;
    }
#else
    // read directory and add files
    while (true) {
        fn = files[files_loaded] = files_list + files_loaded;
        if (pf_readdir(&dir, fn) != FR_OK) break;
        if (!fn->fname[0]) break;
        if ((fn->fattrib & AM_DIR) || check_ext(".VGM", fn->fname)) {
            if (++files_loaded == MAX_DIR_FILES) break;
        }
    }
#endif
    // calculate the page count
    browser_max_pages = ((files_loaded % MAX_FILES_ON_PAGE) ? 1 : 0) + (files_loaded / MAX_FILES_ON_PAGE);
    // return the number of files
    return files_loaded;
}

void file_browser_execute(void) BANKED {
    uint8_t menu_result;
    menu_item_t * current_item = FileBrowserMenuItems;

    browser_last_selection = NULL;

    memset(FileBrowserMenuItems, 0, sizeof(FileBrowserMenuItems));

    for (uint8_t i = 0, k = 0; i != DISPLAY_COLUMNS; i++) {
        for (uint8_t j = 0; j != (MAX_FILES_ON_PAGE / DISPLAY_COLUMNS); j++, current_item++) {
            current_item->ofs_x = i * 10;
            current_item->ofs_y = j;
            current_item->width = 10;
            current_item->id = k++;
            current_item->onPaint = onFileBrowserMenuItemPaint;
        }
    }
    fs_inited = false, current_path[0] = 0;
    read_directory(current_path);
    browser_current_page = 0;
    do {
        load_browser_page(browser_current_page);
        if (browser_last_selection > FileBrowserMenu.last_item) browser_last_selection = FileBrowserMenu.last_item;
        menu_result = menu_execute(&FileBrowserMenu, NULL, browser_last_selection);
        switch (menu_result) {
            case ACTION_EXECUTE_FILE:
#ifdef SD_READ_EMULATION
                MessageBox(VGM_ERRORS[VGM_READ_ERROR]);
#else
                strcpy(text_buffer, current_path);
                if (strlen(text_buffer)) strcat(text_buffer, "/");
                strcat(text_buffer, browser_last_selection->caption);
                if ((play_error = vgm_play_file(text_buffer)) != VGM_OK) {
                    MessageBox(VGM_ERRORS[play_error]);
                }
#endif
                break;
            case ACTION_EXECUTE_DIRECTORY:
                if (!strcmp(browser_last_selection->caption, "..")) {
                    cut_path(current_path);
                } else {
                    if (strlen(current_path)) strcat(current_path, "/");
                    strcat(current_path, browser_last_selection->caption);
                }
                // append current path
            case ACTION_READ_DIRECTORY:
                read_directory(current_path);
                browser_current_page = 0;
                browser_last_selection = NULL;
                break;
            case ACTION_NEXT_PAGE:
            case ACTION_PREV_PAGE:
                break;
            case ACTION_INITIALIZE:
                fs_inited = false, current_path[0] = 0;
                read_directory(current_path);
                browser_current_page = 0;
                browser_last_selection = NULL;
                break;
        }
    } while (menu_result != ACTION_NONE);
}
