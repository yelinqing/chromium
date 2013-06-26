// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/translate_internals/translate_internals_ui.h"

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/translate_internals/translate_internals_handler.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "grit/browser_resources.h"
#include "grit/translate_internals_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

// Sets the languages to |dict|. Each key is a language code and each value is
// a language name in the locale.
void GetLanguages(base::DictionaryValue* dict) {
  DCHECK(dict);

  const std::string app_locale = g_browser_process->GetApplicationLocale();
  std::vector<std::string> language_codes;
  l10n_util::GetAcceptLanguagesForLocale(app_locale, &language_codes);

  for (std::vector<std::string>::iterator it = language_codes.begin();
       it != language_codes.end(); ++it) {
    const std::string& lang_code = *it;
    string16 lang_name =
        l10n_util::GetDisplayNameForLocale(lang_code, app_locale, false);
    dict->SetString(lang_code, lang_name);
  }
}

content::WebUIDataSource* CreateTranslateInternalsHTMLSource() {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUITranslateInternalsHost);
  source->SetDefaultResource(IDR_TRANSLATE_INTERNALS_TRANSLATE_INTERNALS_HTML);
  source->SetJsonPath("strings.js");
  source->AddResourcePath("translate_internals.js",
                          IDR_TRANSLATE_INTERNALS_TRANSLATE_INTERNALS_JS);

  base::DictionaryValue langs;
  GetLanguages(&langs);
  for (base::DictionaryValue::Iterator it(langs); !it.IsAtEnd(); it.Advance()) {
    std::string key = "language-" + it.key();
    std::string value;
    it.value().GetAsString(&value);
    source->AddString(key, value);
  }

  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  bool enable_translate_settings =
      command_line.HasSwitch(switches::kEnableTranslateSettings);
  source->AddBoolean("enable-translate-settings", enable_translate_settings);

  return source;
}

}  // namespace

TranslateInternalsUI::TranslateInternalsUI(content::WebUI* web_ui)
    : WebUIController(web_ui) {
  web_ui->AddMessageHandler(new TranslateInternalsHandler);

  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, CreateTranslateInternalsHTMLSource());
}
