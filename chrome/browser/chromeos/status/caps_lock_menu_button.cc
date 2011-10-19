// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/status/caps_lock_menu_button.h"

#include <string>

#include "base/utf_string_conversions.h"
#include "chrome/browser/chromeos/input_method/xkeyboard.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/pref_names.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/canvas_skia.h"
#include "ui/gfx/font.h"
#include "views/controls/menu/menu_item_view.h"
#include "views/controls/menu/menu_runner.h"
#include "views/controls/menu/submenu_view.h"
#include "views/widget/widget.h"

namespace {

// Spacing between lines of text.
const int kSpacing = 3;
// Width and height of the image.
const int kImageWidth = 22, kImageHeight = 21;
// Constants for status displayed when user clicks button.
// Padding around status.
const int kPadLeftX = 10, kPadRightX = 10, kPadY = 5;
// Padding between image and text.
const int kTextPadX = 10;

// Returns PrefService object associated with |host|.
PrefService* GetPrefService(chromeos::StatusAreaHost* host) {
  if (host->GetProfile())
    return host->GetProfile()->GetPrefs();
  return NULL;
}

}  // namespace

namespace chromeos {

class CapsLockMenuButton::StatusView : public View {
 public:
  explicit StatusView(CapsLockMenuButton* menu_button)
      : menu_button_(menu_button),
        font_(ResourceBundle::GetSharedInstance().GetFont(
            ResourceBundle::BaseFont)) {
  }

  virtual ~StatusView() {
  }

  gfx::Size GetPreferredSize() {
    // TODO(yusukes): For better string localization, we should use either
    // IDS_STATUSBAR_CAPS_LOCK_ENABLED_PRESS_SHIFT_KEYS or
    // IDS_STATUSBAR_CAPS_LOCK_ENABLED_PRESS_SEARCH here instead of just
    // concatenating IDS_STATUSBAR_CAPS_LOCK_ENABLED and GetText(). Find a good
    // way to break the long text into lines and stop concatenating strings.
    const string16 first_line_text = l10n_util::GetStringUTF16(
        IDS_STATUSBAR_CAPS_LOCK_ENABLED);
    const string16 second_line_text = menu_button_->GetText();

    int first_line_width = 0;
    int second_line_width = 0;
    int height = 0;
    gfx::CanvasSkia::SizeStringInt(
        first_line_text, font_, &first_line_width, &height, 0);
    gfx::CanvasSkia::SizeStringInt(
        second_line_text, font_, &second_line_width, &height, 0);

    const gfx::Size size = gfx::Size(
        kPadLeftX + kImageWidth + kTextPadX + kPadRightX +
            std::max(first_line_width, second_line_width),
        (2 * kPadY) +
            std::max(kImageHeight, kSpacing + (2 * font_.GetHeight())));
    return size;
  }

  void Update() {
    PreferredSizeChanged();
    // Force a paint even if the size didn't change.
    SchedulePaint();
  }

 protected:
  void OnPaint(gfx::Canvas* canvas) {
    SkBitmap* image =
        ResourceBundle::GetSharedInstance().GetBitmapNamed(IDR_CAPS_LOCK_ICON);
    const int image_x = kPadLeftX;
    const int image_y = (height() - image->height()) / 2;
    canvas->DrawBitmapInt(*image, image_x, image_y);

    const string16 first_line_text = l10n_util::GetStringUTF16(
        IDS_STATUSBAR_CAPS_LOCK_ENABLED);
    const string16 second_line_text = menu_button_->GetText();

    const int text_height = font_.GetHeight();
    const int text_x = image_x + kImageWidth + kTextPadX;
    const int first_line_text_y =
        (height() - (kSpacing + (2 * text_height))) / 2;
    const int second_line_text_y = first_line_text_y + text_height + kSpacing;
    canvas->DrawStringInt(first_line_text, font_, SK_ColorBLACK,
                          text_x, first_line_text_y,
                          width() - text_x, text_height,
                          gfx::Canvas::TEXT_ALIGN_LEFT);
    canvas->DrawStringInt(second_line_text, font_, SK_ColorBLACK,
                          text_x, second_line_text_y,
                          width() - text_x, text_height,
                          gfx::Canvas::TEXT_ALIGN_LEFT);
  }

  bool OnMousePressed(const views::MouseEvent& event) {
    return true;
  }

  void OnMouseReleased(const views::MouseEvent& event) {
    if (event.IsLeftMouseButton()) {
      DCHECK(menu_button_->menu_runner_.get());
      menu_button_->menu_runner_->Cancel();
    }
  }

 private:
  CapsLockMenuButton* menu_button_;
  gfx::Font font_;

  DISALLOW_COPY_AND_ASSIGN(StatusView);
};

////////////////////////////////////////////////////////////////////////////////
// CapsLockMenuButton

CapsLockMenuButton::CapsLockMenuButton(StatusAreaHost* host)
    : StatusAreaButton(host, this),
      prefs_(GetPrefService(host)),
      status_(NULL) {
  if (prefs_)
    remap_search_key_to_.Init(
        prefs::kLanguageXkbRemapSearchKeyTo, prefs_, this);

  SetIcon(*ResourceBundle::GetSharedInstance().GetBitmapNamed(
      IDR_STATUSBAR_CAPS_LOCK));
  UpdateAccessibleName();
  UpdateUIFromCurrentCapsLock(input_method::XKeyboard::CapsLockIsEnabled());
  if (SystemKeyEventListener::GetInstance())
    SystemKeyEventListener::GetInstance()->AddCapsLockObserver(this);
}

CapsLockMenuButton::~CapsLockMenuButton() {
  if (SystemKeyEventListener::GetInstance())
    SystemKeyEventListener::GetInstance()->RemoveCapsLockObserver(this);
}

////////////////////////////////////////////////////////////////////////////////
// views::View implementation:

void CapsLockMenuButton::OnLocaleChanged() {
  UpdateUIFromCurrentCapsLock(input_method::XKeyboard::CapsLockIsEnabled());
}

////////////////////////////////////////////////////////////////////////////////
// views::ViewMenuDelegate implementation:

string16 CapsLockMenuButton::GetLabel(int id) const {
  return string16();
}

bool CapsLockMenuButton::IsCommandEnabled(int id) const {
  return false;
}

void CapsLockMenuButton::RunMenu(views::View* source, const gfx::Point& pt) {
  static const int kDummyCommandId = 1000;

  views::MenuItemView* menu = new views::MenuItemView(this);
  // MenuRunner takes ownership of |menu|.
  menu_runner_.reset(new views::MenuRunner(menu));
  views::MenuItemView* submenu = menu->AppendMenuItem(
      kDummyCommandId,
      L"",
      views::MenuItemView::NORMAL);
  status_ = new CapsLockMenuButton::StatusView(this);
  submenu->AddChildView(status_);
  menu->CreateSubmenu()->set_resize_open_menu(true);
  menu->SetMargins(0, 0);
  submenu->SetMargins(0, 0);
  menu->ChildrenChanged();

  gfx::Point screen_location;
  views::View::ConvertPointToScreen(source, &screen_location);
  gfx::Rect bounds(screen_location, source->size());
  if (menu_runner_->RunMenuAt(
          source->GetWidget()->GetTopLevelWidget(), this, bounds,
          views::MenuItemView::TOPRIGHT, views::MenuRunner::HAS_MNEMONICS) ==
      views::MenuRunner::MENU_DELETED)
    return;
  status_ = NULL;
  menu_runner_.reset(NULL);
}

////////////////////////////////////////////////////////////////////////////////
// SystemKeyEventListener::CapsLockObserver implementation

void CapsLockMenuButton::OnCapsLockChange(bool enabled) {
  UpdateUIFromCurrentCapsLock(enabled);
}

////////////////////////////////////////////////////////////////////////////////
// content::NotificationObserver implementation

void CapsLockMenuButton::Observe(int type,
                                 const content::NotificationSource& source,
                                 const content::NotificationDetails& details) {
  if (type == chrome::NOTIFICATION_PREF_CHANGED)
    UpdateAccessibleName();
}

void CapsLockMenuButton::UpdateAccessibleName() {
  int id = IDS_STATUSBAR_CAPS_LOCK_ENABLED_PRESS_SHIFT_KEYS;
  if (prefs_ && (remap_search_key_to_.GetValue() == input_method::kCapsLockKey))
    id = IDS_STATUSBAR_CAPS_LOCK_ENABLED_PRESS_SEARCH;
  SetAccessibleName(l10n_util::GetStringUTF16(id));
}

string16 CapsLockMenuButton::GetText() const {
  int id = IDS_STATUSBAR_PRESS_SHIFT_KEYS;
  if (prefs_ && (remap_search_key_to_.GetValue() == input_method::kCapsLockKey))
    id = IDS_STATUSBAR_PRESS_SEARCH;
  return l10n_util::GetStringUTF16(id);
}

void CapsLockMenuButton::UpdateUIFromCurrentCapsLock(bool enabled) {
  SetVisible(enabled);
  SchedulePaint();
  if (enabled && status_)
    status_->Update();
  if (!enabled && menu_runner_.get())
    menu_runner_->Cancel();
}

}  // namespace chromeos
