// ---- Koro sahnesi: yazılımla çizilen 3B figürler (bilge + dört melek). Eklentideki Source/Stage3D.cpp ile aynı
// modeller ve aynı hareket: derinlik tamponlu üçgen tarayıcı doğrudan bir ImageData'ya yazar (WebGL gerekmez).
(function () {
  const PI = Math.PI, CAM_Y = .95, CAM_Z = 9;
  const cRobe = 0xF4EEDF, cGold = 0xD9B14A, cSkin = 0xE8B894, cWhite = 0xECECEC, cDark = 0x3A2A22, cHalo = 0xFFD86B, cWing = 0xFAF7F0, cWing2 = 0xE6DDCB, cWood = 0x8A6A3A;
  const HAIR = [0x7A5230, 0xD9A441, 0xB5651D, 0xE8C877], ROBE = [0xE9EEF6, 0xF4EEDF, 0xF6EDE6, 0xEEF4EA];
  const AX = [-4.5, -2.35, 2.35, 4.5], AZ = [.5, 0, 0, .5];
  const BODY = 0, HEAD = 1, ARML = 2, ARMR = 3, HALO = 4, WINGL = 5, WINGR = 6, MOUTH = 7, STAFF = 8;
  const I = () => [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0];
  function mul(a, b) { const r = new Array(12); for (let i = 0; i < 3; i++) for (let j = 0; j < 4; j++) r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j] + (j === 3 ? a[i * 4 + 3] : 0); return r; }
  const rx = a => { const c = Math.cos(a), s = Math.sin(a); return [1, 0, 0, 0, 0, c, -s, 0, 0, s, c, 0]; };
  const ry = a => { const c = Math.cos(a), s = Math.sin(a); return [c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0]; };
  const rz = a => { const c = Math.cos(a), s = Math.sin(a); return [c, -s, 0, 0, s, c, 0, 0, 0, 0, 1, 0]; };
  const tr = (x, y, z) => [1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z];
  const sc = (x, y, z) => [x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0];
  const part = () => ({ v: [], tri: [], col: [], local: I(), glow: 0 });

  // Döner yüzey: (yarıçap, yükseklik) profili Y ekseni çevresinde döndürülür; cols profil dilimi başına renk
  function lathe(p, prof, segs, cols, xf) {
    const base = p.v.length / 3, R = prof.length;
    for (let i = 0; i < R; i++) for (let s = 0; s < segs; s++) {
      const a = 2 * PI * s / segs, x = prof[i][0] * Math.cos(a), y = prof[i][1], z = prof[i][0] * Math.sin(a);
      p.v.push(xf[0] * x + xf[1] * y + xf[2] * z + xf[3], xf[4] * x + xf[5] * y + xf[6] * z + xf[7], xf[8] * x + xf[9] * y + xf[10] * z + xf[11]);
    }
    for (let i = 0; i + 1 < R; i++) for (let s = 0; s < segs; s++) {
      const a = base + i * segs + s, b = base + i * segs + (s + 1) % segs, c = a + segs, d = b + segs, col = cols[Math.min(i, cols.length - 1)];
      p.tri.push(a, b, c, b, d, c); p.col.push(col, col);
    }
  }
  function ball(p, r, segs, col, xf) {
    const prof = [], rings = Math.max(4, segs >> 1);
    for (let i = 0; i <= rings; i++) { const a = PI * i / rings; prof.push([r * Math.sin(a), r * Math.cos(a)]); }
    lathe(p, prof, segs, [col], xf);
  }
  // Kanat: her tüy ayrı bir şerit (uçları sivri); altta uzun uçuş tüyleri, üstte kısa örtü tüyleri
  function wing(p, side) {
    const NU = 11, NV = 4;
    for (let layer = 0; layer < 2; layer++) for (let k = 0; k < NU; k++) {
      const base = p.v.length / 3;
      for (let j = 0; j <= NV; j++) for (let e = 0; e < 2; e++) {
        const v = j / NV, taper = j === NV ? .3 : j === NV - 1 ? .06 : -.08;
        const u = (k + (e === 0 ? taper : 1 - taper)) / NU, um = (k + .5) / NU;
        const top = .55 * Math.sin(PI * u * .8) - .25 * u * u, len = (.35 + .75 * Math.sin(PI * (um * .85 + .1)) * (1 - .3 * um)) * (layer === 0 ? 1 : .5);
        p.v.push(side * (.05 + 1.05 * u + (layer === 0 ? .12 * v * um : 0)), top - v * len, -.05 - .28 * u * u - .1 * v * v + (layer === 1 ? .04 : 0));
      }
      const col = layer === 1 ? cWing : (k % 2 === 0 ? cWing : cWing2);
      for (let j = 0; j < NV; j++) { const a = base + j * 2; p.tri.push(a, a + 1, a + 2, a + 1, a + 3, a + 2); p.col.push(col, col); }
    }
    p.wing = true;
  }
  function finish(p) {
    const v = p.v = new Float32Array(p.v), n = p.n = new Float32Array(v.length), t = p.tri = new Int32Array(p.tri); p.col = new Int32Array(p.col); p.tv = new Float32Array(v.length / 3 * 5);
    for (let k = 0; k + 2 < t.length; k += 3) {
      const a = t[k] * 3, b = t[k + 1] * 3, c = t[k + 2] * 3;
      const e1x = v[b] - v[a], e1y = v[b + 1] - v[a + 1], e1z = v[b + 2] - v[a + 2], e2x = v[c] - v[a], e2y = v[c + 1] - v[a + 1], e2z = v[c + 2] - v[a + 2];
      const nx = e1y * e2z - e1z * e2y, ny = e1z * e2x - e1x * e2z, nz = e1x * e2y - e1y * e2x;
      for (const q of [a, b, c]) { n[q] += nx; n[q + 1] += ny; n[q + 2] += nz; }
    }
    for (let i = 0; i < n.length; i += 3) { const l = Math.hypot(n[i], n[i + 1], n[i + 2]); if (l > 1e-9) { n[i] /= l; n[i + 1] /= l; n[i + 2] /= l; } else { n[i] = 0; n[i + 1] = 1; n[i + 2] = 0; } }
  }
  function makeFigure(elder, variant) {
    const f = { parts: [], world: I(), dim: 1 }; for (let i = 0; i < 9; i++) f.parts.push(part());
    const robe = elder ? cRobe : ROBE[variant & 3], hair = elder ? cWhite : HAIR[variant & 3], P = f.parts;
    if (elder) lathe(P[BODY], [[0, 1.48], [.09, 1.47], [.27, 1.38], [.31, 1.12], [.26, .86], [.27, .78], [.34, .35], [.5, -.28], [.53, -.4], [0, -.4]], 18, [cGold, robe, robe, robe, cGold, robe, robe, cGold, robe], I());
    else lathe(P[BODY], [[0, 1.46], [.07, 1.45], [.2, 1.37], [.22, 1.15], [.17, .9], [.18, .82], [.26, .4], [.4, -.28], [.43, -.4], [0, -.4]], 16, [cGold, robe, robe, robe, cGold, robe, robe, cGold, robe], I());
    const h = P[HEAD];
    ball(h, .16, 12, cSkin, I());
    ball(h, .178, 12, hair, mul(tr(0, .03, -.05), sc(1.02, 1, 1)));
    lathe(h, [[.14, .08], [.16, -.1], [.12, elder ? -.34 : -.44], [0, elder ? -.38 : -.5]], 10, [hair], tr(0, 0, -.135));
    for (const sx of [-.058, .058]) ball(h, .014, 6, cDark, tr(sx, .025, .148));
    if (elder) lathe(h, [[.125, -.02], [.13, -.1], [.08, -.26], [0, -.38]], 10, [cWhite], mul(tr(0, -.02, .075), sc(1, 1, .7)));
    for (const a of [ARML, ARMR]) {
      const w = elder ? 1.15 : 1;
      lathe(P[a], [[0, .02], [.085 * w, 0], [.1 * w, -.3], [.14 * w, -.56], [.05, -.57]], 10, [robe, robe, robe, cGold], I());
      ball(P[a], .058, 8, cSkin, tr(0, -.63, 0));
    }
    { const ring = [], R = elder ? .27 : .22; for (let i = 0; i <= 8; i++) { const a = 2 * PI * i / 8; ring.push([R + .022 * Math.cos(a), -.022 * Math.sin(a)]); } lathe(P[HALO], ring, 20, [cHalo], I()); P[HALO].glow = 1; }
    if (elder) {
      const s = P[STAFF];
      lathe(s, [[.03, 1.72], [.028, -.4]], 6, [cWood], I());
      ball(s, .11, 10, 0xFFF1B8, tr(0, 1.9, 0));
      const ring = []; for (let i = 0; i <= 6; i++) { const a = 2 * PI * i / 6; ring.push([.2 + .018 * Math.cos(a), -.018 * Math.sin(a)]); }
      lathe(s, ring, 14, [cGold], mul(tr(0, 1.9, 0), rx(PI * .5)));
    } else { wing(P[WINGL], -1); wing(P[WINGR], 1); ball(P[MOUTH], .022, 6, 0x7A3A30, I()); }
    P.forEach(finish);
    return f;
  }

  HG.Stage3D = function () {
    const fig = [0, 1, 2, 3].map(i => makeFigure(false, i)); fig.push(makeFigure(true, 0));
    const box = fig.map(() => ({ x: 0, y: 0, w: 0, h: 0 })); let glows = [], px = null, zb = null, PW = 0, PH = 0;
    const clamp01 = x => Math.max(0, Math.min(1, x));
    function pose(q) {
      const t = q.t, lv = clamp01(q.level), pulse = Math.exp(-5 * q.beat) * lv, sw = Math.sin(2 * PI * q.beat), k = clamp01(lv * 3);
      for (let i = 0; i < 4; i++) {
        const f = fig[i], P = f.parts, on = q.on[i], v = on ? clamp01(q.voice[i]) : 0, side = i < 2 ? 1 : -1;
        const y = .44 + (on ? .12 * Math.sin(t * 1.3 + i * 1.7) + .1 * v : -.12);
        f.dim = on ? 1 : .45;
        f.world = mul(tr(AX[i], y, AZ[i]), mul(ry(side * .32 + .05 * Math.sin(t * .7 + i)), mul(rz(.05 * sw * k), sc(.82, .82, .82))));
        const idle = .18 * Math.sin(t * 1.6 + i * .9), flap = on ? .22 + idle * (1 - k) + (.3 + .45 * lv) * Math.sin(2 * PI * q.beat - i * .4) * k : -.95;
        P[WINGL].local = mul(tr(-.1, 1.24, -.15), mul(ry(.38), rz(-flap)));
        P[WINGR].local = mul(tr(.1, 1.24, -.15), mul(ry(-.38), rz(flap)));
        const lift = .25 + .55 * v, fwd = .95 - .45 * v;      // susarken eller önde, söylerken kollar açılır
        P[ARML].local = mul(tr(-.19, 1.33, 0), mul(rz(-lift), rx(-fwd)));
        P[ARMR].local = mul(tr(.19, 1.33, 0), mul(rz(lift), rx(-fwd)));
        P[HEAD].local = mul(tr(0, 1.6, 0), mul(rx(-.28 * v), rz(.06 * Math.sin(t * 1.1 + i))));
        P[MOUTH].local = mul(P[HEAD].local, mul(tr(0, -.075, .15), sc(1, .3 + 1.5 * v, .6)));
        P[HALO].local = mul(tr(0, 1.9, 0), mul(rx(.22), ry(t * .8)));
        P[HALO].glow = on ? .55 + .45 * v : 0;
      }
      const f = fig[4], P = f.parts;
      f.world = mul(tr(0, -.06 + .03 * Math.sin(t * .9), -.4), mul(ry(.07 * Math.sin(t * .5)), sc(1.2, 1.2, 1.2)));
      P[ARML].local = mul(tr(-.27, 1.34, 0), mul(rz(-.62), rx(-.55)));                         // asayı tutan kol
      const lift = .3 + .95 * lv + .3 * pulse;
      P[ARMR].local = mul(tr(.27, 1.34, 0), mul(rz(lift), rx(-.35 - .25 * pulse)));             // yöneten kol: seviyeyle kalkar, vuruşta iner
      P[HEAD].local = mul(tr(0, 1.63, 0), mul(rx(-.1 * lv), ry(.12 * Math.sin(t * .6))));
      P[HALO].local = mul(tr(0, 1.95, 0), mul(rx(.22), ry(t * .6)));
      P[HALO].glow = .7 + .3 * lv;
      P[STAFF].local = tr(-.66, 0, .3);
      P[STAFF].glow = .25 + .75 * Math.max(lv, pulse);
    }
    function draw(f, W, H, bb) {
      const foc = 2.45 * Math.min(H, W / 3.3), cx = W * .5, cy = H * .5, view = tr(0, -CAM_Y, -CAM_Z), Lx = .33, Ly = .78, Lz = .53;
      let x0 = 1e9, y0 = 1e9, x1 = -1e9, y1 = -1e9;
      for (const p of f.parts) {
        if (!p.tri.length) continue;
        const M = mul(view, mul(f.world, p.local)), sN = 1 / Math.max(1e-6, Math.hypot(M[0], M[4], M[8])), v = p.v, n = p.n, tv = p.tv, g = p.glow;
        for (let i = 0, o = 0; i < v.length; i += 3, o += 5) {
          const x = M[0] * v[i] + M[1] * v[i + 1] + M[2] * v[i + 2] + M[3], y = M[4] * v[i] + M[5] * v[i + 1] + M[6] * v[i + 2] + M[7], z = M[8] * v[i] + M[9] * v[i + 1] + M[10] * v[i + 2] + M[11];
          let nx = (M[0] * n[i] + M[1] * n[i + 1] + M[2] * n[i + 2]) * sN, ny = (M[4] * n[i] + M[5] * n[i + 1] + M[6] * n[i + 2]) * sN, nz = (M[8] * n[i] + M[9] * n[i + 1] + M[10] * n[i + 2]) * sN;
          const d = Math.max(.1, -z), il = 1 / Math.sqrt(x * x + y * y + z * z);
          let nv = (nx * -x + ny * -y + nz * -z) * il; if (nv < 0) { nx = -nx; ny = -ny; nz = -nz; nv = -nv; }   // normal kameraya baksın: kanatlar iki yüzden aydınlanır
          const dif = Math.max(0, nx * Lx + ny * Ly + nz * Lz), rim = (1 - nv) * (1 - nv) * (1 - nv);
          tv[o] = cx + x * foc / d; tv[o + 1] = cy - y * foc / d; tv[o + 2] = d; tv[o + 3] = (.44 + .6 * dif) * (1 - g) + 1.12 * g; tv[o + 4] = rim * .55 * f.dim;
          if (tv[o] < x0) x0 = tv[o]; if (tv[o] > x1) x1 = tv[o]; if (tv[o + 1] < y0) y0 = tv[o + 1]; if (tv[o + 1] > y1) y1 = tv[o + 1];
        }
        const t = p.tri;
        for (let k = 0; k + 2 < t.length; k += 3) {
          const a = t[k] * 5, b = t[k + 1] * 5, c = t[k + 2] * 5;
          const ax = tv[a], ay = tv[a + 1], bx = tv[b], by = tv[b + 1], cxx = tv[c], cyy = tv[c + 1];
          const area = (bx - ax) * (cyy - ay) - (by - ay) * (cxx - ax); if (Math.abs(area) < 1e-4 || (area > 0 && !p.wing)) continue;   // kapalı gövdelerde arka yüzler atlanır; kanatlar iki yüzlüdür
          const minX = Math.max(0, Math.floor(Math.min(ax, bx, cxx))), maxX = Math.min(W - 1, Math.ceil(Math.max(ax, bx, cxx)));
          const minY = Math.max(0, Math.floor(Math.min(ay, by, cyy))), maxY = Math.min(H - 1, Math.ceil(Math.max(ay, by, cyy)));
          if (minX > maxX || minY > maxY) continue;
          const col = p.col[k / 3]; let cr = (col >> 16) & 255, cg = (col >> 8) & 255, cb = col & 255;
          if (f.dim < 1) { const m = (cr + cg + cb) / 3; cr = (cr * .3 + m * .7) * f.dim; cg = (cg * .3 + m * .7) * f.dim; cb = (cb * .3 + m * .7) * f.dim; }
          const inv = 1 / area, az = tv[a + 2], bz = tv[b + 2], cz = tv[c + 2], al = tv[a + 3], bl = tv[b + 3], cl = tv[c + 3], ar = tv[a + 4], br = tv[b + 4], crm = tv[c + 4];
          const d0 = (by - cyy) * inv, d1 = (cyy - ay) * inv, sx = minX + .5;
          for (let y = minY; y <= maxY; y++) {
            const py = y + .5, row = y * W;
            let w0 = ((bx - sx) * (cyy - py) - (by - py) * (cxx - sx)) * inv, w1 = ((cxx - sx) * (ay - py) - (cyy - py) * (ax - sx)) * inv;
            for (let x = minX; x <= maxX; x++, w0 += d0, w1 += d1) {
              const w2 = 1 - w0 - w1;
              if (w0 < -1e-5 || w1 < -1e-5 || w2 < -1e-5) continue;
              const z = w0 * az + w1 * bz + w2 * cz; if (z >= zb[row + x]) continue;
              zb[row + x] = z;
              const lit = w0 * al + w1 * bl + w2 * cl, rim = (w0 * ar + w1 * br + w2 * crm) * 255;
              const r = Math.min(255, cr * lit + rim) | 0, gg = Math.min(255, cg * lit + rim * .94) | 0, bb2 = Math.min(255, cb * lit + rim * .78) | 0;
              px[row + x] = 0xff000000 | (bb2 << 16) | (gg << 8) | r;
            }
          }
        }
      }
      if (x1 > x0) { bb.x = x0 / W; bb.y = y0 / H; bb.w = (x1 - x0) / W; bb.h = (y1 - y0) / H; } else bb.w = bb.h = 0;
    }
    // img: ImageData (arka plan saydam kalır); q: { t, beat, level, voice[4], on[4] }; ss: süper örnekleme (1-3)
    function render(img, q, ss) {
      ss = Math.max(1, Math.min(3, ss | 0 || 2));
      const w = img.width, h = img.height, W = w * ss, H = h * ss;
      if (PW !== W || PH !== H) { px = new Uint32Array(W * H); zb = new Float32Array(W * H); PW = W; PH = H; }
      px.fill(0); zb.fill(1e9);
      pose(q);
      for (let i = 0; i < 5; i++) draw(fig[i], W, H, box[i]);
      const foc = 2.45 * Math.min(H, W / 3.3); glows = [];
      const point = (f, local, x, y, z, r, a) => {
        const M = mul(tr(0, -CAM_Y, -CAM_Z), mul(f.world, local)), d = Math.max(.1, -(M[8] * x + M[9] * y + M[10] * z + M[11]));
        glows.push({ x: (W * .5 + (M[0] * x + M[1] * y + M[2] * z + M[3]) * foc / d) / W, y: (H * .5 - (M[4] * x + M[5] * y + M[6] * z + M[7]) * foc / d) / H, r: r * foc / d / H, a });
      };
      for (let i = 0; i < 4; i++) if (q.on[i]) point(fig[i], fig[i].parts[HALO].local, 0, 0, 0, .34 + .3 * q.voice[i], .22 + .5 * q.voice[i]);
      point(fig[4], fig[4].parts[HALO].local, 0, 0, 0, .5 + .3 * q.level, .3 + .4 * q.level);
      point(fig[4], fig[4].parts[STAFF].local, 0, 1.9, 0, .3 + .5 * q.level, .35 + .55 * q.level);
      const out = img.data, n2 = ss * ss;      // süper örnekleme: kenarlar yumuşar, alfa kapsama oranıdır
      for (let y = 0, o = 0; y < h; y++) for (let x = 0; x < w; x++, o += 4) {
        let r = 0, g = 0, b = 0, cnt = 0;
        for (let j = 0; j < ss; j++) { const s = (y * ss + j) * W + x * ss; for (let i = 0; i < ss; i++) { const c = px[s + i]; if (c === 0) continue; cnt++; r += c & 255; g += (c >> 8) & 255; b += (c >> 16) & 255; } }
        if (cnt === 0) { out[o + 3] = 0; continue; }
        out[o] = r / cnt; out[o + 1] = g / cnt; out[o + 2] = b / cnt; out[o + 3] = cnt * 255 / n2;
      }
    }
    return {
      render, glows: () => glows, bounds: i => box[i],
      hit: (x, y) => { for (let i = 0; i < 4; i++) { const b = box[i]; if (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h) return i; } return -1; },
      triangles: () => fig.reduce((n, f) => n + f.parts.reduce((m, p) => m + p.tri.length / 3, 0), 0)
    };
  };
})();
