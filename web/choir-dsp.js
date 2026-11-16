// ---- Koro (ses işleme): eklentideki Source/Choir.cpp ile aynı yöntem.
// Tek sesten (dahili ünlü ya da kullanıcının kaydı) akorları izleyen dört partili koro: perde tespiti (YIN),
// formant korumalı perde düzeltme (TD-PSOLA), ses yürüyüşü, katedral reverb'ü. Döngü tempoya kilitli ve dikişsizdir.
(function () {
  const C = HG.Choir = {}, PI = Math.PI, TAU = 2 * PI, ACT = 32;
  C.MAX_SECONDS = 12;
  const PART_FORMANT = [.9, .96, 1.08, 1.16], PART_PAN = [-.12, .38, -.42, .2];
  const VOWELS = [{ f: [730, 1090, 2440, 3400], g: [1, .55, .3, .12], bw: [90, 100, 130, 150], noise: .035 },      // Aah
                  { f: [320, 800, 2300, 3200], g: [1, .3, .08, .04], bw: [70, 80, 110, 140], noise: .02 },          // Ooh
                  { f: [420, 1900, 2600, 3350], g: [1, .35, .3, .12], bw: [70, 100, 130, 150], noise: .03 },        // Eeh
                  { f: [260, 1050, 2200, 3000], g: [1, .1, .04, .02], bw: [60, 90, 120, 140], noise: .008 }];       // Mmm
  const m12 = x => ((x % 12) + 12) % 12, lim = (a, x, b) => Math.max(a, Math.min(b, x));
  C.defaults = () => ({ source: 0, rhythm: 0, size: 3, seed: 1, part: [true, true, true, true], level: .7, space: .6, detune: .35, air: .4, vibrato: .3, attack: .4, character: 0 });

  // Dört partinin ölçü başına notaları (MIDI): bas kökte, üst sesler en yakın akor sesine gider
  C.voicing = function (bars) {
    const lo = [40, 48, 55, 60], hi = [52, 60, 67, 76], prev = [45, 55, 62, 69], out = [];
    const near = (part, pc) => { let best = lo[part], bd = 999; for (let n = lo[part]; n <= hi[part]; n++) if (m12(n - pc) === 0) { const d = Math.abs(n - prev[part]); if (d <= bd) { bd = d; best = n; } } return best; };
    for (const b of bars) {
      const t = [m12(b.pc), m12(b.pc + b.third), m12(b.pc + b.fifth)], n = [near(0, t[0]), 0, 0, 0], used = [false, false, false];
      for (const part of [3, 2, 1]) {
        let bi = -1, bd = 999;
        for (let k = 0; k < 3; k++) { if (used[k]) continue; const d = Math.abs(near(part, t[k]) - prev[part]); if (d < bd) { bd = d; bi = k; } }
        used[bi] = true; n[part] = near(part, t[bi]);
      }
      for (let p = 0; p < 4; p++) prev[p] = n[p];
      out.push(n);
    }
    return out;
  };

  // Tek çerçevenin perdesi (YIN); perde yoksa 0
  C.detectPitch = function (x, off, n, sr) {
    const minLag = Math.max(2, Math.floor(sr / 900)), maxLag = Math.min(n >> 1, Math.floor(sr / 65));
    if (maxLag <= minLag + 2) return 0;
    const W = n - maxLag; let en = 0;
    for (let j = 0; j < W; j++) en += x[off + j] * x[off + j];
    if (en / W < 1e-6) return 0;
    const d = new Float64Array(maxLag + 1).fill(1); let run = 0;
    for (let tau = 1; tau <= maxLag; tau++) {
      let s = 0; for (let j = 0; j < W; j++) { const e = x[off + j] - x[off + j + tau]; s += e * e; }
      run += s; d[tau] = run > 0 ? s * tau / run : 1;
    }
    let best = -1;
    for (let tau = minLag; tau < maxLag; tau++) if (d[tau] < .15) { while (tau + 1 < maxLag && d[tau + 1] < d[tau]) tau++; best = tau; break; }
    if (best < 0) { best = minLag; for (let tau = minLag; tau < maxLag; tau++) if (d[tau] < d[best]) best = tau; if (d[best] > .35) return 0; }
    let tau = best;
    if (best > 1 && best < maxLag) { const a = d[best - 1], b = d[best], c = d[best + 1], den = a - 2 * b + c; if (Math.abs(den) > 1e-12) tau += .5 * (a - c) / den; }
    return sr / tau;
  };

  // Sesin çözümlemesi: perde işaretleri ve uzatma bölgesi. mono: Float32Array
  C.analyse = function (mono, sr) {
    const v = { x: new Float32Array(0), sr, f0: 0, mark: [], period: [], loopA: 0, loopB: 0, voiced: 0 };
    let N = Math.min(mono.length, Math.floor(sr * C.MAX_SECONDS)); if (N < sr * .05) return v;
    const x = Float32Array.from(mono.subarray(0, N)); let dc = 0, pk = 0;
    for (let i = 0; i < N; i++) dc += x[i]; dc /= N;
    for (let i = 0; i < N; i++) { x[i] -= dc; pk = Math.max(pk, Math.abs(x[i])); }
    if (pk < 1e-4) return v;
    let a = 0, z = N - 1; const thr = pk * .03, pad = Math.floor(sr * .01);
    while (a < z && Math.abs(x[a]) < thr) a++;
    while (z > a && Math.abs(x[z]) < thr) z--;
    a = Math.max(0, a - pad); z = Math.min(N - 1, z + pad); N = z - a + 1; if (N < sr * .05) return v;
    v.x = x.slice(a, a + N); for (let i = 0; i < N; i++) v.x[i] *= .9 / pk;
    const fadeN = Math.min(N >> 1, Math.floor(sr * .004)); for (let i = 0; i < fadeN; i++) { const g = i / fadeN; v.x[i] *= g; v.x[N - 1 - i] *= g; }
    // perde izi: ~12 kHz'e indirilmiş işaret üzerinde YIN, 10 ms adımla
    const D = Math.max(1, Math.round(sr / 12000)), dsr = sr / D, DN = Math.floor(N / D), ds = new Float32Array(Math.max(1, DN));
    for (let i = 0; i < DN; i++) { let s = 0; for (let k = 0; k < D; k++) s += v.x[i * D + k]; ds[i] = s / D; }
    const F = Math.floor(dsr * .045), H = Math.max(1, Math.floor(dsr * .01)), f0 = [];
    for (let at = 0; at + F <= DN; at += H) f0.push(C.detectPitch(ds, at, F, dsr));
    const vf = f0.filter(f => f > 0).sort((p, q) => p - q);
    v.voiced = f0.length ? vf.length / f0.length : 0;
    if (v.voiced >= .15) { v.f0 = vf[vf.length >> 1]; for (let i = 0; i < f0.length; i++) { if (f0[i] > v.f0 * 1.8) f0[i] *= .5; else if (f0[i] > 0 && f0[i] < v.f0 * .55) f0[i] *= 2; } }
    else f0.fill(0);
    const defP = sr / (v.f0 > 0 ? v.f0 : 160);
    const frameAt = t => f0.length ? f0[lim(0, Math.floor((t / D - F * .5) / H), f0.length - 1)] : 0;
    const peakIn = (from, to) => { from = Math.max(0, from | 0); to = Math.min(N - 1, to | 0); let bi = from; for (let i = from; i <= to; i++) if (v.x[i] > v.x[bi]) bi = i; return bi; };
    const vd = []; let t = peakIn(0, defP);
    while (t < N) {
      const f = frameAt(t), P = f > 0 ? sr / f : defP;
      v.mark.push(t | 0); v.period.push(P); vd.push(f > 0);
      const nx = t + P; t = f > 0 ? Math.max(t + P * .5, peakIn(nx - P * .25, nx + P * .25)) : nx;
    }
    const M = v.mark.length; let ba = 0, bl = 0;
    for (let i = 0; i < M;) { if (!vd[i]) { i++; continue; } let j = i; while (j < M && vd[j]) j++; if (j - i > bl) { bl = j - i; ba = i; } i = j; }
    if (bl >= 12) { v.loopA = ba + Math.floor(bl * .3); v.loopB = ba + bl - 1 - Math.floor(bl * .15); }
    else { v.loopA = M >> 2; v.loopB = Math.max(v.loopA + 1, Math.floor(M * 4 / 5)); }
    v.loopB = lim(0, v.loopB, Math.max(0, M - 1)); v.loopA = lim(0, v.loopA, v.loopB);
    v.mark = Int32Array.from(v.mark); v.period = Float32Array.from(v.period);
    return v;
  };

  const vibCents = (sg, sec) => { const on = lim(0, (sec - .18) / .5, 1); return sg.cents + Math.sin(TAU * sg.vibRate * sec + sg.vibPhase) * sg.vibDepth * on * on + Math.sin(TAU * .31 * sec + sg.vibPhase * 1.7) * 3; };

  // Dahili ünlü: gırtlak darbesi + nefes gürültüsü, dört paralel formant süzgecinden geçer
  function synthNote(dst, len, f0, sg, vowel, sr) {
    const V = VOWELS[lim(0, vowel, 3)], b0 = [0, 0, 0, 0], a1 = [0, 0, 0, 0], a2 = [0, 0, 0, 0], z1 = [0, 0, 0, 0], z2 = [0, 0, 0, 0];
    for (let k = 0; k < 4; k++) {
      let f = V.f[k] * sg.formant; if (k === 0) f = Math.max(f, f0 * 1.02); f = Math.min(f, sr * .45);
      const w = TAU * f / sr, al = Math.sin(w) / (2 * f / V.bw[k]), a0 = 1 + al;
      b0[k] = al / a0; a1[k] = -2 * Math.cos(w) / a0; a2[k] = (1 - al) / a0;
    }
    const r = MB.rng(sg.seed); let ph = r(), lp = 0;
    for (let i = 0; i < len; i++) {
      const sec = i / sr, f = f0 * Math.pow(2, vibCents(sg, sec) / 1200);
      ph += f / sr; if (ph >= 1) ph -= 1;
      const open = ph < .6 ? .5 * (1 - Math.cos(PI * ph / .6)) : ph < .8 ? Math.cos(PI * .5 * (ph - .6) / .2) : 0;
      const src = (open - .36) + (r() * 2 - 1) * V.noise * (.4 + open) * 6, x = src - lp; lp += (src - lp) * .002;
      let y = 0;
      for (let k = 0; k < 4; k++) { const o = b0[k] * x + z1[k]; z1[k] = -a1[k] * o + z2[k]; z2[k] = -b0[k] * x - a2[k] * o; y += o * V.g[k]; }
      dst[i] += y * 2.2;
    }
  }

  // Kullanıcının sesi: TD-PSOLA. Perde hedef notaya sabitlenir, tanecikler formantı korur
  function psolaNote(dst, len, f0, sg, V, hold, sr) {
    const M = V.mark.length; if (M < 4 || V.x.length < 64) return;
    const ratio = V.sr / sr, rho = sg.formant * ratio, first = V.mark[0], end = V.mark[M - 1], X = V.x.length, mk = V.mark;
    const la = lim(0, V.loopA, M - 2), lb = lim(la + 1, V.loopB - Math.floor(sg.loopJit * (V.loopB - V.loopA) * .3), M - 1), A = mk[la], B = mk[lb];
    let pos = hold ? first + sg.startJit * (A - first) * .6 : first, t = 0, dir = 1;
    while (t < len) {
      const f = f0 * Math.pow(2, vibCents(sg, t / sr) / 1200), T = sr / f, ip = Math.floor(pos);
      let lo = 0, hi = M; while (lo < hi) { const mid = (lo + hi) >> 1; if (mk[mid] < ip) lo = mid + 1; else hi = mid; }
      let m = lim(0, lo, M - 1); if (m > 0 && Math.abs(mk[m - 1] - pos) < Math.abs(mk[m] - pos)) m--;
      const Pa = V.period[m], c = mk[m], h = Pa / rho, half = Math.max(2, Math.floor(h)), g = Math.pow(lim(.3, T / h, 3), .7), t0 = Math.floor(t);
      for (let k = -half; k <= half; k++) {
        const o = t0 + k; if (o < 0 || o >= len) continue;
        const sp = c + k * rho, i0 = Math.floor(sp); if (i0 < 0 || i0 + 1 >= X) continue;
        dst[o] += (V.x[i0] + (V.x[i0 + 1] - V.x[i0]) * (sp - i0)) * (.5 + .5 * Math.cos(PI * k / half)) * g;
      }
      t += T; pos += dir * T * ratio;
      if (hold) { if (pos >= B) { pos = Math.max(A, 2 * B - pos); dir = -1; } else if (dir < 0 && pos <= A) { pos = Math.min(B, 2 * A - pos); dir = 1; } }
      else if (pos >= end) break;
    }
  }

  // Freeverb (JUCE'nin Reverb sınıfıyla aynı yapı): 8 tarak + 4 tüm geçiren, kanal başına
  function Reverb(sr, room, damp) {
    const combT = [1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617], apT = [556, 441, 341, 225], k = sr / 44100, fb = room * .28 + .7, dp = damp * .4;
    const mk = (t, spread) => t.map(n => ({ b: new Float32Array(Math.max(1, Math.round((n + spread) * k))), i: 0, last: 0 }));
    const cb = [mk(combT, 0), mk(combT, 23)], ap = [mk(apT, 0), mk(apT, 23)], out = [0, 0];
    return function (l, r) {
      const inp = (l + r) * .015;
      for (let ch = 0; ch < 2; ch++) {
        let o = 0;
        for (const c of cb[ch]) { const y = c.b[c.i]; c.last = y * (1 - dp) + c.last * dp; c.b[c.i] = inp + c.last * fb; if (++c.i >= c.b.length) c.i = 0; o += y; }
        for (const a of ap[ch]) { const y = a.b[a.i]; a.b[a.i] = o + y * .5; if (++a.i >= a.b.length) a.i = 0; o = y - o; }
        out[ch] = o * 3;
      }
      return out;
    };
  }

  // s: ayarlar; bars: [{ pc, third, fifth }]; voice: C.analyse çıktısı ya da null; pause: uzun işte arayüze nefes aldırmak için (isteğe bağlı)
  C.render = async function (s, barsIn, voice, bpm, sr, pause) {
    bpm = lim(40, bpm, 240);
    const bars = barsIn && barsIn.length ? barsIn : [{ pc: 9, third: 3, fifth: 7 }], NB = bars.length, beat = 60 / bpm * sr, barLen = beat * 4;
    const N = Math.max(64, Math.round(barLen * NB)), dl = new Float32Array(N), dr = new Float32Array(N), act = [0, 1, 2, 3].map(() => new Float32Array(NB * ACT));
    const own = s.source >= 4 && voice && voice.mark.length >= 4, words = own && s.source === 5, notes = C.voicing(bars), stab = s.rhythm === 3;
    const atk = stab ? .012 : .012 + s.attack * s.attack * .6, rel = stab ? .09 : .14 + s.attack * .4, charF = Math.pow(2, s.character * .25);
    for (let p = 0; p < 4; p++) {
      if (!s.part[p]) continue;
      const evs = [];
      for (let b = 0; b < NB; b++) {
        const m = notes[b][p], last = evs[evs.length - 1];
        if (s.rhythm === 0) { if (!words && last && last.midi === m && last.at + last.len >= b * barLen - 1) last.len += barLen; else evs.push({ at: b * barLen, len: barLen, midi: m }); }
        else if (s.rhythm === 1) for (let k = 0; k < 2; k++) evs.push({ at: b * barLen + k * beat * 2, len: beat * 2 * .96, midi: m });
        else if (s.rhythm === 2) for (let k = 0; k < 4; k++) evs.push({ at: b * barLen + k * beat, len: beat * .8, midi: m });
        else for (let k = 0; k < 4; k++) evs.push({ at: b * barLen + (k + .5) * beat, len: beat * .4, midi: m });
      }
      const singers = lim(1, s.size | 0, 4), gain = .5 / Math.sqrt(singers);
      for (let q = 0; q < singers; q++) {
        const r = MB.rng(s.seed * 7919 + p * 131 + q * 17 + 5), sp = singers > 1 ? q / (singers - 1) * 2 - 1 : 0, sg = {};
        sg.cents = sp * s.detune * 16 + (r() - .5) * s.detune * 10; sg.delay = q === 0 ? 0 : (r() - .3) * s.detune * (stab ? .012 : .04);
        sg.vibRate = 4.9 + r() * 1.3; sg.vibPhase = r() * TAU; sg.vibDepth = s.vibrato * (22 + r() * 16);
        sg.formant = PART_FORMANT[p] * charF * (1 + (r() - .5) * .06); sg.startJit = q === 0 ? 0 : r(); sg.loopJit = r();
        const pan = lim(-1, PART_PAN[p] + sp * .22, 1), pl = Math.cos((pan + 1) * PI * .25), pr = Math.sin((pan + 1) * PI * .25);
        let ei = 0;
        for (const e of evs) {
          const len = Math.floor(e.len + rel * sr), aN = Math.max(8, Math.floor(atk * sr)), rN = Math.max(8, Math.floor(rel * sr)), gate = Math.floor(e.len), tmp = new Float32Array(len);
          sg.seed = s.seed * 31 + p * 1009 + q * 101 + (ei++) * 7;
          const f0 = 440 * Math.pow(2, (e.midi - 69) / 12);
          if (own) psolaNote(tmp, len, f0, sg, voice, !words, sr); else synthNote(tmp, len, f0, sg, s.source, sr);
          const at = Math.round(e.at + sg.delay * sr);
          for (let i = 0; i < len; i++) {
            const env = Math.min(1, i / aN) * (i < gate ? 1 : Math.max(0, 1 - (i - gate) / rN)), x = tmp[i] * env * gain, o = ((at + i) % N + N) % N;
            dl[o] += x * pl; dr[o] += x * pr;
            if (q === 0 && (i & 255) === 0) { const a = act[p], ai = Math.floor(o * a.length / N); if (env > a[ai]) a[ai] = env; }
          }
        }
        if (pause) await pause();
      }
    }
    // katedral: alçak kesim, hava, ön gecikmeli geniş reverb; iki tur (kuyruk döngünün başına sarar)
    const rev = Reverb(sr, .6 + .38 * s.space, .35), pdN = Math.max(1, Math.floor(sr * .022)), pdL = new Float32Array(pdN), pdR = new Float32Array(pdN);
    const hpC = 1 - Math.exp(-TAU * 90 / sr), airC = 1 - Math.exp(-TAU * 5000 / sr), wet = .95 * s.space, dryG = 1 - .35 * s.space, L = new Float32Array(N), R = new Float32Array(N);
    let hl = 0, hr = 0, al = 0, ar = 0, pd = 0;
    for (let pass = 0; pass < 2; pass++) {
      for (let i = 0; i < N; i++) {
        let xl = dl[i], xr = dr[i];
        hl += (xl - hl) * hpC; hr += (xr - hr) * hpC; xl -= hl; xr -= hr;
        al += (xl - al) * airC; ar += (xr - ar) * airC; xl += (xl - al) * s.air * 1.2; xr += (xr - ar) * s.air * 1.2;
        const o = rev(pdL[pd], pdR[pd]); pdL[pd] = xl; pdR[pd] = xr; if (++pd >= pdN) pd = 0;
        L[i] = xl * dryG + o[0] * wet; R[i] = xr * dryG + o[1] * wet;
      }
      if (pause) await pause();
    }
    let pk = 0; for (let i = 0; i < N; i++) pk = Math.max(pk, Math.abs(L[i]), Math.abs(R[i]));
    if (pk > 1e-6) { const g = .6 * s.level / pk; for (let i = 0; i < N; i++) { L[i] *= g; R[i] *= g; } }
    return { L, R, N, bars: NB, bpm, sr, act };
  };
})();
