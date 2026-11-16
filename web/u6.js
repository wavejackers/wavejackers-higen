// ---- Arayüz: Koro sekmesi. Sesini kaydet ya da yükle; koro o anki fikrin akorlarını izler, ritme kilitli çalar.
// Sahnedeki melekler partilerdir (bas, tenor, alto, soprano): tıklayınca parti açılır / kapanır.
(function () {
  const { $, st, rt, toast, store } = UI, T = (k, v) => HG.t(k, v), C = HG.Choir;
  const KN = [['level', 'chLevel', 0, 1], ['space', 'chSpace', 0, 1], ['detune', 'chDetune', 0, 1], ['air', 'chAir', 0, 1], ['vibrato', 'chVibrato', 0, 1], ['attack', 'chAttack', 0, 1], ['character', 'chChar', -1, 1]];
  const PARTS = ['chBass', 'chTenor', 'chAlto', 'chSoprano'];
  const PRESETS = [
    { size: 4, space: .8, detune: .4, air: .35, vibrato: .3, attack: .55, src: 0 },
    { part: [true, true, false, false], size: 4, space: .7, detune: .25, air: .2, vibrato: .1, attack: .5, character: -.3, src: 1 },
    { part: [false, false, true, true], size: 3, space: .75, detune: .35, air: .7, vibrato: .45, attack: .45, character: .35, src: 0 },
    { rhythm: 3, size: 3, space: .3, detune: .3, air: .5, vibrato: .15, attack: 0, src: 0 }];
  let s = C.defaults(), voice = null, voiceName = '', loop = null, want = '', busy = 0, src = null, gain = null, playing = false, t0 = 0, rec = null, stage = null, raf = 0, shown = false;
  try { const v = JSON.parse(store.get('choir') || 'null'); if (v) Object.assign(s, v, { source: Math.min(v.source | 0, 3) }); } catch (e) {}
  const save = () => store.set('choir', JSON.stringify(s));
  const status = m => { $('ch-status').textContent = m; };
  const chords = () => { const r = UI.res(); return r ? r.chords.slice(0, r.bars).map(c => ({ pc: c.pc, third: c.third, fifth: c.fifth })) : []; };
  const voiceLine = () => {
    if (!voice) return T('chVoiceNone');
    if (!voice.f0) return voiceName + '  ·  ' + T('chNoPitch');
    const m = Math.round(69 + 12 * Math.log2(voice.f0 / 440));
    return voiceName + '  ·  ' + MB.NOTES[((m % 12) + 12) % 12] + (Math.floor(m / 12) - 1) + '  ·  ' + Math.round(voice.f0) + ' Hz';
  };

  // ---------- Döngü: ayar, akor ya da tempo değişince arka planda yeniden işlenir (en son istek kazanır)
  async function rerender() {
    const r = UI.res(); if (!r) return;
    const key = JSON.stringify([s, chords(), st.bpm, !!voice, voiceName]); if (key === want) return; want = key;
    const my = ++busy, sr = (rt.ac && rt.ac.sampleRate) || 48000;
    const pause = () => new Promise(ok => setTimeout(ok, 0)).then(() => { if (my !== busy) throw new Error('stale'); });
    try {
      const L = await C.render(s, chords(), s.source >= 4 ? voice : null, st.bpm, sr, pause);
      if (my !== busy) return;
      loop = L; if (playing) startSource();
    } catch (e) { if (e.message !== 'stale') status(T('audioFail')); }
  }
  let rT = 0; const soon = () => { clearTimeout(rT); rT = setTimeout(rerender, 160); };

  // ---------- Çalma: ritim çalıyorsa koro aynı ölçüye kilitlenir
  function startSource() {
    const ac = rt.ac; if (!ac || !loop) return;
    const buf = ac.createBuffer(2, loop.N, loop.sr); buf.copyToChannel(loop.L, 0); buf.copyToChannel(loop.R, 1);
    const old = src, oldG = gain, now = ac.currentTime + .05, dur = loop.N / loop.sr;
    gain = ac.createGain(); gain.connect(ac.destination);
    src = ac.createBufferSource(); src.buffer = buf; src.loop = true; src.connect(gain);
    let off = 0;
    if (rt.playing && rt.log.length) { const [lt, loc] = rt.log[0], step = 60 / st.bpm / 4; off = ((loc * step + (now - lt)) % dur + dur) % dur; }
    else if (old) off = ((now - t0) % dur + dur) % dur;
    t0 = now - off; src.start(now, off);
    if (old) { oldG.gain.setTargetAtTime(0, now, .03); old.stop(now + .3); }
  }
  function setPlaying(on) {
    if (on && !UI.ensureMix()) return;
    playing = on; $('ch-play').setAttribute('aria-pressed', on); $('ch-play').classList.toggle('on', on);
    if (on) { if (loop) startSource(); else rerender(); }
    else if (src) { gain.gain.setTargetAtTime(0, rt.ac.currentTime, .05); src.stop(rt.ac.currentTime + .4); src = null; }
  }
  const activity = i => { if (!playing || !loop || !rt.ac) return 0; const a = loop.act[i], dur = loop.N / loop.sr, f = (((rt.ac.currentTime - t0) % dur) + dur) % dur / dur; return a[Math.min(a.length - 1, Math.floor(f * a.length))]; };

  // ---------- Ses: kayıt (mikrofon) ve dosya
  async function useBuffer(ab, name) {
    const mono = new Float32Array(ab.length); for (let c = 0; c < ab.numberOfChannels; c++) { const d = ab.getChannelData(c); for (let i = 0; i < d.length; i++) mono[i] += d[i] / ab.numberOfChannels; }
    const v = C.analyse(mono, ab.sampleRate);
    if (v.mark.length < 4) { status(T('chRecFail')); return false; }
    voice = v; voiceName = name; if (s.source < 4) s.source = 4;
    syncControls(); drawWave(); status(voiceLine()); soon(); return true;
  }
  async function toggleRecord() {
    const btn = $('ch-rec');
    if (rec) { rec.stop(); return; }
    if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia || !window.MediaRecorder) { status(T('chNoMic')); return; }
    let stream; try { stream = await navigator.mediaDevices.getUserMedia({ audio: { echoCancellation: false, noiseSuppression: false, autoGainControl: false } }); }
    catch (e) { status(T(e && e.name === 'NotAllowedError' ? 'chMicDenied' : 'chNoMic')); return; }
    const ac = UI.ensureMix() ? rt.ac : null, chunks = [], mr = new MediaRecorder(stream), began = performance.now();
    let an = null, meter = 0; if (ac) { an = ac.createAnalyser(); an.fftSize = 1024; ac.createMediaStreamSource(stream).connect(an); }
    const buf = new Float32Array(1024);
    meter = setInterval(() => {
      const sec = (performance.now() - began) / 1000; let pk = 0; if (an) { an.getFloatTimeDomainData(buf); for (const x of buf) pk = Math.max(pk, Math.abs(x)); }
      status('● ' + sec.toFixed(1) + ' s  ' + '|'.repeat(Math.min(24, Math.round(pk * 30))));
      if (sec >= C.MAX_SECONDS - .1) mr.stop();
    }, 100);
    mr.ondataavailable = e => { if (e.data.size) chunks.push(e.data); };
    mr.onstop = async () => {
      clearInterval(meter); stream.getTracks().forEach(t => t.stop()); rec = null; btn.textContent = T('chRecord'); btn.classList.remove('on');
      try { const ab = await rt.ac.decodeAudioData(await new Blob(chunks).arrayBuffer()); await useBuffer(ab, 'Voice'); }
      catch (e) { status(T('chRecFail')); }
    };
    rec = mr; mr.start(); btn.textContent = T('chStop'); btn.classList.add('on'); status(T('chRecording'));
  }
  async function loadFile(f) {
    if (!f || !UI.ensureMix()) return;
    try { const ab = await rt.ac.decodeAudioData(await f.arrayBuffer()); if (!await useBuffer(ab, f.name.replace(/\.[^.]+$/, ''))) status(T('lfLoadFail')); }
    catch (e) { status(T('lfLoadFail')); }
  }
  async function download() {
    if (!loop) { await rerender(); if (!loop) return; }
    const wav = MB.wav({ numberOfChannels: 2, length: loop.N, sampleRate: loop.sr, getChannelData: c => (c ? loop.R : loop.L) });
    const name = 'HitGen Choir ' + loop.bars + 'bar ' + Math.round(loop.bpm) + 'bpm.wav';
    if (!UI.dl) { toast(T('dlOff')); status(T('dlOff')); return; }
    try { await UI.dl.save({ filename: name, data: new Blob([wav], { type: 'audio/wav' }) }); status(T('saved', { f: name })); }
    catch (e) { status(e && e.code === 'declined' ? T('declined') : T('dlFail', { c: (e && e.code) || '?' })); }
  }

  // ---------- Denetimler
  function fill(sel, keys, val) { sel.innerHTML = ''; keys.forEach((k, i) => { const o = document.createElement('option'); o.value = i; o.textContent = k; sel.appendChild(o); }); sel.value = val; }
  function build() {
    fill($('ch-src'), ['chAah', 'chOoh', 'chEeh', 'chMmm', 'chOwnHold', 'chOwnWords'].map(k => T(k)), s.source);
    fill($('ch-rhy'), ['chHold', 'chHalf', 'chQuarter', 'chStab'].map(k => T(k)), s.rhythm);
    fill($('ch-size'), [1, 2, 3, 4].map(n => T('chSize', { n })), s.size - 1);
    fill($('ch-pre'), [T('chPreset')].concat(['chPre0', 'chPre1', 'chPre2', 'chPre3'].map(k => T(k))), 0);
    $('ch-rec').title = T('chRecordTip'); $('ch-cv').title = T('chStageTip'); $('ch-rec').textContent = T(rec ? 'chStop' : 'chRecord');
    const kn = $('ch-knobs'); kn.innerHTML = '';
    KN.forEach(([id, key, lo, hi]) => {
      const f = document.createElement('div'); f.className = 'field';
      f.innerHTML = '<label class="lab"><span></span> <em></em></label><input type="range" min="' + lo * 100 + '" max="' + hi * 100 + '" step="1">';
      const r = f.querySelector('input'), em = f.querySelector('em'); r.id = 'ch-' + id; f.querySelector('label').htmlFor = r.id; f.querySelector('span').textContent = T(key);
      r.value = Math.round(s[id] * 100); em.textContent = r.value + '%';
      r.addEventListener('input', () => { s[id] = r.value / 100; em.textContent = r.value + '%'; save(); soon(); });
      kn.appendChild(f);
    });
    const tags = $('ch-tags'); tags.innerHTML = '';
    PARTS.forEach((k, i) => { const b = document.createElement('button'); b.type = 'button'; b.className = 'btn sm'; b.id = 'ch-tag' + i; b.textContent = T(k); b.addEventListener('click', () => togglePart(i)); tags.appendChild(b); });
    syncControls(); status(voiceLine());
  }
  function syncControls() {
    $('ch-src').value = s.source; $('ch-rhy').value = s.rhythm; $('ch-size').value = s.size - 1;
    KN.forEach(([id]) => { const r = $('ch-' + id); if (r) { r.value = Math.round(s[id] * 100); r.previousElementSibling.querySelector('em').textContent = r.value + '%'; } });
    PARTS.forEach((k, i) => { const b = $('ch-tag' + i); if (b) b.setAttribute('aria-pressed', s.part[i]); });
  }
  function togglePart(i) { s.part[i] = !s.part[i]; save(); syncControls(); soon(); }
  function applyPreset(i) {
    const p = PRESETS[i], own = s.source >= 4 && voice;
    Object.assign(s, { part: [true, true, true, true], rhythm: 0, character: 0 }, p); delete s.src;
    if (!own) s.source = p.src;
    save(); syncControls(); soon();
  }
  $('ch-src').addEventListener('change', e => { s.source = +e.target.value; if (s.source >= 4 && !voice) status(T('chNeedVoice')); save(); soon(); });
  $('ch-rhy').addEventListener('change', e => { s.rhythm = +e.target.value; save(); soon(); });
  $('ch-size').addEventListener('change', e => { s.size = +e.target.value + 1; save(); soon(); });
  $('ch-pre').addEventListener('change', e => { const i = +e.target.value - 1; e.target.value = 0; if (i >= 0) applyPreset(i); });
  $('ch-rec').addEventListener('click', toggleRecord);
  $('ch-file').addEventListener('change', e => { loadFile(e.target.files[0]); e.target.value = ''; });
  $('ch-play').addEventListener('click', () => setPlaying(!playing));
  $('ch-dl').addEventListener('click', download);
  const pane = $('p-choir');
  pane.addEventListener('dragover', e => { e.preventDefault(); });
  pane.addEventListener('drop', e => { e.preventDefault(); const f = e.dataTransfer && e.dataTransfer.files[0]; if (f) loadFile(f); });

  // ---------- Sahne ve dalga
  function drawWave() {
    const c = $('ch-wave'), w = c.clientWidth, h = c.clientHeight; if (!w) return;
    const dpr = Math.min(2, window.devicePixelRatio || 1); c.width = Math.round(w * dpr); c.height = Math.round(h * dpr);
    const x = c.getContext('2d'), cs = getComputedStyle(document.documentElement); x.setTransform(dpr, 0, 0, dpr, 0, 0); x.clearRect(0, 0, w, h);
    if (!voice) { x.fillStyle = cs.getPropertyValue('--mute'); x.font = '12px sans-serif'; x.textAlign = 'center'; x.fillText(T('chDrop'), w / 2, h / 2 + 4); return; }
    const d = voice.x, n = Math.floor(w - 8); x.fillStyle = s.source >= 4 ? cs.getPropertyValue('--accent') : cs.getPropertyValue('--mute');
    let mx = 1e-4; const pk = new Float32Array(n);
    for (let i = 0; i < n; i++) { const a = Math.floor(i * d.length / n), z = Math.max(a + 1, Math.floor((i + 1) * d.length / n)); for (let k = a; k < z; k++) pk[i] = Math.max(pk[i], Math.abs(d[k])); mx = Math.max(mx, pk[i]); }
    for (let i = 0; i < n; i++) { const y = pk[i] / mx * (h / 2 - 4); x.fillRect(4 + i, h / 2 - y, 1, y * 2 + 1); }
  }
  const smooth = [0, 0, 0, 0]; let lvl = 0, clock = 0, frameN = 0, img = null, off = null;
  function frame() {
    raf = requestAnimationFrame(frame);
    if (!shown) return;
    const motion = document.documentElement.dataset.motion !== 'off', eco = st.quality === 'eco';
    if (!motion && img && frameN > 0) return;
    frameN++; if (eco && frameN % 2) return;
    const cv = $('ch-cv'), w = cv.clientWidth, h = cv.clientHeight; if (!w || !h) return;
    if (!stage) stage = HG.Stage3D();
    const dpr = Math.min(1.5, window.devicePixelRatio || 1), W = Math.round(w * dpr), H = Math.round(h * dpr);
    if (cv.width !== W || cv.height !== H) { cv.width = W; cv.height = H; img = null; }
    if (!img) { off = document.createElement('canvas'); off.width = W; off.height = H; img = off.getContext('2d').createImageData(W, H); }
    // seviye: ana karışımın analizörü (ritim çalarken) ya da koronun kendi etkinliği
    let pk = 0; const an = rt.mx && rt.mx.an;
    if (an && rt.playing) { const b = new Float32Array(an.fftSize); an.getFloatTimeDomainData(b); for (let i = 0; i < b.length; i += 4) pk = Math.max(pk, Math.abs(b[i])); }
    for (let i = 0; i < 4; i++) { const a = playing ? activity(i) : lvl * .55; smooth[i] += (a - smooth[i]) * (a > smooth[i] ? .4 : .15); pk = Math.max(pk, smooth[i] * .7); }
    lvl += (Math.min(1, pk / .891) - lvl) * .35; clock += 1 / 60;
    let beat = 0; if (rt.ac) beat = (((rt.ac.currentTime - (playing ? t0 : 0)) * st.bpm / 60) % 1 + 1) % 1;
    const q = motion ? { t: clock, beat, level: lvl, voice: smooth.slice(), on: s.part } : { t: 0, beat: 0, level: 0, voice: [0, 0, 0, 0], on: s.part };
    stage.render(img, q, eco ? 1 : 2); off.getContext('2d').putImageData(img, 0, 0);
    // arka plan: gökten inen ışık, bulutlar, ışıltılar; sonra figürler
    const x = cv.getContext('2d'), cs = getComputedStyle(document.documentElement), light = document.documentElement.dataset.theme === 'light' || cs.getPropertyValue('color-scheme').includes('light');
    const g0 = x.createLinearGradient(0, 0, 0, H); g0.addColorStop(0, light ? '#f2ead6' : '#2a2618'); g0.addColorStop(1, light ? '#e2e6ec' : '#121418'); x.fillStyle = g0; x.fillRect(0, 0, W, H);
    const eb = stage.bounds(4), top = eb.x * W + eb.w * W / 2;
    const rg = x.createRadialGradient(top, (eb.y + eb.h * .25) * H, 0, top, (eb.y + eb.h * .25) * H, H * 1.25); rg.addColorStop(0, 'rgba(255,214,120,' + (.2 + .3 * lvl) + ')'); rg.addColorStop(1, 'rgba(255,214,120,0)'); x.fillStyle = rg; x.fillRect(0, 0, W, H);
    for (let i = 0; i < 9; i++) {
      const a = (i - 4) * .2 + .05 * Math.sin(q.t * .3 + i), half = .035 + .012 * Math.sin(q.t * .7 + i * 2);
      x.beginPath(); x.moveTo(top, 0); x.lineTo(top + Math.tan(a - half) * H * 1.2, H * 1.2); x.lineTo(top + Math.tan(a + half) * H * 1.2, H * 1.2); x.closePath();
      x.fillStyle = 'rgba(255,214,120,' + ((.035 + .06 * lvl) * (i % 2 ? .6 : 1)) + ')'; x.fill();
    }
    for (let i = 0; i < 5; i++) {
      const b = stage.bounds(i); if (!b.w) continue; const cx = (b.x + b.w / 2) * W, cy = Math.min(H - 10, (b.y + b.h) * H - 4), cw = (i === 4 ? .3 : .2) * H * 2.2;
      x.fillStyle = light ? 'rgba(255,255,255,.5)' : 'rgba(255,255,255,.1)';
      for (let k = -1; k <= 1; k++) { x.beginPath(); x.ellipse(cx + k * cw * .3, cy, cw / 2, k === 0 ? 17 : 12, 0, 0, 2 * Math.PI); x.fill(); }
    }
    const bloom = k => stage.glows().forEach(gl => { const gx = gl.x * W, gy = gl.y * H, r = gl.r * H, gg = x.createRadialGradient(gx, gy, 0, gx, gy, r); gg.addColorStop(0, 'rgba(255,233,168,' + Math.min(1, gl.a * k) + ')'); gg.addColorStop(1, 'rgba(255,233,168,0)'); x.fillStyle = gg; x.fillRect(gx - r, gy - r, r * 2, r * 2); });
    bloom(1); x.drawImage(off, 0, 0); bloom(.35);
    PARTS.forEach((k, i) => { const b = $('ch-tag' + i), r = stage.bounds(i); if (b && r.w) b.style.left = ((r.x + r.w / 2) * 100) + '%'; });
  }
  $('ch-cv').addEventListener('click', e => { if (!stage) return; const r = e.currentTarget.getBoundingClientRect(), i = stage.hit((e.clientX - r.left) / r.width, (e.clientY - r.top) / r.height); if (i >= 0) togglePart(i); });

  // ---------- Bağlantılar: sekme, dil, akor ve tempo değişimi
  UI.choirShow = on => { shown = on; if (on) { frameN = 0; build(); drawWave(); soon(); cancelAnimationFrame(raf); raf = requestAnimationFrame(frame); } };
  const prevLang = UI.onLang2; UI.onLang2 = function () { if (prevLang) prevLang(); if (shown) build(); };
  setInterval(() => { if (shown || playing) { const key = JSON.stringify([chords(), st.bpm]); if (UI.__chKey !== key) { UI.__chKey = key; soon(); } } }, 400);
  UI.choir = { get: () => s, render: rerender, loaded: () => !!loop, togglePart, applyPreset, useBuffer, setPlaying };
})();
