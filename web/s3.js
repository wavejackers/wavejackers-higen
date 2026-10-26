// ---- Ses bankası 2/2: Pluck, Lead, Keys, Vox, davul kitleri, tür varsayılanları
// ---------- PLUCK
add('classic-pluck', 'Klasik Pluck', 'Pluck', 'cm', 'house deep mtechno trance', { o: [['w', 0, 0, .6], ['q', 3, 0, .3]], f: ['lowpass', 900, 2, 4, 0, .15], a: [.002, .25, 0, .1], rv: .3, dl: .2 });
add('pizz', 'Pizzicato', 'Pluck', 'cm', 'house lofi deep', { o: [['t', 0, 0, .8]], f: ['lowpass', 1800, .7, 2, 0, .08], a: [.002, .12, 0, .05], rv: .2 });
add('kalimba', 'Kalimba', 'Pluck', 'cm', 'afro lofi deep', { o: [['s', 0, 0, .8], ['s', 0, 2.43, .18]], f: ['lowpass', 5000, .5, 0, 0, .3], a: [.002, .55, 0, .4], rv: .4, dl: .2 });
add('marimba', 'Marimba', 'Pluck', 'cm', 'afro lofi deep', { o: [['s', 0, 0, 1], ['s', 0, 2, .15]], f: ['lowpass', 3000, .7, 1, 0, .1], noi: .04, a: [.002, .35, 0, .2], rv: .3 });
add('muted-pluck', 'Sessiz Tel', 'Pluck', 'cm', 'lofi deep afro', { o: [['w', 0, 0, .6]], f: ['lowpass', 700, 1, 3, 0, .1], a: [.002, .3, 0, .12], dr: .15, rv: .2 });
add('harp', 'Arp (Harp)', 'Pluck', 'cm', 'trance mtechno lofi', { o: [['t', 0, 0, .7], ['s', 0, 1, .3]], f: ['lowpass', 3500, .7, 1.5, 0, .3], a: [.002, .9, 0, .5], rv: .5 });
add('bell-pluck', 'Çan Pluck', 'Pluck', 'cm', 'trap dnb mtechno lofi', { o: [['s', 0, 0, .6], ['s', 0, 1.5, .3], ['s', 0, 2.4, .2]], f: ['lowpass', 7000, .5, 0, 0, .3], a: [.002, 1.1, 0, .9], rv: .5, dl: .3 });
add('glass-pluck', 'Cam Pluck (FM)', 'Pluck', 'cm', 'mtechno trap trance', { fm: [3.5, 1.6, .2, .1], f: ['lowpass', 8000, .5, 0, 0, .3], a: [.002, .6, 0, .4], rv: .4, dl: .25 });
add('techno-blip', 'Techno Blip', 'Pluck', 'cm', 'techno mtechno', { o: [['q', 0, 0, .7]], f: ['lowpass', 1500, 6, 3, 0, .06], a: [.001, .1, 0, .04], dl: .45, rv: .15 });
add('trance-pluck', 'Trance Pluck', 'Pluck', 'cm', 'trance mtechno house', { u: [3, 18], o: [['w', 0, 0, .5]], f: ['lowpass', 1400, 2, 4, 0, .12], a: [.002, .28, 0, .1], dl: .4, rv: .3, ch: .3 });

// ---------- LEAD
add('super-lead', 'Süper Lead', 'Lead', 'm', 'trance mtechno house', { u: [5, 24], o: [['w', 0, 0, .5]], f: ['lowpass', 3500, .8, 1, 0, .3], a: [.01, .2, .85, .25], vib: [5.5, 8, .25], ch: .4, dl: .3, rv: .3 });
add('trance-lead', 'Trance Lead', 'Lead', 'm', 'trance mtechno', { u: [3, 16], o: [['w', 0, 0, .5], ['q', 0, 0, .3]], f: ['lowpass', 4500, .7, .5, 0, .3], a: [.005, .15, .9, .2], vib: [5.8, 10, .2], dl: .45, rv: .3 });
add('square-lead', 'Kare Lead', 'Lead', 'm', 'house techno trap', { o: [['q', 0, 0, .5], ['q', 6, 0, .3]], f: ['lowpass', 2800, 1, .5, 0, .3], a: [.005, .1, .85, .12], vib: [5, 6, .3], dl: .25 });
add('sine-lead', 'Yumuşak Sinüs Lead', 'Lead', 'm', 'dnb lofi deep trap', { o: [['s', 0, 0, .9], ['t', 0, 1, .15]], f: ['lowpass', 4000, .5, 0, 0, .3], a: [.02, .1, .9, .2], vib: [5, 10, .3], rv: .3, dl: .3 });
add('pulse-lead', 'Pulse Lead', 'Lead', 'm', 'house techno deep', { o: [['q', 0, 0, .5], ['q', -8, 0, .3]], f: ['lowpass', 2000, 2, 2, 0, .2], a: [.004, .18, .6, .1], dl: .3 });
add('sync-lead', 'Sync Lead', 'Lead', 'm', 'techno mtechno', { o: [['w', 0, 0, .5], ['w', 15, 1, .3]], f: ['lowpass', 3200, 3, 3, 0, .18], a: [.003, .15, .75, .1], dr: .2 });
add('whistle-lead', 'Islık Lead', 'Lead', 'm', 'afro lofi deep trance', { o: [['s', 0, 1, .8], ['s', 0, 0, .2]], f: ['lowpass', 6000, .5, 0, 0, .3], noi: .02, a: [.04, .1, .9, .2], vib: [5.2, 14, .25], rv: .3 });
add('mt-lead', 'Melodik Techno Lead', 'Lead', 'm', 'mtechno trance techno', { u: [2, 10], o: [['w', 0, 0, .5], ['t', 0, 0, .4]], f: ['lowpass', 2200, 1.5, 1.5, 0, .25], a: [.004, .2, .6, .18], dl: .5, rv: .4 });
add('mono-lead', 'Mono Lead', 'Lead', 'm', 'techno house dnb trap', { o: [['w', 0, 0, .7], ['q', 0, -1, .3]], f: ['lowpass', 1800, 3, 2, 0, .15], a: [.004, .1, .8, .1], vib: [5, 5, .2], dr: .25 });
add('fm-lead', 'FM Lead', 'Lead', 'm', 'house mtechno dnb', { fm: [2, 3, .35, .35], f: ['lowpass', 6000, .5, 0, 0, .3], a: [.005, .2, .7, .2], ch: .3, dl: .25 });
add('glass-lead', 'Cam Lead', 'Lead', 'm', 'mtechno trance lofi', { o: [['s', 0, 0, .5], ['t', 7, 1, .3]], f: ['lowpass', 6000, .5, 0, 0, .3], a: [.005, .3, .5, .4], rv: .5, dl: .3, ch: .4 });
add('afro-flute', 'Afro Flüt', 'Lead', 'm', 'afro deep lofi', { o: [['t', 0, 0, .9]], f: ['lowpass', 3000, .5, 0, 0, .3], noi: .05, vib: [5.5, 18, .2], a: [.05, .1, .85, .15], rv: .3 });

// ---------- KEYS
add('e-piano', 'Elektrik Piyano (FM)', 'Keys', 'cm', 'house deep lofi', { fm: [1, 1.2, .5, .15], f: ['lowpass', 5000, .5, 0, 0, .3], a: [.003, .9, .25, .35], ch: .3, rv: .3 });
add('rhodes', 'Rhodes', 'Keys', 'cm', 'lofi deep house', { fm: [1, .9, .6, .2], o: [['s', 0, 0, .3]], f: ['lowpass', 4500, .5, 0, 0, .3], a: [.003, 1, .3, .4], ch: .5, rv: .3, dr: .1 });
add('organ', 'Org (Drawbar)', 'Keys', 'cm', 'house deep afro', { o: [['s', 0, -1, .5], ['s', 0, 0, .8], ['s', 0, 1, .5], ['s', 0, 1.58, .35], ['s', 0, 2, .25]], f: ['lowpass', 6000, .5, 0, 0, .3], a: [.01, .05, 1, .06], rv: .2 });
add('wurli', 'Wurli', 'Keys', 'cm', 'lofi deep afro', { o: [['t', 0, 0, .8], ['s', 0, 1, .3]], f: ['lowpass', 2200, .7, 1, 0, .2], a: [.003, .4, .3, .2], dr: .4, ch: .3 });
add('clav', 'Clav', 'Keys', 'cm', 'afro deep house', { o: [['q', 0, 0, .6], ['w', 0, 0, .3]], f: ['lowpass', 2500, 2, 3, 0, .07], a: [.002, .15, 0, .06], rv: .15 });
add('lofi-keys', 'Lo-fi Keys', 'Keys', 'cm', 'lofi', { fm: [1, .9, .6, .2], f: ['lowpass', 3000, .5, 0, 0, .3], vib: [.5, 18, 0], noi: .02, dr: .25, a: [.004, .8, .25, .4], rv: .3 });

// ---------- VOX
add('aah-female', 'Aah Kadın', 'Vox', 'cm', 'house deep trance mtechno', { u: [2, 8], o: [['w', 0, 0, .6]], vow: 'a', vib: [5.5, 16, .3], noi: .05, a: [.1, .1, .9, .3], rv: .4, vol: .5 });
add('ooh-choir', 'Ooh Koro', 'Vox', 'cm', 'deep mtechno trance lofi', { u: [4, 14], o: [['w', 0, 0, .6]], vow: 'o', vib: [5, 10, .4], a: [.3, .2, .9, .6], rv: .5, ch: .5, vol: .5 });
add('eeh-vox', 'Eeh Synth Vox', 'Vox', 'cm', 'techno house trap', { o: [['w', 0, 0, .7]], vow: 'e', vib: [5, 8, .2], a: [.03, .1, .85, .2], rv: .3, dl: .3, vol: .5 });
add('vocoder', 'Vocoder Robot', 'Vox', 'cm', 'techno trance dnb', { o: [['q', 0, 0, .6], ['w', 3, 0, .4]], vow: 'a', a: [.01, .05, .9, .08], dr: .2, dl: .3, vol: .5 });
add('breath-hum', 'Nefes Hum', 'Vox', 'cm', 'lofi deep afro', { o: [['s', 0, 0, .8], ['t', 0, 1, .2]], vow: 'u', noi: .15, a: [.2, .2, .8, .5], rv: .5, vol: .6 });
add('male-ooh', 'Erkek Ooh', 'Vox', 'cm', 'deep house afro dnb', { u: [2, 8], o: [['w', 0, -1, .6]], vow: 'o', male: true, vib: [4.8, 9, .4], a: [.2, .2, .9, .4], rv: .4, vol: .5 });
add('vox-chop', 'Vokal Chop', 'Vox', 'cm', 'house deep trap', { o: [['w', 0, 0, .7]], vow: 'a', a: [.004, .12, .2, .1], dl: .3, rv: .3, vol: .5 });
add('angel-aah', 'Melek Aah', 'Vox', 'cm', 'trance mtechno', { u: [3, 10], o: [['w', 0, 1, .6]], vow: 'a', vib: [5.2, 12, .5], a: [.6, .4, .9, 1.2], rv: .6, ch: .4, vol: .5 });

// ---------- DAVUL KİTLERİ (k: kick, s: snare, c: clap, h: kapalı hat, o: açık hat, r: rim, p: perc)
MB.KITS = {
  house:     { id: 'house', n: 'House Punch', g: ['house', 'trance'], k: { f0: 160, f1: 50, pd: .04, d: .32, click: .3, drv: .5 }, s: { f: 190, td: .1, nf: 2400, nd: .16, mix: .55 }, c: { nf: 1400, d: .22 }, h: { hp: 7500, d: .045, m: .25 }, o: { hp: 6500, d: .28, m: .3 }, r: { f: 1800 }, p: { f: 330, d: .15 }, rv: .08 },
  deep:      { id: 'deep', n: 'Deep Yuvarlak', g: ['deep', 'house', 'afro'], k: { f0: 120, f1: 45, pd: .06, d: .45, click: .12, drv: .3 }, s: { f: 170, td: .12, nf: 2000, nd: .14, mix: .45 }, c: { nf: 1200, d: .25 }, h: { hp: 8500, d: .035, m: 0 }, o: { hp: 7500, d: .25, m: 0 }, r: { f: 1500 }, p: { f: 280, d: .18 }, lp: 9000, rv: .12 },
  techno:    { id: 'techno', n: 'Techno Sert', g: ['techno', 'mtechno'], k: { f0: 200, f1: 42, pd: .03, d: .4, click: .45, drv: 1 }, s: { f: 200, td: .09, nf: 2600, nd: .13, mix: .6 }, c: { nf: 1100, d: .2 }, h: { hp: 8000, d: .04, m: .5 }, o: { hp: 7000, d: .22, m: .5 }, r: { f: 2000 }, p: { f: 400, d: .1, type: 'triangle' }, rv: .06 },
  warehouse: { id: 'warehouse', n: 'Depo Gümbürtüsü', g: ['mtechno', 'techno'], k: { f0: 140, f1: 38, pd: .05, d: .6, click: .25, drv: .8 }, s: { f: 180, td: .12, nf: 2000, nd: .18, mix: .55 }, c: { nf: 1000, d: .3 }, h: { hp: 8200, d: .04, m: .35 }, o: { hp: 7000, d: .3, m: .4 }, r: { f: 1700 }, p: { f: 360, d: .14 }, rv: .14 },
  trance:    { id: 'trance', n: 'Trance Drive', g: ['trance', 'house'], k: { f0: 170, f1: 52, pd: .035, d: .3, click: .35, drv: .6 }, s: { f: 210, td: .1, nf: 3000, nd: .15, mix: .6 }, c: { nf: 1600, d: .2 }, h: { hp: 9000, d: .04, m: .2 }, o: { hp: 7500, d: .3, m: .25 }, r: { f: 2000 }, p: { f: 420, d: .1 }, rv: .1 },
  trap:      { id: 'trap', n: '808 Trap', g: ['trap', 'dnb'], k: { f0: 110, f1: 48, pd: .08, d: .9, click: .1, drv: .4 }, s: { f: 220, td: .1, nf: 3200, nd: .17, mix: .65 }, c: { nf: 1500, d: .25 }, h: { hp: 9000, d: .03, m: .15 }, o: { hp: 8000, d: .2, m: .2 }, r: { f: 1900 }, p: { f: 500, d: .1 }, rv: .1 },
  dnb:       { id: 'dnb', n: 'DnB Sıkı', g: ['dnb', 'trap'], k: { f0: 190, f1: 55, pd: .03, d: .22, click: .35, drv: .5 }, s: { f: 230, td: .08, nf: 3200, nd: .13, mix: .7 }, c: { nf: 1500, d: .18 }, h: { hp: 9000, d: .03, m: .15 }, o: { hp: 8000, d: .18, m: .2 }, r: { f: 2100 }, p: { f: 450, d: .08 }, rv: .07 },
  lofi:      { id: 'lofi', n: 'Lo-fi Tozlu', g: ['lofi', 'deep'], k: { f0: 110, f1: 48, pd: .06, d: .28, click: .1, drv: .3 }, s: { f: 180, td: .11, nf: 1800, nd: .15, mix: .5 }, c: { nf: 1100, d: .2 }, h: { hp: 6500, d: .04, m: 0 }, o: { hp: 6000, d: .22, m: 0 }, r: { f: 1400 }, p: { f: 300, d: .15 }, lp: 5500, rv: .12 },
  afro:      { id: 'afro', n: 'Afro Organik', g: ['afro', 'deep'], k: { f0: 130, f1: 48, pd: .05, d: .3, click: .1, drv: .25 }, s: { f: 200, td: .1, nf: 2200, nd: .12, mix: .4 }, c: { nf: 1300, d: .2 }, h: { hp: 6500, d: .06, m: 0 }, o: { hp: 6000, d: .2, m: 0 }, r: { f: 1600 }, p: { f: 260, d: .22 }, lp: 10000, rv: .12 }
};
MB.KITS_LIST = Object.keys(MB.KITS).map(k => MB.KITS[k]);

// ---------- Tür varsayılanları (pad/stab/arp: akor çalış biçimine göre)
MB.DEFAULTS = {
  house:   { pad: 'sunrise-pad',   stab: 'organ-stab', arp: 'classic-pluck', bass: 'punch-bass',  melody: 'e-piano',      drums: 'house' },
  deep:    { pad: 'dub-pad',       stab: 'deep-stab',  arp: 'kalimba',       bass: 'deep-bass',   melody: 'rhodes',       drums: 'deep' },
  techno:  { pad: 'dark-drone',    stab: 'rave-stab',  arp: 'techno-blip',   bass: 'roll-bass',   melody: 'mono-lead',    drums: 'techno' },
  mtechno: { pad: 'evolving-pad',  stab: 'vapor-stab', arp: 'glass-pluck',   bass: 'roll-bass',   melody: 'mt-lead',      drums: 'warehouse' },
  trance:  { pad: 'super-pad',     stab: 'super-stab', arp: 'trance-pluck',  bass: 'trance-bass', melody: 'trance-lead',  drums: 'trance' },
  afro:    { pad: 'hollow-pad',    stab: 'clav',       arp: 'kalimba',       bass: 'wood-bass',   melody: 'afro-flute',   drums: 'afro' },
  dnb:     { pad: 'sad-pad',       stab: 'dub-chord',  arp: 'bell-pluck',    bass: 'reese',       melody: 'sine-lead',    drums: 'dnb' },
  trap:    { pad: 'glass-pad',     stab: 'vapor-stab', arp: 'bell-pluck',    bass: '808-long',    melody: 'bell-pluck',   drums: 'trap' },
  lofi:    { pad: 'tape-pad',      stab: 'lofi-keys',  arp: 'muted-pluck',   bass: 'upright',     melody: 'wurli',        drums: 'lofi' }
};
MB.byId = {};
MB.SOUNDS.forEach(s => { MB.byId[s.id] = s; });
MB.CATS = ['Bas', 'Pad', 'Stab', 'Pluck', 'Lead', 'Keys', 'Vox'];
