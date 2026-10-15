#!/bin/bash
# Installa SATURA nelle cartelle dei plugin (/Library/Audio/Plug-Ins) e rimuove la quarantena
cd "$(dirname "$0")"
set -e
VST3="/Library/Audio/Plug-Ins/VST3"
AU="/Library/Audio/Plug-Ins/Components"
echo "Serve la password di amministratore per scrivere in /Library/Audio/Plug-Ins."
sudo mkdir -p "$VST3" "$AU"
sudo rm -rf "$VST3/SATURA.vst3" "$AU/SATURA.component"
sudo cp -R "SATURA.vst3" "$VST3/"
sudo cp -R "SATURA.component" "$AU/"
rm -rf "/Applications/SATURA.app" 2>/dev/null || true
if ! cp -R "SATURA.app" "/Applications/" 2>/dev/null; then
  mkdir -p "$HOME/Applications"; rm -rf "$HOME/Applications/SATURA.app"; cp -R "SATURA.app" "$HOME/Applications/"
fi
sudo xattr -dr com.apple.quarantine "$VST3/SATURA.vst3" "$AU/SATURA.component" 2>/dev/null || true
xattr -dr com.apple.quarantine "/Applications/SATURA.app" "$HOME/Applications/SATURA.app" 2>/dev/null || true
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo ""
echo "SATURA installato:"
echo "  VST3: $VST3"
echo "  AU:   $AU"
echo "Riavvia la DAW per vederlo."
