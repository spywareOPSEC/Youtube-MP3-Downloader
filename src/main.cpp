// Simple YouTube downloader frontend in C++
// This program shells out to `yt-dlp` and `ffmpeg` to download and
// convert YouTube videos to MP3 audio files.
// Make by spyware

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <cctype>
#include <optional>

namespace fs = std::filesystem;

bool commandExists(const std::string &name) {
#ifdef _WIN32
    std::string cmd = name + " --version >nul 2>&1";
#else
    std::string cmd = name + " --version >/dev/null 2>&1";
#endif
    int r = std::system(cmd.c_str());
    return r == 0;
}

static inline std::string trim(std::string s) {
    const char *ws = " \t\n\r";
    s.erase(0, s.find_first_not_of(ws));
    s.erase(s.find_last_not_of(ws) + 1);
    return s;
}

std::optional<fs::path> locate_ffmpeg(const fs::path &exeDir) {
    if (commandExists("ffmpeg")) return fs::path("");

    std::vector<fs::path> candidates = {
        exeDir / "ffmpeg.exe",
        exeDir / "ffmpeg" / "bin" / "ffmpeg.exe",
        exeDir / "ffmpeg" / "ffmpeg.exe",
        exeDir / "bin" / "ffmpeg.exe",
    };
    for (auto &p : candidates) if (fs::exists(p)) return p.parent_path();

    return std::nullopt;
}

std::optional<fs::path> download_ffmpeg_windows(const fs::path &exeDir) {
    fs::path zip = exeDir / "ffmpeg.zip";
    fs::path outdir = exeDir / "ffmpeg_dl";
    std::string url = "https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip";

    std::string ps = "powershell -NoProfile -Command \"Invoke-WebRequest -Uri '" + url + "' -OutFile '" + zip.generic_string() + "'; Expand-Archive -Path '" + zip.generic_string() + "' -DestinationPath '" + outdir.generic_string() + "' -Force; Remove-Item '" + zip.generic_string() + "'\"";

    int r = std::system(ps.c_str());
    if (r != 0) return std::nullopt;

    for (auto &p : fs::recursive_directory_iterator(outdir)) {
        if (!p.is_regular_file()) continue;
        if (p.path().filename().string() == "ffmpeg.exe") return p.path().parent_path();
    }
    return std::nullopt;
}

int main(int argc, char** argv) {
    std::cout << "YouTube -> MP3 downloader (C++)\n";
    std::cout << "Entrez les URLs YouTube (une par ligne). Tapez une ligne vide pour terminer.\n";

    std::vector<std::string> links;
    while (true) {
        std::string line;
        std::getline(std::cin, line);
        line = trim(line);
        if (line.empty()) {
            std::cout << "Vous avez entre " << links.size() << " lien(s). Confirmer la fin? (y/n) ";
            std::string ans;
            std::getline(std::cin, ans);
            if (!ans.empty() && (ans[0] == 'y' || ans[0] == 'Y')) break;
            std::cout << "Continuez à entrer les liens (ou ligne vide + y pour finir).\n";
            continue;
        }
        links.push_back(line);
    }

    if (links.empty()) {
        std::cout << "Aucun lien fourni. Fin.\n";
        return 0;
    }

    bool has_yt = commandExists("yt-dlp");

    if (!has_yt) {
        std::cerr << "Erreur: `yt-dlp` introuvable dans le PATH. Installez yt-dlp et assurez-vous qu'il est accessible.\n";
        std::cerr << "Sur Windows: placez yt-dlp.exe dans le dossier et ajoutez au PATH, ou mettez le chemin complet dans le README.\n";
        return 2;
    }

    fs::path exeDir = fs::current_path();
    if (argc > 0) {
        try { exeDir = fs::absolute(fs::path(argv[0])).parent_path(); } catch (...) {}
    }

    auto ff = locate_ffmpeg(exeDir);
    bool use_ffmpeg = false;
    fs::path ff_dir;
    if (ff.has_value()) {
        if (ff->empty()) {
            use_ffmpeg = true;
        } else {
            use_ffmpeg = true;
            ff_dir = *ff;
        }
    }

    if (!use_ffmpeg) {
        std::cout << "ffmpeg introuvable. Voulez-vous que je telecharge une build locale (Windows) dans le dossier de l'application pour convertir en MP3 ? (y/n) ";
        std::string a; std::getline(std::cin, a);
        if (!a.empty() && (a[0]=='y' || a[0]=='Y')) {
            std::cout << "Telechargement de ffmpeg... cela peut prendre du temps.\n";
            auto d = download_ffmpeg_windows(exeDir);
            if (d.has_value()) { use_ffmpeg = true; ff_dir = *d; std::cout << "ffmpeg installe localement dans: " << ff_dir.generic_string() << "\n"; }
            else std::cout << "echec du telechargement automatique de ffmpeg.\n";
        }
    }

    std::cout << "Chemin du dossier de destination (ex: C:\\\\Users\\\\SpyWare\\\\Music), ou appuyez sur Entree pour utiliser le dossier courant: ";
    std::string out;
    std::getline(std::cin, out);
    out = trim(out);
    fs::path outdir;
    if (out.empty()) outdir = fs::current_path();
    else outdir = fs::path(out);

    std::error_code ec;
    if (!fs::exists(outdir)) {
        std::cout << "Le dossier n'existe pas. Voulez-vous le creer? (y/n) ";
        std::string a; std::getline(std::cin, a);
        if (!a.empty() && (a[0]=='y' || a[0]=='Y')) {
            if (!fs::create_directories(outdir, ec)) {
                std::cerr << "Impossible de creer le dossier: " << ec.message() << "\n";
                return 3;
            }
        } else {
            std::cerr << "Sortie: dossier non cree.\n";
            return 4;
        }
    }

    std::cout << "Telechargement dans : " << outdir.generic_string() << "\n";

    int idx = 0;
    for (const auto &url : links) {
        ++idx;
        std::cout << "[" << idx << "/" << links.size() << "] " << url << " -> demarrage...\n";
        std::string outpattern;
        std::string cmd;
        std::string logname = (outdir / ("yt-dlp-" + std::to_string(idx) + ".log")).generic_string();

        std::string quoted_url = '"' + url + '"';

        if (use_ffmpeg) {
            outpattern = (outdir / "%(title)s.%(ext)s").generic_string();
            std::string ff_arg = "";
            if (!ff_dir.empty()) ff_arg = " --ffmpeg-location \"" + ff_dir.generic_string() + "\"";
            cmd = "yt-dlp --no-playlist --extract-audio --audio-format mp3 --no-warnings -q" + ff_arg + " -o \"" + outpattern + "\" " + quoted_url + " > \"" + logname + "\" 2>&1";
        } else {
            outpattern = (outdir / "%(title)s.%(ext)s").generic_string();
            cmd = "yt-dlp --no-playlist -f bestaudio -o \"" + outpattern + "\" --no-warnings -q " + quoted_url + " > \"" + logname + "\" 2>&1";
            std::cout << "ffmpeg non disponible: je telecharge l'audio brut (format natif, pas forcement MP3).\n";
        }

        int r = std::system(cmd.c_str());

        std::ifstream logf(logname);
        bool printedError = false;
        if (logf) {
            std::string line;
            std::vector<std::string> all;
            while (std::getline(logf, line)) {
                all.push_back(line);
                std::string low = line;
                std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c){ return std::tolower(c); });
                if (low.find("error") != std::string::npos) {
                    std::cerr << line << "\n";
                    printedError = true;
                }
            }
            if (r != 0 && !printedError) {
                std::cerr << "Erreur: la commande a retourne le code " << r << ". Sortie recente:\n";
                int start = std::max(0, (int)all.size() - 20);
                for (int i = start; i < (int)all.size(); ++i) std::cerr << all[i] << "\n";
            }
        } else {
            if (r != 0) std::cerr << "Erreur: la commande a retourne le code " << r << ". (Impossible d'ouvrir le fichier de log)\n";
        }

        if (r == 0 && !printedError) std::cout << "Termine pour ce lien.\n";
    }

    std::cout << "Traitement termine. Verifiez le dossier pour les fichiers MP3.\n";
    return 0;
}

// Make by spyware