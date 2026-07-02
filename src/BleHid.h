#pragma once
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEHIDDevice.h>
#include <HIDTypes.h>

// Report IDs
#define REPORT_ID_KEYBOARD  1
#define REPORT_ID_MOUSE     2

// Keyboard modifier bits
#define KEY_MOD_LCTRL   0x01
#define KEY_MOD_LSHIFT  0x02
#define KEY_MOD_LALT    0x04
#define KEY_MOD_LGUI    0x08
#define KEY_MOD_RCTRL   0x10
#define KEY_MOD_RSHIFT  0x20
#define KEY_MOD_RALT    0x40
#define KEY_MOD_RGUI    0x80

// Mouse button bits
#define MOUSE_LEFT      0x01
#define MOUSE_RIGHT     0x02
#define MOUSE_MIDDLE    0x04

// Keyboard report: [modifier, reserved, key0..key5]
struct KeyboardReport {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t keys[6];
};

// Mouse report: [buttons, x, y, wheel]
struct MouseReport {
    uint8_t buttons;
    int8_t  x;
    int8_t  y;
    int8_t  wheel;
};

// Combined HID report descriptor (keyboard + mouse, with report IDs)
static const uint8_t hidDescriptor[] = {
    // --- Keyboard ---
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, REPORT_ID_KEYBOARD,
    // Modifier keys (8 bits)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0xE0,        //   Usage Minimum (224)
    0x29, 0xE7,        //   Usage Maximum (231)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Var, Abs)
    // Reserved byte
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const)
    // Key array (6 keys)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0x00,        //   Usage Minimum (0)
    0x29, 0x65,        //   Usage Maximum (101)
    0x81, 0x00,        //   Input (Data, Array, Abs)
    0xC0,              // End Collection

    // --- Mouse ---
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, REPORT_ID_MOUSE,
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    // Buttons (3 bits)
    0x05, 0x09,        //     Usage Page (Buttons)
    0x19, 0x01,        //     Usage Minimum (1)
    0x29, 0x03,        //     Usage Maximum (3)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x02,        //     Input (Data, Var, Abs)
    // Padding (5 bits)
    0x75, 0x05,        //     Report Size (5)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x01,        //     Input (Const)
    // X, Y, Wheel (-127..127)
    0x05, 0x01,        //     Usage Page (Generic Desktop)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x06,        //     Input (Data, Var, Rel)
    0xC0,              //   End Collection
    0xC0,              // End Collection
};

class BleHid : public NimBLEServerCallbacks {
public:
    bool     connected   = false;
    uint16_t _connHandle = 0;

    void onConnect(NimBLEServer* s, ble_gap_conn_desc* desc) override {
        connected    = true;
        _connHandle  = desc->conn_handle;
        // Request 7.5ms connection interval for lowest latency
        s->updateConnParams(desc->conn_handle, 6, 6, 0, 400);
    }

    void onDisconnect(NimBLEServer* s) override {
        connected   = false;
        _connHandle = 0;
        NimBLEDevice::startAdvertising();
    }

    void begin(const char* deviceName = "Wireless HID") {
        NimBLEDevice::init(deviceName);

        // Bonding with Secure Connections — persists across power cycles
        NimBLEDevice::setSecurityAuth(true, false, true); // bond, no MITM, SC
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

        _server = NimBLEDevice::createServer();
        _server->setCallbacks(this);

        _hid        = new NimBLEHIDDevice(_server);
        _kbInput    = _hid->inputReport(REPORT_ID_KEYBOARD);
        _mouseInput = _hid->inputReport(REPORT_ID_MOUSE);

        _hid->manufacturer()->setValue("Espressif");
        _hid->pnp(0x02, 0x045E, 0x0000, 0x0110);
        _hid->hidInfo(0x00, 0x01);
        _hid->reportMap((uint8_t*)hidDescriptor, sizeof(hidDescriptor));
        _hid->startServices();
        _hid->setBatteryLevel(100);

        NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
        adv->setAppearance(HID_KEYBOARD);
        adv->addServiceUUID(_hid->hidService()->getUUID());
        adv->start();
    }

    bool isConnected() { return connected; }

    void disconnect() {
        if (connected) _server->disconnect(_connHandle);
    }

    // Press and release a key with optional modifier
    void keyPress(uint8_t keycode, uint8_t modifier = 0) {
        KeyboardReport r = {};
        r.modifier = modifier;
        r.keys[0]  = keycode;
        _sendKeyboard(r);
        delay(10);
        _releaseKeyboard();
    }

    // Type an ASCII string
    void typeString(const char* str) {
        for (int i = 0; str[i]; i++) {
            uint8_t key = _asciiToKeycode(str[i]);
            uint8_t mod = _asciiNeedsShift(str[i]) ? KEY_MOD_LSHIFT : 0;
            if (key) keyPress(key, mod);
            delay(20);
        }
    }

    void releaseKeys() { _releaseKeyboard(); }

    // Move mouse relative to current position (preserves held buttons)
    void mouseMove(int8_t x, int8_t y, int8_t wheel = 0) {
        MouseReport r = {_mouseButtons, x, y, wheel};
        _sendMouse(r);
    }

    // Click a mouse button (momentary)
    void mouseClick(uint8_t buttons = MOUSE_LEFT) {
        MouseReport r = {buttons, 0, 0, 0};
        _sendMouse(r);
        delay(10);
        r.buttons = 0;
        _sendMouse(r);
    }

    void mousePress(uint8_t buttons)   { _mouseButtons = buttons; MouseReport r = {buttons, 0, 0, 0}; _sendMouse(r); }
    void mouseRelease()                { _mouseButtons = 0; MouseReport r = {}; _sendMouse(r); }

    uint8_t layout = 0; // 0 = US, 1 = UK

    void setLayout(uint8_t l) { layout = l; }

private:
    NimBLEServer*     _server      = nullptr;
    NimBLEHIDDevice*  _hid         = nullptr;
    NimBLECharacteristic* _kbInput    = nullptr;
    NimBLECharacteristic* _mouseInput = nullptr;
    uint8_t _mouseButtons = 0;

    void _sendKeyboard(const KeyboardReport& r) {
        _kbInput->setValue((uint8_t*)&r, sizeof(r));
        _kbInput->notify();
    }
    void _releaseKeyboard() {
        KeyboardReport r = {};
        _sendKeyboard(r);
    }
    void _sendMouse(const MouseReport& r) {
        _mouseInput->setValue((uint8_t*)&r, sizeof(r));
        _mouseInput->notify();
    }

    // Minimal ASCII -> HID keycode (printable ASCII only)
    // layout 0 = US, 1 = UK
    uint8_t _asciiToKeycode(char c) {
        if (c >= 'a' && c <= 'z') return 4 + (c - 'a');
        if (c >= 'A' && c <= 'Z') return 4 + (c - 'A');
        if (c >= '1' && c <= '9') return 30 + (c - '1');
        if (c == '0') return 39;

        if (layout == 1) {
            // UK layout differences
            switch (c) {
                case '@':  return 0x34; // Shift + apostrophe key
                case '"':  return 0x1F; // Shift + 2
                case '#':  return 0x32; // non-US hash key (unshifted)
                case '~':  return 0x32; // Shift + non-US hash key
                case '\\': return 0x64; // non-US backslash key
                case '|':  return 0x64; // Shift + non-US backslash key
                default:   break;
            }
        }

        switch (c) {
            case ' ':  return 0x2C;
            case '\n': return 0x28;
            case '\t': return 0x2B;
            case '-':  return 0x2D;
            case '=':  return 0x2E;
            case '[':  return 0x2F;
            case ']':  return 0x30;
            case '\\': return 0x31;
            case ';':  return 0x33;
            case '\'': return 0x34;
            case '`':  return 0x35;
            case ',':  return 0x36;
            case '.':  return 0x37;
            case '/':  return 0x38;
            case '!':  return 0x1E;
            case '@':  return 0x1F;
            case '#':  return 0x20;
            case '$':  return 0x21;
            case '%':  return 0x22;
            case '^':  return 0x23;
            case '&':  return 0x24;
            case '*':  return 0x25;
            case '(':  return 0x26;
            case ')':  return 0x27;
            case '_':  return 0x2D;
            case '+':  return 0x2E;
            case '{':  return 0x2F;
            case '}':  return 0x30;
            case '|':  return 0x31;
            case ':':  return 0x33;
            case '"':  return 0x34;
            case '~':  return 0x35;
            case '<':  return 0x36;
            case '>':  return 0x37;
            case '?':  return 0x38;
            default:   return 0;
        }
    }

    bool _asciiNeedsShift(char c) {
        if (layout == 1) {
            // UK: '#' is unshifted, '@' needs shift on a different key,
            //     '"' needs shift (Shift+2), '~' needs shift on 0x32
            return (c >= 'A' && c <= 'Z') ||
                   (c == '!' || c == '"' || c == '@' || c == '$' || c == '%' ||
                    c == '^' || c == '&' || c == '*' || c == '(' || c == ')' ||
                    c == '_' || c == '+' || c == '{' || c == '}' || c == '|' ||
                    c == ':' || c == '~' || c == '<' || c == '>' || c == '?');
        }
        return (c >= 'A' && c <= 'Z') ||
               (c == '!' || c == '@' || c == '#' || c == '$' || c == '%' ||
                c == '^' || c == '&' || c == '*' || c == '(' || c == ')' ||
                c == '_' || c == '+' || c == '{' || c == '}' || c == '|' ||
                c == ':' || c == '"' || c == '~' || c == '<' || c == '>' ||
                c == '?');
    }
};
