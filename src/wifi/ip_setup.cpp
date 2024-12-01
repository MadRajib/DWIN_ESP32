#include "ip_setup.h"
#include "../conf/global_config.h"
#include "lvgl.h"
#include <HTTPClient.h>
#include "core/data_setup.h"
#include "../core/macros_query.h"
#include "../core/http_client.h"
#include "../core/lv_setup.h"
#include "serial/serial_console.h"

lv_obj_t * hostEntry;
lv_obj_t * portEntry;
lv_obj_t * label = NULL;

void show_ip_entry();
void show_auth_entry();

/* Create a custom keyboard to allow hostnames or ip addresses (a-z, 0 - 9, and -) */
static const char * kb_map[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", LV_SYMBOL_OK, "\n",
    LV_SYMBOL_LEFT, "z", "x", "c", "v", "b", "n", "m", ".", "-", LV_SYMBOL_RIGHT, NULL
};

static const lv_btnmatrix_ctrl_t kb_ctrl[] = {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, LV_KEYBOARD_CTRL_BTN_FLAGS | 6,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, LV_KEYBOARD_CTRL_BTN_FLAGS | 5,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 6, 4, 4, 4, 4, 4, 4, 4, 4, 4, LV_KEYBOARD_CTRL_BTN_FLAGS | 6
};

static const char * hex_numpad_map[] = {
    "1", "2", "3", "f", LV_SYMBOL_BACKSPACE, "\n",
    "4", "5", "6", "e", LV_SYMBOL_OK, "\n",
    "7", "8", "9", "d", LV_SYMBOL_LEFT, "\n",
    "0", "a", "b", "c", LV_SYMBOL_RIGHT, NULL
};

static const lv_btnmatrix_ctrl_t hex_numpad_ctrl[] = {
    1, 1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
    1, 1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
    1, 1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
    1, 1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
};

enum connection_status_t {
    CONNECT_FAIL = 0,
    CONNECT_OK = 1,
    CONNECT_AUTH_REQUIRED = 2,
};

connection_status_t verify_ip(){
    SETUP_HTTP_CLIENT_FULL("/printer/info", true, 1000);

    int httpCode;
    try {
        httpCode = client.GET();

        if (httpCode == 401)
            return CONNECT_AUTH_REQUIRED;

        return httpCode == 200 ? CONNECT_OK : CONNECT_FAIL;
    }
    catch (...) {
        LOG_LN("Failed to connect");
        return CONNECT_FAIL;
    }
}

static void keyboard_event_ip_entry(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);

    if ((code == LV_EVENT_FOCUSED || code == LV_EVENT_DEFOCUSED) && ta != NULL)
    {
        // make sure we alter the keymap before taking actions that might
        // destroy the keyboard
        if (lv_obj_has_flag(ta, LV_OBJ_FLAG_USER_1))
        {
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);
        }
        else
        {
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
        }
    }

    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    else if (code == LV_EVENT_READY) 
    {
        strcpy(get_current_printer_config()->klipper_host, lv_textarea_get_text(hostEntry));
        get_current_printer_config()->klipper_port = atoi(lv_textarea_get_text(portEntry));

        connection_status_t status = verify_ip();
        if (status == CONNECT_OK)
        {
            get_current_printer_config()->ip_configured = true;
            write_global_config();
        }
        else if (status == CONNECT_AUTH_REQUIRED)
        {
            show_auth_entry();
        }
        else
        {
            lv_label_set_text(label, "Failed to connect");
        }
    }
    else
    {
        return;
    }
}

static void keyboard_event_auth_entry(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);

    if (code == LV_EVENT_READY) 
    {
        const char * txt = lv_textarea_get_text(ta);
        int len = strlen(txt);
        if (len > 0)
        {
            get_current_printer_config()->auth_configured = true;
            strcpy(get_current_printer_config()->klipper_auth, txt);

            if (verify_ip() == CONNECT_OK)
            {
                get_current_printer_config()->ip_configured = true;
                write_global_config();
            }
            else 
            {
                lv_label_set_text(label, "Failed to connect");
            }
        }
    }
    else if (code == LV_EVENT_CANCEL)
    {
        show_ip_entry();
    }
}

void show_auth_entry()
{
    
}

void show_ip_entry()
{
    
}

void ip_init(){
    if (!get_current_printer_config()->ip_configured)
    {
        show_ip_entry();
    }
    
    while (!get_current_printer_config()->ip_configured)
    {
        lv_handler();
        serial_console::run();
    }
}