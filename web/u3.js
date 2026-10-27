// ---- Arayüz 1/3: durum, dil, vuruş sekmesi, çalma motoru
const UI = {};
(function () {
  const $ = id => document.getElementById(id);
  const T = (k, v) => HG.t(k, v);
  const KEYS = ['chords', 'bass', 'drums', 'melody'];
  const HEIGHT = { chords: 120, bass: 90, drums: 112, melody: 120 };
  const LM = 36;
  const LANES = [['ohat', 'OH'], ['chat', 'CH'], ['perc', 'PRC'], ['rim', 'RIM'], ['clap', 'CLP'], ['snare', 'SNR'], ['kick', 'KCK']];
  const noteName = n => MB.NOTES[n % 12] + (Math.floor(n / 12) - 1);
  const css = n => getComputedStyle(document.documentElement).getPropertyValue(n).trim();
  const toast = m => { const e = $('toast'); if (e) e.textContent = m; const a = $('announce'); if (a) { a.textContent = ''; setTimeout(() => { a.textContent = m; }, 30); } };
  const store = { get: k => { try { return localStorage.getItem('hg-' + k); } catch (e) { return null; } }, set: (k, v) => { try { localStorage.setItem('hg-' + k, v); } catch (e) {} } };
  const st = { style: 'wavejackers', vi: 0, seed: 7, key: 9, scale: 'auto', bpm: 126, bars: 8, orig: 30, energy: 60, human: 20, pump: 100, swing: 0, sevenths: false, thick: 70, quality: 'bal', grp: 'all', fs: 1 };
  const snd = {}, manual = {}, patches = {}, muted = {};
  const rt = { ac: null, mx: null, playing: false, next: 0, nextT: 0, timer: 0, log: [], cur: null, opts: { muted, patches, pump: null, pumpAmt: 1, thick: .7, quality: 'bal' } };
  let res = null, cards = {}, theme = 'system';
  const LABEL = {}; const label = k => T(k);
  Object.assign(UI, { st, snd, manual, patches, muted, rt, KEYS, LABEL, $, css, toast, noteName, T, store });
  UI.res = () => res;
  UI.label = label;

  // ---------- Dil
  function pickLang() {
    const saved = store.get('lang'); if (saved && HG.I18N[saved]) return saved;
    const nav = ((navigator.languages && navigator.languages[0]) || navigator.language || 'en').slice(0, 2).toLowerCase();
    return HG.I18N[nav] ? nav : 'en';
  }
  function translateStatic() {
    document.querySelectorAll('[data-i]').forEach(e => { e.textContent = T(e.dataset.i); });
    document.querySelectorAll('[data-ia]').forEach(e => { e.setAttribute('aria-label', T(e.dataset.ia)); });
    document.querySelectorAll('[data-ih]').forEach(e => { e.innerHTML = T(e.dataset.ih); });
    $('bpm').setAttribute('aria-label', T('bpm')); $('seed').setAttribute('aria-label', T('seed'));
    $('playtxt').textContent = rt.playing ? T('stop') : T('play');
    const bs = $('bars'); [...bs.options].forEach(o => { o.textContent = T('bars', { n: o.value }); });
    const sc = $('scale'), sv = sc.value; sc.innerHTML = ''; sc.add(new Option(T('byStyle'), 'auto')); Object.keys(MB.SCALES).forEach(k => sc.add(new Option(T('scale_' + k), k))); sc.value = sv || 'auto';
    KEYS.forEach(k => { LABEL[k] = T(k); });
  }
  function applyLang(l) {
    HG.lang = l; store.set('lang', l);
    const r = document.documentElement; r.lang = l; r.dir = HG.RTL[l] ? 'rtl' : 'ltr';
    translateStatic(); $('lang').value = l;
    applyTheme(false);
    if (res) { buildNodes(); buildCards(); buildGroups(); buildWall(); buildVars(); fillSelects(); render(); }
    if (UI.onLang) UI.onLang();
  }
  UI.applyLang = applyLang;

  // ---------- Renk: açık temada vurguyu koyulaştır
  const isLight = () => document.documentElement.dataset.theme === 'light' || (!document.documentElement.dataset.theme && window.matchMedia && matchMedia('(prefers-color-scheme: light)').matches);
  const lum = h => { const c = [1, 3, 5].map(i => parseInt(h.substr(i, 2), 16) / 255).map(v => v <= .03928 ? v / 12.92 : Math.pow((v + .055) / 1.055, 2.4)); return .2126 * c[0] + .7152 * c[1] + .0722 * c[2]; };
  const shade = (hex, f) => '#' + [1, 3, 5].map(i => ('0' + Math.round(parseInt(hex.substr(i, 2), 16) * f).toString(16)).slice(-2)).join('');
  function applyAccent() {
    const S = HG.STYLE[st.style], r = document.documentElement.style;
    let hex = S.accent, light = isLight();
    if (light) { let f = .62; while (lum(shade(S.accent, f)) > .16 && f > .3) f -= .04; hex = shade(S.accent, f); }  // açık zeminde en az 4.5:1 kontrast
    r.setProperty('--accent', hex);
    r.setProperty('--accent-ink', lum(hex) > .32 ? '#05090a' : '#ffffff');
    document.querySelectorAll('.sty').forEach(b => { const s = HG.STYLE[b.dataset.id]; b.style.setProperty('--sa', light ? shade(s.accent, .55) : s.accent); });
  }
  UI.applyAccent = applyAccent;

  // ---------- Ses atama
  function setSound(k, id, isManual) {
    snd[k] = id; if (isManual) manual[k] = true;
    if (k !== 'drums') patches[k] = HG.normalizePatch(MB.byId[id]);
  }
  UI.setSound = setSound;
  function autoSounds() { const S = HG.STYLE[st.style]; KEYS.forEach(k => { if (!manual[k] || !snd[k]) setSound(k, S.sound[k], false); }); }
  UI.autoSounds = autoSounds;
  const soundName = k => HG.soundName(snd[k], HG.lang);

  // ---------- Çalma motoru
  function getAC() {
    const AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) return null;
    try {
      if (!rt.ac) {
        rt.ac = new AC({ latencyHint: 'playback' });
        rt.ac.onstatechange = () => { if (rt.playing && rt.ac.state !== 'running' && rt.ac.state !== 'suspended') { stop(); toast(T('audioFail')); } };
      }
      if (rt.ac.state !== 'running') rt.ac.resume();
    } catch (e) { return null; }
    return rt.ac;
  }
  function rebuildMix() {
    if (!rt.ac || !res) return;
    const old = rt.mx;
    rt.opts.pump = res.style.pump; rt.opts.pumpAmt = st.pump / 100; rt.opts.thick = st.thick / 100; rt.opts.quality = st.quality;
    rt.ac.__q = st.quality; rt.ac.__thick = st.thick / 100;
    try { rt.mx = MB.mix(rt.ac, rt.ac.destination, res, snd, rt.opts); } catch (e) { rt.mx = old; toast(T('audioFail')); return; }
    if (old) { old.master.gain.setTargetAtTime(0, rt.ac.currentTime + .05, .5); setTimeout(() => { try { old.dispose(); } catch (e) {} }, 4500); }
  }
  UI.rebuildMix = rebuildMix;
  let rbT = 0; UI.rebuildSoon = () => { clearTimeout(rbT); rbT = setTimeout(rebuildMix, 220); };
  function ensureMix() { if (!getAC()) { toast(T('noAudio')); return false; } if (!rt.mx) rebuildMix(); return !!rt.mx; }
  UI.ensureMix = ensureMix;
  function pumpTick() {
    if (!rt.playing || !rt.mx) return;
    try {
      const ac = rt.ac, spt = 60 / st.bpm / MB.PPQ, steps = res.steps;
      if (rt.nextT < ac.currentTime - .5) { rt.nextT = ac.currentTime + .05; rt.log = []; }   // sekme uykudan dönünce birikmiş notaları atla
      while (rt.nextT < ac.currentTime + .32) {
        const loc = rt.next % steps, dur = (MB.tickOf(loc + 1, res.P.swing) - MB.tickOf(loc, res.P.swing)) * spt;
        rt.mx.schedule(rt.nextT, { s0: loc, s1: loc + 1 });
        rt.log.push([rt.nextT, loc, dur]); if (rt.log.length > 64) rt.log.shift();
        rt.nextT += dur; rt.next++;
      }
    } catch (e) { stop(); toast(T('audioFail')); }
  }
  function start() {
    if (!res || !ensureMix()) return;
    rt.playing = true; rt.next = 0; rt.nextT = rt.ac.currentTime + .08; rt.log = []; rt.cur = null;
    pumpTick(); rt.timer = setInterval(pumpTick, 60);
    document.body.classList.add('playing'); $('playtxt').textContent = T('stop'); $('play').setAttribute('aria-pressed', 'true');
    $('playico').setAttribute('d', 'M2 2h4v10H2zM8 2h4v10H8z');
    cancelAnimationFrame(UI.raf); UI.raf = requestAnimationFrame(frame);
  }
  function stop() {
    if (!rt.playing) return;
    rt.playing = false; clearInterval(rt.timer); rt.log = [];
    document.body.classList.remove('playing'); $('playtxt').textContent = T('play'); $('play').setAttribute('aria-pressed', 'false');
    $('playico').setAttribute('d', 'M3 1.5v11l9-5.5z');
    KEYS.forEach(k => { if (cards[k]) cards[k].ph.style.display = 'none'; });
    document.querySelectorAll('.pc.on').forEach(c => c.classList.remove('on'));
  }
  UI.start = start; UI.stop = stop; UI.toggle = () => (rt.playing ? stop() : start());
  function audition(k) {
    if (!res || !ensureMix()) return;
    if (rt.playing) stop();
    try { rt.mx.schedule(rt.ac.currentTime + .08, { only: k, s0: 0, s1: Math.min(2, res.bars) * 16 }); } catch (e) { toast(T('audioFail')); }
  }
  UI.audition = audition;

  // ---------- Animasyon döngüsü: oynatma imleci, düğüm yanıp sönmesi, spektrum
  let lastLoc = -1, fcount = 0; const hitSteps = {};
  function buildHitSteps() { KEYS.forEach(k => { hitSteps[k] = new Set(res.tracks[k].map(n => n.s)); }); }
  const bins = new Uint8Array(1024);
  function frame() {
    UI.raf = requestAnimationFrame(frame);
    fcount++;
    const ac = rt.ac, eco = st.quality === 'eco';
    if (rt.playing && ac && res) {
      const ct = ac.currentTime;
      while (rt.log.length && rt.log[0][0] <= ct) {
        rt.cur = rt.log.shift();
        const loc = rt.cur[1];
        if (loc !== lastLoc) {
          lastLoc = loc;
          KEYS.forEach(k => { if (hitSteps[k].has(loc)) { const n = $('node-' + k); if (n) { n.classList.remove('hit'); void n.offsetWidth; n.classList.add('hit'); } } });
          const bar = Math.floor(loc / 16), chip = $('pc-' + bar);
          if (loc % 16 === 0) { document.querySelectorAll('.pc.on').forEach(c => c.classList.remove('on')); if (chip) chip.classList.add('on'); }
        }
      }
      if (rt.cur) {
        const f = (rt.cur[1] + Math.max(0, Math.min(1, (ct - rt.cur[0]) / rt.cur[2]))) / res.steps, rtl = document.documentElement.dir === 'rtl';
        KEYS.forEach(k => { const c = cards[k]; if (!c) return; const w = c.cv.clientWidth; c.ph.style.display = 'block'; c.ph.style.transform = 'translateX(' + (rtl ? (w - LM) * (1 - f) : LM + f * (w - LM)) + 'px)'; });
      }
    }
    // Eco modda analiz çizimleri yarı hızda, durunca hiç çizilmez
    const live = rt.playing || (UI.liveUntil || 0) > performance.now();
    if ((live || fcount < 3) && (!eco || fcount % 2 === 0)) { drawSpecMini(); if (UI.onFrame) UI.onFrame(); }
    else if (!rt.playing && fcount % 30 === 0) drawSpecMini();
  }
  function drawSpecMini() {
    const c = $('specmini'); if (!c) return;
    const w = c.clientWidth, h = c.clientHeight; if (!w) return;
    const dpr = Math.min(2, window.devicePixelRatio || 1);
    if (c.width !== Math.round(w * dpr)) { c.width = Math.round(w * dpr); c.height = Math.round(h * dpr); }
    const x = c.getContext('2d'); x.setTransform(dpr, 0, 0, dpr, 0, 0); x.clearRect(0, 0, w, h);
    const an = rt.mx && rt.mx.an, N = Math.floor(w / 5);
    x.fillStyle = css('--accent');
    if (an && rt.playing) an.getByteFrequencyData(bins);
    for (let i = 0; i < N; i++) {
      let v = 0;
      if (an && rt.playing) { const a = Math.floor(Math.pow(i / N, 2) * 300) + 1, b = Math.floor(Math.pow((i + 1) / N, 2) * 300) + 2; let m = 0; for (let j = a; j < b; j++) m = Math.max(m, bins[j]); v = m / 255; }
      const bh = Math.max(2, v * h);
      x.globalAlpha = rt.playing ? .35 + v * .65 : .22;
      x.fillRect(i * 5, h - bh, 3, bh);
    }
    x.globalAlpha = 1;
  }

  // ---------- Üretim
  function generate() {
    autoSounds(); fillSelects();
    const P = { style: st.style, vi: st.vi, seed: st.seed, bars: st.bars, key: st.key, scale: st.scale === 'auto' ? undefined : st.scale, orig: st.orig, energy: st.energy, human: st.human, sevenths: st.sevenths, bpm: st.bpm, swing: st.swing, thick: st.thick };
    let r;
    try { r = HG.generate(P); if (!r || !r.tracks.drums.length) throw new Error('empty'); }
    catch (e) { toast(T('genFail')); if (res) return; throw e; }
    res = r; res.P.bpm = st.bpm; res.P.swing = st.swing;
    buildHitSteps(); render();
    if (rt.ac) rebuildMix();
  }
  UI.generate = generate;

  function render() {
    const S = res.style, P = res.P, ki = MB.keyInfo(P.key, P.scale), tx = HG.stxt(S.id), V = tx.vars[st.vi] || tx.vars[0];
    const prog = $('prog'); prog.innerHTML = '';
    res.chords.forEach((c, i) => {
      const d = document.createElement('div'); d.className = 'pc'; d.id = 'pc-' + i; d.setAttribute('role', 'listitem');
      d.innerHTML = '<b dir="ltr"></b><span></span>'; d.firstChild.textContent = c.name; d.lastChild.textContent = c.roman + ' · ' + T('bar') + ' ' + (i + 1);
      prog.appendChild(d);
    });
    $('meta').textContent = MB.NOTES[P.key] + ' ' + T('scale_' + P.scale) + ' · ' + ki.cam + ' · ' + P.bpm + ' BPM · ' + S.artist + ' / ' + V.n;
    $('note').textContent = tx.blurb + ' ' + V.note + ' ' + T('compat') + ' ' + ki.compat.join(', ') + '.';
    KEYS.forEach(k => {
      const n = res.tracks[k]; let info = T('notes', { n: n.length }), range = '';
      if (k === 'drums') info += ' · ' + T('gm10'); else { const lo = Math.min.apply(null, n.map(a => a.n)), hi = Math.max.apply(null, n.map(a => a.n)); range = noteName(lo) + '–' + noteName(hi); info += ' · ' + range; }
      cards[k].info.textContent = info;
      cards[k].cv.setAttribute('aria-label', T('rollLabel', { t: T(k), n: n.length, r: range || T('gm10') }));
    });
    refreshNodes(); drawAll(); if (UI.onRender) UI.onRender();
  }

  // ---------- Kablo şeridi
  function buildNodes() {
    const host = $('nodes'); host.innerHTML = '';
    KEYS.forEach(k => {
      const b = document.createElement('button'); b.type = 'button'; b.className = 'node'; b.id = 'node-' + k; b.style.setProperty('--tc', 'var(--c-' + k + ')');
      b.innerHTML = '<span class="jk l" aria-hidden="true"></span><span class="jk r" aria-hidden="true"></span><span class="dot" aria-hidden="true"></span><b></b><small></small>';
      b.querySelector('b').textContent = T(k);
      b.title = k === 'drums' ? T('nodeDrums') : T('nodeDesign', { x: T(k) });
      b.addEventListener('click', () => { if (k === 'drums') { UI.bankCat = 'Davul'; UI.tab('bank'); } else { UI.editTrack(k); UI.tab('sound'); } });
      host.appendChild(b);
    });
  }
  function refreshNodes() { KEYS.forEach(k => { const n = $('node-' + k); n.querySelector('small').textContent = soundName(k) || ''; n.setAttribute('aria-label', T(k) + ': ' + (soundName(k) || '') + '. ' + n.title); }); drawCables(); }
  function drawCables() {
    const svg = $('cables'); if (!svg) return; const box = svg.parentNode.getBoundingClientRect(); let d = '';
    const ns = KEYS.map(k => $('node-' + k).getBoundingClientRect());
    for (let i = 0; i < 3; i++) {
      const A = ns[i], B = ns[i + 1], sag = 22 + (i % 2) * 8;
      if (B.left > A.right - 4 || B.right < A.left + 4) {
        const ltr = B.left > A.right - 4, x1 = (ltr ? A.right : A.left) - box.left, y1 = A.top + A.height / 2 - box.top, x2 = (ltr ? B.left : B.right) - box.left, y2 = B.top + B.height / 2 - box.top, dx = (x2 - x1) * .5;
        d += 'M' + x1 + ' ' + y1 + ' C' + (x1 + dx) + ' ' + (y1 + sag) + ' ' + (x2 - dx) + ' ' + (y2 + sag) + ' ' + x2 + ' ' + y2 + ' ';
      } else {
        const x1 = A.left + A.width / 2 - box.left, y1 = A.bottom - box.top, x2 = B.left + B.width / 2 - box.left, y2 = B.top - box.top, dy = (y2 - y1) * .55;
        d += 'M' + x1 + ' ' + y1 + ' C' + x1 + ' ' + (y1 + dy) + ' ' + x2 + ' ' + (y2 - dy) + ' ' + x2 + ' ' + y2 + ' ';
      }
    }
    svg.setAttribute('viewBox', '0 0 ' + box.width + ' ' + box.height);
    svg.innerHTML = '<path class="c1" d="' + d + '"/><path class="c2" d="' + d + '"/>';
  }
  UI.drawCables = drawCables;
  function setBeat() { document.documentElement.style.setProperty('--beat', (60 / st.bpm).toFixed(3) + 's'); }

  // ---------- İz kartları ve piyano roll
  function buildCards() {
    const host = $('tracks'); host.innerHTML = '';
    KEYS.forEach(k => {
      const el = document.createElement('section'); el.className = 'trk'; el.style.setProperty('--tc', 'var(--c-' + k + ')'); el.setAttribute('aria-label', T(k));
      el.innerHTML = '<div class="th"><i aria-hidden="true"></i><b></b><span class="inf"></span>' +
        '<button type="button" class="btn sm" data-mute="' + k + '" aria-pressed="' + !!muted[k] + '"></button>' +
        '<span class="drag" draggable="true" data-drag="' + k + '" role="button" tabindex="0">⠿ .mid</span></div>' +
        '<div class="sr"><select data-snd="' + k + '" id="snd-' + k + '"></select><button type="button" class="btn sm" data-aud="' + k + '"></button>' +
        (k === 'drums' ? '' : '<button type="button" class="btn sm" data-edit="' + k + '"></button>') + '</div>' +
        '<div class="wrap"><canvas class="roll" role="img" style="height:' + HEIGHT[k] + 'px"></canvas><div class="ph" aria-hidden="true"></div></div>';
      el.querySelector('b').textContent = T(k);
      const mb = el.querySelector('[data-mute]'); mb.textContent = muted[k] ? T('unmute') : T('mute'); mb.setAttribute('aria-label', (muted[k] ? T('unmute') : T('mute')) + ': ' + T(k));
      const dg = el.querySelector('[data-drag]'); dg.title = T('dragMid'); dg.setAttribute('aria-label', T('dragMid') + ': ' + T(k));
      el.querySelector('select').setAttribute('aria-label', T('soundOf', { t: T(k) }));
      const ab = el.querySelector('[data-aud]'); ab.textContent = T('listen'); ab.setAttribute('aria-label', T('listen') + ': ' + T(k));
      const eb = el.querySelector('[data-edit]'); if (eb) { eb.textContent = T('design'); eb.setAttribute('aria-label', T('design') + ': ' + T(k)); }
      host.appendChild(el);
      cards[k] = { cv: el.querySelector('canvas'), ph: el.querySelector('.ph'), info: el.querySelector('.inf') };
    });
    host.querySelectorAll('[data-mute]').forEach(b => b.addEventListener('click', () => {
      const k = b.dataset.mute; muted[k] = !muted[k]; b.classList.toggle('on', !!muted[k]); b.textContent = muted[k] ? T('unmute') : T('mute'); b.setAttribute('aria-pressed', !!muted[k]); b.setAttribute('aria-label', b.textContent + ': ' + T(k));
      if (rt.mx && rt.mx.bus[k]) rt.mx.bus[k].gain.value = muted[k] ? 0 : 1;
    }));
    host.querySelectorAll('[data-aud]').forEach(b => b.addEventListener('click', () => audition(b.dataset.aud)));
    host.querySelectorAll('[data-edit]').forEach(b => b.addEventListener('click', () => { UI.editTrack(b.dataset.edit); UI.tab('sound'); }));
    host.querySelectorAll('[data-snd]').forEach(s => s.addEventListener('change', () => { UI.assign(s.dataset.snd, s.value, true); audition(s.dataset.snd); }));
  }
  UI.assign = function (k, id, m) {
    setSound(k, id, m); if (rt.ac) rebuildMix(); refreshNodes(); fillSelects();
    if (UI.onAssign) UI.onAssign(k);
  };
  function fillSelects() {
    const ROLE = { chords: 'c', bass: 'b', melody: 'm' };
    KEYS.forEach(k => {
      const sel = $('snd-' + k); if (!sel) return; sel.innerHTML = '';
      const og = l => { const g = document.createElement('optgroup'); g.label = l; sel.appendChild(g); return g; };
      if (k === 'drums') { const g = og(T('kits')); MB.KITS_LIST.forEach(x => g.appendChild(new Option(HG.soundName(x.id, HG.lang), x.id))); }
      else {
        const pool = MB.SOUNDS.filter(s => s.r.indexOf(ROLE[k]) >= 0), sig = pool.filter(s => HG.SIG.test(s.id));
        const g1 = og(T('sigSounds')); sig.forEach(s => g1.appendChild(new Option(HG.soundName(s.id, HG.lang), s.id)));
        MB.CATS.forEach(c => { const items = pool.filter(s => s.cat === c && !sig.includes(s)); if (!items.length) return; const g = og(T('cat_' + c)); items.forEach(s => g.appendChild(new Option(HG.soundName(s.id, HG.lang), s.id))); });
      }
      sel.value = snd[k];
    });
  }
  UI.fillSelects = fillSelects;

  function draw(k) {
    const c = cards[k].cv, w = c.clientWidth, h = HEIGHT[k]; if (!w || !res) return;
    const dpr = Math.min(2, window.devicePixelRatio || 1); c.width = Math.round(w * dpr); c.height = Math.round(h * dpr);
    const x = c.getContext('2d'); x.setTransform(dpr, 0, 0, dpr, 0, 0);
    const grid = css('--grid'), strong = css('--grid2'), mute = css('--mute'), col = css('--c-' + k);
    const pw = w - LM, total = res.bars * 16 * (MB.PPQ / 4), notes = res.tracks[k], rtl = document.documentElement.dir === 'rtl';
    const ox = rtl ? 0 : LM, lx = rtl ? w - LM + 6 : 6;   // sağdan sola dilde zaman sağdan akar
    const X = s => rtl ? (1 - MB.tickOf(s, res.P.swing) / total) * pw : LM + (MB.tickOf(s, res.P.swing) / total) * pw;
    let rows, rowOf;
    x.font = '9px ' + css('--f-m'); x.textBaseline = 'middle';
    if (k === 'drums') {
      const used = LANES.filter(l => notes.some(n => n.n === MB.GM[l[0]])); rows = used.length || 1;
      rowOf = n => used.findIndex(l => MB.GM[l[0]] === n);
      used.forEach((l, i) => { const y = i * (h / rows); if (i % 2) { x.fillStyle = grid; x.globalAlpha = .6; x.fillRect(ox, y, pw, h / rows); x.globalAlpha = 1; } x.fillStyle = mute; x.fillText(l[1], lx, y + h / rows / 2); });
    } else {
      let lo = Math.min.apply(null, notes.map(n => n.n)) - 1, hi = Math.max.apply(null, notes.map(n => n.n)) + 1;
      if (hi - lo < 11) { const m = Math.ceil((11 - (hi - lo)) / 2); lo -= m; hi += m; }
      rows = hi - lo + 1; rowOf = n => hi - n;
      for (let n = lo; n <= hi; n++) { const y = (hi - n) * (h / rows); if ([1, 3, 6, 8, 10].includes(n % 12)) { x.fillStyle = grid; x.globalAlpha = .7; x.fillRect(ox, y, pw, h / rows); x.globalAlpha = 1; } if (n % 12 === 0) { x.fillStyle = mute; x.fillText(noteName(n), lx, y + h / rows / 2); } }
    }
    for (let s = 0; s <= res.bars * 16; s += 4) { const gx = rtl ? (1 - s * (MB.PPQ / 4) / total) * pw : LM + (s * (MB.PPQ / 4) / total) * pw; x.strokeStyle = s % 16 === 0 ? strong : grid; x.lineWidth = s % 16 === 0 ? 1.5 : 1; x.beginPath(); x.moveTo(gx, 0); x.lineTo(gx, h); x.stroke(); }
    x.fillStyle = col; const rh = h / rows;
    notes.forEach(n => { const r = rowOf(n.n); if (r < 0) return; let x0 = X(n.s), x1 = X(n.s + n.l); if (rtl) { const t = x0; x0 = x1; x1 = t; } x.globalAlpha = .5 + .5 * (n.v / 127); x.fillRect(x0, r * rh + 1, Math.max(2, x1 - x0 - 1), Math.max(2, rh - 2)); });
    x.globalAlpha = 1;
  }
  function drawAll() { if (!res) return; KEYS.forEach(draw); drawCables(); }
  UI.drawAll = drawAll;

  // ---------- Stil duvarı ve kontroller
  const AW = { house: 'awHouse', techno: 'awTechno', trance: 'awTrance', psy: 'awPsy', hard: 'awHard', group: 'awGroup' };
  const GK = { all: 'gAll', techno: 'gTechno', house: 'gHouse', main: 'gMain', trance: 'gTrance', hard: 'gHard', afro: 'gAfro' };
  function buildGroups() {
    const h = $('groups'); h.innerHTML = '';
    HG.GROUPS.forEach(g => {
      const b = document.createElement('button'); b.type = 'button'; b.className = 'btn sm' + (g === st.grp ? ' on' : ''); b.textContent = T(GK[g]); b.setAttribute('aria-pressed', g === st.grp);
      b.addEventListener('click', () => { st.grp = g; buildGroups(); buildWall(); }); h.appendChild(b);
    });
  }
  function buildWall() {
    const w = $('wall'); w.innerHTML = '';
    HG.STYLES.filter(S => st.grp === 'all' || S.grp === st.grp).forEach(S => {
      const tx = HG.stxt(S.id), b = document.createElement('button'); b.type = 'button'; b.className = 'sty'; b.dataset.id = S.id;
      b.innerHTML = '<b></b><span></span><small></small>' + (S.rank || S.award ? '<span class="rk"></span>' : '');
      b.children[0].textContent = S.artist; b.children[1].textContent = tx.tag + ' · '; const bd = document.createElement('bdi'); bd.dir = 'ltr'; bd.textContent = S.bpm[0] + '–' + S.bpm[1] + ' BPM'; b.children[1].appendChild(bd); b.children[2].textContent = tx.blurb;
      if (S.rank || S.award) b.querySelector('.rk').textContent = [S.rank ? T('rank', { n: S.rank }) : '', S.award ? T(AW[S.award]) : ''].filter(Boolean).join(' · ');
      b.setAttribute('aria-pressed', S.id === st.style);
      b.addEventListener('click', () => selectStyle(S.id));
      w.appendChild(b);
    });
    applyAccent();
  }
  function buildVars() {
    const tx = HG.stxt(st.style), h = $('vars'); h.innerHTML = '';
    tx.vars.forEach((V, i) => {
      const b = document.createElement('button'); b.type = 'button'; b.className = 'btn' + (i === st.vi ? ' on' : ''); b.setAttribute('aria-pressed', i === st.vi);
      b.innerHTML = '<span></span><small></small>'; b.children[0].textContent = V.n; b.children[1].textContent = V.note;
      b.addEventListener('click', () => { st.vi = i; buildVars(); generate(); toast(T('styleSel', { s: HG.STYLE[st.style].artist, v: V.n })); });
      h.appendChild(b);
    });
  }
  function selectStyle(id) {
    const S = HG.STYLE[id]; st.style = id; st.vi = 0; st.bpm = S.bpmDef; st.key = S.key == null ? st.key : S.key; st.scale = 'auto'; st.swing = S.swing || 0; st.sevenths = !!S.sev; $('sevenths').checked = st.sevenths;
    KEYS.forEach(k => { manual[k] = false; });
    $('bpm').value = st.bpm; $('key').value = st.key; $('scale').value = 'auto'; $('swing').value = st.swing; $('swingv').textContent = st.swing + '%';
    document.querySelectorAll('.sty').forEach(b => b.setAttribute('aria-pressed', b.dataset.id === id));
    applyAccent(); setBeat(); buildVars(); generate();
    toast(T('styleSel', { s: S.artist, v: HG.stxt(id).vars[0].n }));
  }
  UI.selectStyle = selectStyle;

  function bindControls() {
    MB.NOTES.forEach((n, i) => $('key').add(new Option(n, i)));
    $('key').value = st.key;
    const lsel = $('lang'); HG.LANGS.forEach(([c, n]) => { const o = new Option(n, c); o.lang = c; lsel.add(o); });
    lsel.addEventListener('change', () => applyLang(lsel.value));
    const sl = (id, key, suffix) => { const e = $(id), v = $(id + 'v'); const upd = () => { st[key] = +e.value; v.textContent = e.value + (suffix || '%'); }; e.addEventListener('input', upd); upd(); return e; };
    ['orig', 'energy', 'human', 'swing', 'thick'].forEach(id => sl(id, id).addEventListener('change', () => { if (id === 'thick') { rt.opts.thick = st.thick / 100; if (rt.ac) rt.ac.__thick = st.thick / 100; } generate(); }));
    sl('pump', 'pump').addEventListener('input', () => { rt.opts.pumpAmt = st.pump / 100; });
    $('key').addEventListener('change', () => { st.key = +$('key').value; generate(); });
    $('scale').addEventListener('change', () => { st.scale = $('scale').value; generate(); });
    $('bars').addEventListener('change', () => { st.bars = +$('bars').value; generate(); });
    $('sevenths').addEventListener('change', () => { st.sevenths = $('sevenths').checked; generate(); });
    $('bpm').addEventListener('change', () => { st.bpm = Math.max(60, Math.min(190, parseInt($('bpm').value, 10) || 120)); $('bpm').value = st.bpm; setBeat(); if (res) { res.P.bpm = st.bpm; rebuildMix(); } });
    $('seed').addEventListener('change', () => { st.seed = Math.max(1, Math.min(9999999, Math.abs(parseInt($('seed').value, 10)) || 1)); $('seed').value = st.seed; generate(); });
    const newIdea = () => { st.seed = 1 + Math.floor(Math.random() * 99999); $('seed').value = st.seed; generate(); };
    $('gen').addEventListener('click', newIdea);
    $('play').addEventListener('click', UI.toggle);
    $('quality').value = st.quality;
    $('quality').addEventListener('change', () => { st.quality = $('quality').value; store.set('quality', st.quality); rebuildMix(); });
    const setFs = v => { st.fs = Math.max(.85, Math.min(1.5, Math.round(v * 100) / 100)); document.documentElement.style.setProperty('--fs', st.fs); store.set('fs', st.fs); requestAnimationFrame(drawAll); };
    $('fs-').addEventListener('click', () => setFs(st.fs - .1)); $('fs+').addEventListener('click', () => setFs(st.fs + .1));
    UI.setFs = setFs;
    document.addEventListener('keydown', e => {
      const t = e.target && e.target.tagName; if (t === 'INPUT' || t === 'SELECT' || t === 'TEXTAREA' || e.ctrlKey || e.metaKey || e.altKey) return;
      if (e.code === 'Space' && t !== 'BUTTON' && !(e.target.getAttribute && e.target.getAttribute('role') === 'slider')) { e.preventDefault(); UI.toggle(); }
      else if ((e.key === 'n' || e.key === 'N') && $('p-sound').hidden) { e.preventDefault(); newIdea(); }
    });
    $('theme').addEventListener('click', () => {
      const modes = ['system', 'light', 'dark']; theme = modes[(modes.indexOf(theme) + 1) % 3];
      store.set('theme', theme); applyTheme(true);
    });
    if (window.matchMedia) { const mq = matchMedia('(prefers-color-scheme: dark)'); if (mq.addEventListener) mq.addEventListener('change', () => applyTheme(true)); }
  }
  function applyTheme(redraw) {
    const r = document.documentElement; if (theme === 'system') r.removeAttribute('data-theme'); else r.dataset.theme = theme;
    $('theme').textContent = T('theme') + ': ' + T({ system: 'thSystem', light: 'thLight', dark: 'thDark' }[theme]);
    applyAccent(); if (redraw !== false) requestAnimationFrame(() => { drawAll(); if (UI.onTheme) UI.onTheme(); });
  }
  UI.init1 = function () {
    theme = store.get('theme') || 'system';
    const q = store.get('quality'); if (q === 'eco' || q === 'bal' || q === 'high') st.quality = q;
    const fs = parseFloat(store.get('fs')); if (fs > .8 && fs < 1.6) { st.fs = fs; document.documentElement.style.setProperty('--fs', fs); }
    HG.lang = pickLang();
    bindControls(); translateStatic();
    const r = document.documentElement; r.lang = HG.lang; r.dir = HG.RTL[HG.lang] ? 'rtl' : 'ltr'; $('lang').value = HG.lang;
    buildNodes(); buildCards(); buildGroups(); buildWall(); buildVars(); setBeat(); applyTheme(false);
    if (window.ResizeObserver) { let p = 0; new ResizeObserver(() => { cancelAnimationFrame(p); p = requestAnimationFrame(drawAll); }).observe(document.querySelector('.app')); }
    generate(); cancelAnimationFrame(UI.raf); UI.raf = requestAnimationFrame(frame);
    document.addEventListener('visibilitychange', () => { if (document.hidden && rt.playing) { /* zamanlayıcı arka planda yavaşlar; dönüşte pumpTick toparlar */ } });
  };
})();
