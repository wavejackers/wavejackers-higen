WAVEJACKERS HITGEN 2.7 - KURULUM / INSTALLATION
================================================

TURKCE
------
Bu paket: VST3 eklentisi, (macOS'ta) AU eklentisi ve tek basina calisan uygulama (Standalone).

Windows 10 / 11
  1. "Wavejackers HitGen.vst3" klasorunu su klasore kopyala:
       C:\Program Files\Common Files\VST3
  2. DAW'ini ac ve eklenti taramasi yap (FL Studio: Options > Manage plugins > Find plugins;
     Ableton Live: Preferences > Plug-ins > Rescan; Cubase: Studio > VST Plug-in Manager).
  3. Eklentiyi bir enstruman kanalina ekle (Wavejackers > Wavejackers HitGen).
  Tek basina kullanmak icin "Wavejackers HitGen.exe" dosyasini calistir.

macOS 10.13 ve ustu (Intel ve Apple Silicon)
  1. "Wavejackers HitGen.vst3" -> ~/Library/Audio/Plug-Ins/VST3
     "Wavejackers HitGen.component" -> ~/Library/Audio/Plug-Ins/Components (Logic Pro, GarageBand)
  2. Ilk acilista "dogrulanamayan gelistirici" uyarisi cikarsa: Sistem Ayarlari > Gizlilik ve Guvenlik >
     "Yine de ac". Gerekirse Terminal'de:
       xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Wavejackers HitGen.vst3"
       xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"Wavejackers HitGen.component"
  3. DAW'i yeniden baslat ve eklentileri tarat. Logic Pro icin AU dogrulamasi otomatik yapilir.
  Tek basina kullanmak icin "Wavejackers HitGen.app" uygulamasini ac.

Linux (x86-64)
  1. "Wavejackers HitGen.vst3" klasorunu ~/.vst3 icine kopyala.
  2. DAW'inda (Reaper, Bitwig, Ardour) eklentileri yeniden tara.

Ilk acilista eklenti kisa bir kilavuz gosterir. Kilavuzu yeniden acmak icin Yardim > Kilavuzu baslat ya da F1.
Kaldirmak icin kopyaladigin dosyalari sil. Kullanici verileri (presetler, kitler, DNA profilleri):
  Windows %APPDATA%\Wavejackers\HitGen, macOS ~/Library/Wavejackers/HitGen, Linux ~/.config/Wavejackers/HitGen

ENGLISH
-------
This package contains the VST3 plug-in, the AU plug-in (macOS) and a standalone app.

Windows 10 / 11
  1. Copy the "Wavejackers HitGen.vst3" folder to C:\Program Files\Common Files\VST3
  2. Open your DAW and rescan plug-ins (FL Studio: Options > Manage plugins > Find plugins;
     Ableton Live: Preferences > Plug-ins > Rescan; Cubase: Studio > VST Plug-in Manager).
  3. Add it on an instrument track (Wavejackers > Wavejackers HitGen).
  To use it without a DAW, run "Wavejackers HitGen.exe".

macOS 10.13 or later (Intel and Apple Silicon)
  1. "Wavejackers HitGen.vst3" -> ~/Library/Audio/Plug-Ins/VST3
     "Wavejackers HitGen.component" -> ~/Library/Audio/Plug-Ins/Components (Logic Pro, GarageBand)
  2. If macOS says the developer cannot be verified: System Settings > Privacy & Security > "Open Anyway".
     If needed, in Terminal:
       xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Wavejackers HitGen.vst3"
       xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"Wavejackers HitGen.component"
  3. Restart your DAW and rescan plug-ins.
  To use it without a DAW, open "Wavejackers HitGen.app".

Linux (x86-64)
  1. Copy the "Wavejackers HitGen.vst3" folder to ~/.vst3
  2. Rescan plug-ins in your DAW (Reaper, Bitwig, Ardour).

On first launch the plug-in shows a short guide. Open it again with Help > Start the guide, or F1.
To uninstall, delete the files you copied. User data (presets, kits, DNA profiles):
  Windows %APPDATA%\Wavejackers\HitGen, macOS ~/Library/Wavejackers/HitGen, Linux ~/.config/Wavejackers/HitGen
