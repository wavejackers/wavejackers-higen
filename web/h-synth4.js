// ---- Wavejackers Artist Voices (2.3.1): her sanatçı için ayrı akor, bas ve lead. Özgün sentez; hiçbir ticari preset ya da örnek kullanılmaz.
(function () {
  const add = MB.add, S = HG.STYLE, EN = {};
  const v = (id, tr, en, cat, role, g, p) => { add(id, tr, cat, role, g, p); EN[id] = en; };
  // --- Akorlar ve padler
  v('gu-pop-stab', 'Pop Anthem Stab', 'Pop Anthem Stab', 'Stab', 'c', 'house main', { u: [7, 22], o: [['w', 0, 0, .55], ['q', 0, 0, .25]], f: ['lowpass', 5200, 1, 1.4, 0, .22], a: [.004, .3, .35, .22], ch: .4, rv: .18, dl: .12, vol: .2 });
  v('ti-fest-chord', 'Festival Chord', 'Festival Chord', 'Stab', 'c', 'main trance', { u: [7, 28], o: [['w', 0, 0, .6], ['w', 0, 1, .2]], f: ['lowpass', 4200, 1.2, 2, 0, .3], a: [.006, .4, .6, .35], ch: .5, rv: .3, dl: .18, vol: .2 });
  v('ar-uplift-pad', 'Uplifting Süper Pad', 'Uplifting Super Pad', 'Pad', 'c', 'trance', { u: [7, 34], o: [['w', 0, 0, .55], ['w', 0, -1, .35]], f: ['lowpass', 3000, 1, 2.4, 0, .9], a: [.25, .6, .8, 1.1], ch: .55, rv: .5, vol: .19 });
  v('dd-funk-stab', 'Funk House Stab', 'Funk House Stab', 'Stab', 'c', 'house', { o: [['q', 0, 0, .5], ['w', 0, 0, .35]], f: ['lowpass', 2200, 2.5, 3.2, 0, .1], a: [.002, .14, 0, .08], ch: .15, dl: .22, rv: .12, vol: .22 });
  v('mp-minimal-stab', 'Minimal Kısa Stab', 'Minimal Short Stab', 'Stab', 'c', 'house techno', { o: [['q', 0, 0, .6], ['t', 0, -1, .3]], f: ['lowpass', 1500, 3, 2.6, 0, .09], a: [.001, .1, 0, .05], dl: .3, rv: .1, vol: .22 });
  v('dv-rave-chord', 'Big Room Rave Chord', 'Big Room Rave Chord', 'Stab', 'c', 'main hard', { u: [5, 30], o: [['w', 0, 0, .5], ['q', 0, 0, .4]], f: ['lowpass', 3800, 2, 2.8, 0, .16], a: [.002, .22, .1, .12], fd: .35, ch: .3, rv: .15, vol: .19 });
  v('um-electro-chord', 'Elektro Chord', 'Electro Chord', 'Stab', 'c', 'main techno', { u: [4, 16], o: [['q', 0, 0, .5], ['w', 0, 0, .4]], f: ['lowpass', 2600, 3, 3, 0, .12], a: [.002, .18, 0, .1], fd: .3, dl: .2, rv: .14, vol: .2 });
  v('cw-dark-chord', 'Karanlık Chord', 'Dark Chord', 'Stab', 'c', 'techno', { u: [3, 10], o: [['w', 0, 0, .5], ['w', 0, -1, .5]], f: ['lowpass', 900, 3.5, 2.6, 0, .35], a: [.004, .4, .2, .3], dr: .2, rv: .35, dl: .28, vol: .21 });
  v('al-brazil-chord', 'Brazilian Pluck Chord', 'Brazilian Pluck Chord', 'Keys', 'c', 'house afro', { fm: [1, 1.5, .35, .12], f: ['lowpass', 5600, .6, 0, 0, .3], a: [.002, .5, .15, .3], ch: .3, rv: .22, dl: .15, vol: .21 });
  v('tt-psy-stab', 'Psy Offbeat Stab', 'Psy Offbeat Stab', 'Stab', 'c', 'trance psy', { u: [3, 14], o: [['w', 0, 0, .6], ['q', 0, 0, .3]], f: ['lowpass', 1800, 5, 3.4, 0, .1], a: [.001, .12, 0, .06], dl: .35, rv: .14, vol: .21 });
  v('fa-uk-keys', 'UK Duygusal Piyano', 'UK Emotional Keys', 'Keys', 'c', 'house deep', { fm: [1, .8, .5, .1], o: [['t', 0, 0, .4]], f: ['lowpass', 3800, .5, 0, 0, .3], a: [.003, .9, .2, .5], ch: .35, rv: .45, vol: .2 });
  v('fi-house-organ', 'Tech House Org', 'Tech House Organ', 'Keys', 'c', 'house', { o: [['s', 0, 0, .8], ['s', 0, 1, .5], ['s', 0, 1.58, .3], ['q', 0, 0, .15]], f: ['lowpass', 4200, .8, .6, 0, .2], a: [.004, .12, .4, .1], dl: .2, rv: .1, vol: .2 });
  v('ip-psy-chord', 'Psy Karanlık Chord', 'Dark Psy Chord', 'Stab', 'c', 'techno psy', { u: [4, 18], o: [['w', 0, 0, .55], ['w', 0, 1, .3]], f: ['lowpass', 1300, 4, 3, 0, .2], a: [.003, .3, .1, .2], lfo: 0, dl: .4, rv: .3, vol: .2 });
  v('sl-hard-stab', 'Hard Techno Stab', 'Hard Techno Stab', 'Stab', 'c', 'hard', { u: [3, 12], o: [['q', 0, 0, .6], ['w', 0, 0, .4]], f: ['lowpass', 2000, 4, 3.6, 0, .08], a: [.001, .1, 0, .05], fd: .6, dr: .35, rv: .08, vol: .2 });
  v('mq-prog-pad', 'Progressive Pad', 'Progressive Pad', 'Pad', 'c', 'house deep', { u: [5, 20], o: [['w', 0, 0, .45], ['t', 0, -1, .4]], f: ['lowpass', 2200, 1, 1.8, 0, 1.4], a: [.4, .8, .8, 1.4], l: [[.1, .25, 'cut', 'sine']], ch: .5, rv: .55, vol: .19 });
  // --- Basslar
  v('wj-roll-bass', 'Wavejackers Roll', 'Wavejackers Roll', 'Bas', 'b', 'techno main', { o: [['w', 0, 0, .65], ['s', 0, -1, .9]], sub: .3, f: ['lowpass', 330, 2.5, 3, 0, .12, 1], a: [.002, .14, .6, .07], dr: .2, vol: .2 });
  v('gu-pump-bass', 'Pump Sub', 'Pump Sub', 'Bas', 'b', 'house main', { o: [['s', 0, -1, 1], ['q', 0, 0, .2]], sub: .4, f: ['lowpass', 240, 1, 1.6, 0, .1, 1], a: [.003, .18, .75, .1], dr: .15, vol: .2 });
  v('ti-trance-bass', 'Trance Rolling Bas', 'Trance Rolling Bass', 'Bas', 'b', 'trance main', { o: [['w', 0, 0, .7], ['s', 0, -1, .8]], f: ['lowpass', 460, 3.5, 2.8, 0, .09, 1], a: [.001, .09, .5, .05], dr: .2, vol: .2 });
  v('ar-rolling-16', 'Armin Rolling 16', 'Rolling 16th Bass', 'Bas', 'b', 'trance', { o: [['w', 0, 0, .75], ['q', -5, 0, .25], ['s', 0, -1, .7]], f: ['lowpass', 520, 3, 2.4, 0, .08, 1], a: [.001, .07, .45, .04], dr: .15, vol: .2 });
  v('dd-funk-bass', 'Funk Bas', 'Funk Bass', 'Bas', 'b', 'house', { o: [['q', 0, 0, .8], ['s', 0, -1, .5]], f: ['lowpass', 700, 3, 3.6, 0, .09, 1], a: [.001, .16, .15, .06], dr: .3, vol: .21 });
  v('mp-rubber-bass', 'Rubber Bas', 'Rubber Bass', 'Bas', 'b', 'house techno', { o: [['t', 0, 0, .8], ['s', 0, -1, .6]], f: ['lowpass', 420, 4, 3.2, 0, .1, 1], a: [.001, .12, .2, .05], dr: .25, vol: .21 });
  v('dv-bigroom-bass', 'Big Room Bas', 'Big Room Bass', 'Bas', 'b', 'main hard', { o: [['w', 0, 0, .7], ['q', 0, -1, .5]], sub: .3, f: ['lowpass', 600, 2, 3, 0, .15, 1], a: [.002, .2, .5, .08], fd: .5, dr: .35, vol: .19 });
  v('um-electro-bass', 'Elektro Bas', 'Electro Bass', 'Bas', 'b', 'main techno', { o: [['w', 0, 0, .8], ['s', 0, -1, .5]], f: ['lowpass', 520, 4, 3.8, 0, .07, 1], a: [.001, .1, .35, .05], fd: .4, dr: .3, vol: .2 });
  v('cw-rolling-dark', 'Karanlık Rolling', 'Dark Rolling Bass', 'Bas', 'b', 'techno', { o: [['w', 0, 0, .7], ['s', 0, -1, .85]], f: ['lowpass', 280, 3.5, 2.4, 0, .08, 1], a: [.001, .08, .55, .05], dr: .3, vol: .2 });
  v('mg-prog-bass', 'Progressive Big Bas', 'Progressive Big Bass', 'Bas', 'b', 'main', { o: [['w', 0, 0, .6], ['w', -6, 0, .4], ['s', 0, -1, .8]], f: ['lowpass', 500, 1.8, 2.6, 0, .2, 1], a: [.003, .2, .7, .12], dr: .15, vol: .2 });
  v('an-cine-bass', 'Sinematik Sub', 'Cinematic Sub', 'Bas', 'b', 'mtechno', { o: [['s', 0, -1, 1], ['w', 0, 0, .3]], sub: .45, f: ['lowpass', 220, 1.2, 1.8, 0, .4, 1], a: [.01, .3, .8, .4], l: [[.07, .2, 'cut', 'sine']], dr: .1, vol: .2 });
  v('al-slap-bass', 'Slap Bas', 'Slap Bass', 'Bas', 'b', 'house afro', { o: [['q', 0, 0, .7], ['t', 0, -1, .6]], f: ['lowpass', 900, 2, 4, 0, .06, 1], a: [.001, .12, .08, .05], dr: .25, vol: .21 });
  v('tt-psy-bass', 'Psy Bas', 'Psy Bass', 'Bas', 'b', 'trance psy', { o: [['w', 0, 0, .8], ['s', 0, -1, .6]], f: ['lowpass', 380, 5, 3.4, 0, .06, 1], a: [.001, .06, .4, .03], dr: .2, vol: .2 });
  v('fa-garage-sub', 'UK Garage Sub', 'UK Garage Sub', 'Bas', 'b', 'house deep', { o: [['s', 0, -1, 1], ['t', 0, 0, .3]], sub: .35, f: ['lowpass', 300, 1, 2, 0, .14, 1], a: [.002, .24, .5, .1], dr: .12, vol: .2 });
  v('fi-tech-bass', 'Tech House Bas', 'Tech House Bass', 'Bas', 'b', 'house', { o: [['q', 0, 0, .65], ['s', 0, -1, .8]], f: ['lowpass', 380, 3, 3, 0, .09, 1], a: [.001, .11, .3, .05], dr: .22, vol: .2 });
  v('ip-psy-roll', 'Psy Roll', 'Psy Roll', 'Bas', 'b', 'techno psy', { o: [['w', 0, 0, .75], ['s', 0, -1, .65]], f: ['lowpass', 340, 4.5, 3.4, 0, .07, 1], a: [.001, .07, .5, .04], dr: .3, vol: .2 });
  v('sl-hard-bass', 'Hard Dist Bas', 'Hard Distorted Bass', 'Bas', 'b', 'hard', { o: [['w', 0, 0, .8], ['q', 0, -1, .5]], f: ['lowpass', 450, 3, 3, 0, .1, 1], a: [.001, .1, .6, .05], fd: .9, dr: .5, vol: .17 });
  v('so-deep-sub', 'Deep Sub', 'Deep Sub', 'Bas', 'b', 'deep house', { o: [['s', 0, -1, 1], ['t', 0, 0, .25]], sub: .3, f: ['lowpass', 280, 1, 1.4, 0, .25, 1], a: [.006, .3, .7, .2], dr: .08, vol: .2 });
  v('mq-prog-bass', 'Progressive Bas', 'Progressive Bass', 'Bas', 'b', 'house deep', { o: [['w', 0, 0, .55], ['s', 0, -1, .85]], f: ['lowpass', 360, 2, 2.2, 0, .18, 1], a: [.004, .22, .65, .14], dr: .12, vol: .2 });
  v('mo-oriental-bass', 'Oryantal Bas', 'Oriental Bass', 'Bas', 'b', 'deep house', { o: [['t', 0, 0, .7], ['s', 0, -1, .8]], f: ['lowpass', 520, 1.5, 2.4, 0, .2, 1], a: [.003, .26, .45, .15], dr: .2, vol: .2 });
  v('hu-tumbao', 'Tumbao Bas', 'Tumbao Bass', 'Bas', 'b', 'afro house', { o: [['s', 0, -1, .9], ['t', 0, 0, .5]], f: ['lowpass', 640, 1.5, 2.8, 0, .12, 1], a: [.002, .2, .2, .1], dr: .15, vol: .21 });
  v('bc-soul-bass', 'Soulful Bas', 'Soulful Bass', 'Bas', 'b', 'afro deep', { o: [['s', 0, -1, 1], ['t', 0, 0, .35]], sub: .25, f: ['lowpass', 480, 1, 1.6, 0, .3, 1], a: [.005, .35, .5, .25], dr: .08, vol: .2 });
  // --- Leadler
  v('dd-vox-hook', 'Funk Vokal Hook', 'Funk Vocal Hook', 'Vox', 'm', 'house', { o: [['q', 0, 0, .5], ['w', 0, 0, .4]], vow: 'a', f: ['lowpass', 3200, 2, 1.8, 0, .12], a: [.004, .14, .3, .1], dl: .3, rv: .2, vol: .2 });
  v('mp-vox-hook', 'Mau P Vokal Chop', 'Minimal Vocal Chop', 'Vox', 'm', 'house techno', { o: [['q', 0, 0, .6], ['t', 0, 0, .3]], vow: 'o', f: ['lowpass', 2000, 3, 2, 0, .08], a: [.002, .09, 0, .05], dl: .38, rv: .12, vol: .22 });
  v('al-vox-lead', 'Brazilian Vokal Lead', 'Brazilian Vocal Lead', 'Vox', 'm', 'house afro', { o: [['w', 0, 0, .5], ['t', 0, 0, .4]], vow: 'e', f: ['lowpass', 3600, 1.5, 1.2, 0, .2], a: [.01, .2, .6, .2], vib: [5.4, 10, .15], ch: .3, rv: .25, vol: .2 });
  v('fa-vox-pad-lead', 'Duygusal Vokal Lead', 'Emotional Vocal Lead', 'Vox', 'm', 'house deep', { o: [['s', 0, 0, .6], ['w', 0, 0, .3]], vow: 'u', f: ['lowpass', 2800, 1, 1, 0, .4], a: [.04, .3, .7, .5], vib: [5, 14, .2], ch: .35, rv: .5, dl: .25, vol: .2 });
  v('fi-vox-chop', 'Fisher Chop', 'Chop Hook', 'Vox', 'm', 'house', { o: [['q', 0, 0, .55], ['w', 0, 0, .3]], vow: 'i', f: ['lowpass', 2600, 2.5, 2, 0, .07], a: [.002, .08, 0, .04], dl: .33, rv: .1, vol: .22 });
  v('cc-groove-pluck', 'Groove Pluck', 'Groove Pluck', 'Pluck', 'm', 'techno house', { o: [['w', 0, 0, .6], ['q', 0, 0, .3]], f: ['lowpass', 1500, 3, 3.2, 0, .1], a: [.001, .17, 0, .08], dl: .35, rv: .18, vol: .21 });
  v('an-cine-arp', 'Sinematik Arp', 'Cinematic Arp', 'Pluck', 'm', 'mtechno', { u: [3, 14], o: [['t', 0, 0, .5], ['s', 0, 1, .35]], f: ['lowpass', 4200, .8, 2, 0, .35], a: [.002, .5, 0, .6], ch: .4, rv: .55, dl: .4, vol: .2 });
  // --- Atamalar (bas / akor / lead; davul kiti ve diğer ayarlar aynı kalır)
  const A = { wavejackers: ['', 'wj-roll-bass', ''], guetta: ['gu-pop-stab', 'gu-pump-bass', ''], tiesto: ['ti-fest-chord', 'ti-trance-bass', ''], armin: ['ar-uplift-pad', 'ar-rolling-16', ''],
    domdolla: ['dd-funk-stab', 'dd-funk-bass', 'dd-vox-hook'], maup: ['mp-minimal-stab', 'mp-rubber-bass', 'mp-vox-hook'], dvlm: ['dv-rave-chord', 'dv-bigroom-bass', ''], ummet: ['um-electro-chord', 'um-electro-bass', ''],
    charlotte: ['cw-dark-chord', 'cw-rolling-dark', ''], cox: ['', '', 'cc-groove-pluck'], garrix: ['', 'mg-prog-bass', ''], anyma: ['', 'an-cine-bass', 'an-cine-arp'],
    alok: ['al-brazil-chord', 'al-slap-bass', 'al-vox-lead'], timmy: ['tt-psy-stab', 'tt-psy-bass', ''], fred: ['fa-uk-keys', 'fa-garage-sub', 'fa-vox-pad-lead'], fisher: ['fi-house-organ', 'fi-tech-bass', 'fi-vox-chop'],
    indira: ['ip-psy-chord', 'ip-psy-roll', ''], landry: ['sl-hard-stab', 'sl-hard-bass', ''], solomun: ['', 'so-deep-sub', ''], monique: ['mq-prog-pad', 'mq-prog-bass', ''],
    orhan: ['', 'mo-oriental-bass', ''], hugel: ['', 'hu-tumbao', ''], blackcoffee: ['', 'bc-soul-bass', ''] };
  Object.keys(A).forEach(k => { const s = S[k]; if (!s) return; if (A[k][0]) s.sound.chords = A[k][0]; if (A[k][1]) s.sound.bass = A[k][1]; if (A[k][2]) s.sound.melody = A[k][2]; });
  MB.SOUNDS.forEach(s => { MB.byId[s.id] = s; });
  Object.assign(HG.EN, EN);
})();
