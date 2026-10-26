// ---- Arayüz 2/3: ses tasarımı (knob'lar, wavetable, filtre, LFO, klavye)
(function () {
  const { $, st, snd, patches, rt, css, toast } = UI; const T = (k, v) => HG.t(k, v);
  const TRK = ['chords', 'bass', 'melody'];
  let track = 'chords', knobs = [], dl = null;
  const clamp = (x, a, b) => Math.max(a, Math.min(b, x));
  const WAVES = () => [['w', T('sSaw')], ['q', T('sSq')], ['t', T('sTri')], ['s', T('sSine')], ['x', T('wavetableW')]];
  const P = () => patches[track];

  // ---------- Knob
  function knob(host, o) {
    const el = document.createElement('div'); el.className = 'kn';
    const id = 'kn' + Math.random().toString(36).slice(2, 8);
    el.innerHTML = '<div class="k" role="slider" tabindex="0"><svg viewBox="0 0 50 50"><path class="tr"/><path class="vl"/><line class="nd"/></svg></div><output></output><label></label>';
    const k = el.querySelector('.k'), tr = el.querySelector('.tr'), vl = el.querySelector('.vl'), nd = el.querySelector('.nd'), out = el.querySelector('output');
    el.querySelector('label').textContent = o.label; k.setAttribute('aria-label', o.label); el.querySelector('output').id = id; k.setAttribute('aria-describedby', id);
    const A0 = -135, A1 = 135, cx = 25, cy = 25, r = 19;
    const pt = a => [cx + r * Math.sin(a * Math.PI / 180), cy - r * Math.cos(a * Math.PI / 180)];
    const arc = (a, b) => { const [x1, y1] = pt(a), [x2, y2] = pt(b); return 'M' + x1 + ' ' + y1 + 'A' + r + ' ' + r + ' 0 ' + (b - a > 180 ? 1 : 0) + ' 1 ' + x2 + ' ' + y2; };
    tr.setAttribute('d', arc(A0, A1));
    const toN = v => o.log ? Math.log(v / o.min) / Math.log(o.max / o.min) : (v - o.min) / (o.max - o.min);
    const fromN = n => { let v = o.log ? o.min * Math.pow(o.max / o.min, n) : o.min + n * (o.max - o.min); if (o.step) v = Math.round(v / o.step) * o.step; return clamp(+v.toFixed(4), o.min, o.max); };
    function refresh() {
      const v = clamp(o.get(), o.min, o.max), n = clamp(toN(v), 0, 1), a = A0 + n * (A1 - A0), [x, y] = pt(a);
      vl.setAttribute('d', n < .004 ? '' : arc(A0, a)); nd.setAttribute('x1', cx + 6 * Math.sin(a * Math.PI / 180)); nd.setAttribute('y1', cy - 6 * Math.cos(a * Math.PI / 180)); nd.setAttribute('x2', x); nd.setAttribute('y2', y);
      const txt = o.fmt ? o.fmt(v) : String(+v.toFixed(2)); out.textContent = txt; k.setAttribute('aria-valuenow', v); k.setAttribute('aria-valuemin', o.min); k.setAttribute('aria-valuemax', o.max); k.setAttribute('aria-valuetext', txt);
    }
    function put(n) { o.set(fromN(clamp(n, 0, 1))); refresh(); if (o.fx) UI.rebuildSoon(); if (o.draw) o.draw(); }
    let sy = 0, sn = 0;
    k.addEventListener('pointerdown', e => { k.setPointerCapture(e.pointerId); sy = e.clientY; sn = toN(clamp(o.get(), o.min, o.max)); k.focus(); e.preventDefault(); });
    k.addEventListener('pointermove', e => { if (!k.hasPointerCapture(e.pointerId)) return; put(sn + (sy - e.clientY) / (e.shiftKey ? 600 : 160)); });
    k.addEventListener('wheel', e => { e.preventDefault(); put(toN(o.get()) + (e.deltaY < 0 ? 1 : -1) * .02); }, { passive: false });
    k.addEventListener('dblclick', () => { if (o.def != null) { o.set(o.def); refresh(); if (o.fx) UI.rebuildSoon(); if (o.draw) o.draw(); } });
    k.addEventListener('keydown', e => {
      const step = o.step ? o.step / (o.max - o.min) : .02; let n = toN(o.get());
      if (e.key === 'ArrowUp' || e.key === 'ArrowRight') n += step; else if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') n -= step; else if (e.key === 'Home') n = 0; else if (e.key === 'End') n = 1; else if (e.key === 'PageUp') n += .1; else if (e.key === 'PageDown') n -= .1; else return;
      e.preventDefault(); put(n);
    });
    host.appendChild(el); knobs.push(refresh); refresh(); return el;
  }
  const mod = (cls, title, extra) => { const m = document.createElement('section'); m.className = 'mod ' + cls; m.innerHTML = '<h3><span>' + title + '</span>' + (extra || '') + '</h3>'; $('synth').appendChild(m); return m; };
  const kbox = m => { const d = document.createElement('div'); d.className = 'knobs'; m.appendChild(d); return d; };
  const fHz = v => v >= 1000 ? (v / 1000).toFixed(1) + ' kHz' : Math.round(v) + ' Hz';
  const fS = v => v < 1 ? Math.round(v * 1000) + ' ms' : v.toFixed(2) + ' s';
  const pct = v => Math.round(v * 100) + '%';
  function sel(host, opts, get, set, label) {
    const s = document.createElement('select'); s.setAttribute('aria-label', label);
    opts.forEach(([v, t]) => s.add(new Option(t, v))); s.value = get();
    s.addEventListener('change', () => { set(s.value); redrawAll(); }); host.appendChild(s); knobs.push(() => { s.value = get(); }); return s;
  }
  function tgl(host, text, get, set, fx) {
    const b = document.createElement('button'); b.type = 'button'; b.className = 'btn sm'; b.textContent = text; b.setAttribute('aria-pressed', !!get());
    const r = () => { b.classList.toggle('on', !!get()); b.setAttribute('aria-pressed', !!get()); };
    b.addEventListener('click', () => { set(!get()); r(); redrawAll(); if (fx) UI.rebuildSoon(); }); host.appendChild(b); knobs.push(r); r(); return b;
  }

  // ---------- Canvas yardımcıları
  function cv(host, h) { const c = document.createElement('canvas'); c.className = 'cv'; c.style.height = h + 'px'; host.appendChild(c); return c; }
  function prep(c) { const w = c.clientWidth, h = c.clientHeight; if (!w) return null; const d = window.devicePixelRatio || 1; if (c.width !== Math.round(w * d)) { c.width = Math.round(w * d); c.height = Math.round(h * d); } const x = c.getContext('2d'); x.setTransform(d, 0, 0, d, 0, 0); x.clearRect(0, 0, w, h); return { x, w, h }; }
  let cvWT, cvFilt, cvEnv, cvScope, cvSpec;
  function drawWT() {
    const q = prep(cvWT); if (!q) return; const { x, w, h } = q, p = P(), wt = p.wt;
    const N = 48, amp = new Array(N + 1).fill(0), cut = 2 + (wt[3] == null ? 1 : wt[3]) * (N - 2);
    for (let n = 1; n <= N; n++) amp[n] = (HG.WT[wt[0] | 0].f(n) * (1 - wt[2]) + HG.WT[wt[1] | 0].f(n) * wt[2]) * (n <= cut ? 1 : Math.exp(-(n - cut) / 3));
    let mx = 0; const ys = []; for (let i = 0; i < w; i++) { let y = 0; const t = i / w * 2 * Math.PI; for (let n = 1; n <= N; n++) y += amp[n] * Math.sin(n * t); ys.push(y); mx = Math.max(mx, Math.abs(y)); }
    x.strokeStyle = css('--accent'); x.lineWidth = 2; x.beginPath(); ys.forEach((y, i) => { const yy = h / 2 - (y / (mx || 1)) * (h / 2 - 5); i ? x.lineTo(i, yy) : x.moveTo(i, yy); }); x.stroke();
    x.strokeStyle = css('--grid2'); x.lineWidth = 1; x.beginPath(); x.moveTo(0, h / 2); x.lineTo(w, h / 2); x.stroke();
  }
  function drawFilt() {
    const q = prep(cvFilt); if (!q) return; const { x, w, h } = q, F = P().f, type = F[0], fc = F[1], Q = Math.max(.3, F[2]), s24 = F[6] ? 2 : 1;
    const mag = f => { const r = f / fc; let m; if (type === 'lowpass') m = 1 / Math.sqrt(Math.pow(1 - r * r, 2) + Math.pow(r / Q, 2)); else if (type === 'highpass') m = (r * r) / Math.sqrt(Math.pow(1 - r * r, 2) + Math.pow(r / Q, 2)); else m = (r / Q) / Math.sqrt(Math.pow(1 - r * r, 2) + Math.pow(r / Q, 2)); return Math.pow(m, s24); };
    const X = f => Math.log(f / 20) / Math.log(1000) * w, Y = m => h - clamp((20 * Math.log10(Math.max(m, 1e-4)) + 36) / 60, 0, 1) * (h - 8) - 2;
    x.strokeStyle = css('--grid'); x.lineWidth = 1; [100, 1000, 10000].forEach(f => { x.beginPath(); x.moveTo(X(f), 0); x.lineTo(X(f), h); x.stroke(); });
    const draw = (fcc, alpha, fill) => { x.beginPath(); for (let i = 0; i <= w; i += 2) { const f = 20 * Math.pow(1000, i / w), r = f / fcc; let m; if (type === 'lowpass') m = 1 / Math.sqrt(Math.pow(1 - r * r, 2) + Math.pow(r / Q, 2)); else if (type === 'highpass') m = (r * r) / Math.sqrt(Math.pow(1 - r * r, 2) + Math.pow(r / Q, 2)); else m = (r / Q) / Math.sqrt(Math.pow(1 - r * r, 2) + Math.pow(r / Q, 2)); m = Math.pow(m, s24); const y = Y(m); i ? x.lineTo(i, y) : x.moveTo(i, y); } x.globalAlpha = alpha; x.stroke(); if (fill) { x.lineTo(w, h); x.lineTo(0, h); x.closePath(); x.fillStyle = css('--accent'); x.globalAlpha = .12; x.fill(); } x.globalAlpha = 1; };
    x.strokeStyle = css('--accent'); x.lineWidth = 2;
    if (F[3] > 0) { x.lineWidth = 1; draw(clamp(fc * (1 + F[3]), 60, 18000), .35, false); x.lineWidth = 2; }
    draw(fc, 1, true);
  }
  function drawEnv() {
    const q = prep(cvEnv); if (!q) return; const { x, w, h } = q, a = P().a, tot = a[0] + a[1] + .35 + a[3], sx = t => t / tot * (w - 8) + 4;
    const A = a[0], D = a[1], S = a[2], R = a[3], hold = .35;
    x.beginPath(); x.moveTo(sx(0), h - 4); x.lineTo(sx(A), 4);
    for (let i = 1; i <= 16; i++) { const t = i / 16 * D; x.lineTo(sx(A + t), 4 + (1 - (S + (1 - S) * Math.exp(-3 * t / D))) * 0 + (h - 8) * (1 - (S + (1 - S) * Math.exp(-3 * t / D)))); }
    x.lineTo(sx(A + D + hold), 4 + (h - 8) * (1 - S));
    for (let i = 1; i <= 16; i++) { const t = i / 16 * R; x.lineTo(sx(A + D + hold + t), 4 + (h - 8) * (1 - S * Math.exp(-3 * t / R))); }
    x.strokeStyle = css('--accent'); x.lineWidth = 2; x.stroke(); x.lineTo(sx(tot), h - 4); x.lineTo(sx(0), h - 4); x.fillStyle = css('--accent'); x.globalAlpha = .12; x.fill(); x.globalAlpha = 1;
  }
  const tb = new Uint8Array(2048), fb = new Uint8Array(1024);
  UI.onFrame = function () {
    if ($('p-sound').hidden || !cvScope) return;
    const an = rt.mx && rt.mx.an;
    let q = prep(cvScope);
    if (q) { const { x, w, h } = q; x.strokeStyle = css('--grid'); x.beginPath(); x.moveTo(0, h / 2); x.lineTo(w, h / 2); x.stroke(); if (an) { an.getByteTimeDomainData(tb); let z = 0; for (let i = 1; i < 1024; i++) if (tb[i - 1] < 128 && tb[i] >= 128) { z = i; break; } x.strokeStyle = css('--accent'); x.lineWidth = 1.6; x.beginPath(); for (let i = 0; i < 900; i++) { const px = i / 900 * w, py = h / 2 - ((tb[z + i] - 128) / 128) * (h / 2 - 3) * 1.6; i ? x.lineTo(px, py) : x.moveTo(px, py); } x.stroke(); } }
    q = prep(cvSpec);
    if (q) { const { x, w, h } = q; if (an) { an.getByteFrequencyData(fb); x.fillStyle = css('--accent'); const N = Math.floor(w / 4); for (let i = 0; i < N; i++) { const a = Math.floor(Math.pow(i / N, 2.2) * 480) + 1, b = Math.floor(Math.pow((i + 1) / N, 2.2) * 480) + 2; let m = 0; for (let j = a; j < b; j++) m = Math.max(m, fb[j]); const bh = Math.max(1, m / 255 * (h - 4)); x.globalAlpha = .3 + m / 255 * .7; x.fillRect(i * 4, h - bh, 3, bh); } x.globalAlpha = 1; } }
  };
  function redrawAll() { if (cvWT) { drawWT(); drawFilt(); drawEnv(); } }
  UI.onTheme = redrawAll;

  // ---------- Yama düzenleyici
  function build() {
    knobs = []; $('synth').innerHTML = ''; const p = P(); if (!p) return;
    const def = JSON.parse(JSON.stringify(p));
    // Osilatörler
    let m = mod('w8', T('osc'), '<span class="meta"></span>'); m.querySelector('.meta').textContent = HG.lang === 'tr' ? p.name : (HG.EN[p.id] || p.name);
    while (p.o.length < 3) p.o.push(['s', 0, 0, 0]);
    p.o.forEach((o, i) => {
      const row = document.createElement('div'); row.className = 'oscrow'; m.appendChild(row);
      const c = document.createElement('div'); c.className = 'field'; row.appendChild(c);
      const l = document.createElement('span'); l.className = 'lab'; l.textContent = T('oscN', { x: 'ABC'[i] }); c.appendChild(l);
      sel(c, WAVES(), () => o[0], v => { o[0] = v; }, T('oscN', { x: 'ABC'[i] }) + ' ' + T('wave'));
      const kb = kbox(row); kb.style.gridColumn = 'span 3'; kb.style.gridTemplateColumns = 'repeat(3,1fr)';
      knob(kb, { label: T('level'), min: 0, max: 1, def: def.o[i] ? def.o[i][3] : 0, get: () => o[3], set: v => { o[3] = v; }, fmt: pct, draw: redrawAll });
      knob(kb, { label: T('octave'), min: -2, max: 2, step: 1, def: def.o[i] ? def.o[i][2] : 0, get: () => o[2], set: v => { o[2] = v; }, fmt: v => (v > 0 ? '+' : '') + v });
      knob(kb, { label: T('detune'), min: -50, max: 50, step: 1, def: def.o[i] ? def.o[i][1] : 0, get: () => o[1], set: v => { o[1] = v; }, fmt: v => v + ' ct' });
    });
    const kb = kbox(m);
    knob(kb, { label: T('unison'), min: 1, max: 7, step: 1, def: def.u[0], get: () => p.u[0], set: v => { p.u[0] = v; }, fmt: v => T('voices', { n: v }) });
    knob(kb, { label: T('spread'), min: 0, max: 60, step: 1, def: def.u[1], get: () => p.u[1], set: v => { p.u[1] = v; }, fmt: v => v + ' ct' });
    knob(kb, { label: T('sub'), min: 0, max: 1, def: def.sub || 0, get: () => p.sub || 0, set: v => { p.sub = v || undefined; }, fmt: pct });
    knob(kb, { label: T('noise'), min: 0, max: .3, def: def.noi || 0, get: () => p.noi || 0, set: v => { p.noi = v || undefined; }, fmt: pct });
    knob(kb, { label: T('pitchEnv'), min: 0, max: 24, step: .5, def: def.pe ? def.pe[0] : 0, get: () => p.pe ? p.pe[0] : 0, set: v => { p.pe = v ? [v, p.pe ? p.pe[1] : .08] : undefined; }, fmt: v => v + ' st' });
    knob(kb, { label: T('vibrato'), min: 0, max: 40, step: 1, def: def.vib ? def.vib[1] : 0, get: () => p.vib ? p.vib[1] : 0, set: v => { p.vib = v ? [p.vib ? p.vib[0] : 5.5, v, p.vib ? p.vib[2] : .2] : undefined; }, fmt: v => v + ' ct' });
    // Wavetable
    m = mod('', T('wt')); cvWT = cv(m, 92);
    const s2 = document.createElement('div'); s2.className = 'row2'; m.appendChild(s2);
    const wo = HG.WT.map((t, i) => [i, T('wt' + i)]);
    sel(s2, wo, () => p.wt[0], v => { p.wt[0] = +v; }, T('tableA')); sel(s2, wo, () => p.wt[1], v => { p.wt[1] = +v; }, T('tableB'));
    const kw = kbox(m);
    knob(kw, { label: T('morph'), min: 0, max: 1, def: def.wt[2], get: () => p.wt[2], set: v => { p.wt[2] = v; }, fmt: pct, draw: drawWT });
    knob(kw, { label: T('bright'), min: 0, max: 1, def: def.wt[3], get: () => p.wt[3], set: v => { p.wt[3] = v; }, fmt: pct, draw: drawWT });
    const note = document.createElement('p'); note.className = 'fine'; note.textContent = T('wtNote'); m.appendChild(note);
    // Filtre
    m = mod('w6', T('filter')); cvFilt = cv(m, 88);
    const fr = document.createElement('div'); fr.className = 'sel3'; m.appendChild(fr);
    sel(fr, [['lowpass', T('fLP')], ['highpass', T('fHP')], ['bandpass', T('fBP')]], () => p.f[0], v => { p.f[0] = v; }, T('filterType'));
    tgl(fr, '24 dB', () => p.f[6], v => { p.f[6] = v ? 1 : 0; });
    sel(fr, [['', T('noFormant')], ['a', T('formant') + ' A'], ['o', T('formant') + ' O'], ['u', T('formant') + ' U'], ['e', T('formant') + ' E'], ['i', T('formant') + ' I']], () => p.vow || '', v => { p.vow = v || undefined; }, T('formant'));
    const kf = kbox(m);
    knob(kf, { label: T('cutoff'), min: 60, max: 16000, log: true, def: def.f[1], get: () => p.f[1], set: v => { p.f[1] = v; }, fmt: fHz, draw: drawFilt });
    knob(kf, { label: T('reso'), min: .3, max: 18, def: def.f[2], get: () => p.f[2], set: v => { p.f[2] = v; }, fmt: v => v.toFixed(1), draw: drawFilt });
    knob(kf, { label: T('envAmt'), min: 0, max: 8, def: def.f[3], get: () => p.f[3], set: v => { p.f[3] = v; }, fmt: v => v.toFixed(1) + '×', draw: drawFilt });
    knob(kf, { label: T('envAtk'), min: 0, max: .5, def: def.f[4], get: () => p.f[4], set: v => { p.f[4] = v; }, fmt: fS });
    knob(kf, { label: T('envDec'), min: .02, max: 2, def: def.f[5], get: () => p.f[5], set: v => { p.f[5] = v; }, fmt: fS });
    knob(kf, { label: T('drive'), min: 0, max: 1, def: def.fd || 0, get: () => p.fd || 0, set: v => { p.fd = v; }, fmt: pct });
    knob(kf, { label: T('keytrack'), min: 0, max: 1, def: def.kt == null ? .5 : def.kt, get: () => p.kt == null ? .5 : p.kt, set: v => { p.kt = v; }, fmt: pct });
    // Genlik zarfı
    m = mod('w6', T('ampEnv')); cvEnv = cv(m, 88);
    const ke = kbox(m);
    [[T('attack'), 0, .001, 2, fS], [T('decay'), 1, .01, 3, fS], [T('sustain'), 2, 0, 1, pct], [T('release'), 3, .02, 4, fS]].forEach(([lb, i, mn, mxv, f]) => knob(ke, { label: lb, min: mn, max: mxv, log: i !== 2 && false, def: def.a[i], get: () => p.a[i], set: v => { p.a[i] = v; }, fmt: f, draw: drawEnv }));
    knob(ke, { label: T('volume'), min: .05, max: .45, def: def.vol, get: () => p.vol, set: v => { p.vol = v; }, fmt: v => Math.round(v / .45 * 100) + '%' });
    // LFO'lar
    [0, 1].forEach(i => {
      m = mod('w6', T('lfo', { n: i + 1 })); const L = p.l[i], dl_ = def.l[i];
      const r = document.createElement('div'); r.className = 'sel3'; m.appendChild(r);
      sel(r, [['cut', T('tCut')], ['pit', T('tPit')], ['amp', T('tAmp')]], () => L[2], v => { L[2] = v; }, T('lfo', { n: i + 1 }) + ' ' + T('target'));
      sel(r, [['sine', T('sSine')], ['triangle', T('sTri')], ['square', T('sSq')], ['sawtooth', T('sSaw')]], () => L[3], v => { L[3] = v; }, T('lfo', { n: i + 1 }) + ' ' + T('shape'));
      tgl(r, T('sync'), () => L[4], v => { L[4] = v ? 1 : 0; if (v) L[0] = clamp(L[0], .0625, 4); });
      const kl = kbox(m);
      knob(kl, { label: T('rate'), min: .05, max: 20, log: true, def: dl_[0], get: () => L[0], set: v => { L[0] = v; }, fmt: v => L[4] ? (v >= 1 ? v.toFixed(2) + T('perBeat') : T('beats', { n: Math.round(1 / v) })) : v.toFixed(2) + ' Hz' });
      knob(kl, { label: T('depth'), min: 0, max: 1, def: dl_[1], get: () => L[1], set: v => { L[1] = v; }, fmt: pct });
    });
    // Efektler
    m = mod('w12', T('fx'), '<span class="meta"></span>'); m.querySelector('.meta').textContent = T('fxNote');
    const kx = kbox(m);
    [[T('chorus'), 'ch'], [T('delay'), 'dl'], [T('reverb'), 'rv'], [T('sat'), 'dr']].forEach(([lb, key]) => knob(kx, { label: lb, min: 0, max: 1, fx: true, def: def[key] || 0, get: () => p[key] || 0, set: v => { p[key] = v; }, fmt: pct }));
    // Canlı ölçüm
    m = mod('w12', T('live')); const rr = document.createElement('div'); rr.className = 'row2'; m.appendChild(rr); cvScope = cv(rr, 90); cvSpec = cv(rr, 90); cvScope.setAttribute('role', 'img'); cvScope.setAttribute('aria-label', T('scope')); cvSpec.setAttribute('role', 'img'); cvSpec.setAttribute('aria-label', T('spectrum'));
    requestAnimationFrame(redrawAll);
    $('sd-base').value = snd[track] || '';
  }

  function trackTabs() {
    const h = $('sd-track'); h.innerHTML = '';
    TRK.forEach(k => { const b = document.createElement('button'); b.type = 'button'; b.className = 'btn sm' + (k === track ? ' on' : ''); b.textContent = T(k); b.setAttribute('aria-pressed', k === track); b.addEventListener('click', () => UI.editTrack(k)); h.appendChild(b); });
    const s = $('sd-base'); s.innerHTML = '';
    const role = { chords: 'c', bass: 'b', melody: 'm' }[track], pool = MB.SOUNDS.filter(x => x.r.indexOf(role) >= 0);
    MB.CATS.forEach(c => { const it = pool.filter(x => x.cat === c); if (!it.length) return; const g = document.createElement('optgroup'); g.label = T('cat_' + c); it.forEach(x => g.appendChild(new Option(HG.soundName(x.id, HG.lang), x.id))); s.appendChild(g); });
    s.value = snd[track];
  }
  UI.editTrack = function (k) { track = k; trackTabs(); build(); };
  UI.onAssign = function (k) { if (k === track) { trackTabs(); build(); } };

  // ---------- Rastgele, kaydet, yükle
  function randPatch() {
    const R = Math.random, rp = (a, b) => a + R() * (b - a), pk = a => a[Math.floor(R() * a.length)], base = HG.normalizePatch(MB.byId[snd[track]]);
    const q = JSON.parse(JSON.stringify(base)), bass = track === 'bass', lead = track === 'melody';
    q.id = base.id; q.o = [[pk(['w', 'q', 't', 'x']), 0, bass ? -1 : 0, rp(.5, 1)], [pk(['w', 'q', 's', 'x']), Math.round(rp(-14, 14)), pk(bass ? [-1, -1, 0] : [0, 0, 1]), rp(0, .6)], ['s', 0, 0, 0]];
    q.wt = [Math.floor(R() * HG.WT.length), Math.floor(R() * HG.WT.length), R(), rp(.4, 1)];
    q.u = [bass ? 1 : pk([1, 2, 3, 5]), Math.round(rp(4, 28))];
    q.f = [bass ? 'lowpass' : pk(['lowpass', 'lowpass', 'bandpass']), Math.round(bass ? rp(180, 700) : rp(700, 6500)), rp(.8, bass ? 8 : 5), rp(0, bass ? 4 : 3.5), 0, rp(.06, .6), R() < .4 ? 1 : 0];
    q.a = bass ? [.002, rp(.06, .25), rp(.3, .9), rp(.04, .15)] : track === 'chords' ? [rp(.003, .5), rp(.15, .8), rp(0, .9), rp(.1, 1.4)] : [rp(.003, .05), rp(.1, .4), rp(.3, .9), rp(.1, .5)];
    q.l = [[pk([.2, .5, 1, 2]), rp(0, .5), pk(['cut', 'cut', 'pit', 'amp']), pk(['sine', 'triangle']), R() < .5 ? 1 : 0], [1, 0, 'cut', 'sine']];
    q.ch = bass ? 0 : rp(0, .5); q.dl = bass ? 0 : rp(0, .5); q.rv = bass ? rp(0, .1) : rp(.1, .55); q.dr = rp(0, bass ? .4 : .25); q.fd = rp(0, .4);
    q.vow = undefined; q.name = T('randomName', { t: T(track) }); delete q.sub; delete q.noi; delete q.pe; delete q.vib; delete q.fm; delete q.lf;
    if (lead && R() < .6) q.vib = [5.5, Math.round(rp(4, 14)), .25];
    return q;
  }
  function sanitize(o) {
    if (!o || typeof o !== 'object' || !Array.isArray(o.o) || !Array.isArray(o.a)) throw new Error('bad');
    const n = x => (typeof x === 'number' && isFinite(x) ? x : 0);
    const q = HG.normalizePatch({ id: MB.byId[snd[track]].id, name: String(o.name || T('loadedName')).slice(0, 40), cat: 'Lead', r: 'cmb', g: [], vol: clamp(n(o.vol) || .2, .05, .45), o: o.o.slice(0, 3).map(r => [WAVES.some(w => w[0] === r[0]) ? r[0] : 'w', clamp(n(r[1]), -100, 100), clamp(Math.round(n(r[2])), -3, 3), clamp(n(r[3]), 0, 1)]), u: [clamp(Math.round(n(o.u && o.u[0]) || 1), 1, 7), clamp(n(o.u && o.u[1]), 0, 100)], f: [['lowpass', 'highpass', 'bandpass'].includes(o.f && o.f[0]) ? o.f[0] : 'lowpass', clamp(n(o.f && o.f[1]) || 2000, 60, 16000), clamp(n(o.f && o.f[2]) || 1, .1, 20), clamp(n(o.f && o.f[3]), 0, 10), clamp(n(o.f && o.f[4]), 0, 1), clamp(n(o.f && o.f[5]) || .2, .02, 3), o.f && o.f[6] ? 1 : 0], a: [clamp(n(o.a[0]) || .01, .001, 4), clamp(n(o.a[1]) || .2, .01, 4), clamp(n(o.a[2]), 0, 1), clamp(n(o.a[3]) || .2, .02, 5)], wt: Array.isArray(o.wt) ? [clamp(Math.round(n(o.wt[0])), 0, HG.WT.length - 1), clamp(Math.round(n(o.wt[1])), 0, HG.WT.length - 1), clamp(n(o.wt[2]), 0, 1), clamp(n(o.wt[3]), 0, 1)] : [0, 3, .3, 1], rv: clamp(n(o.rv), 0, 1), dl: clamp(n(o.dl), 0, 1), ch: clamp(n(o.ch), 0, 1), dr: clamp(n(o.dr), 0, 1), fd: clamp(n(o.fd), 0, 1) });
    if (Array.isArray(o.l)) q.l = o.l.slice(0, 2).map(l => [clamp(n(l[0]) || 1, .05, 20), clamp(n(l[1]), 0, 1), ['cut', 'pit', 'amp'].includes(l[2]) ? l[2] : 'cut', ['sine', 'triangle', 'square', 'sawtooth'].includes(l[3]) ? l[3] : 'sine', l[4] ? 1 : 0]);
    while (q.l.length < 2) q.l.push([1, 0, 'cut', 'sine']);
    return q;
  }
  function bindTop() {
    $('sd-base').addEventListener('change', () => { UI.assign(track, $('sd-base').value, true); });
    $('sd-rand').addEventListener('click', () => { patches[track] = randPatch(); UI.rebuildSoon(); build(); playKey(track === 'bass' ? 40 : 60, 1); });
    $('sd-reset').addEventListener('click', () => { UI.assign(track, snd[track], true); });
    $('sd-save').addEventListener('click', async () => {
      if (!dl) { toast(T('dlOff')); return; }
      const name = 'HitGen_yama_' + (P().name || 'ses').replace(/[^\w]+/g, '-').toLowerCase() + '.json';
      try { await dl.save({ filename: name, data: new Blob([JSON.stringify(P(), null, 1)], { type: 'application/json' }) }); toast(T('saved', { f: name })); } catch (e) { toast(e && e.code === 'declined' ? T('declined') : T('saveFail')); }
    });
    $('sd-load').addEventListener('change', async e => {
      const f = e.target.files[0]; if (!f) return;
      try { patches[track] = sanitize(JSON.parse(await f.text())); UI.rebuildSoon(); build(); toast(T('patchLoaded', { n: patches[track].name })); } catch (x) { toast(T('patchBad')); }
      e.target.value = '';
    });
  }

  // ---------- Klavye
  const down = new Set();
  function playKey(n, vel) {
    if (!P() || !UI.ensureMix()) return; const ac = rt.ac, p = P();
    UI.liveUntil = performance.now() + 1500;
    try { MB.voice(ac, rt.mx.ins[track], p, MB.mtof(n), ac.currentTime + .01, track === 'bass' ? .55 : .8, (vel || .8) * 110); } catch (e) { toast(T('audioFail')); }
  }
  function buildKbd() {
    const k = $('kbd'); k.innerHTML = ''; const W = [0, 2, 4, 5, 7, 9, 11], B = { 1: 0, 3: 1, 6: 3, 8: 4, 10: 5 };
    const oct = 2, nW = 7 * oct + 1, ww = 100 / nW; let wi = 0; const base = () => ({ chords: 48, bass: 36, melody: 60 }[track]);
    const mk = (cls, semi, left, width, lab) => { const d = document.createElement('div'); d.className = cls; d.style.left = left + '%'; d.style.width = width + '%'; d.dataset.s = semi; if (lab) d.innerHTML = '<small>' + lab + '</small>'; k.appendChild(d); };
    for (let o = 0; o < oct; o++) W.forEach((s, i) => { mk('wk', o * 12 + s, wi * ww, ww, s === 0 ? 'C' + (Math.floor(base() / 12) - 1 + o) : ''); wi++; });
    mk('wk', 24, wi * ww, ww, '');
    for (let o = 0; o < oct; o++) Object.keys(B).forEach(s => { const pos = o * 7 + B[s] + 1; mk('bk', o * 12 + +s, pos * ww - ww * .3, ww * .6, ''); });
    const on = (e, d) => { e.preventDefault(); d.classList.add('dn'); playKey(base() + +d.dataset.s, .85); };
    k.addEventListener('pointerdown', e => { const d = e.target.closest('[data-s]'); if (d) on(e, d); });
    k.addEventListener('pointerup', () => k.querySelectorAll('.dn').forEach(x => x.classList.remove('dn')));
    k.addEventListener('pointerleave', () => k.querySelectorAll('.dn').forEach(x => x.classList.remove('dn')));
    const MAP = 'awsedftgyhujk';
    document.addEventListener('keydown', e => {
      if ($('p-sound').hidden || e.repeat || e.ctrlKey || e.metaKey || e.altKey) return; const t = e.target && e.target.tagName; if (t === 'INPUT' || t === 'SELECT' || t === 'TEXTAREA') return;
      const i = MAP.indexOf(e.key.toLowerCase()); if (i < 0 || down.has(e.key)) return; down.add(e.key);
      const d = k.querySelector('[data-s="' + i + '"]'); if (d) d.classList.add('dn'); playKey(base() + i, .85);
    });
    document.addEventListener('keyup', e => { down.delete(e.key); const i = MAP.indexOf(e.key.toLowerCase()); const d = i >= 0 && k.querySelector('[data-s="' + i + '"]'); if (d) d.classList.remove('dn'); });
  }
  UI.playKey = playKey;
  UI.init2 = async function () {
    trackTabs(); build(); buildKbd(); bindTop();
    try { dl = (window.claude && window.claude.use) ? await window.claude.use('downloads') : null; } catch (e) { dl = null; }
    UI.dl = dl; if (UI.onDl) UI.onDl();
  };
  UI.editorRefresh = function () { trackTabs(); build(); };
  UI.onLang = function () { trackTabs(); build(); if (UI.onLang2) UI.onLang2(); };
})();
