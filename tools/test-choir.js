// Web korosunun (web/choir-dsp.js) ve 3B sahnenin (web/stage3d.js) sınaması: node tools/test-choir.js
const fs = require('fs'), path = require('path'), web = path.join(__dirname, '..', 'web');
global.MB = {}; global.HG = {};
new Function(fs.readFileSync(path.join(web, 'core.js'), 'utf8').match(/MB\.rng = function[\s\S]*?\n};/)[0])();
for (const f of ['choir-dsp.js', 'stage3d.js']) new Function(fs.readFileSync(path.join(web, f), 'utf8'))();
const C = HG.Choir; let fails = 0; const fail = m => { fails++; console.log('FAIL ' + m); };
const hz = m => 440 * Math.pow(2, (m - 69) / 12), prog = [{ pc: 9, third: 3, fifth: 7 }, { pc: 5, third: 4, fifth: 7 }, { pc: 0, third: 4, fifth: 7 }, { pc: 7, third: 4, fifth: 7 }];
const voic = C.voicing(prog);
if (JSON.stringify(voic) !== JSON.stringify([[45, 52, 60, 69], [41, 53, 60, 69], [48, 52, 60, 67], [43, 50, 59, 67]])) fail('voicing differs from the plugin: ' + JSON.stringify(voic));
function makeVoice(f0, rate, secs) {
  const pad = Math.floor(rate * .1), n = Math.floor(rate * secs), d = new Float32Array(n + pad * 2);
  for (let i = 0; i < n; i++) { let x = 0; for (let h = 1; h * f0 < 5000; h++) { const f = h * f0, g = 1 / h + 2.5 * Math.exp(-Math.pow((f - 1000) / 180, 2)); x += g * Math.sin(2 * Math.PI * f * i / rate + h * .7); } d[pad + i] = x * .12 * Math.min(1, i / (rate * .03)) * Math.min(1, (n - i) / (rate * .05)); }
  return d;
}
const voice = C.analyse(makeVoice(147, 44100, 1.6), 44100);
if (Math.abs(voice.f0 - 147) > 3 || voice.voiced < .6 || voice.mark.length < 100) fail('voice analysis: f0 ' + voice.f0 + ', voiced ' + voice.voiced);
if (C.analyse(new Float32Array(4000), 44100).mark.length) fail('silence should give no voice');
const sr = 48000;
const pitchAt = (L, at) => { const d = new Float32Array(700); for (let i = 0; i < 700; i++) { let s = 0; for (let k = 0; k < 4; k++) s += L.L[at + i * 4 + k] + L.R[at + i * 4 + k]; d[i] = s / 8; } return C.detectPitch(d, 0, 700, sr / 4); };
const band = (L, at, f0, f1) => { let e = 0; for (let f = f0; f <= f1; f += 12) { let re = 0, im = 0; for (let i = 0; i < 8192; i++) { const w = .5 - .5 * Math.cos(2 * Math.PI * i / 8192), x = L.L[at + i] * w; re += x * Math.cos(2 * Math.PI * f * i / sr); im += x * Math.sin(2 * Math.PI * f * i / sr); } e += re * re + im * im; } return e; };
(async () => {
  const dry = Object.assign(C.defaults(), { size: 1, detune: 0, vibrato: 0, space: 0, air: 0, attack: 0, source: 4 });
  for (const part of [0, 3]) {
    const s = Object.assign({}, dry, { part: [part === 0, false, false, part === 3] }), L = await C.render(s, [prog[0]], voice, 120, sr);
    const want = hz(voic[0][part]), got = pitchAt(L, L.N >> 1);
    if (L.N !== Math.round(4 * 60 / 120 * sr)) fail('loop length ' + L.N);
    if (Math.abs(got / want - 1) > .02) fail('part ' + part + ' sings ' + got + ' Hz, expected ' + want);
    if (band(L, L.N >> 1, 800, 1250) < band(L, L.N >> 1, 1700, 2150) * 2) fail('formant moved with the pitch in part ' + part);
  }
  for (let src = 0; src < 6; src++) for (let rh = 0; rh < 4; rh++) {
    const s = Object.assign(C.defaults(), { source: src, rhythm: rh, size: 2 }), L = await C.render(s, prog, voice, 126, sr);
    let pk = 0, step = 0; for (let i = 0; i < L.N; i++) { pk = Math.max(pk, Math.abs(L.L[i]), Math.abs(L.R[i])); if (i) step = Math.max(step, Math.abs(L.L[i] - L.L[i - 1])); }
    if (!isFinite(pk) || pk < .05 || pk > .6 * s.level + 1e-3) fail('source ' + src + ' rhythm ' + rh + ' peak ' + pk);
    if (Math.abs(L.L[0] - L.L[L.N - 1]) > step * 1.05 + 1e-4) fail('loop seam clicks (source ' + src + ', rhythm ' + rh + ')');
    if (!L.act.some(a => a.some(v => v > .5))) fail('part activity is empty');
  }
  {
    const s = Object.assign({}, dry, { source: 0, part: [true, false, false, false] }), L = await C.render(s, [prog[0]], null, 120, sr), got = pitchAt(L, L.N >> 1);
    if (Math.abs(got / hz(voic[0][0]) - 1) > .02) fail('built-in bass sings ' + got);
  }
  // 3B sahne
  const st = HG.Stage3D(), mk = (w, h) => ({ width: w, height: h, data: new Uint8ClampedArray(w * h * 4) }), a = mk(566, 165), b = mk(566, 165);
  const q0 = { t: 0, beat: 0, level: 0, voice: [0, 0, 0, 0], on: [true, true, true, true] }, q1 = { t: 1.3, beat: .25, level: .9, voice: [1, 1, 1, 1], on: [true, true, true, true] };
  st.render(a, q0, 2);
  if (st.triangles() !== 8676) fail('stage triangle count ' + st.triangles() + ' differs from the plugin (8676)');
  let opaque = 0; for (let i = 3; i < a.data.length; i += 4) opaque += a.data[i] > 128 ? 1 : 0;
  if (opaque < 10000) fail('stage: figures not drawn (' + opaque + ')');
  for (let i = 0; i < 5; i++) { const r = st.bounds(i); if (!r.w || r.x < 0 || r.x + r.w > 1 || r.y < 0 || r.y + r.h > 1) fail('stage: figure ' + i + ' is outside the stage'); }
  for (let i = 0; i < 4; i++) { const r = st.bounds(i); if (st.hit(r.x + r.w / 2, r.y + r.h / 2) !== i) fail('stage: angel ' + i + ' not hit'); }
  st.render(b, q1, 2); let diff = 0; for (let i = 0; i < a.data.length; i += 4) diff += a.data[i] !== b.data[i] ? 1 : 0;
  if (diff < 1500) fail('stage: figures do not move with the music');
  if (st.glows().length !== 6) fail('stage: glow count');
  const big = mk(1132, 330), t0 = Date.now(); for (let i = 0; i < 30; i++) { q1.t += .033; st.render(big, q1, 2); }
  console.log('stage: ' + st.triangles() + ' triangles, ' + ((Date.now() - t0) / 30).toFixed(1) + ' ms per frame at 1132x330, 2x supersampling');
  console.log(fails ? 'FAILURES' : 'choir (web): ok'); process.exit(fails ? 1 : 0);
})();
