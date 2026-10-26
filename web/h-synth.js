// ---- Wavetable'lar (PeriodicWave), imza sesler, yama normalizasyonu
HG.WT = [
  { n: 'Testere', f: n => 1 / n },
  { n: 'Kare', f: n => n % 2 ? 1 / n : 0 },
  { n: 'Üçgen', f: n => n % 2 ? (n % 4 === 1 ? 1 : -1) / (n * n) : 0 },
  { n: 'Cam', f: n => [1, .0, .5, 0, .35, 0, 0, .3, 0, 0, 0, 0, 0, 0, 0, .12][n - 1] || 0 },
  { n: 'Org', f: n => [1, .8, .6, .0, .5, 0, 0, .35, 0, 0, 0, 0, 0, 0, 0, .15][n - 1] || 0 },
  { n: 'Çan', f: n => [.8, 0, .1, 0, .2, 0, .5, 0, 0, .35, 0, 0, .2, 0, 0, .25, 0, .15][n - 1] || 0 },
  { n: 'Vokal A', f: n => { const fr = n * 130; return (1 / n) * (1 + 3 * Math.exp(-Math.pow((fr - 850) / 170, 2)) + 2.2 * Math.exp(-Math.pow((fr - 1220) / 200, 2)) + 1.2 * Math.exp(-Math.pow((fr - 2810) / 300, 2))); } },
  { n: 'Vokal O', f: n => { const fr = n * 130; return (1 / n) * (1 + 3 * Math.exp(-Math.pow((fr - 500) / 120, 2)) + 1.6 * Math.exp(-Math.pow((fr - 900) / 160, 2))); } },
  { n: 'Dijital', f: n => { const x = Math.sin(n * 12.9898) * 43758.5453; return (x - Math.floor(x)) * (1.4 / Math.pow(n, .8)); } },
  { n: 'Dar Pals', f: n => Math.sin(Math.PI * n * .18) / n },
  { n: 'Boş (Hollow)', f: n => n % 2 === 0 ? 1 / n : .15 / n },
  { n: 'Ses Duvarı', f: n => 1 / Math.sqrt(n) * (n < 24 ? 1 : .2) }
];
HG._wc = new WeakMap();
// p.wt = [tabloA, tabloB, geçiş 0..1, parlaklık 0..1]
HG.wave = function (ac, wt) {
  wt = wt || [0, 0, 0, 1];
  const A = wt[0] | 0, B = wt[1] | 0, m = Math.round((wt[2] || 0) * 16) / 16, br = Math.round((wt[3] == null ? 1 : wt[3]) * 16) / 16;
  const key = A + '_' + B + '_' + m + '_' + br;
  let c = HG._wc.get(ac); if (!c) { c = {}; HG._wc.set(ac, c); }
  if (c[key]) return c[key];
  const N = 48, re = new Float32Array(N + 1), im = new Float32Array(N + 1), cut = 2 + br * (N - 2);
  for (let n = 1; n <= N; n++) {
    const a = HG.WT[A].f(n) * (1 - m) + HG.WT[B].f(n) * m, roll = n <= cut ? 1 : Math.exp(-(n - cut) / 3);
    im[n] = a * roll;
  }
  return (c[key] = ac.createPeriodicWave(re, im));
};

// ---- İmza sesler (stil motoru bunlara bağlı)
(function () {
  const add = MB.add;
  add('wj-dark-pad', 'WJ Karanlık Pad', 'Pad', 'cm', 'mtechno techno trance', { u: [4, 14], o: [['x', 0, 0, .5], ['w', 0, -1, .25]], wt: [10, 6, .3, .5], f: ['lowpass', 900, 1.5, 1.2, 1.2, 2], a: [.5, .6, .8, 1.4], l: [[.15, .35, 'cut', 'sine']], rv: .55, ch: .45, vol: .22 });
  add('wj-hypno-bass', 'WJ Hipnotik Bas', 'Bas', 'b', 'mtechno techno', { o: [['w', 0, 0, .8], ['q', -6, 0, .35], ['s', 0, -1, .8]], f: ['lowpass', 300, 5, 3.5, 0, .14, 1], a: [.002, .1, .55, .06], fd: .3, dr: .25, vol: .2 });
  add('wj-glass-arp', 'WJ Cam Arp', 'Pluck', 'cm', 'mtechno trance', { u: [2, 8], o: [['x', 0, 0, .6], ['x', 7, 1, .3]], wt: [3, 5, .5, .8], f: ['lowpass', 3200, 2, 2.5, 0, .18], a: [.002, .26, 0, .2], dl: .5, rv: .45, vol: .2 });
  add('wj-sunrise-lead', 'WJ Gündoğumu Lead', 'Lead', 'm', 'mtechno trance', { u: [4, 16], o: [['x', 0, 0, .5], ['w', 0, 0, .3]], wt: [0, 3, .4, .9], f: ['lowpass', 4200, 1, .8, 0, .3], a: [.01, .2, .8, .35], vib: [5.4, 9, .3], dl: .45, rv: .5, ch: .4, vol: .2 });
  add('gt-piano-stab', 'GT Piyano Stab', 'Stab', 'c', 'house trance', { o: [['x', 0, 0, .6], ['t', 5, 0, .4]], wt: [4, 0, .25, .7], f: ['lowpass', 3400, .8, 2, 0, .22], a: [.003, .38, .05, .22], ch: .3, rv: .35, vol: .22 });
  add('gt-pump-bass', 'GT Pump Bas', 'Bas', 'b', 'house trance', { o: [['w', 0, 0, .8], ['s', 0, -1, .8]], f: ['lowpass', 420, 2.5, 3, 0, .15], a: [.003, .12, .8, .08], dr: .25, vol: .2 });
  add('gt-anthem-lead', 'GT Anthem Lead', 'Lead', 'm', 'house trance', { u: [5, 22], o: [['w', 0, 0, .5], ['x', 0, 0, .3]], wt: [0, 1, .3, 1], f: ['lowpass', 3800, 1, 1.2, 0, .3, 1], a: [.01, .2, .85, .25], vib: [5.5, 7, .3], ch: .45, dl: .3, rv: .35, vol: .2 });
  add('ar-uplift-lead', 'AVB Uplift Lead', 'Lead', 'm', 'trance', { u: [6, 28], o: [['w', 0, 0, .5], ['q', 0, 0, .25]], f: ['lowpass', 4800, 1, .5, 0, .3], a: [.008, .15, .9, .3], vib: [5.8, 11, .25], l: [[.2, .1, 'cut', 'sine']], ch: .5, dl: .5, rv: .5, vol: .2 });
  add('ar-rolling-bass', 'AVB Rolling Bas', 'Bas', 'b', 'trance mtechno', { o: [['w', 0, 0, .9], ['s', 0, -1, .5]], f: ['lowpass', 240, 3, 3, 0, .1, 1], a: [.002, .08, .7, .05], dr: .2, vol: .2 });
  add('dd-funk-stab', 'DD Funk Stab', 'Stab', 'c', 'house deep', { o: [['x', 0, 0, .6], ['q', -5, 0, .3]], wt: [4, 1, .4, .6], f: ['lowpass', 2600, 2, 3, 0, .1], a: [.002, .18, 0, .08], ch: .35, dl: .25, rv: .2, vol: .2 });
  add('dd-bounce-bass', 'DD Bounce Bas', 'Bas', 'b', 'house deep', { o: [['q', 0, 0, .6], ['s', 0, -1, .9]], f: ['lowpass', 520, 3, 3.5, 0, .1], a: [.002, .13, .35, .06], dr: .3, vol: .2 });
  add('mp-hook-bass', 'MP Hook Bas', 'Bas', 'b', 'house techno', { o: [['w', 0, 0, .8], ['q', -8, 0, .4], ['s', 0, -1, .8]], f: ['lowpass', 700, 4, 4, 0, .12], a: [.002, .12, .4, .06], fd: .5, dr: .35, vol: .2 });
  add('dv-screech-lead', 'DV Screech Lead', 'Lead', 'm', 'house trap', { u: [3, 26], o: [['q', 0, 0, .5], ['w', 12, 0, .4]], f: ['lowpass', 3600, 6, 2.5, 0, .2, 1], fd: .6, a: [.004, .14, .75, .12], l: [[5.5, .04, 'pit', 'sine']], dl: .3, rv: .25, dr: .5, vol: .17 });
  add('cw-acid-riff', 'CW Acid Riff', 'Bas', 'bm', 'techno mtechno', { o: [['w', 0, 0, .9]], f: ['lowpass', 500, 9, 4.5, 0, .14, 1], a: [.002, .12, .3, .06], fd: .4, dr: .45, dl: .3, vol: .17 });
  add('cc-909-stab', 'CC 909 Stab', 'Stab', 'c', 'techno house', { u: [3, 12], o: [['w', 0, 0, .5], ['q', 0, 0, .35]], f: ['lowpass', 1800, 3, 3, 0, .14], a: [.002, .22, 0, .1], ch: .3, rv: .25, dl: .3, vol: .22 });
  MB.SOUNDS.forEach(s => { MB.byId[s.id] = s; });
})();

// Eksik alanları doldur (ses tasarımı sekmesi için)
HG.normalizePatch = function (p) {
  const q = JSON.parse(JSON.stringify(p));
  q.o = (q.o && q.o.length ? q.o : [['w', 0, 0, .6]]).slice(0, 3);
  q.u = q.u || [1, 0]; q.f = (q.f || ['lowpass', 4000, 1, 0, 0, .2]).slice(); while (q.f.length < 7) q.f.push(0);
  q.a = q.a || [.01, .2, .7, .3]; q.wt = q.wt || [0, 3, .3, 1];
  q.l = q.l || []; while (q.l.length < 2) q.l.push([1, 0, 'cut', 'sine']);
  q.rv = q.rv || 0; q.dl = q.dl || 0; q.ch = q.ch || 0; q.dr = q.dr || 0; q.fd = q.fd || 0; q.vol = q.vol || .2;
  return q;
};
