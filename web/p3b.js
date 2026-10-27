// ---- Akor ilerleme kütüphanesi (derece numaraları; romen rakamları gama göre hesaplanır)
MB.PROGS = [
  { id: 'pop',  n: 'Pop 1-5-6-4',          d: [0,4,5,3] },
  { id: 'emo',  n: 'Duygusal 6-4-1-5',     d: [5,3,0,4] },
  { id: 'fif',  n: '50\'ler 1-6-4-5',      d: [0,5,3,4] },
  { id: 'epic', n: 'Epik 1-7-6-7',         d: [0,6,5,6] },
  { id: 'dark', n: 'Karanlık 1-6-3-7',     d: [0,5,2,6] },
  { id: 'mel',  n: 'Melodik 1-4-7-3',      d: [0,3,6,2] },
  { id: 'jazz', n: 'Caz 2-5-1-1',          d: [1,4,0,0] },
  { id: 'dor',  n: 'Dorian 1-4-1-7',       d: [0,3,0,6] },
  { id: 'hyp',  n: 'Hipnotik 1-1-2-1',     d: [0,0,1,0] },
  { id: 'lift', n: 'Yükselen 1-3-4-5',     d: [0,2,3,4] }
];

// ---- Hızlı başlangıç (arama niyetine göre: tür + his + tempo)
MB.PRESETS = [
  { n: 'Karanlık Techno',    sub: 'A Phrygian · 132',  key: 9, scale: 'phrygian', genre: 'techno',  bpm: 132, prog: 'hyp',  sev: false, seed: 11 },
  { n: 'Deep House Akşamı',  sub: 'D Dorian · 122',    key: 2, scale: 'dorian',   genre: 'deep',    bpm: 122, prog: 'dor',  sev: true,  seed: 5 },
  { n: 'Melodik Techno',     sub: 'A Minör · 123',     key: 9, scale: 'minor',    genre: 'mtechno', bpm: 123, prog: 'dark', sev: false, seed: 21 },
  { n: 'Trance Marşı',       sub: 'E Minör · 138',     key: 4, scale: 'minor',    genre: 'trance',  bpm: 138, prog: 'dark', sev: false, seed: 9 },
  { n: 'Afro House',         sub: 'G Dorian · 122',    key: 7, scale: 'dorian',   genre: 'afro',    bpm: 122, prog: 'dor',  sev: true,  seed: 14 },
  { n: 'Drum & Bass Roller', sub: 'F Minör · 174',     key: 5, scale: 'minor',    genre: 'dnb',     bpm: 174, prog: 'epic', sev: false, seed: 3 },
  { n: 'Lo-fi Gece',         sub: 'C Majör · 82',      key: 0, scale: 'major',    genre: 'lofi',    bpm: 82,  prog: 'jazz', sev: true,  seed: 8 },
  { n: 'Trap Karanlık',      sub: 'C# Minör · 140',    key: 1, scale: 'minor',    genre: 'trap',    bpm: 140, prog: 'dark', sev: false, seed: 6 }
];

// ---- Aranjman şablonları: [bölüm, bar]
MB.ARR = {
  house:   [['Intro',16],['Build',8],['Drop',32],['Break',16],['Drop',32],['Outro',16]],
  deep:    [['Intro',16],['Groove',32],['Break',16],['Groove',32],['Outro',16]],
  techno:  [['Intro',32],['Build',16],['Peak',32],['Break',16],['Peak',32],['Outro',32]],
  mtechno: [['Intro',32],['Build',16],['Drop',32],['Breakdown',32],['Drop',32],['Outro',16]],
  trance:  [['Intro',32],['Build',16],['Drop',32],['Breakdown',32],['Build',16],['Drop',32],['Outro',32]],
  afro:    [['Intro',16],['Groove',32],['Break',16],['Groove',32],['Outro',16]],
  dnb:     [['Intro',16],['Build',16],['Drop',32],['Break',16],['Drop',32],['Outro',16]],
  trap:    [['Intro',8],['Verse',16],['Hook',16],['Verse',16],['Hook',16],['Outro',8]],
  lofi:    [['Intro',4],['A',8],['B',8],['A',8],['Outro',4]]
};

// ---- Tonalite bilgisi: Camelot kodu, uyumlu tonlar, gam notaları, MIDI anahtar imzası
MB.keyInfo = function (key, scale) {
  const off = { major: 0, minor: 3, dorian: 10, phrygian: 8, mixolydian: 5, harmonic: 3, hijaz: 8 }[scale];
  const rp = (key + off) % 12;                         // ilişkili majör tonik
  const num = ((7 + (rp * 7) % 12) % 12) + 1;
  const letter = (scale === 'major' || scale === 'mixolydian') ? 'B' : 'A';
  const other = letter === 'A' ? 'B' : 'A';
  const up = num % 12 + 1, down = ((num + 10) % 12) + 1;
  return {
    cam: num + letter,
    compat: [down + letter, up + letter, num + other],
    sf: ((rp * 7 + 6) % 12) - 6,
    mi: (scale === 'minor' || scale === 'harmonic' || scale === 'hijaz') ? 1 : 0,
    notes: MB.SCALES[scale].iv.map(i => MB.NOTES[(key + i) % 12])
  };
};
