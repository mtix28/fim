#include <iostream>
#include <filesystem>
#include <string>
#include "../include/picosha2.h"
#include "../include/json.hpp"
#include <fstream>

struct Files {
    std::string path;
    uintmax_t file_size;
    std::string hash;
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
    file << std::setw(4) << file_arry;
    file.close();

    return true;
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
    //weniger als 2 argumente
    if(argc < 2 || argc > 2) {
        std::cout << "[Usage] /fim <Pfad>\n";
        return 1;
    }

    //Eingabe in std::filesystem::path umwandeln
    const std::filesystem::path pfad = argv[1];

    //Überprüfen ob der Pfad existiert und ein Ordner ist
    if(std::filesystem::exists(pfad) && std::filesystem::is_directory(pfad)) {
        std::cout << "[+] Der Pfad " << pfad << " wurde ausgewaehlt und wird gescannt\n\n";
    } else {
        std::cout << "[-] Pfad ungueltig oder kein Ordner\n";
        return 1;
    }

    //load alte base file
    std::vector<Files> old_base = loadFile("base.json");
    std::cout << "[DEBUG] Alte Eintraege: " << old_base.size() << "\n\n";

    std::cout << "[*] Starte scan fuer: " << pfad << "\n";

    //pfad scannen
    std::vector<Files> finished_files = scanDir(pfad);

    std::cout << "[+] Scan abgeschlossen. Gefundene Dateien: " << finished_files.size() << "\n\n";
    std::cout << std::string(130, '-') << "\n";

    //schöne darstellung mit tabelle und std::iomanip
    std::cout << std::left 
              << std::setw(68) << "SHA-256 Hash" 
              << std::setw(20) << "Groesse (Bytes)" 
              << "Dateipfad\n";
    std::cout << std::string(130, '-') << "\n";

    for(const auto& file : finished_files) {
        std::cout << std::left 
                  << std::setw(68) << file.hash 
                  << std::setw(20) << file.file_size
                  << file.path<< "\n";
    }

    saveFile(finished_files, "base.json");

    return 0;
}
