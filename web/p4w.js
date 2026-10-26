// ---- MIDI yazıcı: SMF format 1 (varsayılan) ve format 0, tüm DAW'larda açılacak şekilde
MB.PPQ = 480;
MB.CH = { chords: 0, bass: 1, melody: 2, drums: 9 };
MB.TNAME = { chords: 'Chords', bass: 'Bass', melody: 'Melody', drums: 'Drums' };
(function () {
  const STEP = MB.PPQ / 4;
  const vlq = n => {
    const b = [n & 0x7f]; n >>= 7;
    while (n > 0) { b.unshift((n & 0x7f) | 0x80); n >>= 7; }
    return b;
  };
  const str = s => Array.from(unescape(encodeURIComponent(s))).map(c => c.charCodeAt(0));
  const u32 = n => [(n >>> 24) & 255, (n >>> 16) & 255, (n >>> 8) & 255, n & 255];
  const u16 = n => [(n >> 8) & 255, n & 255];
  const tickOf = (s, sw) => {
    const sh = (Number.isInteger(s) && s % 2 === 1) ? (sw / 100) * STEP : 0;
    return Math.round(s * STEP + sh);
  };
  const meta = (t, type, bytes) => ({ t, o: -1, b: [0xFF, type].concat(vlq(bytes.length), bytes) });
  const chunk = (id, body) => id.split('').map(c => c.charCodeAt(0)).concat(u32(body.length), body);

  function headEvents(res, title) {
    const P = res.P, us = Math.round(60000000 / P.bpm), ki = MB.keyInfo(P.key, P.scale);
    const ev = [
      meta(0, 0x03, str(title)),
      meta(0, 0x51, [(us >> 16) & 255, (us >> 8) & 255, us & 255]),
      meta(0, 0x58, [4, 2, 24, 8]),
      meta(0, 0x59, [ki.sf & 255, ki.mi])
    ];
    res.chords.forEach((c, b) => ev.push(meta(b * 16 * STEP, 0x06, str(c.name))));
    return ev;
  }
  function noteEvents(res, key) {
    const P = res.P, ch = MB.CH[key], H = (P.human || 0) / 100;
    const rj = MB.rng(P.seed * 977 + key.length * 13);
    const ev = [];
    for (const n of res.tracks[key]) {
      let on = tickOf(n.s, P.swing);
      let off = tickOf(n.s + n.l, P.swing) - 2;          // 2 tick boşluk: aynı notalar üst üste binmez
      const jit = (key === 'drums' && n.n === MB.GM.kick) ? 0 : Math.round(rj() * H * STEP * 0.45);
      on = Math.min(on + jit, Math.max(on, off - 1));
      if (off <= on) off = on + 1;
      ev.push({ t: on, o: 1, b: [0x90 | ch, n.n, n.v] }, { t: off, o: 0, b: [0x80 | ch, n.n, 0] });
    }
    return ev;
  }
  function pack(ev, total) {
    ev.sort((a, b) => a.t - b.t || a.o - b.o);
    const out = []; let last = 0;
    for (const e of ev) { out.push.apply(out, vlq(e.t - last).concat(e.b)); last = e.t; }
    out.push.apply(out, vlq(Math.max(0, total - last)).concat([0xFF, 0x2F, 0x00]));
    return out;
  }

  MB.midiFile = function (res, keys, fmt0) {
    const P = res.P, total = res.bars * 16 * STEP;
    const title = 'Wavejackers HitGen ' + MB.NOTES[P.key] + ' ' + MB.SCALES[P.scale].n + ' ' + P.bpm + ' BPM';
    let trks;
    if (fmt0) {
      let ev = headEvents(res, title);
      keys.forEach(k => { ev = ev.concat(noteEvents(res, k)); });
      trks = [chunk('MTrk', pack(ev, total))];
    } else {
      trks = [chunk('MTrk', pack(headEvents(res, title), total))];
      keys.forEach(k => {
        const ev = [meta(0, 0x03, str(MB.TNAME[k]))].concat(noteEvents(res, k));
        trks.push(chunk('MTrk', pack(ev, total)));
      });
    }
    const head = chunk('MThd', u16(fmt0 ? 0 : 1).concat(u16(trks.length), u16(MB.PPQ)));
    return new Uint8Array(head.concat.apply(head, trks));
  };
  MB.tickOf = tickOf;

  MB.readme = function (res, snd) {
    const P = res.P, ki = MB.keyInfo(P.key, P.scale), g = MB.GENRES[P.genre];
    const sn = id => (MB.byId && MB.byId[id] ? MB.byId[id].name : id);
    return [
      'LOOPSMITH MIDI',
      '==============',
      '',
      'Tonalite     : ' + MB.NOTES[P.key] + ' ' + MB.SCALES[P.scale].n,
      'Gam notaları : ' + ki.notes.join(' '),
      'Camelot      : ' + ki.cam + '  (uyumlu: ' + ki.compat.join(', ') + ')',
      'Tempo        : ' + P.bpm + ' BPM, 4/4, ' + P.bars + ' bar, ' + g.n + ', tohum ' + P.seed,
      'Akorlar      : ' + res.chords.map(c => c.name).join(' | '),
      'Romen        : ' + res.chords.map(c => c.roman).join(' | '),
      'Önizleme sesi: ' + (snd ? 'Akor ' + sn(snd.chords) + ', Bas ' + sn(snd.bass) + ', Melodi ' + sn(snd.melody) + ', Davul ' + ((MB.KITS && MB.KITS[snd.drums]) ? MB.KITS[snd.drums].n : '') : '-'),
      '                (MIDI dosyasında ses yoktur; sesi DAW\'da kendi enstrümanınla ver.)',
      '',
      'DOSYALAR',
      '  *_chords.mid / _bass.mid / _drums.mid / _melody.mid : tek izli dosyalar, enstrümanına ayrı ayrı at',
      '  *_tum-izler_format1.mid : dört iz tek dosyada (çoğu DAW için en iyisi)',
      '  *_tek-iz_format0.mid    : tüm kanallar tek izde (eski DAW ve donanım için)',
      '',
      'DAVUL HARİTASI (General MIDI, kanal 10)',
      '  Kick 36 (C1), Rim 37, Snare 38, Clap 39, Closed Hat 42, Open Hat 46, Perc 64',
      '',
      'DAW İÇİNE AKTARMA',
      '  FL Studio   : .mid dosyasını Channel Rack\'e sürükle. Tüm izler dosyası her izi ayrı kanala böler.',
      '                Tek izi piano roll\'a atmak için piano roll > sol üst menü > File > Import MIDI file.',
      '                Davul için kanalı FPC\'ye çevirip aynı yöntemi kullan.',
      '  Ableton Live: Dosyayı bir MIDI track\'e sürükle. Davul için Drum Rack, kick C1\'e denk gelir.',
      '  Logic Pro   : Dosyayı Tracks alanına sürükle. Drum Machine Designer veya Ultrabeat kullan.',
      '  Cubase      : Dosyayı Project penceresine sürükle ya da File > Import > MIDI File.',
      '  Reaper      : Dosyayı arrange alanına sürükle. Tempo bilgisini içe aktarmak istersen onayla.',
      '  Bitwig      : Dosyayı bir track\'e sürükle. Format 1 dosyası izlere ayrılır.',
      '  Studio One  : Dosyayı Song sayfasına sürükle.',
      '  GarageBand  : Tek izli dosyaları bir yazılım enstrümanı track\'ine sürükle.',
      '',
      'Dosyalar rastgele üretilmiş fikirlerdir; kendi eserinde dilediğin gibi kullanabilirsin.'
    ].join('\r\n');
  };
})();
