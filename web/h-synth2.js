// ---- Yeni imza sesler, sert kit, ses adlarının İngilizce karşılıkları
(function () {
  const add = MB.add;
  add('mg-wide-chord', 'Wide Festival Chord', 'Stab', 'c', 'house trance', { u: [7, 22], o: [['w', 0, 0, .5], ['w', 0, -1, .35]], f: ['lowpass', 3200, 1, 2, 0, .25], a: [.004, .3, .6, .25], ch: .4, rv: .3, dl: .2, vol: .2 });
  add('mg-pluck-lead', 'Festival Pluck Lead', 'Pluck', 'cm', 'house trance', { u: [4, 16], o: [['x', 0, 0, .6], ['w', 0, 1, .25]], wt: [0, 3, .35, .9], f: ['lowpass', 2600, 1.5, 3.5, 0, .16], a: [.002, .3, .15, .2], dl: .4, rv: .35, vol: .2 });
  add('an-cine-pad', 'Cinematic Pad', 'Pad', 'c', 'mtechno techno', { u: [6, 18], o: [['x', 0, 0, .5], ['w', 0, -1, .3]], wt: [11, 7, .4, .6], f: ['lowpass', 1400, 1, 1.5, 1.8, 2.5], a: [.9, .8, .85, 2], l: [[.08, .3, 'cut', 'sine']], rv: .6, ch: .5, vol: .22 });
  add('an-pulse-bass', 'Afterhours Pulse Bass', 'Bas', 'b', 'mtechno techno', { o: [['q', 0, 0, .6], ['w', 0, 0, .5], ['s', 0, -1, .9]], f: ['lowpass', 360, 4, 3, 0, .12, 1], a: [.002, .09, .6, .05], fd: .25, dr: .25, vol: .2 });
  add('an-crystal-arp', 'Crystal Arp', 'Pluck', 'cm', 'mtechno trance', { o: [['x', 0, 0, .6], ['x', 12, 1, .25]], wt: [3, 8, .3, .85], f: ['lowpass', 4200, 2, 2, 0, .2], a: [.002, .25, 0, .25], dl: .55, rv: .5, ch: .3, vol: .2 });
  add('al-slap-bass', 'Slap House Bass', 'Bas', 'b', 'house', { fm: [2, 3.5, .12, .2, .8], o: [['s', 0, -1, .9]], f: ['lowpass', 1600, 1, 2, 0, .12], a: [.002, .16, .35, .06], dr: .3, vol: .2 });
  add('tt-brass-lead', 'Brass Hook Lead', 'Lead', 'm', 'trance house', { u: [3, 18], o: [['w', 0, 0, .6], ['q', 0, 0, .3]], pe: [-3, .06], f: ['lowpass', 2600, 2, 2.5, .02, .25], a: [.006, .15, .8, .12], vib: [5.5, 8, .3], dr: .3, dl: .25, rv: .25, vol: .2 });
  add('fa-felt-keys', 'Felt Keys', 'Keys', 'cm', 'house deep', { fm: [1, .8, .6, .15, .6], o: [['t', 0, 0, .5], ['s', 0, -1, .3]], f: ['lowpass', 2200, .7, .8, 0, .4], a: [.004, 1.2, .2, .6], ch: .25, rv: .4, dr: .1, vol: .22 });
  add('fi-rubber-bass', 'Rubber Tech Bass', 'Bas', 'b', 'house techno', { o: [['w', 0, 0, .7], ['s', 0, -1, 1]], f: ['lowpass', 380, 6, 4, 0, .09, 1], a: [.002, .1, .45, .05], fd: .45, dr: .35, vol: .2 });
  add('ip-psy-bass', 'Psy Roll Bass', 'Bas', 'b', 'trance techno', { o: [['w', 0, 0, .9], ['s', 0, -1, .6]], f: ['lowpass', 420, 3, 4, 0, .07, 1], a: [.001, .07, .25, .03], dr: .3, vol: .2 });
  add('ip-acid-lead', 'Psy Acid Lead', 'Lead', 'm', 'trance techno', { o: [['w', 0, 0, .9]], f: ['lowpass', 700, 11, 4.5, 0, .15, 1], a: [.002, .14, .4, .06], fd: .5, dr: .4, dl: .35, rv: .3, vol: .17 });
  add('sl-rumble-bass', 'Hard Rumble', 'Bas', 'b', 'techno', { o: [['s', 0, 0, 1], ['t', 0, -1, .4]], f: ['lowpass', 260, 1.5, 1, 0, .2], a: [.002, .18, .5, .12], fd: .8, dr: .7, rv: .25, vol: .2 });
  add('sl-hard-lead', 'Hard Screech', 'Lead', 'm', 'techno', { u: [3, 30], o: [['q', 0, 0, .5], ['w', 7, 0, .4]], f: ['bandpass', 1800, 3, 2, 0, .15], fd: .7, a: [.003, .12, .7, .1], dr: .6, dl: .25, rv: .3, vol: .16 });
  MB.KITS.hard = { id: 'hard', n: 'Hard Techno', g: ['techno'], k: { f0: 230, f1: 44, pd: .025, d: .45, click: .6, drv: 2.2 }, s: { f: 210, td: .09, nf: 2800, nd: .13, mix: .6 }, c: { nf: 1200, d: .18 }, h: { hp: 9000, d: .035, m: .6 }, o: { hp: 7500, d: .2, m: .6 }, r: { f: 2200 }, p: { f: 450, d: .08, type: 'square' }, rv: .08 };
  MB.KITS_LIST = Object.keys(MB.KITS).map(k => MB.KITS[k]);
  MB.SOUNDS.forEach(s => { MB.byId[s.id] = s; });
  HG.SIG = /^(wj|gt|ar|dd|mp|dv|cw|cc|mg|an|al|tt|fa|fi|ip|sl)-/;
  // İngilizce adlar (Türkçe dışındaki dillerde gösterilir; sektörde preset adları İngilizce kullanılır)
  HG.EN = { 'sub-sine': 'Sub Sine', 'deep-bass': 'Deep House Bass', 'roll-bass': 'Rolling Techno Bass', '808-long': '808 Long', 'pluck-bass': 'Pluck Bass', 'funk-bass': 'Funky Square Bass', 'neuro-lite': 'Neuro Lite', 'wood-bass': 'Wood Bass', 'upright': 'Lo-fi Upright', 'punch-bass': 'Mono Punch', 'trance-bass': 'Trance Offbeat', 'fm-bass': 'FM Bass',
    'warm-pad': 'Warm Analog Pad', 'glass-pad': 'Glass Pad', 'dark-drone': 'Dark Drone', 'choir-pad': 'Choir Pad', 'super-pad': 'Lush Super Pad', 'evolving-pad': 'Evolving Pad', 'tape-pad': 'Tape Pad', 'sunrise-pad': 'Sunrise Pad', 'sad-pad': 'Melancholy Pad', 'hollow-pad': 'Hollow Pad',
    'organ-stab': 'House Organ Stab', 'italo-stab': 'Italo Piano Stab', 'deep-stab': 'Deep Chord Stab', 'super-stab': 'Detuned Super Stab', 'dub-chord': 'Dub Chord', 'classic-pluck': 'Classic Pluck', 'muted-pluck': 'Muted String', 'harp': 'Harp', 'bell-pluck': 'Bell Pluck', 'glass-pluck': 'Glass Pluck (FM)',
    'super-lead': 'Super Lead', 'square-lead': 'Square Lead', 'sine-lead': 'Soft Sine Lead', 'whistle-lead': 'Whistle Lead', 'mt-lead': 'Melodic Techno Lead', 'glass-lead': 'Glass Lead', 'afro-flute': 'Afro Flute', 'e-piano': 'Electric Piano (FM)', 'organ': 'Drawbar Organ',
    'aah-female': 'Female Aah', 'ooh-choir': 'Ooh Choir', 'vocoder': 'Robot Vocoder', 'breath-hum': 'Breath Hum', 'male-ooh': 'Male Ooh', 'vox-chop': 'Vocal Chop', 'angel-aah': 'Angel Aah',
    'wj-dark-pad': 'WJ Dark Pad', 'wj-hypno-bass': 'WJ Hypnotic Bass', 'wj-glass-arp': 'WJ Glass Arp', 'wj-sunrise-lead': 'WJ Sunrise Lead', 'gt-piano-stab': 'Anthem Piano Stab', 'gt-pump-bass': 'Anthem Pump Bass', 'gt-anthem-lead': 'Anthem Lead', 'ar-uplift-lead': 'Uplift Lead', 'ar-rolling-bass': 'Trance Rolling Bass',
    'dd-funk-stab': 'Funk Stab', 'dd-bounce-bass': 'Bounce Bass', 'mp-hook-bass': 'Hook Bass', 'dv-screech-lead': 'Mainstage Screech', 'cw-acid-riff': 'Dark Acid Riff', 'cc-909-stab': '909 Stab',
    house: 'House Punch', deep: 'Deep Round', techno: 'Techno Hard', warehouse: 'Warehouse Boom', trance: 'Trance Drive', trap: '808 Trap', dnb: 'DnB Tight', lofi: 'Lo-fi Dusty', afro: 'Afro Organic', hard: 'Hard Techno' };
  // Eski "GT/AVB/DD..." kısaltmalı Türkçe adları sanatçı adı ima etmeyecek biçimde sadeleştir
  const TRN = { 'gt-piano-stab': 'Marş Piyano Stab', 'gt-pump-bass': 'Marş Pump Bas', 'gt-anthem-lead': 'Marş Lead', 'ar-uplift-lead': 'Uplift Lead', 'ar-rolling-bass': 'Trance Rolling Bas', 'dd-funk-stab': 'Funk Stab', 'dd-bounce-bass': 'Bounce Bas', 'mp-hook-bass': 'Hook Bas', 'dv-screech-lead': 'Ana Sahne Screech', 'cw-acid-riff': 'Karanlık Acid Riff', 'cc-909-stab': '909 Stab',
    'mg-wide-chord': 'Geniş Festival Akoru', 'mg-pluck-lead': 'Festival Pluck Lead', 'an-cine-pad': 'Sinematik Pad', 'an-pulse-bass': 'Gece Pulse Bas', 'an-crystal-arp': 'Kristal Arp', 'al-slap-bass': 'Slap House Bas', 'tt-brass-lead': 'Brass Hook Lead', 'fa-felt-keys': 'Keçe Piyano', 'fi-rubber-bass': 'Lastik Tech Bas', 'ip-psy-bass': 'Psy Rolling Bas', 'ip-acid-lead': 'Psy Acid Lead', 'sl-rumble-bass': 'Sert Rumble', 'sl-hard-lead': 'Sert Screech' };
  Object.keys(TRN).forEach(id => { MB.byId[id].name = TRN[id]; });
  HG.soundName = (id, lang) => { const s = MB.byId[id] || MB.KITS[id]; if (!s) return id; if (lang === 'tr') return s.name || s.n; return HG.EN[id] || s.name || s.n; };
})();
