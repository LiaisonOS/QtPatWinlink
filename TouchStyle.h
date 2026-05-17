//
// Author  : Sylvain Deguire (VA2OPS)
// Date    : May 2026
// Purpose : Canonical touch mode style sheets and dimensions, matching QtDashboard look
//

#pragma once

#include <QString>

namespace touchStyle {

// ── Dimensions ──────────────────────────────────────────────────────────────
constexpr int tapTarget     = 48;   // standard touch button height (px)
constexpr int fieldHeight   = 44;   // QLineEdit min-height in touch
constexpr int radius        = 6;    // border-radius (px)
constexpr int buttonFontPx  = 15;   // button label size (matches QtDashboard)
constexpr int bodyFontPt    = 13;   // body / field font (pt)

// ── Colors (match QtDashboard) ──────────────────────────────────────────────
inline const QString bgDark        = "#111111";
inline const QString bgPanel       = "#2d2d2d";
inline const QString bgField       = "#1e1e1e";
inline const QString bgPressed     = "#444444";
inline const QString border        = "#404040";
inline const QString borderField   = "#333333";
inline const QString textPrimary   = "#e0e0e0";
inline const QString textMuted     = "#888888";
inline const QString accentGreen   = "#4ade80";
inline const QString accentGreenBg = "#1a3a1a";

// ── Button stylesheets ──────────────────────────────────────────────────────
// Neutral / secondary button — toolbar, Cancel, mode switches
inline const QString neutralButton =
    "QPushButton {"
    "  background: #2d2d2d; color: #e0e0e0;"
    "  border: 1px solid #404040; border-radius: 6px;"
    "  font-size: 15px; padding: 0 14px; min-height: 48px;"
    "}"
    "QPushButton:pressed { background: #444444; }"
    "QPushButton:checked { background: #1a3a1a; color: #4ade80; border-color: #4ade80; }"
    "QPushButton:disabled { background: #1a1a1a; color: #555555; border-color: #2a2a2a; }";

// Primary action — Send to Outbox, Connect
inline const QString primaryButton =
    "QPushButton {"
    "  background: #ffa500; color: #000000;"
    "  border: none; border-radius: 6px;"
    "  font-size: 15px; font-weight: bold; padding: 0 18px; min-height: 48px;"
    "}"
    "QPushButton:pressed { background: #cc7a00; }"
    "QPushButton:disabled { background: #5a3a00; color: #888888; }";

// Destructive action — Disconnect, Delete
inline const QString dangerButton =
    "QPushButton {"
    "  background: #c92a2a; color: white;"
    "  border: none; border-radius: 6px;"
    "  font-size: 15px; padding: 0 18px; min-height: 48px;"
    "}"
    "QPushButton:pressed { background: #a51e1e; }"
    "QPushButton:disabled { background: #5a2a2a; color: #888888; }";

// ── Popup menus (QMenu) ─────────────────────────────────────────────────────
inline const QString menuStyle =
    "QMenu {"
    "  background: #2d2d2d; color: #e0e0e0;"
    "  border: 1px solid #404040; padding: 6px;"
    "}"
    "QMenu::item {"
    "  padding: 14px 28px; font-size: 15px;"
    "  border-radius: 4px;"
    "}"
    "QMenu::item:selected { background: #ffa500; color: #000000; }"
    "QMenu::item:disabled { color: #666666; }"
    "QMenu::separator { height: 1px; background: #404040; margin: 6px 8px; }";

// ── Form fields ─────────────────────────────────────────────────────────────
inline const QString fieldStyle =
    "QLineEdit, QTextEdit, QPlainTextEdit {"
    "  background: #1e1e1e; color: #e0e0e0;"
    "  border: 1px solid #333333; border-radius: 6px;"
    "  padding: 8px 10px; font-size: 13pt;"
    "}"
    "QLineEdit { min-height: 44px; }"
    "QLabel { color: #e0e0e0; font-size: 13pt; }";

}  // namespace touchStyle
