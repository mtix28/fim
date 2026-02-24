#include <iostream>
#include <filesystem>
#include <string>
#include "../include/picosha2.h";

struct Files {
    std::string path;
    uintmax_t file_size;
    std::string hash;
};

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
    }

    //Eingabe in std::filesystem::path umwandeln
    const std::filesystem::path pfad = argv[1];

    //Überprüfen ob der Pfad existiert und ein Ordner ist
    if(std::filesystem::exists(pfad) && std::filesystem::is_directory(pfad)) {
        std::cout << "[+] Der Pfad " << pfad << " wurde ausgewaehlt und wird ueberwacht\n";
    } else {
        std::cout << "[-] Pfad ungueltig oder kein Ordner\n";
        return 1;
    }

    std::vector<Files> finished_files = scanDir(pfad);
    for(const auto &file : finished_files) {
        std::cout << file.path << ": " << file.file_size << ": " << file.hash << "\n";
    }

    return 0;
}
