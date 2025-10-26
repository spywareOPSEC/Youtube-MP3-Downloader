# 🎧 YouTube MP3 Downloader

> Un petit utilitaire **C++** rapide et efficace pour télécharger l’audio d’une vidéo YouTube au format MP3.  
> Conçu avec soin par **SpyWare** 🕶️

---

## 🚀 Fonctionnalités

- 🔹 Téléchargement simple via lien YouTube  
- 🔹 Conversion automatique en **MP3**  
- 🔹 Interface console minimaliste et rapide  
- 🔹 Compatible Windows / (Soon for Linux)  
- 🔹 Code source clair et commenté

---

## 🧠 Prérequis

Avant de compiler, assure-toi d’avoir installé :

- **g++** (support du standard **C++17** ou plus récent)  
- **ffmpeg** (nécessaire pour la conversion audio) / (Installation automatique dans l'application, si besoin)
- Une connexion Internet

---

## ⚙️ Compilation

Clone le projet et compile avec la commande suivante :

```bash
g++ -std=c++17 -O2 -o youtube_downloader.exe src\main.cpp
