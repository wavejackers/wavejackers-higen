// ---- DJ Mag Top 100 (2026) analizine göre eklenen stiller, gruplar, sıralamalar
// Kaynak: DJ Mag Top 100 DJs 2026 sonuçları (djmag.com) ve özetleri. Sıralama yalnızca bilgi amaçlıdır.
HG.BP.slap = [[0,2,1,110],[3,1,3,96],[6,2,1,104],[8,1,1,108],[10,1,3,98],[11,1,1,88],[14,2,3,100]];
HG.BP.garage = [[0,3,1,104],[7,2,1,92],[10,3,3,98],[14,2,1,90]];
HG.BP.rubber = [[0,1,1,110],[2,2,1,96],[4,1,3,90],[6,1,1,104],[7,1,1,86],[10,2,2,98],[12,1,1,108],[14,2,3,96]];
HG.BP.psy = [[1,1,1,92],[2,1,1,104],[3,1,1,96],[5,1,1,92],[6,1,1,104],[7,1,1,96],[9,1,1,92],[10,1,1,104],[11,1,1,96],[13,1,1,92],[14,1,1,104],[15,1,1,96]];
HG.BP.rumble = [[2,1,1,104],[3,1,1,84],[6,1,1,104],[7,1,1,84],[10,1,1,104],[11,1,1,84],[14,1,1,104],[15,1,1,84]];

(function () {
  const S = HG.STYLE;
  S.ummet.artist = 'Ummet Ozcan';
  const meta = { wavejackers: ['techno'], guetta: ['main', 1], tiesto: ['main'], armin: ['trance', 7, 'trance'], domdolla: ['house'], maup: ['house', 55], dvlm: ['main', 5, 'group'], ummet: ['main'], charlotte: ['techno', 6, 'techno'], cox: ['techno', 23] };
  Object.keys(meta).forEach(id => { S[id].grp = meta[id][0]; if (meta[id][1]) S[id].rank = meta[id][1]; if (meta[id][2]) S[id].award = meta[id][2]; });
  const add = s => { HG.STYLES.push(s); S[s.id] = s; };
  add({ id: 'garrix', artist: 'Martin Garrix', grp: 'main', rank: 2, accent: '#3fa9ff', bpm: [126, 128], bpmDef: 128, key: 2, scales: ['major', 'minor'], swing: 0,
    chord: { mode: 'stab', pats: ['house', 'quarter'] }, bass: { pats: ['pumpRoot'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8O, ohat: OFF, fill: 'snare' },
    mel: { mode: 'hook', hr: ['h3', 'h1', 'h6'], dens: .45, oct: 72, rep: .45 },
    sound: { chords: 'mg-wide-chord', bass: 'gt-pump-bass', melody: 'mg-pluck-lead', drums: 'trance' },
    pump: { chords: [.6, .22], bass: [.6, .2], melody: [.35, .2] },
    arr: [['Intro', 16], ['Build', 8], ['Drop', 16], ['Breakdown', 16], ['Build', 8], ['Drop', 32], ['Outro', 16]],
    var: [{ prog: [0, 4, 5, 3] }, { scale: 'minor', prog: [0, 5, 2, 6] }, { prog: [3, 4, 5, 5], cm: 'sustain', cp: 'whole', mm: 'arp', arp: 'up', md: .6 }, { prog: [5, 3, 0, 4], hr: ['h4'], md: .6 }] });
  add({ id: 'anyma', artist: 'Anyma', grp: 'techno', rank: 3, accent: '#9b7bff', bpm: [122, 126], bpmDef: 124, key: 4, scales: ['minor', 'phrygian'], swing: 0,
    chord: { mode: 'sustain', pats: ['whole', 'long'] }, bass: { pats: ['roll16'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8O, ohat: OFF, perc: '..x.......x.....', fill: 'clap' },
    mel: { mode: 'arp', arp: 'up', dens: .6, oct: 72 },
    sound: { chords: 'an-cine-pad', bass: 'an-pulse-bass', melody: 'an-crystal-arp', drums: 'warehouse' },
    pump: { chords: [.45, .25], bass: [.4, .2], melody: [.15, .2] },
    arr: [['Intro', 32], ['Build', 16], ['Drop', 32], ['Breakdown', 32], ['Build', 16], ['Drop', 32], ['Outro', 32]],
    var: [{ prog: [0, 5, 6, 4] }, { scale: 'phrygian', prog: [0, 1, 6, 0], md: .7 }, { prog: [0, 3, 5, 6], mm: 'hook', hr: ['h5', 'h2'], md: .5 }, { prog: [0, 6, 5, 4], cm: 'arp', arp: 'skip', md: .8 }] });
  add({ id: 'alok', artist: 'Alok', grp: 'house', rank: 4, accent: '#ffcf33', bpm: [122, 126], bpmDef: 124, key: 0, scales: ['minor', 'dorian'], swing: 0,
    chord: { mode: 'stab', pats: ['sparse', 'offbeat'] }, bass: { pats: ['slap'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8O, ohat: OFF, perc: '...x......x..x..', fill: 'snare' },
    mel: { mode: 'chop', dens: .45, oct: 72 },
    sound: { chords: 'gt-piano-stab', bass: 'al-slap-bass', melody: 'vox-chop', drums: 'house' },
    pump: { chords: [.5, .2], bass: [.35, .18], melody: [.2, .2] },
    arr: [['Intro', 16], ['Verse', 16], ['Build', 8], ['Drop', 16], ['Break', 16], ['Build', 8], ['Drop', 16], ['Outro', 16]],
    var: [{ prog: [0, 5, 2, 6] }, { prog: [0, 0, 5, 6], bp: 'bounce' }, { scale: 'dorian', prog: [0, 3, 0, 6], mm: 'hook', hr: ['h3'], md: .5 }, { prog: [0, 6, 5, 6], cm: 'sustain', cp: 'long', md: .8 }] });
  add({ id: 'timmy', artist: 'Timmy Trumpet', grp: 'trance', rank: 8, accent: '#ff6b3d', bpm: [128, 140], bpmDef: 138, key: 9, scales: ['minor', 'phrygian'], swing: 0,
    chord: { mode: 'stab', pats: ['rave', 'sparse'] }, bass: { pats: ['psy'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8O, ohat: OFF, fill: 'snare' },
    mel: { mode: 'hook', hr: ['h4', 'h1'], dens: .55, oct: 72, rep: .55 },
    sound: { chords: 'rave-stab', bass: 'ip-psy-bass', melody: 'tt-brass-lead', drums: 'trance' },
    pump: { chords: [.5, .2], bass: [.2, .12], melody: [.3, .2] },
    arr: [['Intro', 16], ['Build', 16], ['Drop', 32], ['Breakdown', 16], ['Build', 16], ['Drop', 32], ['Outro', 16]],
    var: [{ prog: [0, 5, 2, 6] }, { scale: 'phrygian', prog: [0, 1, 0, 6], mm: 'riff', md: .6 }, { prog: [0, 0, 5, 6], bp: 'pumpRoot', hr: ['h1', 'h3'], md: .5 }, { scale: 'harmonic', prog: [0, 5, 3, 4], mm: 'arp', arp: 'up', md: .7 }] });
  add({ id: 'fred', artist: 'Fred again..', grp: 'house', rank: 16, accent: '#8ad4ff', bpm: [128, 136], bpmDef: 132, key: 2, scales: ['major', 'minor'], swing: 22,
    chord: { mode: 'sustain', pats: ['long', 'whole'] }, bass: { pats: ['garage'], oct: 36 },
    drums: { kick: 'x......x..x.....', clap: C24, chat: H8, ohat: '..x.......x.....', rim: '.....x.......x..', fill: 'none' },
    mel: { mode: 'chop', dens: .55, oct: 72 },
    sound: { chords: 'fa-felt-keys', bass: 'sub-sine', melody: 'vox-chop', drums: 'deep' },
    pump: { chords: [.45, .3], bass: [.3, .2], melody: [.2, .25] },
    arr: [['Intro', 16], ['Verse', 16], ['Build', 8], ['Drop', 16], ['Breakdown', 16], ['Drop', 16], ['Outro', 16]],
    var: [{ prog: [3, 0, 4, 5] }, { scale: 'minor', prog: [0, 5, 2, 6] }, { prog: [5, 3, 0, 4], cm: 'stab', cp: 'tres', md: .8 }, { prog: [0, 2, 3, 4], mm: 'hook', hr: ['h2', 'h5'], md: .5 }] });
  add({ id: 'fisher', artist: 'FISHER', grp: 'house', award: 'house', accent: '#7dff8f', bpm: [125, 128], bpmDef: 126, key: 5, scales: ['minor', 'dorian'], swing: 10,
    chord: { mode: 'stab', pats: ['sparse', 'push'] }, bass: { pats: ['rubber'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8O, ohat: OFF, rim: '...x...x...x...x', perc: 'x..x..x..x..x...', fill: 'clap' },
    mel: { mode: 'chop', dens: .3, oct: 72 },
    sound: { chords: 'organ-stab', bass: 'fi-rubber-bass', melody: 'vox-chop', drums: 'house' },
    pump: { chords: [.4, .2], bass: [.45, .18], melody: [.15, .2] },
    arr: [['Intro', 16], ['Groove', 32], ['Break', 16], ['Groove', 32], ['Outro', 16]],
    var: [{ prog: [0, 0, 0, 0] }, { prog: [0, 0, 6, 5], bp: 'funk' }, { scale: 'dorian', prog: [0, 3, 0, 3], mm: 'riff', md: .3 }, { prog: [0, 0, 5, 6], mm: 'hook', hr: ['h1'], md: .4 }] });
  add({ id: 'indira', artist: 'Indira Paganotto', grp: 'trance', rank: 26, award: 'psy', accent: '#c86bff', bpm: [136, 142], bpmDef: 140, key: 9, scales: ['phrygian', 'minor'], swing: 0,
    chord: { mode: 'sustain', pats: ['whole'] }, bass: { pats: ['psy'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8O, ohat: OFF, rim: '.x.....x.......x', perc: '...x...x...x...x', fill: 'clap' },
    mel: { mode: 'riff', dens: .55, oct: 60 },
    sound: { chords: 'dark-drone', bass: 'ip-psy-bass', melody: 'ip-acid-lead', drums: 'techno' },
    pump: { chords: [.3, .2], bass: [.1, .1], melody: [.1, .15] },
    arr: [['Intro', 32], ['Build', 16], ['Peak', 32], ['Break', 16], ['Peak', 32], ['Outro', 32]],
    var: [{ prog: [0, 0, 1, 0] }, { scale: 'minor', prog: [0, 0, 6, 5], mm: 'arp', arp: 'up', md: .8 }, { prog: [0, 1, 0, 6], bp: 'acid', md: .6 }, { prog: [0, 0, 0, 1], mm: 'riff', md: .3 }] });
  add({ id: 'landry', artist: 'Sara Landry', grp: 'hard', rank: 64, award: 'hard', accent: '#ff2e63', bpm: [145, 155], bpmDef: 150, key: 9, scales: ['minor', 'phrygian'], swing: 0,
    chord: { mode: 'stab', pats: ['sparse', 'rave'] }, bass: { pats: ['rumble'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H16, ohat: OFF, rim: '...x..x....x..x.', fill: 'clap' },
    mel: { mode: 'riff', dens: .4, oct: 72 },
    sound: { chords: 'rave-stab', bass: 'sl-rumble-bass', melody: 'sl-hard-lead', drums: 'hard' },
    pump: { chords: [.5, .2], bass: [.7, .15], melody: [.3, .15] },
    arr: [['Intro', 16], ['Build', 16], ['Peak', 32], ['Break', 16], ['Peak', 32], ['Outro', 16]],
    var: [{ prog: [0, 0, 0, 6] }, { scale: 'phrygian', prog: [0, 1, 0, 1], md: .5 }, { prog: [0, 5, 6, 0], cm: 'sustain', cp: 'whole', mm: 'hook', hr: ['h4'], md: .5 }, { prog: [0, 0, 5, 6], bp: 'roll16', dr: { chat: H8 } }] });
  // ---- 2026 eklemeleri: melodik, afro ve organik house (Solomun, Miss Monique, Mahmut Orhan, HUGEL, Black Coffee)
  HG.CP.montuno = [[0,1,96],[2,1,84],[3,1,92],[5,1,84],[6,1,96],[8,1,88],[10,1,96],[11,1,84],[13,1,92],[14,1,86]];
  HG.CP.afroStab = [[0,3,96],[6,2,84],[10,3,92]];
  HG.BP.solo = [[0,3,1,104],[3,1,1,86],[6,2,1,96],[8,1,1,100],[10,2,2,90],[13,1,3,86],[14,2,1,94]];
  HG.BP.deepSync = [[0,2,1,100],[3,1,1,84],[6,1,3,90],[7,1,1,82],[10,2,1,96],[12,1,2,86],[14,2,1,92]];
  HG.BP.tumbao = [[0,2,1,104],[6,2,2,98],[10,1,3,90],[12,2,1,100],[14,1,2,88]];
  HG.BP.afroSub = [[0,3,1,104],[6,1,1,88],[7,2,3,92],[10,2,1,96],[14,2,2,88]];
  add({ id: 'solomun', artist: 'Solomun', grp: 'house', rank: 41, accent: '#e8b86b', bpm: [120, 124], bpmDef: 122, key: 4, scales: ['minor', 'dorian'], swing: 6,
    chord: { mode: 'sustain', pats: ['long', 'whole'] }, bass: { pats: ['solo'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8O, ohat: OFF, rim: '...x.......x..x.', perc: '......x.......x.', fill: 'clap' },
    mel: { mode: 'hook', hr: ['h5', 'h2', 'h6'], dens: .4, oct: 72, rep: .45 },
    sound: { chords: 'so-velvet-pad', bass: 'so-deep-bass', melody: 'so-emotive-lead', drums: 'deep' },
    pump: { chords: [.4, .25], bass: [.35, .2], melody: [.15, .2] },
    arr: [['Intro', 32], ['Build', 16], ['Drop', 32], ['Breakdown', 32], ['Build', 16], ['Drop', 32], ['Outro', 32]],
    var: [{ prog: [0, 5, 2, 6] }, { scale: 'dorian', prog: [0, 3, 0, 6], cm: 'stab', cp: 'house' }, { prog: [0, 6, 5, 4], mm: 'arp', arp: 'pp', md: .7 }, { prog: [0, 3, 5, 6], cm: 'arp', mm: 'hook', hr: ['h2'], md: .5 }] });
  add({ id: 'monique', artist: 'Miss Monique', grp: 'techno', rank: 61, accent: '#ff7eb6', bpm: [122, 126], bpmDef: 124, key: 7, scales: ['minor', 'dorian'], swing: 0,
    chord: { mode: 'arp', pats: ['whole'], arp: 'skip' }, bass: { pats: ['roll16'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8O, ohat: OFF, rim: '.......x.......x', perc: '..x.......x.....', fill: 'snare' },
    mel: { mode: 'arp', arp: 'up', dens: .55, oct: 72 },
    sound: { chords: 'mm-dream-pluck', bass: 'mm-prog-bass', melody: 'mm-silk-lead', drums: 'warehouse' },
    pump: { chords: [.45, .22], bass: [.5, .2], melody: [.2, .2] },
    arr: [['Intro', 32], ['Build', 16], ['Drop', 32], ['Breakdown', 32], ['Build', 16], ['Drop', 32], ['Outro', 32]],
    var: [{ prog: [0, 5, 6, 4] }, { prog: [0, 3, 6, 5], cm: 'sustain', cp: 'long', mm: 'hook', hr: ['h5', 'h2'], md: .5 }, { scale: 'dorian', prog: [0, 3, 0, 6], arp: 'pp' }, { prog: [5, 6, 0, 0], bp: 'trance16', md: .8 }] });
  add({ id: 'orhan', artist: 'Mahmut Orhan', grp: 'afro', accent: '#ffb057', bpm: [118, 124], bpmDef: 121, key: 4, scales: ['hijaz', 'minor', 'harmonic'], swing: 8,
    chord: { mode: 'stab', pats: ['offbeat', 'house'] }, bass: { pats: ['deepSync'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8O, ohat: OFF, rim: '.......x.......x', perc: 'x..x..x...x..x..', fill: 'clap' },
    mel: { mode: 'hook', hr: ['h6', 'h1', 'h3'], dens: .45, oct: 72, rep: .4 },
    sound: { chords: 'mo-warm-keys', bass: 'mo-deep-bass', melody: 'mo-sax-lead', drums: 'deep' },
    pump: { chords: [.4, .25], bass: [.35, .2], melody: [.15, .2] },
    arr: [['Intro', 16], ['Verse', 16], ['Build', 8], ['Drop', 32], ['Breakdown', 16], ['Build', 8], ['Drop', 32], ['Outro', 16]],
    var: [{ prog: [0, 1, 0, 6] }, { scale: 'minor', prog: [0, 5, 2, 6], hr: ['h5', 'h2'], md: .8 }, { scale: 'harmonic', prog: [0, 3, 4, 0], cm: 'sustain', cp: 'long' }, { prog: [0, 1, 6, 1], mm: 'arp', arp: 'pp', md: .6 }] });
  add({ id: 'hugel', artist: 'HUGEL', grp: 'afro', rank: 48, accent: '#ff5e3a', bpm: [122, 126], bpmDef: 124, key: 9, scales: ['minor', 'harmonic'], swing: 4,
    chord: { mode: 'stab', pats: ['montuno', 'tres'] }, bass: { pats: ['tumbao'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8O, ohat: OFF, rim: '...x..x.....x...', perc: 'x..x..x...x.x...', fill: 'snare' },
    mel: { mode: 'chop', dens: .5, oct: 72 },
    sound: { chords: 'hu-latin-piano', bass: 'hu-latin-bass', melody: 'hu-brass', drums: 'house' },
    pump: { chords: [.45, .2], bass: [.45, .18], melody: [.25, .2] },
    arr: [['Intro', 16], ['Verse', 16], ['Build', 8], ['Drop', 16], ['Break', 16], ['Build', 8], ['Drop', 16], ['Outro', 16]],
    var: [{ prog: [0, 6, 5, 6] }, { scale: 'harmonic', prog: [0, 3, 5, 4] }, { prog: [0, 0, 5, 6], mm: 'hook', hr: ['h1', 'h4'], md: .6 }, { scale: 'dorian', prog: [0, 3, 0, 3], bp: 'funk', md: 1.1 }] });
  add({ id: 'blackcoffee', artist: 'Black Coffee', grp: 'afro', rank: 14, accent: '#c89a63', bpm: [118, 122], bpmDef: 120, key: 5, scales: ['minor', 'dorian'], swing: 14, sev: true,
    chord: { mode: 'stab', pats: ['afroStab', 'sparse'] }, bass: { pats: ['afroSub'], oct: 36 },
    drums: { kick: K4, clap: '........x.......', chat: H8, ohat: OFF, rim: '...x...x..x....x', perc: '..x..x.x..x..x.x', fill: 'none' },
    mel: { mode: 'hook', hr: ['h2', 'h5', 'h6'], dens: .3, oct: 72, rep: .5 },
    sound: { chords: 'bc-soul-keys', bass: 'bc-afro-sub', melody: 'bc-kalimba', drums: 'afro' },
    pump: { chords: [.3, .25], bass: [.3, .2], melody: [.1, .2] },
    arr: [['Intro', 32], ['Groove', 32], ['Break', 16], ['Groove', 32], ['Outro', 32]],
    var: [{ prog: [0, 3, 0, 3] }, { scale: 'dorian', prog: [0, 3, 6, 3] }, { prog: [0, 5, 3, 4], cm: 'sustain', cp: 'long', md: .6 }, { prog: [0, 6, 5, 6], mm: 'arp', arp: 'gate', md: .6 }] });
  HG.GROUPS = ['all', 'techno', 'house', 'main', 'trance', 'hard', 'afro'];
})();
