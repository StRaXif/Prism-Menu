#include <Geode/utils/file.hpp>
#include <Geode/modify/MenuLayer.hpp>
#ifndef NO_IMGUI
#include <imgui-cocos.hpp>
#include <codecvt>
#include <algorithm>
#include <cctype>
#include "PrismButton.hpp"
#include "hacks.hpp"
#include "Languages.hpp"
#include "Themes.hpp"

int currentMenuIndex;

struct EditMode {
    std::string name = "";
    HackValue editedValue = 0.F;
    bool isEditing = false;
};

EditMode editingMode;

bool firstLoadI = false;
bool changedOpacity = false;
bool changedMenuScale = false;

// ---------------------------------------------------------------------------
// Eclipse-style layout helpers
// ---------------------------------------------------------------------------
static char g_searchBuf[64] = "";

static std::string toLowerStr(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

// language keys start with an icon glyph + space ("§ Global"), the window title doesn't need the icon
static std::string stripIcon(const std::string& s) {
    if (s.empty() || static_cast<unsigned char>(s[0]) < 0x80) return s; // no icon glyph
    auto pos = s.find(' ');
    return pos == std::string::npos ? s : s.substr(pos + 1);
}

static bool matchesSearch(const matjson::Value& obj, const std::unique_ptr<Lang>& lang) {
    if (g_searchBuf[0] == '\0') return true;
    std::string name = obj.get("name").unwrapOr("Name").asString().unwrapOrDefault();
    auto shown = toLowerStr(lang->name(name));
    return shown.find(toLowerStr(g_searchBuf)) != std::string::npos
        || toLowerStr(name).find(toLowerStr(g_searchBuf)) != std::string::npos;
}

// ---------------------------------------------------------------------------
// One hack row (logic is unchanged, it was just moved out of the old big loop)
// ---------------------------------------------------------------------------
static void renderHackItem(const matjson::Value& obj, const std::unique_ptr<Lang>& currentLanguage, PrismButton* prismButton) {
    auto settings = Mod::get()->getSavedValue<SettingHackStruct>("values");
    matjson::Value val = 0;
    {
                        const auto& obj = jsonArray[i];
                        std::string name = obj.get("name").unwrapOr("Name").asString().unwrapOrDefault();
                        std::string desc = obj.get("desc").unwrapOr("Desc").asString().unwrapOrDefault();
                        HackItem* hack = Hacks::getHack(name);
                        if (hack == nullptr) return;
                        auto opcodes = hack->opcodes;
                        if (hack != nullptr) {
                            if (obj.contains("win")) { // yeah idk what property okay
                                #ifndef GEODE_IS_WINDOWS
                                ImGui::BeginDisabled(true);
                                #endif
                            }
                            if (hack->value.type == ValueType::Int && hack->type != "dropdown" && hack->type != "char") {
                                int min = obj.get("min").unwrapOr(val).asInt().unwrapOrDefault();
                                int max = obj.get("min").unwrapOr(val).asInt().unwrapOrDefault();
                                int step = (obj.contains("step")) ? obj.get("step").unwrapOr(val).asInt().unwrapOrDefault() : 5;
                                int oldValue = hack->value.intValue;
                                if (ImGui::InputInt(currentLanguage->name(name).c_str(), &hack->value.intValue, step, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
                                    if (min > hack->value.intValue || hack->value.intValue > max) {
                                        hack->value.intValue = oldValue;
                                        return;
                                    }
                                    auto prismButton = OverlayManager::get()->getChildByID("prism-icon");
                                    if (name == "FPS Bypass") {
                                        // from mats fps unlocker
                                        //Hacks::Settings::setSettingValue(&settings, *hack, hack->value.floatValue);
                                        
                                        auto GM = GameManager::sharedState();
                                        auto app = CCApplication::sharedApplication();
                                        app->setAnimationInterval(1.0 / static_cast<double>(hack->value.intValue));
                                        // ^^^ is this even necessary
                                        GM->m_customFPSTarget = hack->value.intValue;
                                        GM->setGameVariable("0116", true);
                                        GM->updateCustomFPS();
                                    } else if (name == "Button Position X") {
                                        Hacks::Settings::setSettingValue(&settings, *hack, hack->value.intValue);
                                        if (prismButton != nullptr) prismButton->setPositionX(hack->value.intValue);
                                    } else if (name == "Button Position Y") {
                                        Hacks::Settings::setSettingValue(&settings, *hack, hack->value.intValue);
                                        if (prismButton != nullptr) prismButton->setPositionY(hack->value.intValue);
                                    } else if (name == "Menu-Style") {
                                        Hacks::Settings::setSettingValue(&settings, *hack, hack->value.intValue);
                                        static_cast<PrismButton*>(prismButton)->showImGuiMenu = false;
                                    } else {
                                        Hacks::Settings::setSettingValue(&settings, *hack, hack->value.intValue);
                                    }
                                }
                            } else if (hack->value.type == ValueType::Float) {
                                int min = obj.get("min").unwrapOr(val).asInt().unwrapOrDefault();
                                int max = obj.get("min").unwrapOr(val).asInt().unwrapOrDefault();
                                if (!editingMode.isEditing || editingMode.name != name.c_str()) {
                                    if (ImGui::SliderFloat(currentLanguage->name(name).c_str(), &hack->value.floatValue, min, max, "%.3f", ImGuiSliderFlags_NoInput)) {
                                        if (name == "Speedhack") {
                                            if (hack->value.floatValue < 0.0F) return;
                                            Hacks::setPitch(hack->value.floatValue);
                                        } else {
                                            if (name == "Menu Opacity") {
                                                changedOpacity = true;
                                            } else if (name == "Menu Scale") {
                                                changedMenuScale = true;
                                            }
                                            Hacks::Settings::setSettingValue(&settings, *hack, hack->value.floatValue);
                                        }
                                    }
                                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                                        editingMode.isEditing = true;
                                        editingMode.name = hack->name;
                                        editingMode.editedValue.floatValue = hack->value.floatValue;
                                    }
                                } else {
                                    ImGui::BeginDisabled();
                                    ImGui::SliderFloat(currentLanguage->name(name).c_str(), &editingMode.editedValue.floatValue, min, max);
                                    ImGui::EndDisabled();
                                    ImGui::SetKeyboardFocusHere();
                                    if (ImGui::InputFloat("##EditValue", &editingMode.editedValue.floatValue, 0, 0, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) {
                                        hack->value.floatValue = editingMode.editedValue.floatValue;
                                        editingMode.isEditing = false;
                                        editingMode.name = "";
                                        Hacks::Settings::setSettingValue(&settings, *hack, hack->value.floatValue);
                                        if (name == "Speedhack") {
                                            if (hack->value.floatValue < 0.0F) {
                                                hack->value.floatValue = 1.0F;
                                            }
                                            Hacks::setPitch(hack->value.floatValue);
                                        } else if (name == "Menu Opacity") {
                                            changedOpacity = true;
                                        } else if (name == "Menu Scale") {
                                            changedMenuScale = true;
                                        }
                                    }
                                    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                                        editingMode.isEditing = false;
                                        editingMode.name = "";
                                    }
                                }
                            } else if (hack->value.type == ValueType::Bool) {
                                if ((Hacks::isHackEnabled("Enable Patching") && obj.contains("winOnly")) || !obj.contains("winOnly") || name == "Enable Patching") {
                                    if (ImGui::Checkbox(currentLanguage->name(name).c_str(), &hack->value.boolValue)) {
#if 0
                                        Hacks::Settings::setSettingValue(&settings, *hack, hack->value.boolValue);
                                        if (name == "Show Button") {
                                            auto prismButton = OverlayManager::get()->getChildByID("prism-icon");
                                            if (prismButton != nullptr) prismButton->setVisible(hack->value.boolValue);
                                        }
                                        if (name == "Instant Complete" && hack->value.boolValue) {
                                            FLAlertLayer::create(nullptr, "Cheater!", "Just a warning, you will be <cr>banned off leaderboards</c> if you use this on rated levels. Consider this your <cy>warning</c>.", "OK", nullptr)->show();
                                        }
                                        if (Hacks::isHackEnabled("Enable Patching") && !opcodes.empty()) {
                                            #ifdef GEODE_IS_WINDOWS
                                            Hacks::applyPatches(name, opcodes, hack->value.boolValue);
                                            #endif
                                        }
#endif
                                    }
                                }
                            }
                            #if 0 
                            else if (hack->value.type == ValueType::Char) {
                                auto oldValue = hack->value.charValue;
                                if (ImGui::InputTextWithHint(currentLanguage->name(name).c_str(), "C", hack->value.charValue, 2, ImGuiInputTextFlags_EnterReturnsTrue)) {
                                    if (name == "Open Menu Keybind") {
                                        auto it = charToKeyMap.find(*hack->value.charValue);
                                        if (it != charToKeyMap.end()) {
                                            std::cout << "Save" << std::endl;
                                            auto inputKeybind = it->second;
                                            BindManager::get()->removeBindable("prism-menu"_spr);
                                            BindManager::get()->registerBindable({
                                                "prism-menu"_spr,
                                                "Toggle Prism Menu",
                                                "Keybind for toggling when to show Prism Menu's Mod Menu.",
                                                { Keybind::create(KEY_A, Modifier::None) },
                                                "Prism Menu/ModMenu"
                                            });
                                            BindManager::get()->save();
                                        } else {
                                            hack->value.charValue = oldValue;
                                        }
                                        
                                    }
                                }
                            #endif
                            else if (hack->type == "dropdown" || hack->value.type == ValueType::Custom) {
                                auto type = obj.get("type").unwrapOr(val).asString().unwrapOrDefault();
                                if (type == "button") {
                                    if (ImGui::Button(currentLanguage->name(name).c_str())) {
                                        // TODO: move this to hacks.cpp
                                        if (name == "Restore Defaults") {
                                            Hacks::processJSON(true);
                                        } else if (name == "Import Theme") {

#if 0
                                            file::pickFile(
                                                file::PickMode::OpenFile,
                                                {
                                                    dirs::getGameDir(),
                                                    { filter }
                                                },
                                                [&](std::filesystem::path path) {
                                                    auto saveDir = Mod::get()->getSaveDir().string();
                                                    if (!std::filesystem::exists(saveDir + "/themes")) {
                                                        std::filesystem::create_directory(saveDir + "/themes");
                                                    }
                                                    std::filesystem::copy_file(path, saveDir + "/themes/" + path.filename().string());
                                                    //ImGui::OpenPopup("Success");
                                                    FLAlertLayer::create("Success!", "The <cy>theme</c> has successfully been imported! Restart your game to use it.", "OK")->show();
                                                }
                                            );
#endif
                                        } else if (name == "Reset Speedhack") {
                                            HackItem* speedHack = Hacks::getHack("Speedhack");
                                            speedHack->value.floatValue = 1.0F;
                                            Hacks::Settings::setSettingValue(&settings, *speedHack, 1.0F);
                                            Hacks::setPitch(1.0F);
                                            CCDirector::sharedDirector()->getScheduler()->setTimeScale(1.0F);
                                        } else if (name == "Show Graphic Options") {
                                            #ifdef GEODE_IS_ANDROID 
                                            VideoOptionsLayer::create()->show();
                                            #else 
                                            FLAlertLayer::create("Error", "This option can only be used on <cy>Android</c>!", "OK")->show();
                                            #endif
                                        } else if (name == "Uncomplete Level") {
                                            if (auto scene = CCScene::get()) {
                                                if (auto levelInfoLayer = scene->getChildByType<LevelInfoLayer>(0)) {
                                                    Hacks::resetLevel(levelInfoLayer, levelInfoLayer->m_level);
                                                } else {
                                                    FLAlertLayer::create("Error", "You are not <cy>currently in the level page</c>! Please enter in a level page in order to <cg>reset the stats</c>.", "OK")->show();
                                                }
                                            }
                                        } else { // how will this happen
                                            FLAlertLayer::create("unimplemented", "this button is not implemented", "okay")->show();
                                        }
                                        ImGui::OpenPopup(name.c_str());

                                    }
                                    // Popups
                                    if (ImGui::BeginPopupModal("Credits", nullptr)) {
                                        auto creditLine1 = currentLanguage->name(currentLanguage->name(
"Thank you to Electrify (ES), Jouca (FR), dank_meme01, savvacorgi, МегаСвятой (RU), Gazonk (BR), huhnmitferrari (DE), and Ignis (CS) for translations!"
                                        ));
                                        auto creditLine2 = currentLanguage->name(currentLanguage->name("And thank you for using the mod! I hope you enjoy using Prism Menu!"));
                                        ImGui::Text("%s", creditLine1.c_str());
                                        ImGui::Separator();
                                        ImGui::Text("%s", creditLine2.c_str());
                                        if (ImGui::Button(currentLanguage->name(currentLanguage->name("Close Popup").c_str()).c_str()))
                                            ImGui::CloseCurrentPopup();
                                        ImGui::EndPopup();
                                    }

                                } else if (type == "dropdown") {
                                    auto values = obj.get("values").unwrapOr(val).asArray().unwrapOr(std::vector<matjson::Value> {});
                                    if (hack->name == "Theme") {
                                        values = Themes::getCurrentThemes();
                                    }
                                    int selectedIndex = hack->value.intValue;
                                    if (ImGui::BeginCombo(currentLanguage->name(name).c_str(), values[selectedIndex].asString().unwrapOrDefault().c_str())) {
                                        for (size_t i = 0; i < values.size(); i++) {
                                            const bool isSelected = (selectedIndex == i);
                                            if (ImGui::Selectable(values[i].asString().unwrapOrDefault().c_str(), isSelected)) {
                                                hack->value.intValue = i;
                                                Hacks::Settings::setSettingValue(&settings, *hack, hack->value.intValue);
                                                if (name == "Menu-Style") {
                                                    prismButton->showImGuiMenu = !prismButton->showImGuiMenu;
                                                }
                                                if (name == "Language") {
                                                    Themes::LoadTheme(Themes::getCurrentTheme());
                                                }
                                            }
                                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                            if (isSelected)
                                                ImGui::SetItemDefaultFocus();
                                        }
                                        ImGui::EndCombo();
                                    }
                                }
                            }
                            if (obj.contains("win")) { // yeah idk what property okay
                                #ifndef GEODE_IS_WINDOWS
                                ImGui::EndDisabled();
                                #endif
                            }
                            if (ImGui::IsItemHovered() && Hacks::isHackEnabled("Show Tooltips")) {
                                if ((obj.contains("winOnly") && Hacks::isHackEnabled("Enable Patching")) || !obj.contains("winOnly")) {
                                    ImGui::BeginTooltip();
                                    ImGui::Text("%s", currentLanguage->desc(name, desc).c_str());
                                    ImGui::EndTooltip();
                                }
                            }
                        }
    }
}

struct CategoryWindow {
    const char* langKey;   // key in language files
    int index;             // same indices as the old sidebar
};

static std::vector<matjson::Value> getCategoryJson(int index) {
    switch (index) {
        case 0: return matjson::parse(Hacks::getGlobalHacks()).unwrapOrDefault().asArray().unwrapOrDefault();
        case 1: return matjson::parse(Hacks::getPlayerHacks()).unwrapOrDefault().asArray().unwrapOrDefault();
        case 2: return matjson::parse(Hacks::getBypassHacks()).unwrapOrDefault().asArray().unwrapOrDefault();
        case 3: return matjson::parse(Hacks::getCreatorHacks()).unwrapOrDefault().asArray().unwrapOrDefault();
        case 5: return matjson::parse(Hacks::getMiscHacks()).unwrapOrDefault().asArray().unwrapOrDefault();
        case 6: return matjson::parse(Hacks::getSettings()).unwrapOrDefault().asArray().unwrapOrDefault();
        default: return {};
    }
}

static void drawCategoryWindow(const CategoryWindow& cat, int slot, const std::unique_ptr<Lang>& lang, PrismButton* prismButton, const CCSize& frameSize) {
    auto currentTheme = Themes::getCurrentTheme();

    std::vector<matjson::Value> items = getCategoryJson(cat.index);
    bool searching = g_searchBuf[0] != '\0';
    if (searching) {
        // hide windows that have nothing matching the search
        bool any = false;
        for (auto& o : items) if (matchesSearch(o, lang)) { any = true; break; }
        if (!any) return;
    }

    float winW = (250.0f * frameSize.width) / 1366.0f;
    float maxH = frameSize.height - 40.0f;
    float gap = 12.0f;
    // windows are laid out in a row on the first launch, you can drag them wherever you want after that
    float x = 20.0f + slot * (winW + gap);
    float y = 70.0f;
    if (x + winW > frameSize.width) { // wrap to the next row if the screen is narrow
        int perRow = (std::max)(1, static_cast<int>((frameSize.width - 20.0f) / (winW + gap)));
        x = 20.0f + (slot % perRow) * (winW + gap);
        y = 70.0f + (slot / perRow) * (maxH * 0.45f);
    }
    ImGui::SetNextWindowPos({ x, y }, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({ winW, 0.0f }, { winW, maxH });

    // spacing values taken from Eclipse's theme files (windowPadding/windowRounding/frameRounding/spacing...)
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f, 3.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(8.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 25.0f);
    ImGui::PushStyleColor(ImGuiCol_TitleBg, Themes::RGBAToImVec4(currentTheme["Button"]));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, Themes::RGBAToImVec4(currentTheme["ButtonActive"]));
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, Themes::RGBAToImVec4(currentTheme["Button"]));

    // "###" keeps the window position/state stable when the language changes
    std::string title = stripIcon(lang->name(cat.langKey)) + "###prism_cat_" + std::to_string(cat.index);
    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin(title.c_str(), nullptr, flags)) {
        if (cat.index == 6) { // Settings: keep the version/credits header
            ImGui::Text("%s", lang->name("Prism Menu by Firee").c_str());
            #ifdef GITHUB_ACTIONS
            auto version = fmt::format("{} (Geode)", Mod::get()->getVersion());
            #else
            auto version = fmt::format("{}-Dev (Geode)", Mod::get()->getVersion());
            #endif
            #ifdef GEODE_IS_WINDOWS
            ImGui::Text("%s - Windows", version.c_str());
            #else
            #ifdef GEODE_IS_ANDROID
            ImGui::Text("%s - Android", version.c_str());
            #else
            ImGui::Text("%s - HOW by Spu7nix", version.c_str());
            #endif
            #endif
            ImGui::Separator();
        }
        for (auto& obj : items) {
            if (!matchesSearch(obj, lang)) continue;
            renderHackItem(obj, lang, prismButton);
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(8);
}

static void drawSearchWindow(const std::unique_ptr<Lang>& lang, PrismButton* prismButton, const CCSize& frameSize) {
    auto currentTheme = Themes::getCurrentTheme();
    float winW = (250.0f * frameSize.width) / 1366.0f;
    ImGui::SetNextWindowPos({ 20.0f, 20.0f }, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({ winW * 2.0f, 0.0f }, { winW * 2.0f, 200.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_TitleBg, Themes::RGBAToImVec4(currentTheme["Button"]));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, Themes::RGBAToImVec4(currentTheme["ButtonActive"]));
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, Themes::RGBAToImVec4(currentTheme["Button"]));
    if (ImGui::Begin("Prism Menu###prism_search", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::SetNextItemWidth(winW * 2.0f - 90.0f);
        ImGui::InputTextWithHint("##prism_search_input", "Search...", g_searchBuf, sizeof(g_searchBuf));
        ImGui::SameLine();
        if (ImGui::Button("X")) {
            prismButton->showImGuiMenu = false;
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (firstLoadI) return true;
        firstLoadI = true;
        ImGuiCocos::get().setup([] {
            Themes::LoadTheme(Themes::getCurrentTheme());
        }).draw([] {
            if (CCScene::get() == nullptr) return;
            auto prismButton = typeinfo_cast<PrismButton*>(OverlayManager::get()->getChildByID("prism-icon"));
            if (prismButton == nullptr) return;
            if (!prismButton->showImGuiMenu) return;

            // i cant use LoadTheme because 0160:err:virtual:allocate_virtual_memory out of memory for allocation, base (nil) size
            // well just in case
            if (Hacks::isHackEnabled("Live Theme Editing") || changedMenuScale) {
                Themes::LoadTheme(Themes::getCurrentTheme());
                changedMenuScale = false;
            } else {
                if (changedOpacity) {
                    Themes::UpdateOpacity(Themes::getCurrentTheme());
                    changedOpacity = false;
                }
            }

            auto* director = CCDirector::sharedDirector();
            const auto frameSize = director->getOpenGLView()->getFrameSize();
            auto currentLanguage = Lang::getLanguage();

            drawSearchWindow(currentLanguage, prismButton, frameSize);

            static const CategoryWindow categories[] = {
                { "§ Global", 0 },
                { "¬ Player", 1 },
                { "ª Bypass", 2 },
                { "« Creator", 3 },
                { "··· Misc", 5 },
                { "¶ Settings", 6 },
            };
            int slot = 0;
            for (auto& cat : categories) {
                drawCategoryWindow(cat, slot++, currentLanguage, prismButton, frameSize);
            }

        });
        return true;
    }
};
#endif
