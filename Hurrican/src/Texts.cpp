// Datei : Texts.cpp

// --------------------------------------------------------------------------------------
//
// Beinhaltet die Defines und Funktionen für die Texte in Hurrican
// Es ist möglich, verschiedene Language-Files zu laden
//
// (c) 2002-2004 Jörg M. Winterstein
//
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------------------

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <iostream>
#include <sstream>
#include <string>
namespace fs = std::filesystem;

#include "Gameplay.hpp"
#include "GetKeyName.hpp"
#include "Logdatei.hpp"
#include "Main.hpp"
#include "Player.hpp"
#include "Texts.hpp"
#include "Tileengine.hpp"

extern Logdatei Protokoll;

std::vector<std::string> LanguageFiles;
std::string ActualLanguage;  // Gewählte Language
char TextArray[TEXT::LASTTEXT][1024];
namespace {
std::array<std::vector<TutorialWord>, TEXT::TUTORIAL39 - TEXT::TUTORIAL1 + 1> tutorialWords;
}

namespace {
std::map<std::string, std::string> menuStrings;
std::vector<std::string> paragraphs, creditRows;
int wrapWidth = 0, wrapScale = 0;

// Latin-1 plus sixteen otherwise unused atlas cells for Polish letters.
std::string AtlasText(const std::string &utf8) {
    constexpr char32_t polish[] = U"ĄąĆćĘęŁłŃńŚśŹźŻż";
    std::string result;
    for (size_t i = 0; i < utf8.size();) {
        unsigned c = static_cast<unsigned char>(utf8[i++]);
        if (c < 128) { result += static_cast<char>(c); continue; }
        int rest = c < 224 ? 1 : c < 240 ? 2 : 3;
        c &= (1u << (6 - rest)) - 1;
        while (rest-- && i < utf8.size())
            c = (c << 6) | (static_cast<unsigned char>(utf8[i++]) & 63);
        const auto it = std::find(std::begin(polish), std::end(polish) - 1, c);
        result += it != std::end(polish) - 1 ? static_cast<char>(128 + (it - polish)) :
                  c <= 255 ? static_cast<char>(c) : '?';
    }
    return result;
}

void ReadMenuCatalog(const std::string &name) {
    const auto path = fs::path(g_storage_ext) / "lang" / "menus" / (fs::path(name).stem().string() + ".txt");
    std::ifstream file(path);
    bool credits = false;
    for (std::string line; std::getline(file, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!credits && line == "[credits]") {
            credits = true;
            paragraphs.clear();
        } else if (credits) paragraphs.push_back(AtlasText(line));
        else if (const auto cut = line.find('='); cut != std::string::npos)
            menuStrings[line.substr(0, cut)] = AtlasText(line.substr(cut + 1));
    }
}
}

const char *MenuText(const char *key) {
    const auto it = menuStrings.find(key);
    return it != menuStrings.end() ? it->second.c_str() : key;
}

const std::vector<std::string> &CreditLines(int width) {
    const int scale = pDefaultFont->GetScaleFactor();
    if (width != wrapWidth || scale != wrapScale) {
        creditRows.clear();
        for (const auto &para : paragraphs) {
            std::istringstream in(para);
            std::string line;
            for (std::string word; in >> word;) {
                const auto next = line.empty() ? word : line + " " + word;
                if (!line.empty() && pDefaultFont->StringLength(next.c_str(), 0) > width) {
                    creditRows.push_back(line);
                    line = word;
                } else line = next;
            }
            creditRows.push_back(line);
        }
        wrapWidth = width;
        wrapScale = scale;
    }
    return creditRows;
}

char Cheats[MAX_CHEATS][256] = {"$.345&&",            // 99 Extras = dnstuff
                                ")$&!",               // Alle Waffen = idfa
                                "\"%.+%.752-53",      // Shield = Benkenwurmus
                                "42!6%,,%2`).`4)-%",  // 999 Zeit = traveller in time
                                "$%2&.!-`:.%24",      // Godmode = derfnam znert
                                "42!),\",!:%2",       // Radmode = trailblazer
                                "$/.4`34/0`-%`./7",   // Autofire = dont stop me now
                                "350%23):%`-%",       // Riesenshot = supersize me
                                "42!.4/2`7!3`(%2%"};  // Flammenwerfer = trantor was here

// --------------------------------------------------------------------------------------
// Bestimmte Sprachdatei laden
// --------------------------------------------------------------------------------------
//
bool LoadLanguage(const std::string& filename) {
    std::ifstream in;
    std::string temp;

    if (CommandLineParams.RunOwnLevelList) {
        temp = g_storage_ext +  "/data/levels/" + CommandLineParams.OwnLevelList + "/custom.lng";
        if (fs::exists(temp) && fs::is_regular_file(temp))
            goto loadfile;
    }

    // DKS - Fixed language loading (it did not look in lang/ folder at all)
    // First, always try the lang/ folder
    temp = g_storage_ext + "/lang/" + filename;

    if (fs::exists(temp) && fs::is_regular_file(temp))
        goto loadfile;

    // If not found in the lang/ folder, try the root game folder
    temp = g_storage_ext + "/" + filename;
    if (!fs::exists(temp) && fs::is_regular_file(temp))
        return false;

loadfile:

    std::ifstream Datei(temp);  // Language File öffnen
    if (!Datei)                 // Fehler beim öffnen ?
    {
        Protokoll << "-> Error opening language-file\n";
        Protokoll << "'" << temp << "'" << std::endl;
        GameRunning = false;
        return false;
    } else {
        Protokoll << "Using language-file '" << temp << "'" << std::endl;
    }

    // Texte auslesen
    //
    for (int i = 0; i < TEXT::LASTTEXT; ++i) {
        std::string row;
        std::getline(Datei, row);
        if (!row.empty() && row.back() == '\r') row.pop_back();
        const auto n = row.copy(TextArray[i], sizeof(TextArray[i]) - 1);
        TextArray[i][n] = 0;
    }
    menuStrings.clear();
    paragraphs.clear();
    wrapWidth = 0;
    ReadMenuCatalog("english.lng");
    if (filename != "english.lng") ReadMenuCatalog(filename);
    InvalidateTutorialText();

    //
    // Stage Reihenfolge laden
    //
    //
    if (CommandLineParams.RunOwnLevelList) {
        std::string levelfile = g_storage_ext + "/data/levels/" + CommandLineParams.OwnLevelList + "/levellist.dat";

        in.open(levelfile);  // Reihenfolge Level laden
        if (!in) {
            // Fehler beim öffnen ? Dann standard Liste öffnen
            CommandLineParams.RunOwnLevelList = false;
            Protokoll << "-> Error opening level-order file" << levelfile << std::endl;
            GameRunning = false;
            return false;
        }
    } else {
        // DKS - levellist.dat now resides in its new subfolder, data/levels/levellist.dat, along with
        //      the rest of the level data.
        // Checken, ob sich das File im Standard Ordner befindet
        std::string Temp = g_storage_ext + "/data/levels/levellist.dat";
        if (fs::exists(Temp) && fs::is_regular_file(Temp))
            goto loadfilelevel;

        Protokoll << "\n-> Error loading " << Temp << "!" << std::endl;
        GameRunning = false;
        return false;

    loadfilelevel:
        in.open(Temp);  // Datei öffnen
    }

    // Levels auslesen
    //
    MAX_LEVELS = 0;

    while (!in.eof()) {
        char tmpchar[256];
        in.getline(tmpchar, 256);

        /* CHECKME: Removing CR from line ending (stegerg) */
        char *cr;
        if ((cr = strchr(tmpchar, '\r'))) {
            *cr = '\0';
        }

        if (strlen(tmpchar) > 0) {  // DKS - Added check for strlen > 0
            StageReihenfolge[MAX_LEVELS] = tmpchar;
            MAX_LEVELS++;
        }
    }
    in.close();

    return true;

}  // LoadLanguage

void FindLanguageFiles(const char *path) {
    if (!path) {
        Protokoll << "ERROR: NULL path passed to find_language_files() in " << __FILE__ << std::endl;
    }

    Protokoll << "Searching for language files in " << path << std::endl;

    for (auto &file : fs::directory_iterator(path)) {
        if (!fs::is_regular_file(file)) {
            Protokoll << "File " << file.path().filename() << " is not a regular file, skipping" << std::endl;
            continue;
        }

        if (file.path().extension() == ".lng") {
            Protokoll << "Found language file " << file.path().filename() << " in path " << path << std::endl;
            LanguageFiles.push_back(file.path().filename().string());
        }
    }
}

// DKS - Added function to split a longer line into two shorter lines, for when
//      running on a lower-resolution device w/ scaled font
void SplitLine(char *dst1, char *dst2, const char *source) {
    unsigned int const source_length = strlen(source);

    if (!source || source_length < 10)
        return;

    unsigned int split_point = source_length / 2;  // Begin in the middle

    // Find the first space past the middle
    while (split_point < source_length && source[split_point] != ' ') {
        split_point++;
    }

    // Copy source string to dst1, up to the split point
    unsigned int i;
    for (i = 0; i < split_point; i++) {
        dst1[i] = source[i];
    }
    dst1[i] = '\0';

    // Copy rest of source string starting one char past the split-point
    int dst2_ctr = 0;
    for (i = split_point + 1; i < source_length; i++, dst2_ctr++) {
        dst2[dst2_ctr] = source[i];
    }
    dst2[dst2_ctr] = '\0';
}

// DKS - Added function to split a longer line into one, dst1, sized up to
//      width (in pixels) based on font passed, and one, dst2, is the remainder,
//      minus any leading whitespace. Returns true if line needed to be split.
bool ExtractStringOfLength(char *dst1, char *dst2, char *source, int width, DirectGraphicsFont *font) {
    if (!source || !dst1 || !dst2 || !font || width <= 0)
        return false;

    if (font->StringLength(source) <= width)
        return false;

    int split_pos = 0;
    int const source_len = strlen(source);
    dst1[0] = '\0';
    dst2[0] = '\0';

    while (split_pos < source_len && font->StringLength(dst1) <= width) {
        dst1[split_pos] = source[split_pos];
        dst1[split_pos + 1] = '\0';
        split_pos++;
    }

    split_pos--;

    // Now, see if we've stopped at a space and if not, backtrack to the previous space.
    while (dst1[split_pos] != ' ' && split_pos >= 0) {
        split_pos--;
    }

    // Replace the space with a newline and NULL terminator
    dst1[split_pos] = '\n';
    dst1[split_pos + 1] = '\0';

    // Now, copy the remainder, minus the leading space and including the NULL terminator, into dst2
    strcpy(dst2, &source[split_pos + 1]);

    return true;
}

// DKS - Added function to replace any instance of a substring with another substring,
//      primarily for use on gaming/phone systems where "key" should be replaced with
//      "button"
void ReplaceAll(std::string &str, const std::string &from, const std::string &to) {
    if (str.empty() || from.empty())
        return;
    std::string new_str;
    new_str.reserve(str.length());
    size_t start_pos = 0, pos;
    while ((pos = str.find(from, start_pos)) != std::string::npos) {
        new_str += str.substr(start_pos, pos - start_pos);
        new_str += to;
        pos += from.length();
        start_pos = pos;
    }
    new_str += str.substr(start_pos);
    str.swap(new_str);
}

// --------------------------------------------------------------------------------------
// Tasten ErsetzungsStrings für die TutorialTexte initialisieren
// --------------------------------------------------------------------------------------

void InvalidateTutorialText() {
    for (auto &words : tutorialWords)
        words.clear();
}

const std::vector<TutorialWord> &TutorialWords(int lesson) {
    auto &words = tutorialWords.at(lesson);
    if (!words.empty())
        return words;
    constexpr const char *tokens[] = {"KEY_LEFT", "KEY_RIGHT", "KEY_CROUCH", "KEY_UP", "KEY_DOWN",
        "KEY_JUMP", "KEY_SHOOT", "KEY_BLITZ", "KEY_POWERLINE", "KEY_GRANATE", "KEY_SHOCKWAVE",
        "KEY_WAFFE_CYCLE", "KEY_WAFFE_SPREAD", "KEY_WAFFE_LASER", "KEY_WAFFE_BOUNCE", "KEY_WAFFE_PREV", "KEY_SUMMARY"};
    static_assert(std::size(tokens) == ACTION_SUMMARY + 1);
    std::istringstream source(TextArray[TEXT::TUTORIAL1 + lesson]);
    for (std::string word; source >> word;) {
        TutorialWord forms{word, word};
        for (int p = 0; p < 2; ++p) {
            const auto &player = Player[p];
            const auto *pad = player.GetJoystick();
            for (int action = 0; action <= ACTION_SUMMARY; ++action) {
                if (word.find(tokens[action]) == std::string::npos)
                    continue;
                InputBinding selected{};
                for (const auto binding : player.BindingsFor(pad)[action]) {
                    if (binding.kind == (pad ? BindingKind::Pad : BindingKind::Key)) {
                        selected = binding;
                        break;
                    }
                    if (binding.kind == BindingKind::Key)
                        selected = binding;
                }
                ReplaceAll(forms[p], tokens[action], selected.Label(pad));
            }
            if (pad && (word == "key" || word == "keys" || word == "key." || word == "keys."))
                forms[p].replace(0, 3, "button");
        }
        words.push_back(std::move(forms));
    }
    return words;
}
