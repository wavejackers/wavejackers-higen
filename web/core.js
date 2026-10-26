const MB = {};
MB.NOTES = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
MB.SCALES = {
  major:      { n: 'Majör',          iv: [0,2,4,5,7,9,11] },
  minor:      { n: 'Minör',  iv: [0,2,3,5,7,8,10] },
  dorian:     { n: 'Dorian',         iv: [0,2,3,5,7,9,10] },
  phrygian:   { n: 'Phrygian',       iv: [0,1,3,5,7,8,10] },
  mixolydian: { n: 'Mixolydian',     iv: [0,2,4,5,7,9,10] },
  harmonic:   { n: 'Harmonik minör', iv: [0,2,3,5,7,8,11] }
};
// Akor ilerlemeleri: gam derecesi indeksleri (0 = I/i)
MB.POOLS = {
  major:      [[0,4,5,3],[0,5,3,4],[0,3,4,3],[5,3,0,4],[0,2,3,4]],
  minor:      [[0,5,2,6],[0,6,5,6],[0,3,6,2],[0,2,6,5],[0,3,0,6]],
  dorian:     [[0,3,0,6],[0,1,3,0],[0,6,3,0],[0,3,6,3]],
  phrygian:   [[0,1,0,6],[0,1,2,1],[0,6,5,6],[0,5,6,1]],
  mixolydian: [[0,6,3,0],[0,3,6,3],[0,6,5,6],[0,4,6,3]],
  harmonic:   [[0,3,4,0],[0,5,4,0],[0,3,4,4],[0,5,3,4]]
};
const GM = { kick: 36, rim: 37, snare: 38, clap: 39, chat: 42, ohat: 46, perc: 64 };
MB.GM = GM;
MB.GENRES = {
  house: { n: 'House', bpm: [122,128], swing: 8, cstyle: 'stab', bass: 'offbeat', mel: .30,
    dr: { kick:'x...x...x...x...', clap:'....x.......x...', chat:'.x.x.x.x.x.x.x.x', ohat:'..x...x...x...x.' },
    ghost: [['rim',[3,7,11,15],.12]], fill: true },
  deep: { n: 'Deep House', bpm: [118,124], swing: 14, cstyle: 'stab', bass: 'offbeat', mel: .24,
    dr: { kick:'x...x...x...x...', clap:'....x.......x...', chat:'x.x.x.x.x.x.x.x.', ohat:'..x...x...x...x.', rim:'.......x.......x' },
    ghost: [['perc',[3,6,11,14],.18]], fill: true },
  techno: { n: 'Techno', bpm: [128,136], swing: 0, cstyle: 'sustain', bass: 'roll', mel: .18,
    dr: { kick:'x...x...x...x...', clap:'....x.......x...', chat:'x.x.x.x.x.x.x.x.', ohat:'..x...x...x...x.' },
    ghost: [['rim',[3,5,7,11,13,15],.28]], fill: true },
  mtechno: { n: 'Melodic Techno', bpm: [120,126], swing: 0, cstyle: 'arp', bass: 'roll', mel: .42,
    dr: { kick:'x...x...x...x...', clap:'....x.......x...', chat:'.x.x.x.x.x.x.x.x', ohat:'..x...x...x...x.' },
    ghost: [['rim',[7,11,15],.2]], fill: true },
  trance: { n: 'Trance', bpm: [136,140], swing: 0, cstyle: 'arp', bass: 'roll', mel: .55,
    dr: { kick:'x...x...x...x...', clap:'....x.......x...', chat:'xxxxxxxxxxxxxxxx', ohat:'..x...x...x...x.' },
    ghost: [], fill: true },
  afro: { n: 'Afro House', bpm: [120,124], swing: 10, cstyle: 'stab', bass: 'sync', mel: .36,
    dr: { kick:'x...x...x...x...', clap:'........x.......', chat:'x.x.x.x.x.x.x.x.', perc:'x..x..x...x..x..' },
    ghost: [['rim',[2,6,10,14],.25]], fill: true },
  dnb: { n: 'Drum & Bass', bpm: [170,176], swing: 0, cstyle: 'sustain', bass: 'dnb', low: true, mel: .30,
    dr: { kick:'x.........x.....', snare:'....x.......x...', chat:'x.x.x.x.x.x.x.x.' },
    ghost: [['snare',[7,15],.25],['kick',[2,14],.2]], fill: true },
  trap: { n: 'Trap', bpm: [135,150], swing: 0, cstyle: 'arp', bass: 'trap', low: true, mel: .36, hd: .14,
    dr: { kick:'x.........x.....', snare:'........x.......', chat:'xxxxxxxxxxxxxxxx' },
    ghost: [['kick',[3,6,13,14],.3]], fill: false },
  lofi: { n: 'Lo-fi Hip-Hop', bpm: [78,90], swing: 40, cstyle: 'stab', bass: 'lofi', low: true, mel: .22,
    dr: { kick:'x.....x..x......', snare:'....x.......x...', chat:'x.x.x.x.x.x.x.x.' },
    ghost: [['kick',[11,14],.3]], fill: false }
};
MB.STABS = [
  [[0,2],[3,2],[6,2],[10,2]],
  [[2,2],[6,2],[10,2],[14,2]],
  [[0,3],[3,3],[6,3],[10,2],[13,3]]
];
MB.BASSPAT = {
  offbeat: [[2,2,1],[6,2,1],[10,2,1],[14,2,1]],
  sync:    [[0,3,1],[3,3,1],[6,2,2,.6],[10,3,1],[13,3,1,.8]],
  dnb:     [[0,6,1],[6,2,1,.5],[10,5,1],[15,1,2,.4]],
  trap:    [[0,6,1],[7,2,1,.5],[10,4,1,.6],[14,2,2,.5]],
  lofi:    [[0,5,1],[6,3,1,.7],[10,5,1],[14,2,2,.5]]
};

MB.rng = function (seed) {
  let a = seed >>> 0;
  return function () {
    a = (a + 0x6D2B79F5) >>> 0;
    let t = a;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
};
const mod12 = x => ((x % 12) + 12) % 12;
const pick = (r, a) => a[Math.floor(r() * a.length)];
const clamp = (x, a, b) => Math.max(a, Math.min(b, x));
const ROMAN = ['I','II','III','IV','V','VI','VII'];
const MAJ = [0,2,4,5,7,9,11];

