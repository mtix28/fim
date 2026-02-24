#include <iostream>
#include <filesystem>

int main(int argc, char *argv[]) {
    //weniger als 2 argumente
    if(argc < 2 || argc > 2) {
        std::cout << "[Usage] /fim <Pfad>\n";
    }

    //Eingabe in std::filesystem::path umwandeln
    const std::filesystem::path pfad = argv[1];

    //Überprüfen ob der Pfad existiert und ein Ordner ist
    if(std::filesystem::exists(pfad) && std::filesystem::is_directory(pfad)) {
        std::cout << "[+] Der Pfad " << pfad << " wurde ausgewaehlt\n";
    } else {
        std::cout << "[-] Pfad ungueltig oder kein Ordner\n";
        return 1;
    }

    return 0;
}
