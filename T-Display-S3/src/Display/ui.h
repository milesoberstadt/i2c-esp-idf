#if !defined(__UI_H__)
#define __UI_H__

#include <memory>

#include "Arduino.h"
#include "Arduino_GFX_Library.h"

#define GFX_DEV_DEVICE LILYGO_T_DISPLAY_S3
#define GFX_BL 38

class UI 
{
    public:
        static UI& instance()
        {
            static UI instance;
            return instance;
        }

        UI(const UI&) = delete;
        UI& operator=(const UI&) = delete;

        Arduino_GFX* getGFX() { return gfx.get(); }

        void background();
        void background(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
        void titleStyle();
        void textStyle();
        void smallTextStyle();

        uint8_t width() { return w; }
        uint8_t height() { return h; }
        uint8_t size() { return n; }
        uint8_t center_x() { return cx; }
        uint8_t center_y() { return cy; }

    private:
        UI();
        ~UI();
        int32_t w, h, n, n1, cx, cy, cx1, cy1, cn, cn1;
        uint8_t tsa, tsb, tsc, ds;
        std::unique_ptr<Arduino_GFX> gfx;
        std::unique_ptr<Arduino_DataBus> bus;
};

#endif // __UI_H__
