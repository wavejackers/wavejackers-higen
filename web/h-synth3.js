// ---- Wavejackers Studio Bank (2.3): referans şarkıların tonal dengesinden öğrenilmiş, dolgun ve geniş özgün sesler.
// Hedefler (6 referans şarkının ölçümünden): alt bölge (20-250 Hz) toplam enerjinin ~%60-70'i, 250-2k arası dolu orta,
// stereo yayılma yeterli, tepe -1 dBFS altında. Her ses: unison/sub katmanı + ılık doygunluk + uzun kuyruk.
(function () {
  const add = MB.add;
  // Basslar: sub sinüs + üst katman, filtre zarfı ile vurgu, sürüş ile doygunluk
  add('ws-sub-heavy', 'Studio Sub Heavy', 'Bas', 'b', 'house techno afro', { o: [['s', 0, -1, 1], ['w', 0, 0, .35]], sub: .5, f: ['lowpass', 260, 1.2, 2.2, 0, .16, 1], a: [.003, .22, .7, .12], dr: .22, vol: .2 });
  add('ws-fat-roll', 'Studio Fat Roll', 'Bas', 'b', 'techno trance house', { o: [['w', 0, 0, .7], ['q', -7, 0, .4], ['s', 0, -1, .9]], sub: .35, f: ['lowpass', 380, 3, 3.2, 0, .1, 1], a: [.002, .1, .55, .06], fd: .3, dr: .3, vol: .2 });
  add('ws-reese-wide', 'Studio Reese', 'Bas', 'b', 'techno mtechno', { o: [['w', 0, 0, .6], ['w', 0, 1, .6], ['s', 0, -1, .8]], f: ['lowpass', 420, 2, 2.4, .02, .35, 1], a: [.01, .3, .75, .2], l: [[.18, .3, 'cut', 'sine']], dr: .25, vol: .2 });
  add('ws-deep-warm-bass', 'Studio Warm Deep Bass', 'Bas', 'b', 'deep house afro', { o: [['t', 0, 0, .6], ['s', 0, -1, .95]], sub: .3, f: ['lowpass', 520, 1, 1.5, 0, .2], a: [.004, .25, .65, .14], dr: .18, ch: .05, vol: .2 });
  // Akor / stab / pad: yedi sesli unison, alt oktav katmanı, yavaş parlama
  add('ws-super-chord', 'Studio Super Chord', 'Stab', 'c', 'house trance main', { u: [7, 26], o: [['w', 0, 0, .55], ['w', 0, -1, .4], ['q', 0, 0, .25]], f: ['lowpass', 3600, 1, 2.2, 0, .28], a: [.005, .35, .65, .3], ch: .45, rv: .35, dl: .15, dr: .08, vol: .2 });
  add('ws-lush-pad', 'Studio Lush Pad', 'Pad', 'c', 'mtechno deep afro', { u: [7, 20], o: [['x', 0, 0, .5], ['w', 0, -1, .35], ['t', 0, 0, .25]], wt: [0, 10, .5, .6], f: ['lowpass', 1500, 1, 1.8, 1.4, 2.8], a: [.7, .8, .85, 2.2], l: [[.1, .3, 'cut', 'sine']], ch: .55, rv: .65, vol: .22 });
  add('ws-warm-stack', 'Studio Warm Stack', 'Keys', 'c', 'house deep afro', { u: [5, 12], o: [['t', 0, 0, .5], ['w', 0, -1, .3], ['s', 0, 0, .2]], f: ['lowpass', 2400, .8, 1.4, 0, .4], a: [.006, .9, .35, .6], ch: .4, rv: .45, vol: .22 });
  add('ws-rave-organ', 'Studio Rave Organ', 'Stab', 'c', 'house techno hard', { u: [5, 16], o: [['q', 0, 0, .55], ['q', 12, 0, .25], ['w', 0, -1, .35]], f: ['lowpass', 4200, 1.4, 1.8, 0, .2], a: [.003, .25, .55, .12], ch: .3, rv: .25, dr: .25, vol: .2 });
  add('ws-dark-wall', 'Studio Dark Wall', 'Pad', 'c', 'techno mtechno', { u: [7, 26], o: [['w', 0, 0, .5], ['w', 0, -1, .45]], f: ['lowpass', 900, 2, 1.8, 1.2, 2.2], a: [.9, .9, .9, 2.6], l: [[.07, .35, 'cut', 'sine']], ch: .5, rv: .7, dr: .15, vol: .22 });
  // Lead / pluck: kalın unison, vibrato, yankı
  add('ws-anthem-lead', 'Studio Anthem Lead', 'Lead', 'm', 'main trance house', { u: [7, 18], o: [['w', 0, 0, .55], ['q', 0, 0, .3], ['w', 0, -1, .25]], pe: [-2, .04], f: ['lowpass', 3800, 1.4, 2.4, .01, .25], a: [.008, .2, .8, .3], vib: [5.4, 8, .3], dl: .45, rv: .4, ch: .25, dr: .2, vol: .2 });
  add('ws-silk-lead', 'Studio Silk Lead', 'Lead', 'm', 'mtechno deep afro', { u: [5, 12], o: [['x', 0, 0, .5], ['t', 0, 0, .3], ['w', 0, -1, .2]], wt: [3, 7, .4, .8], f: ['lowpass', 3000, 1, 1.3, 0, .35], a: [.02, .3, .75, .5], vib: [5.1, 9, .35], dl: .5, rv: .55, ch: .35, vol: .2 });
  add('ws-wide-pluck', 'Studio Wide Pluck', 'Pluck', 'cm', 'mtechno trance house', { u: [6, 16], o: [['w', 0, 0, .5], ['q', 0, 1, .3], ['s', 0, -1, .25]], f: ['lowpass', 3200, 2, 3.2, 0, .24], a: [.002, .4, .1, .3], dl: .5, rv: .5, ch: .3, vol: .2 });
  add('ws-grand-warm', 'Studio Warm Piano', 'Keys', 'c', 'house deep afro', { fm: [1, 1.1, .5, .12, .55], u: [3, 6], o: [['t', 0, 0, .5], ['s', 0, -1, .3], ['q', 0, 1, .12]], f: ['lowpass', 3000, .7, 1.2, 0, .4], a: [.003, 1.3, .15, .7], ch: .2, rv: .45, vol: .22 });
  add('ws-brass-hit', 'Studio Brass Hit', 'Lead', 'cm', 'house afro main', { u: [5, 14], o: [['w', 0, 0, .6], ['q', 0, 1, .3], ['w', 0, -1, .25]], pe: [-2, .05], f: ['lowpass', 2200, 1.6, 2.8, .02, .22], a: [.008, .2, .55, .15], vib: [5.4, 6, .2], dr: .3, dl: .25, rv: .3, vol: .2 });

  // Stil varsayılanları: ince (3 kattan az) sesleri, role ve türe uygun dolgun Studio seslerle değiştir.
  // Kimliği belirleyen asit/ekran sesleri ve Afrika/Orient tınıları (nefes/ney/saksafon/kalimba) olduğu gibi kalır.
  const keep = new Set(['acid-303', 'cw-acid-riff', 'ip-acid-lead', 'sl-hard-lead', 'dv-screech-lead', 'mo-sax-lead', 'mo-ney-flute', 'bc-kalimba', 'vox-chop', 'hu-brass', 'bc-soul-keys', 'mo-warm-keys', 'hu-latin-piano']);
  const layers = id => { const d = MB.byId[id]; if (!d) return 9; return (d.o || []).length * (d.u ? d.u[0] : 1) + (d.sub ? 1 : 0) + (d.fm ? 1 : 0); };
  const pick = { house: { c: 'ws-warm-stack', b: 'ws-deep-warm-bass', m: 'ws-silk-lead' }, afro: { c: 'ws-warm-stack', b: 'ws-deep-warm-bass', m: 'ws-silk-lead' },
    techno: { c: 'ws-dark-wall', b: 'ws-fat-roll', m: 'ws-wide-pluck' }, trance: { c: 'ws-super-chord', b: 'ws-fat-roll', m: 'ws-anthem-lead' },
    main: { c: 'ws-super-chord', b: 'ws-fat-roll', m: 'ws-anthem-lead' }, hard: { c: 'ws-rave-organ', b: 'ws-reese-wide', m: 'ws-anthem-lead' } };
  HG.STYLES.forEach(S => {
    const g = pick[S.grp] || pick.house;
    [['chords', 'c'], ['bass', 'b'], ['melody', 'm']].forEach(([part, k]) => {
      const cur = S.sound[part];
      if (!keep.has(cur) && layers(cur) < 4) S.sound[part] = g[k];
    });
  });
  // Black Coffee ve Solomun gibi imza gruplarına özel Studio seçimleri
  HG.STYLE.solomun.sound.chords = 'ws-lush-pad'; HG.STYLE.solomun.sound.bass = 'ws-deep-warm-bass';
  HG.STYLE.monique.sound.chords = 'ws-wide-pluck'; HG.STYLE.monique.sound.bass = 'ws-fat-roll'; HG.STYLE.monique.sound.melody = 'ws-silk-lead';
  HG.STYLE.orhan.sound.bass = 'ws-deep-warm-bass'; HG.STYLE.hugel.sound.bass = 'ws-sub-heavy'; HG.STYLE.blackcoffee.sound.bass = 'ws-sub-heavy'; HG.STYLE.blackcoffee.sound.chords = 'ws-grand-warm';
  MB.SOUNDS.forEach(s => { MB.byId[s.id] = s; });
  HG.SIG = /^(wj|gt|ar|dd|mp|dv|cw|cc|mg|an|al|tt|fa|fi|ip|sl|so|mm|mo|hu|bc|ws)-/;
  Object.assign(HG.EN, { 'ws-sub-heavy': 'Studio Sub Heavy', 'ws-fat-roll': 'Studio Fat Roll', 'ws-reese-wide': 'Studio Reese', 'ws-deep-warm-bass': 'Studio Warm Deep Bass', 'ws-super-chord': 'Studio Super Chord', 'ws-lush-pad': 'Studio Lush Pad', 'ws-warm-stack': 'Studio Warm Stack', 'ws-rave-organ': 'Studio Rave Organ', 'ws-dark-wall': 'Studio Dark Wall', 'ws-anthem-lead': 'Studio Anthem Lead', 'ws-silk-lead': 'Studio Silk Lead', 'ws-wide-pluck': 'Studio Wide Pluck', 'ws-grand-warm': 'Studio Warm Piano', 'ws-brass-hit': 'Studio Brass Hit' });
})();
