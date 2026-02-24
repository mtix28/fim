#include <iostream>
#include <filesystem>

// durchlaufen der datein des pfades und prüfen ob rechte vorhanden sind
void scanDir(const std::filesystem::path &target_pfad) {
    auto options = std::filesystem::directory_options::skip_permission_denied;

    try {
        for(const auto &dir_entry : std::filesystem::recursive_directory_iterator(target_pfad, options)) {
            try {
                if(std::filesystem::is_regular_file(dir_entry)) {
                    std::cout << dir_entry.path().string() << "\n";
                }
            } catch(const std::filesystem::filesystem_error &e) {
                std::cerr << "[-] Zugriff verweigert: " << e.what() << "\n";
            }
        }
    } catch(const std::filesystem::filesystem_error &e) {
        std::cerr << "[-] Fehler beim scannen des Ordners: " << e.what() << "\n";
    }
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

    scanDir(pfad);

    return 0;
}
