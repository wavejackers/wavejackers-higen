// ---- Sanatçı ilhamlı stil kütüphanesi
// Her stil, o ismin genel üslubunu (tempo, armoni dili, ritim hissi, ses paleti) taşır.
// Hiçbir parçanın notaları kopyalanmaz; her üretim tohuma göre özgündür.
const HG = {};
HG.CP = { // akor kalıpları: [adım, uzunluk, velocity]
  quarter: [[0,4,100],[4,4,86],[8,4,92],[12,4,86]],
  offbeat: [[2,2,100],[6,2,92],[10,2,100],[14,2,92]],
  tres:    [[0,3,100],[3,3,88],[6,2,92],[8,3,96],[11,3,88],[14,2,90]],
  house:   [[0,2,100],[3,2,86],[6,2,90],[10,2,96]],
  push:    [[0,2,100],[3,1,84],[6,2,92],[8,2,96],[11,1,84],[14,2,92]],
  sparse:  [[0,2,100],[10,2,90]],
  long:    [[0,8,90],[8,8,84]],
  whole:   [[0,16,84]],
  funk:    [[1,1,92],[3,1,84],[6,2,96],[9,1,88],[11,2,92],[14,1,84]],
  rave:    [[0,2,104],[4,1,80],[6,2,98],[8,2,100],[12,1,80],[14,2,96]]
};
// bas kalıpları: [adım, uzunluk, nota kodu, velocity]; kod 1 kök, 2 beşli, 3 oktav, 4 üçlü, 5 b7
HG.BP = {
  pumpRoot: [[0,4,1,104],[4,4,1,96],[8,4,1,100],[12,4,1,96]],
  longRoot: [[0,16,1,100]],
  offbeat8: [[2,2,1,100],[6,2,1,100],[10,2,1,100],[14,2,1,100]],
  roll16:   [[1,1,1,84],[2,1,1,96],[3,1,3,92],[5,1,1,84],[6,1,1,96],[7,1,1,88],[9,1,1,84],[10,1,1,96],[11,1,3,92],[13,1,1,84],[14,1,1,96],[15,1,2,90]],
  trance16: [[1,1,1,82],[2,1,1,100],[3,1,1,86],[5,1,1,82],[6,1,1,100],[7,1,1,86],[9,1,1,82],[10,1,1,100],[11,1,1,86],[13,1,1,82],[14,1,1,100],[15,1,1,86]],
  hook:     [[0,2,1,108],[3,1,1,92],[6,2,3,100],[8,2,1,106],[10,1,2,90],[12,2,1,100],[15,1,3,88]],
  funk:     [[0,1,1,106],[2,1,1,86],[3,2,3,96],[6,2,1,100],[8,1,1,104],[10,2,2,92],[11,1,1,84],[14,2,3,96]],
  bounce:   [[0,2,1,108],[2,1,1,80],[4,1,3,92],[6,2,1,100],[8,2,1,106],[11,1,2,88],[12,2,1,98],[14,1,5,90]],
  acid:     [[0,1,1,104],[1,1,1,80],[3,1,3,100],[4,1,1,90],[6,1,5,96],[7,1,1,82],[8,1,1,104],[10,1,3,98],[11,1,1,84],[12,1,2,92],[14,1,1,96],[15,1,3,88]],
  tres:     [[0,3,1,104],[3,3,1,92],[6,2,2,96],[8,3,1,104],[11,3,1,92],[14,2,3,92]]
};
// melodi ritim şablonları (hook modu): [adım, uzunluk]
HG.HR = {
  h1: [[0,3],[3,3],[6,2],[8,3],[11,3],[14,2]],
  h2: [[0,6],[6,2],[8,4],[12,4]],
  h3: [[0,2],[2,2],[4,4],[8,2],[10,2],[12,4]],
  h4: [[0,1],[1,1],[2,2],[4,1],[5,1],[6,2],[8,1],[9,1],[10,2],[12,2],[14,2]],
  h5: [[0,8],[8,4],[12,4]],
  h6: [[0,3],[3,2],[5,3],[8,2],[10,3],[13,3]]
};
// arp sıraları: akor sesleri (iki oktav) üzerinde indeks, -1 sus
HG.ARP = {
  up:     [0,1,2,3,4,5,4,3],
  pp:     [0,1,2,3,2,1],
  trance: [0,2,4,2],
  skip:   [0,2,1,3,2,4,3,5],
  gate:   [0,-1,2,-1,1,-1,2,-1]
};

const K4 = 'x...x...x...x...', C24 = '....x.......x...', OFF = '..x...x...x...x.', H8 = 'x.x.x.x.x.x.x.x.', H8O = '.x.x.x.x.x.x.x.x', H16 = 'xxxxxxxxxxxxxxxx';

HG.STYLES = [
  { id: 'wavejackers', artist: 'Wavejackers', tag: 'Melodik driving techno', accent: '#2fd6c6',
    blurb: 'Sürükleyici techno kick\'i üstünde duygusal arp ve geniş pad. Just Culture çizgisinde.',
    bpm: [124, 128], bpmDef: 126, key: 9, scales: ['minor', 'dorian'], swing: 0,
    chord: { mode: 'sustain', pats: ['whole', 'long'] }, bass: { pats: ['roll16'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8, ohat: OFF, rim: '...x...x...x...x', fill: 'clap' },
    mel: { mode: 'arp', arp: 'trance', dens: .5, oct: 72 },
    sound: { chords: 'wj-dark-pad', bass: 'wj-hypno-bass', melody: 'wj-glass-arp', drums: 'warehouse' },
    pump: { chords: [.5, .25], bass: [.45, .2], melody: [.15, .2] },
    arr: [['Intro', 32], ['Build', 16], ['Drop', 32], ['Breakdown', 32], ['Build', 16], ['Drop', 32], ['Outro', 32]],
    var: [
      { n: 'Sürükleyici', prog: [0,5,2,6], note: 'Klasik melodik techno hareketi, rolling bas.' },
      { n: 'Gece sürüşü', scale: 'dorian', prog: [0,3,0,6], md: .8, note: 'Dorian rengi, daha açık ve dans edilesi.' },
      { n: 'Duygusal', prog: [0,6,5,6], mm: 'hook', hr: ['h5','h2'], md: .6, dr: { chat: OFF }, note: 'Uzun notalı melodi, hafif davul.' },
      { n: 'Sert', scale: 'phrygian', prog: [0,0,1,0], bp: 'acid', mm: 'riff', cm: 'stab', cp: 'sparse', dr: { chat: H16 }, note: 'Phrygian gerilim, acid bas, kısa riff.' }
    ] },
  { id: 'guetta', artist: 'David Guetta', tag: 'Big room pop', accent: '#ff5a7a',
    blurb: 'Kulüp marşı: basit akorlar, piyano stab\'ları ve kicke kilitli pompa.',
    bpm: [126, 130], bpmDef: 128, key: 9, scales: ['minor', 'major'], swing: 0,
    chord: { mode: 'stab', pats: ['quarter', 'tres'] }, bass: { pats: ['pumpRoot'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8, ohat: OFF, fill: 'snare' },
    mel: { mode: 'hook', hr: ['h1','h2','h3'], dens: .35, oct: 72, rep: .5 },
    sound: { chords: 'gt-piano-stab', bass: 'gt-pump-bass', melody: 'gt-anthem-lead', drums: 'house' },
    pump: { chords: [.55, .22], bass: [.6, .2], melody: [.35, .2] },
    arr: [['Intro', 16], ['Build', 8], ['Drop', 16], ['Breakdown', 16], ['Build', 8], ['Drop', 32], ['Outro', 16]],
    var: [
      { n: 'Marş', prog: [0,5,2,6], note: 'i–VI–III–VII, çeyrek stab\'lar.' },
      { n: 'Parıltı', scale: 'major', prog: [0,4,5,3], cp: 'tres', note: 'Majör, pop uyumu, 3-3-2 stab.' },
      { n: 'Kulüp gecesi', scale: 'harmonic', prog: [0,3,5,4], cp: 'offbeat', note: 'Harmonik minör gerilimi, offbeat stab.' },
      { n: 'Drop odaklı', prog: [5,6,0,0], cm: 'sustain', cp: 'whole', mm: 'hook', hr: ['h4'], md: .5, note: 'Pad altında hızlı lead riff\'i.' }
    ] },
  { id: 'tiesto', artist: 'Tiësto', tag: 'Festival ve progressive', accent: '#ffb23f',
    blurb: 'Festival enerjisi: parlak lead, yükselen akorlar, net build-up ve drop.',
    bpm: [126, 128], bpmDef: 128, key: 7, scales: ['minor', 'major'], swing: 0,
    chord: { mode: 'stab', pats: ['house', 'rave'] }, bass: { pats: ['offbeat8'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8, ohat: OFF, perc: 'x......x..x.....', fill: 'snare' },
    mel: { mode: 'hook', hr: ['h3','h6','h1'], dens: .4, oct: 72, rep: .4 },
    sound: { chords: 'super-stab', bass: 'ar-rolling-bass', melody: 'super-lead', drums: 'trance' },
    pump: { chords: [.5, .22], bass: [.55, .2], melody: [.3, .2] },
    arr: [['Intro', 16], ['Build', 8], ['Drop', 32], ['Breakdown', 16], ['Build', 8], ['Drop', 32], ['Outro', 16]],
    var: [
      { n: 'Festival', prog: [0,5,2,6], note: 'Klasik festival döngüsü.' },
      { n: 'Yükseliş', scale: 'major', prog: [0,4,5,3], md: .45, note: 'Majör, parlak ve umutlu.' },
      { n: 'Tech groove', prog: [0,0,6,6], cm: 'stab', cp: 'house', bp: 'bounce', mm: 'chop', md: .3, dr: { chat: H8O, rim: '.......x.......x' }, note: 'Tech house\'a kayan daha sade hal.' },
      { n: 'Trance kökenli', prog: [0,5,3,4], cm: 'arp', mm: 'arp', md: .55, bp: 'trance16', note: 'Trance köklerine selam: arp ve rolling bas.' }
    ] },
  { id: 'armin', artist: 'Armin van Buuren', tag: 'Trance', accent: '#6f8cff',
    blurb: 'Duygusal trance: rolling 16\'lık bas, arp\'lar ve genişleyen supersaw.',
    bpm: [136, 140], bpmDef: 138, key: 9, scales: ['minor', 'harmonic'], swing: 0,
    chord: { mode: 'arp', pats: ['whole'], arp: 'trance' }, bass: { pats: ['trance16'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8, ohat: OFF, fill: 'snare' },
    mel: { mode: 'hook', hr: ['h5','h2','h3'], dens: .4, oct: 72, rep: .35 },
    sound: { chords: 'trance-pluck', bass: 'ar-rolling-bass', melody: 'ar-uplift-lead', drums: 'trance' },
    pump: { chords: [.4, .22], bass: [.5, .2], melody: [.25, .2] },
    arr: [['Intro', 32], ['Build', 16], ['Drop', 32], ['Breakdown', 32], ['Build', 16], ['Drop', 32], ['Outro', 32]],
    var: [
      { n: 'Yolculuk', prog: [0,5,2,6], note: 'Arp akorlar, uzun notalı duygusal lead.' },
      { n: 'Euphoria', prog: [5,3,0,4], mm: 'arp', md: .6, note: 'Yükselen arp melodisi.' },
      { n: 'Piyano dinginliği', prog: [0,6,5,6], cm: 'sustain', cp: 'whole', md: .4, dr: { chat: OFF, kick: K4 }, note: 'Breakdown havası, pad ve sade nota.' },
      { n: 'Psy dokunuşu', scale: 'phrygian', prog: [0,0,1,0], mm: 'arp', arp: 'up', md: .7, note: 'Phrygian ve sürekli akan 16\'lıklar.' }
    ] },
  { id: 'domdolla', artist: 'Dom Dolla', tag: 'Tech house', accent: '#ff8a2a',
    blurb: 'Funk\'lı, zıplayan bas, shuffle hi-hat\'ler ve vokal chop çizgileri.',
    bpm: [124, 127], bpmDef: 126, key: 9, scales: ['minor', 'dorian'], swing: 12,
    chord: { mode: 'stab', pats: ['funk', 'push'] }, bass: { pats: ['funk'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8, ohat: OFF, rim: '.......x.......x', perc: 'x..x..x...x..x..', fill: 'clap' },
    mel: { mode: 'chop', dens: .4, oct: 72 },
    sound: { chords: 'dd-funk-stab', bass: 'dd-bounce-bass', melody: 'vox-chop', drums: 'house' },
    pump: { chords: [.35, .2], bass: [.5, .18], melody: [.1, .2] },
    arr: [['Intro', 16], ['Groove', 32], ['Break', 16], ['Groove', 32], ['Outro', 16]],
    var: [
      { n: 'Funk', prog: [0,0,3,0], note: 'Tek akor üzerinde funk stab ve zıplayan bas.' },
      { n: 'Dorian groove', scale: 'dorian', prog: [0,3,0,6], cp: 'push', note: 'Dorian rengi, daha melodik.' },
      { n: 'Vokal avı', prog: [0,0,5,6], bp: 'bounce', mm: 'chop', md: 1.2, note: 'Yoğun vokal chop ritimleri.' },
      { n: 'Gece', prog: [0,6,0,5], cm: 'sustain', cp: 'long', bp: 'hook', mm: 'chop', md: .5, note: 'Daha koyu, uzun akorlar.' }
    ] },
  { id: 'maup', artist: 'Mau P', tag: 'Minimal tech house', accent: '#c4ee2f',
    blurb: 'Çıplak groove: hipnotik tek bas riff\'i, az akor, tek vokal hook.',
    bpm: [126, 130], bpmDef: 128, key: 9, scales: ['minor', 'phrygian'], swing: 6,
    chord: { mode: 'stab', pats: ['sparse'] }, bass: { pats: ['hook'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8O, ohat: OFF, rim: '.......x.......x', perc: 'x......x..x.....', fill: 'none' },
    mel: { mode: 'riff', dens: .22, oct: 72 },
    sound: { chords: 'rave-stab', bass: 'mp-hook-bass', melody: 'vox-chop', drums: 'house' },
    pump: { chords: [.3, .2], bass: [.5, .18], melody: [.1, .2] },
    arr: [['Intro', 16], ['Groove', 32], ['Break', 8], ['Groove', 32], ['Outro', 16]],
    var: [
      { n: 'Hipnotik', prog: [0,0,0,0], note: 'Tek akor, bas riff\'i her şey.' },
      { n: 'Zıplayan', prog: [0,0,6,0], bp: 'bounce', note: 'Hafif armonik hareket, daha oynak bas.' },
      { n: 'Phrygian', scale: 'phrygian', prog: [0,0,1,0], note: 'Küçük ikili gerilimi.' },
      { n: 'Çıplak', prog: [0,0,0,0], mm: 'riff', md: .15, dr: { perc: '.......x........' }, note: 'Neredeyse sadece kick, hat ve bas.' }
    ] },
  { id: 'dvlm', artist: 'Dimitri Vegas & Like Mike', tag: 'Mainstage', accent: '#ff3ad1',
    blurb: 'Ana sahne gücü: ağır kick, kalın stab\'lar ve hızlı lead riff\'leri.',
    bpm: [126, 130], bpmDef: 128, key: 9, scales: ['minor', 'harmonic'], swing: 0,
    chord: { mode: 'stab', pats: ['rave', 'quarter'] }, bass: { pats: ['roll16'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H16, ohat: OFF, fill: 'snare' },
    mel: { mode: 'hook', hr: ['h4','h1'], dens: .55, oct: 72, rep: .55 },
    sound: { chords: 'rave-stab', bass: 'roll-bass', melody: 'dv-screech-lead', drums: 'trance' },
    pump: { chords: [.65, .2], bass: [.65, .18], melody: [.35, .2] },
    arr: [['Intro', 16], ['Build', 16], ['Drop', 32], ['Breakdown', 16], ['Build', 16], ['Drop', 32], ['Outro', 16]],
    var: [
      { n: 'Ana sahne', prog: [0,5,2,6], note: 'Büyük akorlar ve hızlı lead.' },
      { n: 'Karanlık festival', scale: 'harmonic', prog: [0,5,3,4], note: 'Harmonik minör ve gerilimli geçişler.' },
      { n: 'Hard dance', prog: [0,6,5,6], bp: 'trance16', md: .7, dr: { chat: H8 }, note: 'Daha sert, kısa notalı riff.' },
      { n: 'Açık hava', scale: 'major', prog: [0,4,5,3], hr: ['h1','h3'], md: .45, note: 'Majör ve neşeli kalabalık hook\'u.' }
    ] },
  { id: 'ummet', artist: 'Ümit Özcan', tag: 'Elektro big room', accent: '#29d3ff',
    blurb: 'Keskin elektro lead\'ler, vokal esintili hook\'lar ve sert drop\'lar.',
    bpm: [126, 130], bpmDef: 128, key: 4, scales: ['minor', 'harmonic'], swing: 0,
    chord: { mode: 'sustain', pats: ['whole', 'long'] }, bass: { pats: ['offbeat8'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8, ohat: OFF, perc: 'x......x........', fill: 'snare' },
    mel: { mode: 'hook', hr: ['h6','h4','h3'], dens: .45, oct: 72, rep: .45 },
    sound: { chords: 'rave-stab', bass: 'trance-bass', melody: 'sync-lead', drums: 'trance' },
    pump: { chords: [.55, .22], bass: [.6, .2], melody: [.3, .2] },
    arr: [['Intro', 16], ['Build', 8], ['Drop', 32], ['Breakdown', 16], ['Build', 8], ['Drop', 32], ['Outro', 16]],
    var: [
      { n: 'Elektro', prog: [0,5,2,6], note: 'Sürekli pad, keskin lead hook\'u.' },
      { n: 'Dutch house', prog: [0,3,0,6], cm: 'stab', cp: 'rave', mm: 'hook', hr: ['h4'], md: .5, bp: 'roll16', note: 'Kısa stab ve kıvrak lead.' },
      { n: 'Gerilim', scale: 'harmonic', prog: [0,3,4,0], note: 'Harmonik minör ve baskın akorlar.' },
      { n: 'Vokal drop', prog: [0,5,3,4], mm: 'chop', md: .9, note: 'Vokal chop esintili drop.' }
    ] },
  { id: 'charlotte', artist: 'Charlotte de Witte', tag: 'Dark techno', accent: '#ff3b3b',
    blurb: 'Karanlık ve hipnotik: sert kick, rolling bas, az nota, endüstriyel doku.',
    bpm: [132, 138], bpmDef: 135, key: 9, scales: ['phrygian', 'minor'], swing: 0,
    chord: { mode: 'sustain', pats: ['whole'] }, bass: { pats: ['roll16'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8, ohat: OFF, rim: '...x..x....x..x.', perc: 'x..x..x.x..x..x.', fill: 'clap' },
    mel: { mode: 'riff', dens: .45, oct: 60 },
    sound: { chords: 'dark-drone', bass: 'roll-bass', melody: 'cw-acid-riff', drums: 'techno' },
    pump: { chords: [.3, .22], bass: [.3, .2], melody: [.1, .2] },
    arr: [['Intro', 32], ['Build', 16], ['Peak', 32], ['Break', 16], ['Peak', 32], ['Outro', 32]],
    var: [
      { n: 'Karanlık', scale: 'phrygian', prog: [0,0,1,0], note: 'Phrygian drone, sert rolling bas.' },
      { n: 'Endüstriyel', scale: 'minor', prog: [0,0,0,6], dr: { chat: H16 }, note: 'Daha yoğun hat\'ler ve sade armoni.' },
      { n: 'Acid', prog: [0,0,5,0], bp: 'acid', md: .6, note: 'Acid bas ve riff.' },
      { n: 'Hipnotik', scale: 'phrygian', prog: [0,1,0,1], md: .25, note: 'Yavaş değişen, tekrarlı.' }
    ] },
  { id: 'cox', artist: 'Carl Cox', tag: 'Groove techno', accent: '#ffd23f',
    blurb: 'Klasik 909 grooveu: swing\'li hat\'ler, piyano house stab\'ı ve acid hattı.',
    bpm: [128, 132], bpmDef: 130, key: 9, scales: ['minor', 'dorian'], swing: 8,
    chord: { mode: 'stab', pats: ['house', 'tres'] }, bass: { pats: ['acid'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8, ohat: OFF, perc: '..x..x..x..x..x.', fill: 'snare' },
    mel: { mode: 'riff', dens: .5, oct: 60 },
    sound: { chords: 'cc-909-stab', bass: 'acid-303', melody: 'pulse-lead', drums: 'techno' },
    pump: { chords: [.3, .2], bass: [.35, .18], melody: [.1, .2] },
    arr: [['Intro', 16], ['Build', 8], ['Groove', 32], ['Break', 16], ['Groove', 32], ['Outro', 16]],
    var: [
      { n: 'Rolling', prog: [0,0,3,0], bp: 'roll16', note: 'Akan bas ve basit stab.' },
      { n: 'Piyano house', scale: 'dorian', prog: [0,3,0,6], cp: 'house', note: 'Dorian piyano stab, sıcak ve oynak.' },
      { n: 'Acid', prog: [0,0,5,6], bp: 'acid', md: .6, note: 'Acid hattı öne çıkar.' },
      { n: 'Peak time', prog: [0,6,5,6], cm: 'sustain', cp: 'whole', mm: 'hook', hr: ['h6'], md: .4, note: 'Geniş pad ve sade hook.' }
    ] }
];
HG.STYLE = {}; HG.STYLES.forEach(s => { HG.STYLE[s.id] = s; });
