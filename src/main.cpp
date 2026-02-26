#include <iostream>
#include <filesystem>
#include <string>
#include "../include/picosha2.h"
#include "../include/json.hpp"
#include <fstream>
#include <ftxui/component/component.hpp>           // Für interaktive Elemente (Buttons, Inputs)
#include <ftxui/component/screen_interactive.hpp>  // Für die Event-Loop
#include <ftxui/dom/elements.hpp>                  // Für das reine Zeichnen (Rahmen, Farben, Text)>
#include <thread>

struct Files {
    std::string path;
    uintmax_t file_size;
    std::string hash;
};

struct ScanReport {
    std::vector<Files> new_files;
    std::vector<Files> modified_files;
    std::vector<Files> deleted_files;
};

std::vector<Files> loadFile(const std::string &inPath) {
    std::vector<Files> loadedFile;

    std::ifstream file(inPath);
    if(!file.is_open()) {
        std::cerr << "[-] Base File konnte nicht gefunden werden\n";
        return loadedFile;
    }

    try {

        nlohmann::json json_data = nlohmann::json::parse(file);

        for(const auto &entry : json_data) {
            std::string path = entry["path"];
            uintmax_t size = entry["file_size"];
            std::string hash = entry["hash"];
            Files temp = {path, size, hash};
            loadedFile.push_back(temp);
        }
    } catch(const nlohmann::json::exception &e) {
        std::cerr << "[-] Fehler beim lesen der base file: " << e.what() << "\n";
    }

    file.close();
    return loadedFile;
}

//gescannte datei in json file abspeichern
bool saveFile(const std::vector<Files> &scanned_files, const std::string &outPath) {
    nlohmann::json file_arry = nlohmann::json::array();

    for(const auto &entry : scanned_files) {
        nlohmann::json temp_array = nlohmann::json::object();
        temp_array["path"] = entry.path;
        temp_array["file_size"] = entry.file_size;
        temp_array["hash"] = entry.hash;
        file_arry.push_back(temp_array);
    }

    std::ofstream file(outPath);
    if(!file.is_open()) {
        std::cerr << "[-] Datei: " << outPath << " konnte zum schreiben nicht genutzt werden";
        return false;
    }

    try {
        file << file_arry.dump(4, ' ', false, nlohmann::json::error_handler_t::replace);
    } catch(const std::exception &e) {
        std::cerr << "[-] Exception in saveFile abgefangen: " << e.what() << "\n";
    }

    file.close();

    return true;
}

ScanReport compareScans(const std::vector<Files>& old_base, const std::vector<Files>& new_scan) {
    ScanReport report;
    
    if (old_base.empty()) {
        report.new_files = new_scan;
        return report;
    }

    std::unordered_map<std::string, Files> old_map;
    for (const auto& file : old_base) {
        old_map[file.path] = file;
    }

    for (const auto& new_file : new_scan) {
        auto it = old_map.find(new_file.path);
        
        if (it == old_map.end()) {
            report.new_files.push_back(new_file);
        } else {
            if (it->second.hash != new_file.hash) {
                report.modified_files.push_back(new_file);
            }
            old_map.erase(it);
        }
    }
    for (const auto& pair : old_map) {
        report.deleted_files.push_back(pair.second);
    }

    return report;
}

//hash mit pico library generieren
std::string calcHash(const std::string &filepath) {
    //Datei öffnen
    std::ifstream file(filepath, std::ios::binary);
    if(!file.is_open()) {
        return "";
    }

    picosha2::hash256_one_by_one hasher;
    const int bufferSize = 4096;
    std::vector<char> buffer(bufferSize);

    while(file.read(buffer.data(), buffer.size())) {
        hasher.process(buffer.begin(), buffer.end());
    }

    if(file.gcount() > 0) {
        hasher.process(buffer.begin(), buffer.begin() + file.gcount());
    }

    hasher.finish();

    std::string hash_string;
    picosha2::get_hash_hex_string(hasher, hash_string);

    return hash_string;
}

// durchlaufen der datein des pfades und prüfen ob rechte vorhanden sind, zum Files struct hinzufügen
std::vector<Files> scanDir(const std::filesystem::path &target_pfad) {
    std::vector<Files> scanned_files;
    auto options = std::filesystem::directory_options::skip_permission_denied;

    try {
        for(const auto &dir_entry : std::filesystem::recursive_directory_iterator(target_pfad, options)) {
            try {
                if(std::filesystem::is_regular_file(dir_entry)) {
                    Files file = {dir_entry.path().string(), std::filesystem::file_size(dir_entry), calcHash(dir_entry.path().string())};
                    scanned_files.push_back(file);
                }
            } catch(const std::filesystem::filesystem_error &e) {
                std::cerr << "[-] Zugriff verweigert: " << e.what() << "\n";
            }
        }
    } catch(const std::filesystem::filesystem_error &e) {
        std::cerr << "[-] Fehler beim scannen des Ordners: " << e.what() << "\n";
    }
    return scanned_files;
}

int main(int argc, char *argv[]) {
    // Namensraum für bessere Lesbarkeit
    using namespace ftxui;

    // 1. Die Event-Loop definieren
    auto screen = ScreenInteractive::TerminalOutput();

    // 2. Der "State" (Zustand) unserer Anwendung
    std::string target_path = (argc == 2) ? argv[1] : "C:\\"; 
    std::string status_text = "Bereit fuer den Scan.";
    bool is_scanning = false;
    
    // Speichert den kompletten Report, damit die UI die Listen zeichnen kann
    ScanReport current_report;

    // 3. Interaktive Komponenten (Controller)
    Component input_path = Input(&target_path, "Ordnerpfad hier eingeben...");
    
    Component btn_scan = Button("Scan starten", [&] {
        // Verhindern, dass der Scan mehrfach parallel gestartet wird
        if (is_scanning) return; 
        is_scanning = true;

        // Multithreading: Wir übergeben nur explizite Referenzen, um Memory-Leaks zu vermeiden
        std::thread([&screen, &status_text, &current_report, &is_scanning, target_copy = target_path] {
            
            screen.Post([&status_text] { status_text = "Lade alte Baseline..."; });
            auto old_base = loadFile("base.json");

            screen.Post([&status_text, target_copy] { status_text = "Scanne Ordner: " + target_copy + " ..."; });
            auto finished_files = scanDir(target_copy);

            screen.Post([&status_text] { status_text = "Vergleiche Hashes..."; });
            auto report = compareScans(old_base, finished_files);

            screen.Post([&status_text] { status_text = "Speichere neue Baseline..."; });
            saveFile(finished_files, "base.json");

            // Die Ergebnisse sauber per Move-Semantik in den Main-Thread schieben
            screen.Post([&current_report, &status_text, &is_scanning, report_copy = std::move(report)] {
                current_report = report_copy;
                status_text = "Scan erfolgreich beendet!";
                is_scanning = false;
            });

        }).detach(); 
    });
    
    Component btn_quit = Button("Beenden", [&] { screen.Exit(); });

    // Der Container verwaltet die Eingaben (Tab-Navigation)
    auto layout = Container::Vertical({
        input_path,
        Container::Horizontal({btn_scan, btn_quit})
    });

    // 4. Hilfsfunktion: Wandelt Vektoren in eine UI-Liste um (mit Überlastungsschutz!)
    auto create_file_list = [](const std::vector<Files>& files, Color color, const std::string& title) {
        Elements elements;
        elements.push_back(text(title) | bold | ftxui::color(color));
        elements.push_back(separator());
        
        if (files.empty()) {
            elements.push_back(text(" Keine Dateien gefunden.") | dim);
        } else {
            int limit = 50; // Max. 50 Elemente rendern, um Speicherüberlauf zu verhindern
            int count = 0;
            
            for (const auto& f : files) {
                elements.push_back(text(" - " + f.path) | ftxui::color(color));
                count++;
                
                // Sobald das Limit erreicht ist, brechen wir die GUI-Generierung ab
                if (count >= limit) {
                    elements.push_back(text(" ... und " + std::to_string(files.size() - limit) + " weitere Dateien.") | dim | italic | ftxui::color(color));
                    break;
                }
            }
        }
        return vbox(std::move(elements));
    };

    // 5. Der Renderer (Die View / Das visuelle Layout)
    auto renderer = Renderer(layout, [&] {
        
        // Die Detail-Listen generieren
        auto detail_lists = vbox({
            create_file_list(current_report.new_files, Color::Green, "NEUE DATEIEN"),
            text(""), // Optischer Abstand
            create_file_list(current_report.modified_files, Color::Yellow, "MODIFIZIERTE DATEIEN"),
            text(""), 
            create_file_list(current_report.deleted_files, Color::Red, "GELOESCHTE DATEIEN"),
        });

        // Das finale Fenster zusammenbauen
        return vbox({
            // Header
            text(" [ FILE INTEGRITY MONITOR ] ") | bold | center | color(Color::Cyan),
            separator(),
            
            // Eingabebereich
            hbox({
                text(" Zielpfad: ") | bold,
                input_path->Render() | flex,
            }) | border,
            
            // Dashboard für Ergebnisse (Zusammenfassung)
            vbox({
                text(" ZUSAMMENFASSUNG ") | bold | center,
                separator(),
                text(" Neue Dateien:         " + std::to_string(current_report.new_files.size())) | color(Color::Green),
                text(" Modifizierte Dateien: " + std::to_string(current_report.modified_files.size())) | color(Color::Yellow),
                text(" Geloeschte Dateien:   " + std::to_string(current_report.deleted_files.size())) | color(Color::Red),
            }) | border,

            // Scrollbarer Detail-Bereich
            vbox({
                text(" DETAILANSICHT ") | bold | center,
                separator(),
                detail_lists | vscroll_indicator | frame | flex, 
            }) | border | flex, 
            
            // Footer mit Statusleiste und Buttons
            hbox({
                text(" Status: " + status_text) | flex, 
                btn_scan->Render() | color(Color::Green),
                text(" "), 
                btn_quit->Render() | color(Color::Red),
            }),
        }) | border; 
    });

    // 6. Anwendung starten
    screen.Loop(renderer);

    return 0;
}