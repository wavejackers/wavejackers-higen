// ---- Ses bankası 1/2: Bas, Pad, Stab
// o: [dalga, detune(cent), oktav, kazanç]  u:[sayı, açılım(cent)]  f:[tip, kesim, Q, zarf, atak, düşüş]
// a:[A,D,S,R]  vib:[Hz, cent, gecikme]  lf:[Hz, derinlik]  rv/dl/ch: efekt gönderimi  dr: doygunluk
MB.SOUNDS = [];
MB.add = function (id, name, cat, r, g, o) { MB.SOUNDS.push(Object.assign({ id, name, cat, r, g: g.split(' '), vol: .2 }, o)); };
const ALLG = 'house deep techno mtechno trance afro dnb trap lofi';
const add = MB.add;

// ---------- BAS
add('sub-sine', 'Sub Sinüs', 'Bas', 'b', 'deep afro lofi trap house', { o: [['s', 0, 0, 1], ['t', 0, 0, .18]], f: ['lowpass', 500, .5, 0, 0, .2], a: [.005, .1, 1, .09] });
add('deep-bass', 'Deep House Bas', 'Bas', 'b', 'deep house afro', { o: [['w', 0, 0, .8], ['s', 0, -1, .9]], f: ['lowpass', 380, 2, 2.5, 0, .18], a: [.004, .2, .55, .1], dr: .15 });
add('roll-bass', 'Rolling Techno Bas', 'Bas', 'b', 'techno mtechno trance', { o: [['w', 0, 0, .9], ['q', -7, 0, .4]], f: ['lowpass', 260, 5, 3, 0, .12], a: [.002, .08, .6, .05], dr: .2 });
add('808-long', '808 Uzun', 'Bas', 'b', 'trap lofi dnb', { o: [['s', 0, 0, 1]], pe: [7, .05], a: [.002, 1.3, .6, .45], f: ['lowpass', 900, .5, 0, 0, .2], dr: .5 });
add('reese', 'Reese', 'Bas', 'b', 'dnb techno', { o: [['w', -14, 0, .6], ['w', 14, 0, .6], ['w', 0, -1, .5]], f: ['lowpass', 520, 1.5, .5, 0, .5], a: [.01, .3, .9, .2], ch: .3, dr: .25 });
add('acid-303', 'Acid 303', 'Bas', 'b', 'techno trance house', { o: [['w', 0, 0, 1]], f: ['lowpass', 400, 14, 5, 0, .22], a: [.002, .2, .7, .05], dr: .3, dl: .2 });
add('pluck-bass', 'Pluck Bas', 'Bas', 'b', 'deep house mtechno', { o: [['q', 0, 0, .7], ['w', 0, -1, .5]], f: ['lowpass', 300, 3, 4, 0, .09], a: [.002, .18, .1, .06] });
add('funk-bass', 'Funky Kare Bas', 'Bas', 'b', 'afro deep house', { o: [['q', 0, 0, .8], ['t', 0, -1, .6]], f: ['lowpass', 600, 4, 3, 0, .14], a: [.003, .12, .5, .08] });
add('neuro-lite', 'Neuro Hafif', 'Bas', 'b', 'dnb techno', { o: [['w', -20, 0, .5], ['w', 20, 0, .5], ['q', 0, -1, .4]], f: ['bandpass', 700, 3, 1.5, 0, .3], lf: [6, .7], a: [.005, .2, .8, .12], dr: .4 });
add('wood-bass', 'Ahşap Bas', 'Bas', 'b', 'afro lofi deep', { o: [['t', 0, 0, 1], ['s', 0, -1, .6]], f: ['lowpass', 450, 1, 2, 0, .1], noi: .04, a: [.003, .2, .3, .1] });
add('upright', 'Lo-fi Kontrbas', 'Bas', 'b', 'lofi deep', { o: [['t', 0, 0, 1], ['s', 0, -1, .5]], f: ['lowpass', 380, 1, 1.5, 0, .25], noi: .03, a: [.01, .3, .4, .2], dr: .2 });
add('punch-bass', 'Tek Ses Punch', 'Bas', 'b', 'house techno deep', { o: [['w', 0, 0, 1], ['w', 6, 0, .5]], f: ['lowpass', 500, 2, 3, 0, .1], a: [.002, .15, .7, .06], dr: .15 });
add('trance-bass', 'Trance Offbeat', 'Bas', 'b', 'trance mtechno', { o: [['w', -8, 0, .7], ['w', 8, 0, .7], ['s', 0, -1, .7]], f: ['lowpass', 700, 3, 2, 0, .12], a: [.002, .1, .8, .05] });
add('fm-bass', 'FM Bas', 'Bas', 'b', 'house mtechno dnb', { fm: [1, 2.5, .15, .25], a: [.002, .2, .3, .08], f: ['lowpass', 1800, .7, 0, 0, .2] });

// ---------- PAD
add('warm-pad', 'Sıcak Analog Pad', 'Pad', 'c', 'house deep mtechno lofi', { u: [3, 12], o: [['w', 0, 0, .5], ['t', 0, 0, .5]], f: ['lowpass', 1400, .7, .6, .8, 1.5], a: [.6, .8, .85, 1.4], ch: .5, rv: .35 });
add('dub-pad', 'Dub Pad', 'Pad', 'c', 'deep techno house', { o: [['t', 0, 0, .8], ['w', -6, 0, .3]], f: ['lowpass', 800, 1.5, 1.2, .5, 1], a: [.4, .6, .8, 1], dl: .35, rv: .3, ch: .3 });
add('glass-pad', 'Cam Pad', 'Pad', 'c', 'trance mtechno trap', { o: [['s', 0, 0, .6], ['s', 3, 1, .4], ['t', -4, 2, .15]], f: ['lowpass', 5000, .7, 0, 0, .3], a: [.7, 1, .8, 1.6], rv: .5, ch: .4 });
add('dark-drone', 'Karanlık Drone', 'Pad', 'c', 'techno mtechno dnb', { u: [2, 8], o: [['w', 0, -1, .6], ['w', 5, 0, .4]], f: ['lowpass', 500, 2, .3, 1.5, 2], lf: [.12, .25], a: [1.2, 1, .9, 1.8], rv: .4 });
add('string-machine', 'String Machine', 'Pad', 'c', 'house deep lofi trance', { u: [4, 16], o: [['w', 0, 0, .6]], f: ['lowpass', 2400, .5, 0, 0, .3], vib: [5.2, 6, .4], a: [.35, .5, .9, .9], ch: .6, rv: .25 });
add('choir-pad', 'Koro Pad', 'Pad', 'c', 'trance mtechno deep afro', { u: [3, 10], o: [['w', 0, 0, .6]], vow: 'o', vib: [5, 8, .5], a: [.8, .6, .85, 1.4], rv: .5, ch: .3, vol: .5 });
add('super-pad', 'Lush Süper Pad', 'Pad', 'c', 'trance mtechno house', { u: [5, 22], o: [['w', 0, 0, .5]], f: ['lowpass', 3000, .7, .5, 1, 2], a: [.9, 1, .85, 1.8], ch: .7, rv: .45 });
add('evolving-pad', 'Evrilen Pad', 'Pad', 'c', 'mtechno techno trance', { u: [3, 10], o: [['w', 0, 0, .5], ['q', 2, 1, .25]], f: ['lowpass', 1000, 3, 1.5, 2, 3], lf: [.2, .5], a: [1.5, 1, .85, 2], rv: .5, ch: .4 });
add('tape-pad', 'Teyp Pad', 'Pad', 'c', 'lofi deep house', { o: [['t', 0, 0, .7], ['w', 4, 0, .3]], f: ['lowpass', 1100, .7, .3, 0, .5], vib: [.6, 14, 0], a: [.5, .6, .85, 1.2], dr: .2, rv: .3 });
add('sunrise-pad', 'Gün Doğumu Pad', 'Pad', 'c', 'house trance mtechno afro', { u: [3, 14], o: [['w', 0, 0, .45], ['w', 0, 1, .2]], f: ['lowpass', 2000, .7, 1.2, .9, 1.2], a: [.7, .6, .9, 1.5], ch: .5, rv: .4 });
add('sad-pad', 'Hüzünlü Pad', 'Pad', 'c', 'lofi dnb deep mtechno', { o: [['t', 0, 0, .6], ['s', 0, 1, .3]], f: ['lowpass', 900, .7, .5, .6, 1.4], vib: [4.5, 8, .3], a: [.8, .8, .8, 1.6], rv: .5 });
add('hollow-pad', 'Boşluk Pad', 'Pad', 'c', 'afro deep techno', { o: [['q', 0, 0, .35], ['q', 9, 0, .3]], f: ['lowpass', 1100, 1, 1, .7, 1.2], a: [.6, .8, .8, 1.4], rv: .4, ch: .3 });

// ---------- STAB
add('organ-stab', 'House Org Stab', 'Stab', 'c', 'house deep trance', { o: [['q', 0, 0, .4], ['s', 0, 1, .4], ['s', 0, 2, .2], ['s', 0, -1, .3]], f: ['lowpass', 4500, .7, 0, 0, .2], vib: [6, 5, 0], a: [.004, .15, .5, .08], rv: .2, dl: .2 });
add('italo-stab', 'Italo Piyano Stab', 'Stab', 'c', 'house deep', { o: [['t', 0, 0, .8], ['s', 0, 1, .3]], f: ['lowpass', 3500, .7, 2, 0, .12], a: [.002, .35, .15, .2], rv: .25 });
add('rave-stab', 'Rave Stab', 'Stab', 'c', 'techno house trance', { u: [3, 25], o: [['w', 0, 0, .6], ['q', 5, 0, .4]], f: ['lowpass', 1800, 2, 4, 0, .12], a: [.003, .15, .35, .08], dl: .3, rv: .2 });
add('deep-stab', 'Deep Akor Stab', 'Stab', 'c', 'deep house afro', { o: [['w', 0, 0, .5], ['t', 0, 0, .5]], f: ['lowpass', 900, 3, 3, 0, .18], a: [.003, .22, .25, .12], dl: .4, rv: .35, ch: .3 });
add('super-stab', 'Detune Süper Stab', 'Stab', 'c', 'trance house mtechno', { u: [5, 28], o: [['w', 0, 0, .5]], f: ['lowpass', 2200, 1, 2.5, 0, .15], a: [.003, .2, .3, .1], ch: .5, rv: .25 });
add('vapor-stab', 'Vapor Stab', 'Stab', 'c', 'mtechno trap deep lofi', { u: [3, 18], o: [['w', 0, 0, .5], ['t', 0, 1, .3]], f: ['lowpass', 2600, .7, 1.5, 0, .25], a: [.004, .3, .3, .2], ch: .8, rv: .35, dl: .2 });
add('brass-stab', 'Brass Stab', 'Stab', 'c', 'house trance afro', { o: [['w', -4, 0, .5], ['w', 4, 0, .5]], f: ['lowpass', 1000, 1, 3, .03, .2], a: [.02, .2, .6, .1], rv: .2 });
add('dub-chord', 'Dub Akor', 'Stab', 'c', 'deep techno dnb', { o: [['q', 0, 0, .4], ['t', 0, 0, .4]], f: ['lowpass', 700, 4, 2, 0, .22], a: [.003, .25, .1, .2], dl: .55, rv: .3 });
