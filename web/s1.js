// ---- Ses motoru: Web Audio ile sentez, efekt barları, davul kitleri, WAV
(function () {
  const W = { w: 'sawtooth', q: 'square', t: 'triangle', s: 'sine' };
  const FORM = { // vokal formantları [F1,F2,F3], kazançlar
    a: [[850, 1220, 2810], [1, .55, .25]], o: [[500, 900, 2830], [1, .45, .12]],
    u: [[370, 950, 2670], [1, .35, .10]], e: [[520, 1850, 2600], [1, .50, .20]],
    i: [[350, 2300, 3050], [1, .45, .20]]
  };
  const clamp = (x, a, b) => Math.max(a, Math.min(b, x));
  const mtof = n => 440 * Math.pow(2, (n - 69) / 12);
  MB.mtof = mtof;

  function noiseBuf(ac) {
    if (ac.__nb) return ac.__nb;
    const b = ac.createBuffer(1, ac.sampleRate * 2, ac.sampleRate), d = b.getChannelData(0);
    let s = 1;
    for (let i = 0; i < d.length; i++) { s = (s * 16807) % 2147483647; d[i] = s / 1073741823 - 1; }
    return (ac.__nb = b);
  }
  const curves = {};
  function satCurve(k) {
    const key = Math.round(k * 100);
    if (curves[key]) return curves[key];
    const n = 1024, c = new Float32Array(n), a = 1 + k * 4, nm = Math.tanh(a);
    for (let i = 0; i < n; i++) { const x = i / (n - 1) * 2 - 1; c[i] = Math.tanh(x * a) / nm; }
    return (curves[key] = c);
  }
  function makeIR(ac, sec) {
    if (ac.__ir) return ac.__ir;
    const sr = ac.sampleRate, len = Math.floor(sr * sec), buf = ac.createBuffer(2, len, sr);
    for (let c = 0; c < 2; c++) {
      const d = buf.getChannelData(c); let s = 12345 + c * 777, y = 0;
      for (let i = 0; i < len; i++) {
        s = (s * 16807) % 2147483647;
        const x = s / 1073741823 - 1, k = i / len;
        y += (.85 * Math.pow(1 - k, 1.5) + .08) * (x - y);          // yüksekler önce sönsün
        d[i] = (i < sr * .012 ? 0 : y) * Math.pow(1 - k, 2.2) * Math.exp(-2.5 * k) * .9;
      }
    }
    return (ac.__ir = buf);
  }

  // ---------- Tek nota: enstrüman sesi
  MB.voice = function (ac, dest, p, f0, t, dur, vel) {
    const v = vel / 127, a = p.a, trim = (MB.TRIM && MB.TRIM[p.id]) || 1;
    const A = Math.max(.002, a[0]), D = Math.max(.01, a[1]), S = a[2], R = Math.max(.02, a[3]);
    const end = t + Math.max(dur, .05), rs = Math.max(end, t + A + .01);
    const peak = (p.vol || .2) * trim * (.4 + .6 * v);
    const stopAt = rs + R * 2 + .05;
    if (ac.__vc) { const a = ac.__vc; let i = a.length; while (i > 0 && a[i - 1] > stopAt) i--; a.splice(i, 0, stopAt); }
    const amp = ac.createGain(), g = amp.gain;
    g.setValueAtTime(.0001, t);
    g.linearRampToValueAtTime(peak, t + A);
    g.setTargetAtTime(Math.max(.0001, peak * S), t + A, D / 3);
    g.setTargetAtTime(.0001, rs, R / 3);

    const mixG = ac.createGain(), oscs = [];
    // performans: kaliteye göre unison sınırı ve aktif ses sayacı
    const Q = { eco: [2, 28], bal: [4, 56], high: [7, 120] }[ac.__q || 'bal'];
    const vc = ac.__vc || (ac.__vc = []); while (vc.length && vc[0] < t) vc.shift();
    let n = Math.min(p.u ? p.u[0] : 1, Q[0]); if (vc.length > Q[1]) n = 1;
    const sp = p.u ? p.u[1] : 0;
    for (let i = 0; i < n; i++) {
      const spread = n > 1 ? (i / (n - 1) * 2 - 1) * sp : 0;
      for (const L of (p.o || [])) {
        if (!(L[3] > .001)) continue;
        const o = ac.createOscillator(), og = ac.createGain(), fr = f0 * Math.pow(2, L[2]);
        if (L[0] === 'x') o.setPeriodicWave(HG.wave(ac, p.wt)); else o.type = W[L[0]]; o.frequency.value = fr; o.detune.value = L[1] + spread;
        if (p.pe) { o.frequency.setValueAtTime(fr * Math.pow(2, p.pe[0] / 12), t); o.frequency.setTargetAtTime(fr, t, p.pe[1] / 3); }
        og.gain.value = L[3] / Math.sqrt(n);
        o.connect(og); og.connect(mixG); o.start(t); o.stop(stopAt); oscs.push(o);
      }
    }
    const th = ac.__thick || 0;
    if (th > .05 && p.cat !== 'Vox') {                              // kalınlık: bir oktav alt (bas için temel) katmanı
      const o = ac.createOscillator(), og = ac.createGain();
      o.type = p.cat === 'Bas' ? 'sine' : 'triangle'; o.frequency.value = p.cat === 'Bas' ? f0 : f0 / 2;
      og.gain.value = (p.cat === 'Bas' ? .3 : .2) * th;
      o.connect(og); og.connect(mixG); o.start(t); o.stop(stopAt);
    }
    if (p.sub) {
      const o = ac.createOscillator(), og = ac.createGain();
      o.type = 'sine'; o.frequency.value = f0 / 2; og.gain.value = p.sub;
      o.connect(og); og.connect(mixG); o.start(t); o.stop(stopAt);
    }
    if (p.fm) {
      const car = ac.createOscillator(), mod = ac.createOscillator(), mg = ac.createGain(), cg = ac.createGain();
      const dev = p.fm[1] * f0 * p.fm[0];
      car.type = 'sine'; car.frequency.value = f0; mod.type = 'sine'; mod.frequency.value = f0 * p.fm[0];
      mg.gain.setValueAtTime(dev, t); mg.gain.setTargetAtTime(dev * (p.fm[3] == null ? .25 : p.fm[3]), t, Math.max(.02, p.fm[2]) / 3);
      cg.gain.value = p.fm[4] || 1;
      mod.connect(mg); mg.connect(car.frequency); car.connect(cg); cg.connect(mixG);
      car.start(t); mod.start(t); car.stop(stopAt); mod.stop(stopAt); oscs.push(car);
    }
    if (p.noi) {
      const ns = ac.createBufferSource(), hp = ac.createBiquadFilter(), ng = ac.createGain();
      ns.buffer = noiseBuf(ac); ns.loop = true; hp.type = 'highpass'; hp.frequency.value = p.vow ? 1200 : 2500; ng.gain.value = p.noi;
      ns.connect(hp); hp.connect(ng); ng.connect(mixG); ns.start(t, .3); ns.stop(stopAt);
    }
    if (p.vib) {
      const vo = ac.createOscillator(), vg = ac.createGain();
      vo.frequency.value = p.vib[0]; vg.gain.setValueAtTime(0, t);
      vg.gain.linearRampToValueAtTime(p.vib[1], t + (p.vib[2] || 0) + .25);
      vo.connect(vg); oscs.forEach(o => vg.connect(o.detune)); vo.start(t); vo.stop(stopAt);
    }

    // filtre (12 veya 24 dB), isteğe bağlı sürücü, formant ve LFO'lar
    let src = mixG;
    if (p.fd) { const ws = ac.createWaveShaper(); ws.curve = satCurve(p.fd); mixG.connect(ws); src = ws; }
    const F = p.f || ['lowpass', 8000, .7, 0, 0, .2], fls = [ac.createBiquadFilter()];
    if (F[6]) fls.push(ac.createBiquadFilter());
    const c = clamp(F[1] * Math.pow(f0 / 261.63, p.kt == null ? .5 : p.kt), 60, 16000), pk = clamp(c * (1 + F[3]), 60, 18000);
    fls.forEach(fl => {
      fl.type = F[0]; fl.Q.value = F[6] ? F[2] * .7 : F[2];
      if (F[3] > 0) {
        if (F[4] > 0) { fl.frequency.setValueAtTime(c, t); fl.frequency.linearRampToValueAtTime(pk, t + F[4]); fl.frequency.setTargetAtTime(c, t + F[4], Math.max(.02, F[5]) / 3); }
        else { fl.frequency.setValueAtTime(pk, t); fl.frequency.setTargetAtTime(c, t, Math.max(.02, F[5]) / 3); }
      } else fl.frequency.value = c;
    });
    if (p.lf) {
      const l = ac.createOscillator(), lg = ac.createGain();
      l.frequency.value = p.lf[0]; lg.gain.value = c * p.lf[1]; l.connect(lg); fls.forEach(fl => lg.connect(fl.frequency)); l.start(t); l.stop(stopAt);
    }
    let tail = fls[fls.length - 1];
    if (p.vow) {
      const fo = FORM[p.vow], sc = p.male ? .85 : 1, sum = ac.createGain();
      fo[0].forEach((fq, i) => {
        const bp = ac.createBiquadFilter(), bg = ac.createGain();
        bp.type = 'bandpass'; bp.frequency.value = fq * sc; bp.Q.value = [7, 9, 11][i]; bg.gain.value = fo[1][i];
        src.connect(bp); bp.connect(bg); bg.connect(sum);
      });
      sum.connect(fls[0]);
    } else src.connect(fls[0]);
    for (let i = 1; i < fls.length; i++) fls[i - 1].connect(fls[i]);
    // genel LFO'lar: [hız, derinlik, hedef(cut|pit|amp), şekil, senkron]
    (p.l || []).forEach(L => {
      if (!L || !L[1]) return;
      const lo = ac.createOscillator(), lg = ac.createGain(), hz = L[4] ? (ac.__bpm || 120) / 60 * L[0] : L[0];
      lo.type = L[3] || 'sine'; lo.frequency.value = Math.max(.01, hz); lo.connect(lg);
      if (L[2] === 'cut') { lg.gain.value = c * L[1]; fls.forEach(fl => lg.connect(fl.frequency)); }
      else if (L[2] === 'pit') { lg.gain.value = L[1] * 100; oscs.forEach(o => lg.connect(o.detune)); }
      else { const tr = ac.createGain(); tr.gain.value = 1 - L[1] / 2; lg.gain.value = L[1] / 2; lg.connect(tr.gain); tail.connect(tr); tail = tr; }
      lo.start(t); lo.stop(stopAt);
    });
    tail.connect(amp); amp.connect(dest);
  };

  // ---------- Efekt barı: chorus ve doygunluk
  function chorus(ac, input, amt, starts) {
    const out = ac.createGain(); out.gain.value = .85; input.connect(out);
    [[.014, .7, -1], [.021, .53, 1]].forEach(([d0, rate, pan]) => {
      const dn = ac.createDelay(.1), l = ac.createOscillator(), lg = ac.createGain(), pn = ac.createStereoPanner(), wg = ac.createGain();
      dn.delayTime.value = d0; l.frequency.value = rate; lg.gain.value = .004; pn.pan.value = pan * .9; wg.gain.value = amt;
      l.connect(lg); lg.connect(dn.delayTime); l.start(0); starts.push(l);
      input.connect(dn); dn.connect(pn); pn.connect(wg); wg.connect(out);
    });
    return out;
  }

  // ---------- Davul sesleri
  MB.drumHit = function (ac, dest, kit, n, t, vel) {
    const v = vel / 127, G = MB.GM;
    const burst = (hp, fType, fq, q, d, amp, at, off) => {
      const s = ac.createBufferSource(), f = ac.createBiquadFilter(), e = ac.createGain();
      s.buffer = noiseBuf(ac); f.type = fType; f.frequency.value = fq; f.Q.value = q;
      e.gain.setValueAtTime(amp, at); e.gain.setTargetAtTime(.0001, at, d / 4);
      s.connect(f); f.connect(e); e.connect(dest); s.start(at, off || 0); s.stop(at + d * 1.6 + .02);
    };
    if (n === G.kick) {
      const K = kit.k, o = ac.createOscillator(), e = ac.createGain();
      o.frequency.setValueAtTime(K.f0, t); o.frequency.setTargetAtTime(K.f1, t, K.pd);
      e.gain.setValueAtTime(v * .95, t); e.gain.setTargetAtTime(.0001, t + K.d * .15, K.d / 4);
      o.connect(e);
      if (K.drv) { const ws = ac.createWaveShaper(); ws.curve = satCurve(K.drv); e.connect(ws); ws.connect(dest); } else e.connect(dest);
      o.start(t); o.stop(t + K.d * 1.6 + .1);
      if (K.click) burst(0, 'highpass', 1800, .7, .02, v * K.click * .5, t, .1);
    } else if (n === G.snare) {
      const S = kit.s, o = ac.createOscillator(), e = ac.createGain();
      o.type = 'triangle'; o.frequency.setValueAtTime(S.f * 1.4, t); o.frequency.setTargetAtTime(S.f, t, .02);
      e.gain.setValueAtTime(v * (1 - S.mix) * .8, t); e.gain.setTargetAtTime(.0001, t, S.td / 4);
      o.connect(e); e.connect(dest); o.start(t); o.stop(t + S.td * 1.6 + .05);
      burst(0, 'bandpass', S.nf, .8, S.nd, v * S.mix * 1.1, t);
    } else if (n === G.clap) {
      const C = kit.c;
      [0, .011, .022].forEach((o, i) => burst(0, 'bandpass', C.nf, 1.3, i < 2 ? .02 : C.d, v * .55, t + o, i * .07));
    } else if (n === G.chat || n === G.ohat) {
      const H = n === G.chat ? kit.h : kit.o, e = ac.createGain(), hp = ac.createBiquadFilter();
      hp.type = 'highpass'; hp.frequency.value = H.hp;
      e.gain.setValueAtTime(v * .42, t); e.gain.setTargetAtTime(.0001, t, H.d / 4);
      hp.connect(e); e.connect(dest);
      const s = ac.createBufferSource(), ng = ac.createGain();
      s.buffer = noiseBuf(ac); ng.gain.value = 1 - H.m * .6; s.connect(ng); ng.connect(hp); s.start(t, Math.random() * .8); s.stop(t + H.d * 1.8 + .02);
      if (H.m > 0) {
        const mg = ac.createGain(); mg.gain.value = H.m * .18; mg.connect(hp);
        [263, 400, 421, 474, 587, 845].forEach(fq => { const o = ac.createOscillator(); o.type = 'square'; o.frequency.value = fq * 1.7; o.connect(mg); o.start(t); o.stop(t + H.d * 1.8 + .02); });
      }
    } else if (n === G.rim) {
      const o = ac.createOscillator(), e = ac.createGain();
      o.frequency.value = kit.r.f; e.gain.setValueAtTime(v * .5, t); e.gain.setTargetAtTime(.0001, t, .008);
      o.connect(e); e.connect(dest); o.start(t); o.stop(t + .08);
      burst(0, 'bandpass', 3400, 1, .03, v * .3, t);
    } else {
      const P = kit.p, o = ac.createOscillator(), e = ac.createGain();
      o.type = P.type || 'sine'; o.frequency.setValueAtTime(P.f * 1.25, t); o.frequency.setTargetAtTime(P.f, t, .02);
      e.gain.setValueAtTime(v * .7, t); e.gain.setTargetAtTime(.0001, t, P.d / 4);
      o.connect(e); e.connect(dest); o.start(t); o.stop(t + P.d * 1.6 + .05);
      burst(0, 'bandpass', P.f * 3, 1.2, .03, v * .25, t);
    }
  };

  // ---------- Karışım: master, reverb, delay, iz barları
  MB.mix = function (ac, dest, res, snd, o) {
    o = o || {};
    const P = res.P, spt = 60 / P.bpm / MB.PPQ, starts = [], KEYS = ['chords', 'bass', 'drums', 'melody'];
    const master = ac.createGain(), comp = ac.createDynamicsCompressor();
    const lim = ac.createWaveShaper();                              // son güvenlik: yumuşak sınırlayıcı
    lim.curve = satCurve(0);
    master.gain.value = .42; comp.threshold.value = -16; comp.knee.value = 10; comp.ratio.value = 6; comp.attack.value = .003; comp.release.value = .16;
    master.connect(comp); comp.connect(lim); lim.connect(dest);
    const an = ac.createAnalyser ? ac.createAnalyser() : null; if (an) { an.fftSize = 2048; an.smoothingTimeConstant = .8; lim.connect(an); }
    ac.__bpm = P.bpm; ac.__q = o.quality || ac.__q || 'bal'; ac.__thick = o.thick == null ? (ac.__thick == null ? .7 : ac.__thick) : o.thick; if (!ac.__vc) ac.__vc = [];

    const rvIn = ac.createGain(), rv = ac.createConvolver(), rvOut = ac.createGain();
    rv.buffer = makeIR(ac, 2.6); rvOut.gain.value = .55; rvIn.connect(rv); rv.connect(rvOut); rvOut.connect(master);
    const dlIn = ac.createGain(), dl = ac.createDelay(2), fb = ac.createGain(), dlp = ac.createBiquadFilter(), dlOut = ac.createGain(), dlRv = ac.createGain();
    dl.delayTime.value = Math.min(1.9, 60 / P.bpm * .75); fb.gain.value = .38; dlp.type = 'lowpass'; dlp.frequency.value = 2800; dlOut.gain.value = .6; dlRv.gain.value = .3;
    dlIn.connect(dl); dl.connect(dlp); dlp.connect(fb); fb.connect(dl); dlp.connect(dlOut); dlOut.connect(master); dlp.connect(dlRv); dlRv.connect(rvIn);

    const bus = {}, ins = {}, preset = {}, pg = {}, kit = MB.KITS[snd.drums] || MB.KITS.house;
    KEYS.forEach(k => {
      const mg = ac.createGain(); mg.gain.value = (o.muted && o.muted[k]) ? 0 : (o.vol && o.vol[k] != null ? o.vol[k] : 1); mg.connect(master); bus[k] = mg;
      const send = (amt, tgt) => { if (amt) { const s = ac.createGain(); s.gain.value = amt; mg.connect(s); s.connect(tgt); } };
      const inG = ac.createGain();
      if (k === 'drums') {
        let node = inG;
        if (kit.lp) { const lp = ac.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = kit.lp; inG.connect(lp); node = lp; }
        node.connect(mg); send(kit.rv || .08, rvIn); ins[k] = inG; return;
      }
      const p = preset[k] = (o.patches && o.patches[k]) || MB.byId[snd[k]] || MB.byId[MB.DEFAULTS.house[k === 'chords' ? 'pad' : k]];
      inG.gain.value = (k === 'chords' && p.r.indexOf('m') >= 0) ? .6 : 1;
      let node = inG;
      if (p.ch && ac.__q !== 'eco') node = chorus(ac, node, p.ch, starts);
      if (p.dr) { const ws = ac.createWaveShaper(); ws.curve = satCurve(p.dr); ws.oversample = '2x'; node.connect(ws); node = ws; }
      const pump = ac.createGain(); pg[k] = pump; node.connect(pump); pump.connect(mg); send(p.rv, rvIn); send(p.dl, dlIn); ins[k] = inG;
    });

    // t0: s0 adımının zamanı. so.s0/so.s1: adım aralığı, so.only: tek iz
    function schedule(t0, so) {
      so = so || {};
      const s0 = so.s0 || 0, s1 = so.s1 != null ? so.s1 : (so.bars ? Math.min(so.bars, res.bars) : res.bars) * 16;
      const at = s => t0 + (MB.tickOf(s, P.swing) - MB.tickOf(s0, P.swing)) * spt, pa = o.pump, ps = o.pumpAmt == null ? 1 : o.pumpAmt;
      KEYS.forEach(k => {
        if (so.only && so.only !== k) return;
        res.tracks[k].forEach(n => {
          if (n.s < s0 || n.s >= s1) return;
          const t = at(n.s), e = at(n.s + n.l);
          if (k === 'drums') MB.drumHit(ac, ins[k], kit, n.n, t, n.v);
          else MB.voice(ac, ins[k], preset[k], mtof(n.n), t, Math.max(.03, e - t), n.v);
        });
      });
      if (pa && ps > 0 && !so.only) {
        const kicks = res.tracks.drums.filter(n => n.n === MB.GM.kick && n.s >= s0 && n.s < s1).sort((x, y) => x.s - y.s);
        ['chords', 'bass', 'melody'].forEach(k => {
          const cfg = pa[k]; if (!cfg || !cfg[0]) return; const g = pg[k].gain, d = Math.min(.95, cfg[0] * ps);
          kicks.forEach(n => { const t = at(n.s); g.setValueAtTime(1 - d, t); g.setTargetAtTime(1, t + .012, Math.max(.04, cfg[1]) / 3); });
        });
      }
      return (s1 - s0) * (MB.PPQ / 4) * spt;
    }
    function dispose() {
      starts.forEach(l => { try { l.stop(); } catch (e) {} });
      try { master.disconnect(); comp.disconnect(); lim.disconnect(); } catch (e) {}
    }
    return { bus, master, schedule, dispose, an, ins, pg, preset, kit };
  };

  MB._h = { noiseBuf, satCurve, makeIR, clamp, W, FORM };

  // ---------- WAV (16 bit stereo)
  MB.wav = function (buf) {
    const ch = buf.numberOfChannels, len = buf.length, sr = buf.sampleRate, out = new DataView(new ArrayBuffer(44 + len * 4));
    const w = (o, s) => { for (let i = 0; i < s.length; i++) out.setUint8(o + i, s.charCodeAt(i)); };
    w(0, 'RIFF'); out.setUint32(4, 36 + len * 4, true); w(8, 'WAVE'); w(12, 'fmt '); out.setUint32(16, 16, true);
    out.setUint16(20, 1, true); out.setUint16(22, 2, true); out.setUint32(24, sr, true); out.setUint32(28, sr * 4, true);
    out.setUint16(32, 4, true); out.setUint16(34, 16, true); w(36, 'data'); out.setUint32(40, len * 4, true);
    const L = buf.getChannelData(0), Rr = buf.getChannelData(ch > 1 ? 1 : 0);
    let pos = 44;
    for (let i = 0; i < len; i++) {
      out.setInt16(pos, clamp(L[i], -1, 1) * 32767, true); out.setInt16(pos + 2, clamp(Rr[i], -1, 1) * 32767, true); pos += 4;
    }
    return new Uint8Array(out.buffer);
  };
})();
