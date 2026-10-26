// ---- Arayüz 3/3: ses bankası, dışa aktarma, aranjman, sekmeler, başlangıç
(function () {
  const { $, st, snd, patches, KEYS, toast } = UI;
  const T = (k, v) => HG.t(k, v);
  const res = () => UI.res();
  UI.bankCat = 'all'; let bankOnly = false;

  // ---------- Sekmeler (ok tuşlarıyla gezilebilir)
  const TABS = ['hit', 'sound', 'bank', 'export'];
  UI.tab = function (name, focus) {
    document.querySelectorAll('.tab').forEach(t => { const on = t.dataset.tab === name; t.setAttribute('aria-selected', on); t.tabIndex = on ? 0 : -1; if (on && focus) t.focus(); });
    document.querySelectorAll('.pane').forEach(p => { p.hidden = p.id !== 'p-' + name; });
    if (name === 'hit') requestAnimationFrame(UI.drawAll);
    if (name === 'sound') UI.editorRefresh();
    if (name === 'bank') { if (UI.bankCat === 'Davul') UI.bankCat = 'Davul'; buildBankBar(); renderBank(); }
    if (name === 'export') renderArr();
  };
  document.querySelectorAll('.tab').forEach(t => {
    t.addEventListener('click', () => UI.tab(t.dataset.tab));
    t.addEventListener('keydown', e => {
      const i = TABS.indexOf(t.dataset.tab), rtl = document.documentElement.dir === 'rtl';
      let j = -1;
      if (e.key === 'ArrowRight') j = rtl ? i - 1 : i + 1; else if (e.key === 'ArrowLeft') j = rtl ? i + 1 : i - 1; else if (e.key === 'Home') j = 0; else if (e.key === 'End') j = TABS.length - 1;
      if (j < 0 && e.key !== 'ArrowLeft' && e.key !== 'ArrowRight') return;
      e.preventDefault(); UI.tab(TABS[(j + TABS.length) % TABS.length], true);
    });
  });

  // ---------- Ses bankası
  const RM = { c: 'chords', b: 'bass', m: 'melody' };
  function buildBankBar() {
    const bar = $('bankbar'); bar.innerHTML = '';
    [['all', T('gAll')], ['sig', T('sig')]].concat(MB.CATS.map(c => [c, T('cat_' + c)]), [['Davul', T('cat_Davul')]]).forEach(([c, lab]) => {
      const b = document.createElement('button'); b.type = 'button'; b.className = 'btn sm' + (c === UI.bankCat ? ' on' : ''); b.textContent = lab; b.setAttribute('aria-pressed', c === UI.bankCat);
      b.addEventListener('click', () => { UI.bankCat = c; buildBankBar(); renderBank(); }); bar.appendChild(b);
    });
    const l = document.createElement('label'); l.className = 'checkrow'; l.innerHTML = '<input type="checkbox"> <span></span>'; l.querySelector('span').textContent = T('onlyStyle');
    const cb = l.querySelector('input'); cb.checked = bankOnly; cb.addEventListener('change', () => { bankOnly = cb.checked; renderBank(); }); bar.appendChild(l);
  }
  function renderBank() {
    const host = $('bank'); host.innerHTML = '';
    const S = HG.STYLE[st.style], used = Object.values(S.sound);
    let items = MB.SOUNDS.map(s => ({ id: s.id, cat: s.cat, roles: s.r.split('').map(c => RM[c]), sig: HG.SIG.test(s.id), fit: used.includes(s.id) }));
    MB.KITS_LIST.forEach(x => items.push({ id: x.id, cat: 'Davul', roles: ['drums'], sig: false, fit: x.id === S.sound.drums }));
    if (UI.bankCat === 'sig') items = items.filter(i => i.sig); else if (UI.bankCat !== 'all') items = items.filter(i => i.cat === UI.bankCat);
    if (bankOnly) items = items.filter(i => i.fit);
    $('bankcount').textContent = T('nSounds', { n: items.length });
    const frag = document.createDocumentFragment();
    items.forEach(i => {
      const el = document.createElement('div'); el.className = 'bc' + (i.sig ? ' sig' : ''); el.setAttribute('role', 'group');
      const nm = HG.soundName(i.id, HG.lang); el.setAttribute('aria-label', nm);
      el.innerHTML = '<div><b></b><br><span></span></div><div class="bb"></div>';
      el.querySelector('b').textContent = nm; el.querySelector('span').textContent = T('cat_' + i.cat) + (i.sig ? ' · ' + T('sigTag') : '');
      const bb = el.querySelector('.bb');
      i.roles.forEach(r => {
        const b = document.createElement('button'); b.type = 'button'; b.className = 'btn sm' + (snd[r] === i.id ? ' on' : ''); b.textContent = '▸ ' + T(r); b.title = T('assign', { r: T(r) }); b.setAttribute('aria-label', nm + ': ' + T('assign', { r: T(r) })); b.setAttribute('aria-pressed', snd[r] === i.id);
        b.addEventListener('click', () => { UI.assign(r, i.id, true); UI.audition(r); renderBank(); }); bb.appendChild(b);
      });
      if (i.cat !== 'Davul') {
        const e = document.createElement('button'); e.type = 'button'; e.className = 'btn sm'; e.textContent = T('design'); e.title = T('openDesign'); e.setAttribute('aria-label', nm + ': ' + T('openDesign'));
        e.addEventListener('click', () => { const r = i.roles[0]; UI.assign(r, i.id, true); UI.editTrack(r); UI.tab('sound'); }); bb.appendChild(e);
      }
      frag.appendChild(el);
    });
    host.appendChild(frag);
  }

  // ---------- Dışa aktarma
  const slug = s => s.toLowerCase().normalize('NFD').replace(/[̀-ͯ]/g, '').replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '');
  function baseName() { const P = res().P; return 'HitGen_' + slug(res().style.artist) + '_' + MB.NOTES[P.key].replace('#', 's') + '_' + P.scale + '_' + P.bpm + 'bpm_v' + (st.vi + 1) + '_seed' + P.seed; }
  function readme(extra) {
    const r = res(), P = r.P, ki = MB.keyInfo(P.key, P.scale), S = r.style, tx = HG.stxt(S.id), V = tx.vars[st.vi] || tx.vars[0], sn = id => HG.soundName(id, HG.lang);
    const pad = (k) => (T(k) + '               ').slice(0, 15) + ': ';
    return ['WAVEJACKERS HITGEN', '==================', '',
      pad('rmStyle') + T('rmInspired', { a: S.artist, t: tx.tag, v: V.n }),
      pad('rmKey') + MB.NOTES[P.key] + ' ' + T('scale_' + P.scale), pad('rmScaleNotes') + ki.notes.join(' '), pad('rmCamelot') + ki.cam + '  (' + T('rmCompat') + ': ' + ki.compat.join(', ') + ')',
      pad('rmTempo') + T('rmTempoV', { bpm: P.bpm, bars: P.bars, seed: P.seed }),
      pad('rmChords') + r.chords.map(c => c.name).join(' | '), pad('rmRoman') + r.chords.map(c => c.roman).join(' | '),
      pad('rmSound') + T('chords') + ' ' + sn(snd.chords) + ', ' + T('bass') + ' ' + sn(snd.bass) + ', ' + T('melody') + ' ' + sn(snd.melody) + ', ' + T('drums') + ' ' + sn(snd.drums), '  ' + T('rmNoAudio'), '',
      T('rmFiles'), '  *_chords.mid / _bass.mid / _drums.mid / _melody.mid : ' + T('rmF1'), '  *_tum-izler_format1.mid : ' + T('rmF2'), '  *_tek-iz_format0.mid    : ' + T('rmF3'), '  *_tam-parca.mid         : ' + T('rmF4'), '',
      T('rmDrum'), '  Kick 36 (C1), Rim 37, Snare 38, Clap 39, Closed Hat 42, Open Hat 46, Perc 64', '',
      T('rmNote'), T('rmFree')].concat(extra ? ['', extra] : []).join('\r\n');
  }
  const utf = s => new TextEncoder().encode('﻿' + s);
  const readmeFile = extra => ({ name: T('readme'), data: utf(readme(extra)) });
  async function save(name, bytes, ok) {
    const dl = UI.dl; if (!dl) { toast(T('dlOff')); return; }
    try { await dl.save({ filename: name, data: new Blob([bytes], { type: 'application/zip' }) }); toast(T('saved', { f: name }) + (ok == null ? T('zipMid') : ok)); }
    catch (e) { toast(e && e.code === 'declined' ? T('declined') : T('dlFail', { c: (e && e.code) || '?' })); }
  }
  async function dlLoop() {
    const base = baseName(), files = KEYS.map(k => ({ name: base + '_' + k + '.mid', data: MB.midiFile(res(), [k]) }));
    files.push({ name: base + '_tum-izler_format1.mid', data: MB.midiFile(res(), KEYS) }, { name: base + '_tek-iz_format0.mid', data: MB.midiFile(res(), KEYS, true) }, readmeFile());
    await save(base + '_midi.zip', MB.zip(files));
  }
  async function dlSong() {
    const base = baseName(), a = HG.assemble(res()), files = [{ name: base + '_tam-parca.mid', data: MB.midiFile(a, KEYS) }];
    KEYS.forEach(k => files.push({ name: base + '_tam-' + k + '.mid', data: MB.midiFile(a, [k]) }));
    files.push(readmeFile(T('rmFull') + ': ' + a.sections.map(s => s.name + ' ' + s.bars).join(', ') + ' (' + a.bars + ')'));
    await save(base + '_tam-parca.zip', MB.zip(files));
  }
  async function render(mutedMap) {
    const OAC = window.OfflineAudioContext || window.webkitOfflineAudioContext; if (!OAC) throw new Error('no');
    const r = res(), sr = 44100, secs = r.bars * 240 / r.P.bpm + 3, oc = new OAC(2, Math.ceil(sr * secs), sr);
    oc.__q = 'high';                                               // dışa aktarımda her zaman tam kalite
    const mx = MB.mix(oc, oc.destination, r, snd, { muted: mutedMap || {}, patches, pump: r.style.pump, pumpAmt: st.pump / 100, thick: st.thick / 100, quality: 'high' });
    mx.schedule(0.02, {}); return MB.wav(await oc.startRendering());
  }
  let busy = false;
  async function dlWav() {
    if (busy) return; busy = true; toast(T('wavPrep')); const base = baseName();
    try { const wav = await render(); await save(base + '_wav.zip', MB.zip([{ name: base + '_onizleme.wav', data: wav }, readmeFile()]), T('zipWav')); } catch (e) { toast(T('wavFail')); }
    busy = false;
  }
  async function dlStems() {
    if (busy) return; busy = true; const base = baseName(), files = [];
    try {
      for (let i = 0; i < KEYS.length; i++) {
        toast(T('stemPrep', { t: T(KEYS[i]), i: i + 1 })); const m = {}; KEYS.forEach(k => { if (k !== KEYS[i]) m[k] = true; });
        files.push({ name: base + '_' + KEYS[i] + '.wav', data: await render(m) });
      }
      files.push(readmeFile()); await save(base + '_izler-wav.zip', MB.zip(files), T('zipStems'));
    } catch (e) { toast(T('stemFail')); }
    busy = false;
  }
  UI.onDl = function () { ['dl-loop', 'dl-song', 'dl-wav', 'dl-stems'].forEach(id => { $(id).disabled = !UI.dl; }); if (!UI.dl) toast(T('dlOff')); };

  // ---------- Sürükle-bırak (.mid); klavyeyle Enter zip indirir
  const toB64 = b => { let s = ''; for (let i = 0; i < b.length; i += 4096) s += String.fromCharCode.apply(null, b.subarray(i, i + 4096)); return btoa(s); };
  document.addEventListener('dragstart', e => {
    const el = e.target && e.target.closest ? e.target.closest('[data-drag]') : null; if (!el || !res()) return;
    const k = el.dataset.drag, keys = k === 'all' ? KEYS : [k], name = baseName() + '_' + (k === 'all' ? 'tum-izler' : k) + '.mid', bytes = MB.midiFile(res(), keys);
    try { e.dataTransfer.items.add(new File([bytes], name, { type: 'audio/midi' })); } catch (x) {}
    try { e.dataTransfer.setData('DownloadURL', 'audio/midi:' + name + ':data:audio/midi;base64,' + toB64(bytes)); } catch (x) {}
    e.dataTransfer.effectAllowed = 'copy'; toast(T('dragging', { f: name }));
  });
  document.addEventListener('keydown', e => {
    const el = e.target && e.target.closest ? e.target.closest('[data-drag]') : null;
    if (el && (e.key === 'Enter' || e.key === ' ')) { e.preventDefault(); dlLoop(); }
  });

  // ---------- Aranjman
  const mmss = s => Math.floor(s / 60) + ':' + ('0' + Math.round(s % 60)).slice(-2);
  const secColor = n => /intro|outro/i.test(n) ? 'var(--grid2)' : /break/i.test(n) ? 'var(--c-chords)' : /build|verse/i.test(n) ? 'var(--c-melody)' : /drop|peak|groove|hook|main/i.test(n) ? 'var(--c-drums)' : 'var(--c-bass)';
  function renderArr() {
    const r = res(), S = r.style, tot = S.arr.reduce((s, x) => s + x[1], 0), bs = 240 / r.P.bpm; let at = 0;
    const arr = $('arr'), mx = $('mx'); arr.innerHTML = ''; mx.innerHTML = '<span></span>' + KEYS.map(k => '<span style="color:var(--c-' + k + ')"></span>').join('');
    mx.querySelectorAll('span').forEach((e, i) => { if (i) e.textContent = T(KEYS[i - 1]); });
    const rtl = document.documentElement.dir === 'rtl';
    S.arr.forEach(([n, b]) => {
      const row = document.createElement('div'); row.className = 'ar';
      row.innerHTML = '<span></span><div class="tl" aria-hidden="true"><i></i></div><span class="t" dir="ltr"></span>';
      row.children[0].textContent = n; const i = row.querySelector('i'); i.style[rtl ? 'right' : 'left'] = (at / tot * 100) + '%'; i.style.width = (b / tot * 100) + '%'; i.style.setProperty('--sc', secColor(n));
      row.children[2].textContent = T('bar') + ' ' + (at + 1) + '–' + (at + b) + ' · ' + mmss(at * bs);
      arr.appendChild(row);
      const m = HG.maskFor(n, Math.floor(b / 2), b);
      const lab = document.createElement('span'); lab.textContent = n; mx.appendChild(lab);
      KEYS.forEach(k => { const c = document.createElement('i'); if (k === 'drums' ? m.drums !== 'none' : m[k]) c.className = 'on'; c.style.setProperty('--tc', 'var(--c-' + k + ')'); mx.appendChild(c); });
      at += b;
    });
    const p = document.createElement('p'); p.className = 'fine'; p.textContent = T('arrTotal', { b: tot, t: mmss(tot * bs), bpm: r.P.bpm }); arr.appendChild(p);
  }
  const DAWS = [['FL Studio', 'daw_fl'], ['Ableton Live', 'daw_ab'], ['Logic Pro', 'daw_lo'], ['Cubase', 'daw_cu'], ['Reaper', 'daw_re'], ['Bitwig', 'daw_bi'], ['Studio One', 'daw_so'], ['GarageBand', 'daw_ga']];
  function renderDaws() { const h = $('daws'); h.innerHTML = ''; DAWS.forEach(([n, k]) => { const d = document.createElement('div'); d.className = 'daw'; d.innerHTML = '<b></b><span></span>'; d.firstChild.textContent = n; d.lastChild.textContent = T(k); h.appendChild(d); }); }
  UI.onLang2 = function () { renderDaws(); if (!$('p-bank').hidden) { buildBankBar(); renderBank(); } if (!$('p-export').hidden) renderArr(); };

  // ---------- Başlat
  $('dl-loop').addEventListener('click', dlLoop); $('dl-song').addEventListener('click', dlSong); $('dl-wav').addEventListener('click', dlWav); $('dl-stems').addEventListener('click', dlStems);
  UI.init1(); renderDaws(); UI.init2();
  UI.test = { baseName, readme, render, renderBank };
})();
