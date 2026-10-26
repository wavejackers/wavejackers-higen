// ---- Stil tabanlı MIDI üretici
const HGK = ['chords', 'bass', 'drums', 'melody'];
HG.generate = function (P) {
  const S = HG.STYLE[P.style], vi = P.vi || 0, V = S.var[vi] || S.var[0];
  const scale = P.scale || V.scale || S.scales[0], iv = MB.SCALES[scale].iv, bars = P.bars;
  const orig = (P.orig == null ? 30 : P.orig) / 100, energy = (P.energy == null ? 60 : P.energy) / 100;
  const hv = (P.human || 0) / 100, key = P.key;
  const deg = i => iv[((i % 7) + 7) % 7] + 12 * Math.floor(i / 7);
  const rp = MB.rng(P.seed * 7919 + 11 + vi * 13);

  // ---- akor ilerlemesi
  const prog = (V.prog || [0,5,2,6]).slice();
  for (let i = 1; i < 4; i++) if (rp() < orig * .45) prog[i] = pick(rp, [(prog[i] + 2) % 7, (prog[i] + 5) % 7, (prog[i] + 3) % 7, (prog[i] + 4) % 7]);
  const turn = prog.slice(); turn[3] = pick(rp, [4, 6, 3, prog[3]]);
  const degAt = b => { const g = Math.floor(b / 4); return (g % 4 === 3 ? turn : prog)[b % 4]; };
  const info = d => {
    const root = deg(d), third = mod12(deg(d + 2) - root), fifth = mod12(deg(d + 4) - root), sev = mod12(deg(d + 6) - root);
    let q = 'maj';
    if (third === 3 && fifth === 7) q = 'min'; else if (third === 3 && fifth === 6) q = 'dim'; else if (third === 4 && fifth === 8) q = 'aug';
    let suf = q === 'min' ? 'm' : q === 'dim' ? 'dim' : q === 'aug' ? 'aug' : '', rsuf = q === 'dim' ? '°' : q === 'aug' ? '+' : '';
    if (P.sevenths) {
      if (q === 'maj' && sev === 11) { suf = 'maj7'; rsuf = 'maj7'; } else if (q === 'maj' && sev === 10) { suf = '7'; rsuf = '7'; }
      else if (q === 'min' && sev === 10) { suf = 'm7'; rsuf = '7'; } else if (q === 'dim' && sev === 10) { suf = 'm7b5'; rsuf = 'ø7'; }
    }
    const acc = root - MAJ[d % 7]; let rn = ROMAN[d % 7];
    if (q === 'min' || q === 'dim') rn = rn.toLowerCase();
    return { name: MB.NOTES[mod12(key + root)] + suf, roman: (acc < 0 ? 'b' : acc > 0 ? '#' : '') + rn + rsuf, pc: mod12(key + root), fifth, third };
  };
  const rots = n => { const out = []; let c = n.slice().sort((a, b) => a - b); for (let k = 0; k < c.length; k++) { out.push(c.slice()); c = c.slice(1).concat([c[0] + 12]); } return out; };
  const avg = a => a.reduce((s, x) => s + x, 0) / a.length;
  const lead = (prev, notes) => {
    let best = null, bc = 1e9;
    for (const r of rots(notes)) for (const sh of [-24, -12, 0, 12]) {
      const c = r.map(x => x + sh), av = avg(c); let cost;
      if (prev) { const m = Math.min(c.length, prev.length); cost = 0; for (let i = 0; i < m; i++) cost += Math.abs(c[i] - prev[i]); cost += Math.abs(av - avg(prev)) * 2; } else cost = Math.abs(av - 61);
      if (av < 54 || av > 69) cost += 50; if (c[0] < 45) cost += 50;
      if (cost < bc) { bc = cost; best = c; }
    }
    return best;
  };
  const chords = []; let prev = null;
  for (let b = 0; b < bars; b++) {
    const d = degAt(b), idx = [d, d + 2, d + 4]; if (P.sevenths) idx.push(d + 6);
    const midi = lead(prev, idx.map(i => 48 + key + deg(i))); prev = midi;
    chords.push(Object.assign({ d, midi }, info(d)));
  }

  const T = { chords: [], bass: [], drums: [], melody: [] };
  const rh = MB.rng(P.seed * 31 + 17);
  const add = (tr, s, l, n, v) => T[tr].push({ s, l, n: clamp(Math.round(n), 0, 127), v: clamp(Math.round(v + (rh() - .5) * hv * 30), 1, 127) });
  const eV = .8 + .2 * energy;

  // ---- akor izi
  const rc = MB.rng(P.seed * 1297 + 3 + vi);
  const cmode = V.cm || S.chord.mode;
  const cpKeys = V.cp ? [V.cp] : S.chord.pats, cpats = cpKeys.map(k => HG.CP[k]);
  const arpSeq = HG.ARP[S.chord.arp || 'trance'];
  for (let b = 0; b < bars; b++) {
    const base = b * 16, ch = chords[b].midi;
    if (cmode === 'arp') {
      const ext = ch.concat(ch.map(n => n + 12));
      for (let s = 0; s < 16; s++) {
        const ix = arpSeq[s % arpSeq.length]; if (ix < 0) continue;
        if (rc() < (1 - energy) * .12 + orig * .05) continue;
        add('chords', base + s, 1, ext[ix % ext.length], (s % 4 === 0 ? 96 : 76) * eV);
      }
    } else {
      let pat = cpats[(b % 4 === 3 && cpats.length > 1) ? 1 : 0];
      if (cmode === 'sustain' && !V.cp) pat = HG.CP[S.chord.pats[0] === 'whole' || S.chord.pats[0] === 'long' ? S.chord.pats[0] : 'whole'];
      pat.forEach(([s, l, v]) => {
        if (orig > .2 && rc() < orig * .12 && s > 0) return;
        ch.forEach(n => add('chords', base + s, l, n, v * eV));
      });
    }
  }

  // ---- bas izi
  const rb = MB.rng(P.seed * 3571 + 5 + vi);
  const bpat = HG.BP[V.bp || S.bass.pats[0]];
  for (let b = 0; b < bars; b++) {
    const base = b * 16, c = chords[b]; let root = (S.bass.oct || 36) + c.pc; if (c.pc > 6) root -= 12;
    const note = code => code === 1 ? root : code === 2 ? root + c.fifth : code === 3 ? root + 12 : code === 4 ? root + c.third : root + 10;
    bpat.forEach(([s, l, code, v]) => {
      if (energy < .4 && v < 90 && s % 2 === 1 && rb() < .5) return;
      if (rb() < orig * .1) return;
      let cd = code; if (rb() < orig * .12 && cd === 1) cd = 3;
      add('bass', base + s, l, note(cd), v);
    });
  }

  // ---- davul izi
  const rd = MB.rng(P.seed * 6151 + 9 + vi);
  const D = Object.assign({}, S.drums, V.dr || {}), G = MB.GM;
  const lanes = [['kick', G.kick, 118], ['clap', G.clap, 100], ['snare', G.snare, 106], ['chat', G.chat, 80], ['ohat', G.ohat, 90], ['rim', G.rim, 70], ['perc', G.perc, 80]];
  for (let b = 0; b < bars; b++) {
    const base = b * 16;
    lanes.forEach(([lane, note, bv]) => {
      const pat = D[lane]; if (!pat) return;
      for (let s = 0; s < 16; s++) {
        const c = pat[s]; if (c !== 'x' && c !== 'X') continue;
        if (lane === 'chat' && s % 2 === 1 && rd() > .55 + energy * .45) continue;
        if ((lane === 'perc' || lane === 'rim' || lane === 'chat') && rd() < orig * .07) continue;
        let v = bv + (c === 'X' ? 8 : 0);
        if (lane === 'chat') v = s % 4 === 0 ? 92 : s % 2 === 0 ? 78 : 58;
        add('drums', base + s, .9, note, v + Math.floor(rd() * 10) - 5);
      }
    });
    if (orig > .1) for (let s = 1; s < 16; s += 2) if (rd() < orig * .06 && !T.drums.some(x => x.s === base + s)) add('drums', base + s, .9, rd() < .5 ? G.rim : G.perc, 48 + Math.floor(rd() * 20));
    if (D.fill !== 'none' && b % 4 === 3 && b > 0 && bars > 4) {
      const note = D.fill === 'snare' ? G.snare : G.clap;
      [12, 13, 14, 15].forEach((s, i) => { if (rd() < .8 && !T.drums.some(x => x.s === base + s && x.n === note)) add('drums', base + s, .9, note, 70 + i * 12); });
    }
  }

  // ---- melodi izi
  const rm = MB.rng(P.seed * 104729 + 7 + vi * 5);
  const mmode = V.mm || S.mel.mode, dens = clamp(S.mel.dens * (V.md == null ? 1 : V.md), .08, 1.4);
  const mbase = (S.mel.oct || 72) - 12 + key;
  const cTones = c => [c.d % 7, (c.d + 2) % 7, (c.d + 4) % 7];
  const snapDeg = (m, tones) => { let best = m, bd = 99; for (let k = m - 3; k <= m + 3; k++) { if (!tones.includes(((k % 7) + 7) % 7)) continue; const d = Math.abs(k - m) + rm() * .1; if (d < bd) { bd = d; best = k; } } return best; };
  const addM = (b, s, l, m, v) => { let x = m; while (x > 96) x -= 12; while (x < 36) x += 12; add('melody', b * 16 + s, Math.max(1, l), x, v); };
  if (mmode === 'hook') {
    const hrk = V.hr || S.mel.hr, tpls = hrk.map(k => HG.HR[k]), rep = S.mel.rep == null ? .4 : S.mel.rep, leap = .12 + orig * .3;
    let cur = 3 + Math.floor(rm() * 3);
    for (let b = 0; b < bars; b++) {
      const c = chords[b], tp = tpls[(b >> 1) % tpls.length], odd = b % 2 === 1;
      let T2 = odd ? tp.slice(0, Math.max(2, tp.length - 1)) : tp;
      const keep = clamp(.5 + dens, .55, 1);
      T2.forEach(([s, l], j) => {
        if (j > 0 && rm() > keep) return;
        if (j === 0) cur = snapDeg(cur, cTones(c));
        else if (rm() < rep) { /* aynı nota */ }
        else { const r = rm(), mag = r < .6 ? 1 : r < .85 ? 2 : (rm() < leap * 4 ? 4 : 3); let up = rm() < .5; if (cur > 8) up = rm() < .3; else if (cur < 2) up = rm() < .75; cur += up ? mag : -mag; }
        cur = clamp(cur, 0, 10);
        const last = j === T2.length - 1;
        if (last && odd) cur = snapDeg(cur, [c.d % 7, (c.d + 4) % 7]);
        const nx = j + 1 < T2.length ? T2[j + 1][0] : 16;
        addM(b, s, Math.min(l, nx - s), mbase + deg(cur), s % 4 === 0 ? 100 : 84);
      });
    }
  } else if (mmode === 'arp') {
    const seq = HG.ARP[V.arp || S.mel.arp || 'trance'];
    for (let b = 0; b < bars; b++) {
      const c = chords[b]; let ext = [];
      c.midi.forEach(n => { let x = n; while (x < (S.mel.oct || 72) - 4) x += 12; while (x > (S.mel.oct || 72) + 8) x -= 12; ext.push(x); });
      ext = ext.concat(ext.map(n => n + 12)).sort((a, z) => a - z);
      const lift = (b % 4 === 3 && orig > .3) ? 12 : 0, shift = (b % 8 >= 4) ? 1 : 0;
      for (let s = 0; s < 16; s++) {
        const ix = seq[(s + shift) % seq.length]; if (ix < 0) continue;
        if (rm() < (1 - clamp(dens, 0, 1)) * .55) continue;
        const m = ext[ix % ext.length] + (s >= 12 ? lift : 0);
        addM(b, s, 1, m, s % 4 === 0 ? 98 : 78);
      }
    }
  } else if (mmode === 'riff') {
    const cand = [0, 2, 3, 4, 6, 7, 8, 10, 11, 12, 14, 15], offs = [0, 0, 0, 7, 10, 12, 3, 5];
    const cnt = Math.round(3 + dens * 8);
    let riff = [], pool = cand.slice();
    while (riff.length < cnt && pool.length) { const i = Math.floor(rm() * pool.length); riff.push([pool.splice(i, 1)[0], pick(rm, offs)]); }
    riff.sort((a, z) => a[0] - z[0]);
    for (let b = 0; b < bars; b++) {
      const c = chords[b];
      if (b % 4 === 3 && riff.length > 2 && rm() < .4 + orig * .5) { const i = Math.floor(rm() * riff.length); riff[i] = [riff[i][0], pick(rm, offs)]; }
      const rootM = (S.mel.oct || 60) + ((c.pc - ((S.mel.oct || 60) % 12) + 12) % 12);
      riff.forEach(([s, o], j) => { const nx = j + 1 < riff.length ? riff[j + 1][0] : 16; addM(b, s, Math.min(nx - s, rm() < .3 ? 2 : 1), rootM + o, s % 4 === 0 ? 102 : 84); });
    }
  } else { // chop
    const cand = [0, 2, 3, 5, 6, 7, 10, 11, 14], cnt = Math.round(3 + dens * 6);
    const mk = () => { const p = cand.slice(), out = []; while (out.length < cnt && p.length) out.push(p.splice(Math.floor(rm() * p.length), 1)[0]); return out.sort((a, z) => a - z); };
    let A = mk(), B = A.slice(); B[Math.floor(rm() * B.length)] = pick(rm, cand); B = Array.from(new Set(B)).sort((a, z) => a - z);
    for (let b = 0; b < bars; b++) {
      const c = chords[b], hits = (b % 2 === 0) ? A : B, tones = [0, 4, 7, 12, 16, 19].map(o => o);
      const rootM = (S.mel.oct || 72) + ((c.pc - ((S.mel.oct || 72) % 12) + 12) % 12) - 12;
      let last = 0;
      hits.forEach((s, j) => { const off = rm() < .45 ? last : pick(rm, [0, c.third, c.fifth, 12, 12 + c.third, 12 + c.fifth]); last = off; const nx = j + 1 < hits.length ? hits[j + 1] : 16; addM(b, s, Math.min(nx - s, rm() < .5 ? 1 : 2), rootM + off, s % 4 === 0 ? 100 : 82); });
    }
  }
  // ---- kalınlık: akor kökünü bir oktav alta, yüksek değerde melodiyi de alt oktavla katla
  const thick = P.thick == null ? 70 : P.thick;
  if (thick >= 40) {
    const groups = {}; T.chords.forEach(n => { (groups[n.s] = groups[n.s] || []).push(n); });
    if (cmode === 'arp') for (let b = 0; b < bars; b++) { const c = chords[b], low = Math.min.apply(null, c.midi); let r = low - mod12(low - c.pc); if (r === low) r -= 12; if (r >= 38) T.chords.push({ s: b * 16, l: 16, n: r, v: Math.round(70 + thick * .2) }); }
    else Object.keys(groups).forEach(s => { const g = groups[s], b = Math.floor(s / 16), c = chords[b], low = Math.min.apply(null, g.map(n => n.n)); let r = low - mod12(low - c.pc); if (r === low) r -= 12; if (r >= 38) T.chords.push({ s: +s, l: g[0].l, n: r, v: clamp(Math.round(Math.max.apply(null, g.map(n => n.v)) * .85), 1, 127) }); });
    T.chords.sort((x, y) => x.s - y.s || x.n - y.n);
  }
  if (thick >= 60) {
    const dbl = []; T.melody.forEach(n => { if (n.n - 12 >= 48) dbl.push({ s: n.s, l: n.l, n: n.n - 12, v: clamp(Math.round(n.v * .72), 1, 127) }); });
    T.melody = T.melody.concat(dbl).sort((x, y) => x.s - y.s || x.n - y.n);
  }
  return { P: Object.assign({}, P, { scale }), chords, tracks: T, steps: bars * 16, bars, style: S, variation: V };
};

// ---- Loop'tan tam parça aranjmanı
HG.maskFor = function (name, bi, B) {
  const n = name.toLowerCase();
  if (/intro/.test(n)) return { drums: 'light', bass: bi >= B / 2, chords: bi >= B * .75, melody: false };
  if (/outro/.test(n)) return { drums: 'light', bass: bi < B / 2, chords: bi < B / 4, melody: false };
  if (/build/.test(n)) return { drums: 'build', bass: bi < B - 2, chords: true, melody: false, roll: bi >= B - 4 ? bi - (B - 4) : -1 };
  if (/break/.test(n)) return { drums: 'none', bass: false, chords: true, melody: true };
  if (/verse/.test(n)) return { drums: 'full', bass: true, chords: true, melody: false };
  return { drums: 'full', bass: true, chords: true, melody: true };
};
HG.assemble = function (res) {
  const S = res.style, L = res.bars, G = MB.GM;
  const perBar = {}; HGK.forEach(k => { perBar[k] = Array.from({ length: L }, () => []); res.tracks[k].forEach(n => { const b = Math.floor(n.s / 16); if (b < L) perBar[k][b].push(n); }); });
  const T = { chords: [], bass: [], drums: [], melody: [] }, chords = [], sections = [];
  const rollNote = S.drums.fill === 'snare' ? G.snare : G.clap;
  let off = 0;
  S.arr.forEach(([name, B]) => {
    sections.push({ name, bar: off, bars: B });
    for (let bi = 0; bi < B; bi++) {
      const src = bi % L, m = HG.maskFor(name, bi, B), shift = (off + bi - src) * 16;
      chords.push(res.chords[src]);
      ['chords', 'bass', 'melody'].forEach(k => { if (m[k]) perBar[k][src].forEach(n => T[k].push(Object.assign({}, n, { s: n.s + shift }))); });
      if (m.drums !== 'none') perBar.drums[src].forEach(n => {
        if (m.drums === 'light' && n.n !== G.kick && n.n !== G.chat) return;
        if (m.drums === 'build' && n.n !== G.kick && n.n !== G.chat && n.n !== G.ohat) return;
        T.drums.push(Object.assign({}, n, { s: n.s + shift }));
      });
      if (m.roll != null && m.roll >= 0) {
        const steps = m.roll === 0 ? [0, 4, 8, 12] : m.roll === 1 ? [0, 2, 4, 6, 8, 10, 12, 14] : [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15];
        steps.forEach(s => T.drums.push({ s: (off + bi) * 16 + s, l: .9, n: rollNote, v: clamp(Math.round(58 + ((m.roll * 16 + s) / 64) * 66), 1, 127) }));
      }
    }
    off += B;
  });
  return { P: res.P, chords, tracks: T, steps: off * 16, bars: off, style: S, variation: res.variation, sections, loopBars: L };
};
