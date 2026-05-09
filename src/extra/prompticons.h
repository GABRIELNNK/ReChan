#pragma once

#include "gen/config.h"

#if CUSTOM_MENU

#include "core.h"
#include "pc/inputaction.h"
#include "pc/tim.h"
#include "p3d/texture.h"
#include <cstring>

class PromptIcons {
public:
    struct PromptRect {
        s16 x = -1;
        s16 y = -1;
        s16 w = 0;
        s16 h = 0;

        bool IsValid() const {
            return x >= 0 && y >= 0 && w > 0 && h > 0;
        }
    };

    struct ResolvedPrompt {
        tTexture* tex = nullptr;
        s32 texW = 0;
        s32 texH = 0;
        PromptRect rect;
        char fallback[24] = {};

        bool HasIcon() const {
            return tex && texW > 0 && texH > 0 && rect.IsValid();
        }

        bool HasFallback() const {
            return fallback[0] != '\0';
        }
    };

    struct PromptGrid {
        s16 baseTexW = 0;
        s16 baseTexH = 0;
        s16 originX = 0;
        s16 originY = 0;
        s16 cellW = 0;
        s16 cellH = 0;
        s16 stepX = 0;
        s16 stepY = 0;

        bool IsValid() const {
            return baseTexW > 0 && baseTexH > 0 && cellW > 0 && cellH > 0;
        }
    };

    struct PromptGridIndex {
        const PromptGrid* grid = nullptr;
        s16 col = -1;
        s16 row = -1;

        bool IsValid() const {
            return grid && grid->IsValid() && col >= 0 && row >= 0;
        }
    };

    static bool ParseInlineActionToken(const char* text, s32 start, s32* outConsumed, Action* outAction) {
        if (!text || start < 0 || text[start] != '<') {
            return false;
        }

        static const char kPrefix[] = "<ACT:";
        const s32 prefixLen = (s32)(sizeof(kPrefix) - 1);
        if (strncmp(text + start, kPrefix, prefixLen) != 0) {
            return false;
        }

        const char* tokenStart = text + start + prefixLen;
        const char* tokenEnd = strchr(tokenStart, '>');
        if (!tokenEnd || tokenEnd == tokenStart) {
            return false;
        }

        char token[32] = {};
        s32 tokenLen = (s32)(tokenEnd - tokenStart);
        if (tokenLen >= (s32)sizeof(token)) {
            return false;
        }
        memcpy(token, tokenStart, tokenLen);
        token[tokenLen] = '\0';

        Action action = ::ActionFromToken(token);
        if (action < 0 || action >= ACTION_COUNT) {
            return false;
        }

        if (outConsumed) {
            *outConsumed = prefixLen + tokenLen + 1;
        }
        if (outAction) {
            *outAction = action;
        }
        return true;
    }

    static bool ResolveActionPrompt(Action action, ResolvedPrompt& out) {
        out = {};

        const bool useGamepad = g_actionInput && g_actionInput->IsGamepadActive();
        if (useGamepad) {
            const s32 button = g_actionInput ? g_actionInput->GetGamepadButtonBinding(action) : GpBtn::NONE;
            CopyText(out.fallback, (s32)sizeof(out.fallback), GamepadButtonToLabel(button));

            SheetState& sheet = GetXboxSheet();
            if (!EnsureSheetLoaded(sheet, false)) {
                return out.HasFallback();
            }

            PromptRect rect = GamepadButtonToRect(button, sheet.width, sheet.height);
            if (!rect.IsValid()) {
                return out.HasFallback();
            }

            out.tex = sheet.tex;
            out.texW = sheet.width;
            out.texH = sheet.height;
            out.rect = rect;
            return true;
        }

        const s32 key = g_actionInput ? g_actionInput->GetKeyBinding(action) : 0;
        CopyText(out.fallback, (s32)sizeof(out.fallback), KeyToLabel(key));

        SheetState& sheet = GetKeyboardSheet();
        if (!EnsureSheetLoaded(sheet, true)) {
            return out.HasFallback();
        }

        PromptRect rect = KeyToRect(key, sheet.width, sheet.height);
        if (!rect.IsValid()) {
            return out.HasFallback();
        }

        out.tex = sheet.tex;
        out.texW = sheet.width;
        out.texH = sheet.height;
        out.rect = rect;
        return true;
    }

    static f32 GetDrawWidth(const ResolvedPrompt& prompt, f32 drawHeight) {
        if (!prompt.HasIcon() || drawHeight <= 0.0f) {
            return 0.0f;
        }
        return drawHeight * ((f32)prompt.rect.w / (f32)prompt.rect.h);
    }

    static void DrawPrompt(const ResolvedPrompt& prompt, f32 x, f32 y, f32 drawHeight, u8 alpha) {
        if (!prompt.HasIcon() || drawHeight <= 0.0f) {
            return;
        }

        const f32 drawWidth = GetDrawWidth(prompt, drawHeight);
        const f32 u0 = (f32)prompt.rect.x / (f32)prompt.texW;
        const f32 v0 = (f32)prompt.rect.y / (f32)prompt.texH;
        const f32 u1 = (f32)(prompt.rect.x + prompt.rect.w) / (f32)prompt.texW;
        const f32 v1 = (f32)(prompt.rect.y + prompt.rect.h) / (f32)prompt.texH;
        ScreenDraw::DrawQuad(prompt.tex, x, y, drawWidth, drawHeight,
                             u0, v0, u1, v1,
                             128, 128, 128, alpha);
    }

private:
    struct SheetState {
        tTexture* tex = nullptr;
        s32 width = 0;
        s32 height = 0;
        bool tried = false;
    };

    static void CopyText(char* dst, s32 dstLen, const char* src) {
        if (!dst || dstLen <= 0) {
            return;
        }
        dst[0] = '\0';
        if (!src) {
            return;
        }
        s32 write = 0;
        while (write + 1 < dstLen && src[write] != '\0') {
            dst[write] = src[write];
            write++;
        }
        dst[write] = '\0';
    }

    static const char* KeyToLabel(s32 key) {
        static char dynLabel[24];

        if (key >= KEY_A && key <= KEY_Z) {
            dynLabel[0] = (char)key;
            dynLabel[1] = '\0';
            return dynLabel;
        }

        if (key >= KEY_0 && key <= KEY_9) {
            dynLabel[0] = (char)key;
            dynLabel[1] = '\0';
            return dynLabel;
        }

        if (key >= KEY_F1 && key <= KEY_F12) {
            const s32 fn = key - KEY_F1 + 1;
            dynLabel[0] = 'F';
            if (fn >= 10) {
                dynLabel[1] = (char)('0' + (fn / 10));
                dynLabel[2] = (char)('0' + (fn % 10));
                dynLabel[3] = '\0';
            }
            else {
                dynLabel[1] = (char)('0' + fn);
                dynLabel[2] = '\0';
            }
            return dynLabel;
        }

        switch (key) {
            case KEY_ENTER: return "Enter";
            case KEY_ESCAPE: return "Esc";
            case KEY_LEFT: return "Left";
            case KEY_RIGHT: return "Right";
            case KEY_UP: return "Up";
            case KEY_DOWN: return "Down";
            case KEY_SPACE: return "Space";
            case KEY_TAB: return "Tab";
            case KEY_BACKSPACE: return "Backsp";
            case KEY_INSERT: return "Ins";
            case KEY_DELETE: return "Del";
            case KEY_HOME: return "Home";
            case KEY_END: return "End";
            case KEY_PAGE_UP: return "PgUp";
            case KEY_PAGE_DOWN: return "PgDn";
            case KEY_CAPS_LOCK: return "Caps";
            case KEY_LEFT_SHIFT: return "LShift";
            case KEY_RIGHT_SHIFT: return "RShift";
            case KEY_LEFT_CONTROL: return "LCtrl";
            case KEY_RIGHT_CONTROL: return "RCtrl";
            case KEY_LEFT_ALT: return "LAlt";
            case KEY_RIGHT_ALT: return "RAlt";
            case KEY_MINUS: return "-";
            case KEY_EQUAL: return "=";
            case KEY_LEFT_BRACKET: return "[";
            case KEY_RIGHT_BRACKET: return "]";
            case KEY_BACKSLASH: return "\\";
            case KEY_SEMICOLON: return ";";
            case KEY_APOSTROPHE: return "'";
            case KEY_COMMA: return ",";
            case KEY_PERIOD: return ".";
            case KEY_SLASH: return "/";
            case KEY_GRAVE: return "`";
            default: return nullptr;
        }
    }

    static const char* GamepadButtonToLabel(s32 button) {
        switch (button) {
            case GpBtn::A: return "A";
            case GpBtn::B: return "B";
            case GpBtn::X: return "X";
            case GpBtn::Y: return "Y";
            case GpBtn::LB: return "LB";
            case GpBtn::RB: return "RB";
            case GpBtn::Back: return "View";
            case GpBtn::Start: return "Menu";
            case GpBtn::DpadLeft:
            case GpBtn::DpadRight:
            case GpBtn::DpadUp:
            case GpBtn::DpadDown:
                return "D-Pad";
            default: return nullptr;
        }
    }

    static PromptRect KeyToRect(s32 key, s32 texW, s32 texH) {
        PromptGridIndex idx;
        switch (key) {
            case KEY_A: idx = MakeGridIndex(GetKeyboardMainGrid(), 0, 0); break;
            case KEY_B: idx = MakeGridIndex(GetKeyboardMainGrid(), 1, 0); break;
            case KEY_C: idx = MakeGridIndex(GetKeyboardMainGrid(), 2, 0); break;
            case KEY_D: idx = MakeGridIndex(GetKeyboardMainGrid(), 3, 0); break;
            case KEY_E: idx = MakeGridIndex(GetKeyboardMainGrid(), 4, 0); break;
            case KEY_F: idx = MakeGridIndex(GetKeyboardMainGrid(), 5, 0); break;
            case KEY_G: idx = MakeGridIndex(GetKeyboardMainGrid(), 6, 0); break;
            case KEY_H: idx = MakeGridIndex(GetKeyboardMainGrid(), 7, 0); break;
            case KEY_I: idx = MakeGridIndex(GetKeyboardMainGrid(), 8, 0); break;
            case KEY_J: idx = MakeGridIndex(GetKeyboardMainGrid(), 9, 0); break;

            case KEY_K: idx = MakeGridIndex(GetKeyboardMainGrid(), 0, 1); break;
            case KEY_L: idx = MakeGridIndex(GetKeyboardMainGrid(), 1, 1); break;
            case KEY_M: idx = MakeGridIndex(GetKeyboardMainGrid(), 2, 1); break;
            case KEY_N: idx = MakeGridIndex(GetKeyboardMainGrid(), 3, 1); break;
            case KEY_O: idx = MakeGridIndex(GetKeyboardMainGrid(), 4, 1); break;
            case KEY_P: idx = MakeGridIndex(GetKeyboardMainGrid(), 5, 1); break;
            case KEY_Q: idx = MakeGridIndex(GetKeyboardMainGrid(), 6, 1); break;
            case KEY_R: idx = MakeGridIndex(GetKeyboardMainGrid(), 7, 1); break;
            case KEY_S: idx = MakeGridIndex(GetKeyboardMainGrid(), 8, 1); break;
            case KEY_T: idx = MakeGridIndex(GetKeyboardMainGrid(), 9, 1); break;

            case KEY_U: idx = MakeGridIndex(GetKeyboardMainGrid(), 0, 2); break;
            case KEY_V: idx = MakeGridIndex(GetKeyboardMainGrid(), 1, 2); break;
            case KEY_X: idx = MakeGridIndex(GetKeyboardMainGrid(), 2, 2); break;
            case KEY_W: idx = MakeGridIndex(GetKeyboardMainGrid(), 3, 2); break;
            case KEY_Y: idx = MakeGridIndex(GetKeyboardMainGrid(), 4, 2); break;
            case KEY_Z: idx = MakeGridIndex(GetKeyboardMainGrid(), 5, 2); break;

            case KEY_0: idx = MakeGridIndex(GetKeyboardMainGrid(), 6, 2); break;
            case KEY_1: idx = MakeGridIndex(GetKeyboardMainGrid(), 7, 2); break;
            case KEY_2: idx = MakeGridIndex(GetKeyboardMainGrid(), 8, 2); break;
            case KEY_3: idx = MakeGridIndex(GetKeyboardMainGrid(), 9, 2); break;
            case KEY_4: idx = MakeGridIndex(GetKeyboardMainGrid(), 0, 3); break;
            case KEY_5: idx = MakeGridIndex(GetKeyboardMainGrid(), 1, 3); break;
            case KEY_6: idx = MakeGridIndex(GetKeyboardMainGrid(), 2, 3); break;
            case KEY_7: idx = MakeGridIndex(GetKeyboardMainGrid(), 3, 3); break;
            case KEY_8: idx = MakeGridIndex(GetKeyboardMainGrid(), 4, 3); break;
            case KEY_9: idx = MakeGridIndex(GetKeyboardMainGrid(), 5, 3); break;

            case KEY_F1: idx = MakeGridIndex(GetKeyboardMainGrid(), 6, 3); break;
            case KEY_F2: idx = MakeGridIndex(GetKeyboardMainGrid(), 7, 3); break;
            case KEY_F3: idx = MakeGridIndex(GetKeyboardMainGrid(), 8, 3); break;
            case KEY_F4: idx = MakeGridIndex(GetKeyboardMainGrid(), 9, 3); break;
            case KEY_F5: idx = MakeGridIndex(GetKeyboardMainGrid(), 0, 4); break;
            case KEY_F6: idx = MakeGridIndex(GetKeyboardMainGrid(), 1, 4); break;
            case KEY_F7: idx = MakeGridIndex(GetKeyboardMainGrid(), 2, 4); break;
            case KEY_F8: idx = MakeGridIndex(GetKeyboardMainGrid(), 3, 4); break;
            case KEY_F9: idx = MakeGridIndex(GetKeyboardMainGrid(), 4, 4); break;
            case KEY_F10: idx = MakeGridIndex(GetKeyboardMainGrid(), 5, 4); break;
            case KEY_F11: idx = MakeGridIndex(GetKeyboardMainGrid(), 6, 4); break;
            case KEY_F12: idx = MakeGridIndex(GetKeyboardMainGrid(), 7, 4); break;

            case KEY_MINUS: idx = MakeGridIndex(GetKeyboardMainGrid(), 8, 4); break;
            case KEY_EQUAL: idx = MakeGridIndex(GetKeyboardMainGrid(), 9, 4); break;
            case KEY_GRAVE: idx = MakeGridIndex(GetKeyboardMainGrid(), 0, 5); break;
            case KEY_SEMICOLON: idx = MakeGridIndex(GetKeyboardMainGrid(), 2, 5); break;
            case KEY_SLASH: idx = MakeGridIndex(GetKeyboardMainGrid(), 3, 5); break;
            case KEY_LEFT_BRACKET: idx = MakeGridIndex(GetKeyboardMainGrid(), 4, 5); break;
            case KEY_RIGHT_BRACKET: idx = MakeGridIndex(GetKeyboardMainGrid(), 5, 5); break;
            case KEY_APOSTROPHE: idx = MakeGridIndex(GetKeyboardMainGrid(), 6, 5); break;
            case KEY_BACKSLASH: idx = MakeGridIndex(GetKeyboardMainGrid(), 7, 5); break;
            case KEY_LEFT_ALT:
            case KEY_RIGHT_ALT:
                idx = MakeGridIndex(GetKeyboardMainGrid(), 8, 5);
                break;
            case KEY_COMMA: idx = MakeGridIndex(GetKeyboardMainGrid(), 9, 5); break;

            case KEY_PERIOD:
                return RectFromAtlasPixels(5, 323, 32, 33, texW, texH);

            case KEY_ENTER:
                return RectFromAtlasPixels(162, 372, 34, 33, texW, texH);
            case KEY_ESCAPE:
                return RectFromAtlasPixels(5, 424, 32, 33, texW, texH);
            case KEY_TAB:
                return RectFromAtlasPixels(213, 477, 34, 33, texW, texH);
            case KEY_BACKSPACE:
                return RectFromAtlasPixels(317, 477, 32, 33, texW, texH);
            case KEY_INSERT:
                return RectFromAtlasPixels(265, 477, 32, 33, texW, texH);
            case KEY_DELETE:
                return RectFromAtlasPixels(317, 424, 32, 33, texW, texH);

            case KEY_LEFT:
                return RectFromAtlasPixels(5, 477, 32, 33, texW, texH);
            case KEY_RIGHT:
                return RectFromAtlasPixels(57, 477, 32, 33, texW, texH);
            case KEY_UP:
                return RectFromAtlasPixels(421, 424, 32, 33, texW, texH);
            case KEY_DOWN:
                return RectFromAtlasPixels(473, 424, 32, 33, texW, texH);

            case KEY_PAGE_UP:
                return RectFromAtlasPixels(213, 424, 32, 33, texW, texH);
            case KEY_PAGE_DOWN:
                return RectFromAtlasPixels(161, 424, 32, 33, texW, texH);
            case KEY_HOME:
                return RectFromAtlasPixels(161, 477, 32, 33, texW, texH);
            case KEY_END:
                return RectFromAtlasPixels(109, 424, 32, 33, texW, texH);

            case KEY_CAPS_LOCK:
                return RectFromAtlasPixels(473, 372, 39, 33, texW, texH);
            case KEY_LEFT_SHIFT:
                return RectFromAtlasPixels(317, 372, 44, 33, texW, texH);
            case KEY_RIGHT_SHIFT:
                return RectFromAtlasPixels(369, 372, 44, 33, texW, texH);
            case KEY_LEFT_CONTROL:
            case KEY_RIGHT_CONTROL:
                return RectFromAtlasPixels(57, 424, 32, 33, texW, texH);
            case KEY_SPACE:
                return RectFromAtlasPixels(369, 424, 47, 33, texW, texH);

            default:
                return {};
        }
        return RectFromGridIndex(idx, texW, texH);
    }

    static PromptRect GamepadButtonToRect(s32 button, s32 texW, s32 texH) {
        PromptGridIndex idx;
        switch (button) {
            case GpBtn::Y:
                idx = MakeGridIndex(GetControllerFaceGrid(), 0, 0);
                break;
            case GpBtn::B:
                idx = MakeGridIndex(GetControllerFaceGrid(), 1, 0);
                break;
            case GpBtn::A:
                idx = MakeGridIndex(GetControllerFaceGrid(), 2, 0);
                break;
            case GpBtn::X:
                idx = MakeGridIndex(GetControllerFaceGrid(), 3, 0);
                break;
            case GpBtn::Start:
                idx = MakeGridIndex(GetControllerMenuGrid(), 0, 0);
                break;
            case GpBtn::Back:
                idx = MakeGridIndex(GetControllerMenuGrid(), 1, 0);
                break;
            case GpBtn::DpadLeft:
            case GpBtn::DpadRight:
            case GpBtn::DpadUp:
            case GpBtn::DpadDown:
                idx = MakeGridIndex(GetControllerDPadGrid(), 0, 0);
                break;
            default:
                return {};
        }
        return RectFromGridIndex(idx, texW, texH);
    }

    static PromptGridIndex MakeGridIndex(const PromptGrid& grid, s16 col, s16 row) {
        PromptGridIndex idx;
        idx.grid = &grid;
        idx.col = col;
        idx.row = row;
        return idx;
    }

    static PromptRect RectFromGridIndex(const PromptGridIndex& idx, s32 texW, s32 texH) {
        if (!idx.IsValid() || texW <= 0 || texH <= 0) {
            return {};
        }

        const PromptGrid& grid = *idx.grid;
        const f32 scaleX = (f32)texW / (f32)grid.baseTexW;
        const f32 scaleY = (f32)texH / (f32)grid.baseTexH;

        s32 x = (s32)(((f32)grid.originX + (f32)idx.col * (f32)grid.stepX) * scaleX + 0.5f);
        s32 y = (s32)(((f32)grid.originY + (f32)idx.row * (f32)grid.stepY) * scaleY + 0.5f);
        s32 w = (s32)((f32)grid.cellW * scaleX + 0.5f);
        s32 h = (s32)((f32)grid.cellH * scaleY + 0.5f);

        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= texW || y >= texH) {
            return {};
        }
        if (x + w > texW) w = texW - x;
        if (y + h > texH) h = texH - y;
        if (w <= 0 || h <= 0) {
            return {};
        }

        PromptRect out;
        out.x = (s16)x;
        out.y = (s16)y;
        out.w = (s16)w;
        out.h = (s16)h;
        return out;
    }

    static PromptRect RectFromAtlasPixels(s16 baseX, s16 baseY, s16 baseW, s16 baseH, s32 texW, s32 texH) {
        if (texW <= 0 || texH <= 0 || baseW <= 0 || baseH <= 0) {
            return {};
        }

        const f32 scaleX = (f32)texW / 512.0f;
        const f32 scaleY = (f32)texH / 512.0f;

        s32 x = (s32)((f32)baseX * scaleX + 0.5f);
        s32 y = (s32)((f32)baseY * scaleY + 0.5f);
        s32 w = (s32)((f32)baseW * scaleX + 0.5f);
        s32 h = (s32)((f32)baseH * scaleY + 0.5f);

        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= texW || y >= texH) {
            return {};
        }
        if (x + w > texW) w = texW - x;
        if (y + h > texH) h = texH - y;
        if (w <= 0 || h <= 0) {
            return {};
        }

        PromptRect out;
        out.x = (s16)x;
        out.y = (s16)y;
        out.w = (s16)w;
        out.h = (s16)h;
        return out;
    }

    static const PromptGrid& GetControllerFaceGrid() {
        static const PromptGrid grid = {
            632, 1808, // base atlas size
            16, 16,    // first glyph top-left
            96, 96,    // glyph size
            168, 168   // column/row step
        };
        return grid;
    }

    static const PromptGrid& GetControllerMenuGrid() {
        static const PromptGrid grid = {
            632, 1808,
            16, 1696,
            96, 96,
            154, 154
        };
        return grid;
    }

    static const PromptGrid& GetControllerDPadGrid() {
        static const PromptGrid grid = {
            632, 1808,
            4, 1350,
            120, 121,
            120, 121
        };
        return grid;
    }

    static const PromptGrid& GetKeyboardMainGrid() {
        static const PromptGrid grid = {
            512, 512,
            5, 5,
            32, 32,
            52, 53
        };
        return grid;
    }

    static bool EnsureSheetLoaded(SheetState& sheet, bool keyboard) {
        if (sheet.tex) {
            return true;
        }
        if (sheet.tried) {
            return false;
        }
        sheet.tried = true;

        const char* path = keyboard
            ? "pc/textures/frontend/keyboard_mouse_sheet_default.png"
            : "pc/textures/frontend/controller_sheet_default.png";
        sheet.tex = tTexture::LoadFromImagePath(path, &sheet.width, &sheet.height);
        return sheet.tex != nullptr;
    }

    static SheetState& GetKeyboardSheet() {
        static SheetState sheet;
        return sheet;
    }

    static SheetState& GetXboxSheet() {
        static SheetState sheet;
        return sheet;
    }
};

#endif