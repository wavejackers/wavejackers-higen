// ---- HitGen 3.2: tür stilleri (elektronik alt türler, rap, R&B).
// Yeni stiller listenin sonuna eklenir; kayıtlı projelerdeki stil dizinleri değişmez.
(function () {
  const S = HG.STYLE;
  Object.assign(HG.CP, {
    garage: [[3,2,96],[6,2,88],[11,2,96],[14,2,88]],
    neo:    [[0,6,92],[6,2,80],[8,6,90],[14,2,78]],
    boom:   [[0,3,96],[7,2,84],[10,3,90]]
  });
  Object.assign(HG.BP, {
    sub808:  [[0,6,1,112],[7,1,1,90],[10,4,1,104],[14,2,3,92]],
    drill:   [[0,5,1,112],[6,2,3,98],[8,3,1,106],[11,2,5,94],[14,2,2,96]],
    boom:    [[0,3,1,108],[6,1,1,88],[7,3,1,100],[10,2,2,92]],
    rnb:     [[0,5,1,104],[6,2,3,88],[9,3,1,98],[14,2,2,88]],
    dnb:     [[0,6,1,108],[6,4,1,96],[10,4,3,100],[14,2,2,92]],
    reese:   [[0,10,1,108],[10,3,5,98],[13,3,2,96]],
    wobble:  [[0,4,1,112],[4,2,1,96],[6,2,3,100],[8,6,1,110],[14,2,5,96]],
    ukg:     [[0,2,1,106],[3,2,3,92],[6,1,1,88],[10,3,1,104],[14,2,2,92]],
    disco:   [[0,2,1,106],[2,2,3,92],[4,2,1,100],[6,2,3,92],[8,2,1,106],[10,2,3,92],[12,2,1,100],[14,2,2,94]],
    eighth:  [0,2,4,6,8,10,12,14].map(s => [s, 2, 1, s % 4 ? 90 : 104]),
    log:     [[0,2,1,110],[3,2,1,96],[6,1,3,92],[10,2,1,104],[13,2,2,94]],
    minimal: [[0,1,1,104],[3,1,1,88],[10,1,1,98],[11,1,3,86]]
  });
  const SN = '....x.......x...', HALF = '........x.......', BRK = 'x.........x.....', GH = '.......x.x......';
  const ARR = {
    club: [['Intro', 16], ['Build', 8], ['Drop', 16], ['Break', 16], ['Build', 8], ['Drop', 16], ['Outro', 16]],
    song: [['Intro', 8], ['Verse', 16], ['Groove', 8], ['Verse', 16], ['Groove', 8], ['Break', 8], ['Groove', 16], ['Outro', 8]]
  };
  const PUMP = { hard: { chords: [.5, .22], bass: [.5, .2], melody: [.25, .2] }, soft: { chords: [.1, .2], bass: [.1, .2], melody: [0, .2] } };
  // "Beni şaşırt" ve yeni fikir için tür paleti (kütüphanedeki tekno ağırlıklı sesler bu türlere uymaz)
  const PAL = {
    bass:  { c: ['rhodes', 'dark-drone', 'wf-stab-future', 'wf-stab-tape', 'wf-keys-organ', 'wf-pad-festival', 'wf-pad-nocturne', 'wf-tex-drone'], b: ['wf-bass-reese', 'wf-bass-wobble', 'reese', 'neuro-lite', 'sub-sine', 'wf-sub-pressure', 'wf-roll-grit'], m: ['fm-lead', 'wf-vox-chop', 'aah-female', 'wf-pluck-future', 'wf-lead-razor', 'square-lead'] },
    urban: { c: ['wf-keys-tine', 'wf-keys-dusty', 'wf-pad-tape', 'rhodes', 'wurli', 'sad-pad', 'choir-pad'], b: ['wf-808-clean', 'wf-808-dirt', 'wf-808-slide', 'wf-bass-felt', '808-long', 'sub-sine'], m: ['wf-pluck-trapbell', 'wf-pluck-pizz', 'wf-lead-whistle', 'wf-metal-cowbell', 'wf-vox-chop', 'harp', 'sine-lead', 'kalimba', 'muted-pluck'] },
    chill: { c: ['wf-keys-dusty', 'wf-keys-tine', 'wf-pad-tape', 'wf-stab-tape', 'rhodes', 'string-machine', 'glass-pad'], b: ['wf-bass-felt', 'upright', 'deep-bass', 'sub-sine', 'pluck-bass'], m: ['wf-lead-whistle', 'harp', 'bell-pluck', 'sine-lead', 'kalimba', 'pulse-lead', 'marimba'] }
  };
  const soft = { urban: 1, chill: 1 };
  // add(kimlik, ad, grup, renk, [bpm alt, üst, varsayılan], ton, gamlar, swing, parçalar, [İngilizce metin], [Türkçe metin])
  const add = (id, artist, grp, accent, bpm, key, scales, swing, o, en, tr) => {
    const s = Object.assign({ id, artist, grp, accent, bpm: [bpm[0], bpm[1]], bpmDef: bpm[2], key, scales, swing,
      pump: PUMP[soft[grp] ? 'soft' : 'hard'], arr: ARR[soft[grp] ? 'song' : 'club'] }, PAL[grp] ? { pal: PAL[grp] } : {}, o);
    HG.STYLES.push(s); S[id] = s;
    HG.LANGS.forEach(([l]) => { HG.ST[l][id] = l === 'tr' ? tr : en; });
  };
  const snd = (chords, bass, melody, drums) => ({ chords, bass, melody, drums });

  // ---- techno
  add('minimal', 'Minimal Techno', 'techno', '#9aa7b5', [124, 128, 126], 9, ['minor', 'dorian'], 6, {
    chord: { mode: 'stab', pats: ['sparse'] }, bass: { pats: ['minimal'], oct: 36 },
    drums: { kick: K4, clap: SN, chat: OFF, rim: '...x..x....x.x..', perc: '......x......x..', fill: 'perc' },
    mel: { mode: 'riff', dens: .12, oct: 60 }, sound: snd('deep-stab', 'sub-sine', 'techno-blip', 'techno'),
    var: [{ prog: [0, 0, 0, 0] }, { scale: 'dorian', prog: [0, 0, 6, 0] }, { prog: [0, 0, 3, 0], mm: 'motif', md: .8 }, { prog: [0, 5, 0, 5], bp: 'offbeat8' }] },
    ['Minimal techno', 'Few elements, lots of space: a tight kick, clicks and a tiny riff.', ['Reduced', 'One chord, one idea.'], ['Dorian', 'A slightly warmer colour.'], ['Motif', 'A short repeating motif.'], ['Offbeat', 'Offbeat bass, two chords.']],
    ['Minimal techno', 'Az eleman, çok boşluk: sıkı kick, tıkırtılar ve minik bir riff.', ['Sade', 'Tek akor, tek fikir.'], ['Dorian', 'Biraz daha sıcak renk.'], ['Motif', 'Kısa, tekrar eden motif.'], ['Offbeat', 'Offbeat bas, iki akor.']]);
  add('acidtechno', 'Acid Techno', 'techno', '#c6ff3d', [132, 140, 136], 9, ['minor', 'phrygian'], 0, {
    chord: { mode: 'stab', pats: ['sparse'] }, bass: { pats: ['acid'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H16, ohat: OFF, fill: 'snare' },
    mel: { mode: 'motif', dens: .4, oct: 72 }, sound: snd('rave-stab', 'acid-303', 'wf-acid-scream', 'warehouse'),
    var: [{ prog: [0, 0, 0, 0] }, { scale: 'phrygian', prog: [0, 0, 1, 0] }, { prog: [0, 0, 6, 0], mm: 'riff', md: .6 }, { prog: [0, 3, 0, 6], bp: 'roll16' }] },
    ['Acid techno', 'A squelching 303 line over a hard, fast kick.', ['303', 'One chord, pure acid.'], ['Phrygian', 'Dark half-step tension.'], ['Riff', 'A second acid riff on top.'], ['Rolling', 'Rolling bass under the acid.']],
    ['Acid techno', 'Sert ve hızlı kick üstünde vıcık 303 hattı.', ['303', 'Tek akor, saf acid.'], ['Frigyen', 'Karanlık yarım ses gerilimi.'], ['Riff', 'Üstte ikinci acid riff.'], ['Rolling', 'Acid altında akan bas.']]);
  add('dubtechno', 'Dub Techno', 'techno', '#6fb3a8', [118, 124, 122], 2, ['minor', 'dorian'], 4, { sev: true,
    chord: { mode: 'stab', pats: ['sparse', 'offbeat'] }, bass: { pats: ['longRoot'], oct: 36 },
    drums: { kick: K4, chat: OFF, rim: SN, perc: '..........x.....', fill: 'none' },
    mel: { mode: 'motif', dens: .25, oct: 72 }, sound: snd('dub-chord', 'sub-sine', 'wf-pluck-dub', 'deep'),
    var: [{ prog: [0, 0, 0, 0] }, { prog: [0, 0, 3, 3] }, { scale: 'dorian', prog: [0, 6, 0, 6] }, { prog: [0, 0, 5, 5], cm: 'sustain', cp: 'whole' }] },
    ['Dub techno', 'Echoing minor-seventh chords, deep sub and endless space.', ['Echo', 'One chord in the delay.'], ['Two chords', 'A slow move to the fourth.'], ['Dorian', 'Lighter, floating colour.'], ['Wash', 'Long held chords.']],
    ['Dub techno', 'Yankılı minör yedili akorlar, derin sub ve sonsuz boşluk.', ['Yankı', 'Delay içinde tek akor.'], ['İki akor', 'Dörtlüye yavaş geçiş.'], ['Dorian', 'Daha hafif, süzülen renk.'], ['Yıkama', 'Uzun tutulan akorlar.']]);

  // ---- house
  add('deephouse', 'Deep House', 'house', '#7a9cff', [120, 124, 122], 5, ['minor', 'dorian'], 10, { sev: true,
    chord: { mode: 'stab', pats: ['house', 'push'] }, bass: { pats: ['funk'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8, ohat: OFF, rim: '.......x........', fill: 'none' },
    mel: { mode: 'motif', dens: .3, oct: 72 }, sound: snd('e-piano', 'deep-bass', 'bell-pluck', 'deep'),
    var: [{ prog: [0, 3, 0, 3] }, { prog: [0, 5, 3, 4] }, { scale: 'dorian', prog: [0, 6, 5, 6], cm: 'sustain', cp: 'long' }, { prog: [3, 4, 0, 0], bp: 'bounce', mm: 'chop' }] },
    ['Deep house', 'Warm seventh chords on electric piano, a round bass and a swung groove.', ['Two chords', 'Home and the fourth.'], ['Four chords', 'A full minor loop.'], ['Dorian pads', 'Long, soft chords.'], ['Bounce', 'Bouncing bass and chops.']],
    ['Deep house', 'Elektrikli piyanoda sıcak yedili akorlar, yuvarlak bas ve swing\'li groove.', ['İki akor', 'Ana akor ve dörtlü.'], ['Dört akor', 'Tam minör döngü.'], ['Dorian pad', 'Uzun, yumuşak akorlar.'], ['Bounce', 'Zıplayan bas ve chop.']]);
  add('futurehouse', 'Future House', 'house', '#3de0ff', [124, 128, 126], 7, ['minor'], 0, {
    chord: { mode: 'stab', pats: ['push', 'offbeat'] }, bass: { pats: ['bounce'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H16, ohat: OFF, fill: 'snare' },
    mel: { mode: 'chop', dens: .45, oct: 72 }, sound: snd('super-stab', 'fm-bass', 'wf-pluck-future', 'house'),
    var: [{ prog: [0, 5, 2, 6] }, { prog: [0, 3, 5, 4] }, { prog: [5, 6, 0, 0], mm: 'hook', hr: ['h1', 'h3'] }, { prog: [0, 0, 5, 6], bp: 'hook' }] },
    ['Future house', 'Metallic bouncing bass, bright stabs and a clean, punchy drop.', ['Drop', 'Classic four-chord drop.'], ['Lift', 'A rising progression.'], ['Hook', 'A singable lead line.'], ['Bass lead', 'The bass carries the hook.']],
    ['Future house', 'Metalik zıplayan bas, parlak stab\'ler ve temiz, vurucu drop.', ['Drop', 'Klasik dört akorlu drop.'], ['Yükseliş', 'Yükselen akor yürüyüşü.'], ['Hook', 'Söylenebilir lead hattı.'], ['Bas lead', 'Hook\'u bas taşır.']]);
  add('basshouse', 'Bass House', 'house', '#ffd23d', [124, 128, 126], 4, ['minor', 'phrygian'], 0, {
    chord: { mode: 'stab', pats: ['sparse'] }, bass: { pats: ['hook'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8O, ohat: OFF, fill: 'snare' },
    mel: { mode: 'riff', dens: .2, oct: 60 }, sound: snd('rave-stab', 'neuro-lite', 'square-lead', 'club'),
    var: [{ prog: [0, 0, 0, 0] }, { prog: [0, 0, 5, 6] }, { scale: 'phrygian', prog: [0, 1, 0, 1] }, { prog: [0, 3, 0, 6], bp: 'bounce', md: 1.3 }] },
    ['Bass house', 'A growling bass riff does the talking; everything else stays out of its way.', ['Growl', 'One note, all attitude.'], ['Turn', 'A two-chord turnaround.'], ['Phrygian', 'Half-step menace.'], ['Busy', 'Bouncier bass, denser riff.']],
    ['Bass house', 'Hırlayan bas riff\'i konuşur; geri kalan her şey yolundan çekilir.', ['Hırıltı', 'Tek nota, tam tavır.'], ['Dönüş', 'İki akorlu dönüş.'], ['Frigyen', 'Yarım seslik tehdit.'], ['Yoğun', 'Daha zıplak bas, yoğun riff.']]);
  add('nudisco', 'Nu-Disco', 'house', '#ff7ac8', [114, 122, 118], 9, ['minor', 'dorian', 'major'], 8, { sev: true,
    chord: { mode: 'stab', pats: ['funk', 'house'] }, bass: { pats: ['disco'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H16, ohat: OFF, fill: 'snare' },
    mel: { mode: 'hook', hr: ['h1', 'h3', 'h6'], dens: .4, oct: 72, rep: .5 }, sound: snd('italo-stab', 'funk-bass', 'pulse-lead', 'house'),
    var: [{ prog: [0, 3, 6, 2] }, { prog: [5, 6, 0, 0] }, { scale: 'major', prog: [0, 5, 3, 4] }, { prog: [0, 6, 5, 4], cm: 'sustain', cp: 'long', mm: 'arp', arp: 'up' }] },
    ['Nu-disco', 'Octave bass, funky chord stabs and open hats: disco with modern punch.', ['Disco', 'A circling minor loop.'], ['Lift', 'Rising to the home chord.'], ['Major', 'Sunny and bright.'], ['Italo', 'Long chords and an arp.']],
    ['Nu-disco', 'Oktav bas, funky akor vuruşları ve açık hat\'ler: modern vuruşlu disko.', ['Disko', 'Dönen minör döngü.'], ['Yükseliş', 'Ana akora tırmanış.'], ['Majör', 'Güneşli ve parlak.'], ['Italo', 'Uzun akorlar ve arp.']]);
  add('pianohouse', 'Piano House', 'house', '#ffb86b', [122, 126, 124], 0, ['major', 'minor'], 4, {
    chord: { mode: 'stab', pats: ['tres', 'house'] }, bass: { pats: ['offbeat8'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8, ohat: OFF, fill: 'snare' },
    mel: { mode: 'hook', hr: ['h2', 'h5', 'h1'], dens: .35, oct: 72, rep: .5 }, sound: snd('wf-keys-house', 'punch-bass', 'aah-female', 'house'),
    var: [{ scale: 'major', prog: [0, 4, 5, 3] }, { scale: 'minor', prog: [0, 5, 2, 6] }, { scale: 'major', prog: [5, 3, 0, 4] }, { scale: 'minor', prog: [0, 3, 5, 4], bp: 'pumpRoot' }] },
    ['Piano house', 'Big syncopated piano chords and a vocal-style hook: hands-in-the-air house.', ['Anthem', 'The classic major loop.'], ['Minor', 'More emotional.'], ['Uplift', 'Starts away from home.'], ['Drive', 'Pumping root bass.']],
    ['Piano house', 'Büyük senkoplu piyano akorları ve vokal tadında hook: eller havaya house.', ['Marş', 'Klasik majör döngü.'], ['Minör', 'Daha duygusal.'], ['Yükseliş', 'Ana akorun dışından başlar.'], ['Sürüş', 'Pompalayan kök bas.']]);
  add('slaphouse', 'Slap House', 'house', '#b08cff', [120, 126, 124], 6, ['minor'], 0, {
    chord: { mode: 'sustain', pats: ['whole', 'long'] }, bass: { pats: ['bounce'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H8O, fill: 'snare' },
    mel: { mode: 'hook', hr: ['h2', 'h3', 'h5'], dens: .35, oct: 72, rep: .5 }, sound: snd('warm-pad', 'wf-bass-slap', 'wf-lead-whistle', 'club'),
    var: [{ prog: [0, 5, 2, 6] }, { prog: [5, 6, 0, 0] }, { prog: [0, 2, 6, 3] }, { prog: [0, 5, 3, 4], mm: 'chop' }] },
    ['Slap house', 'A plucky slap bass, dark minor chords and a simple radio hook.', ['Radio', 'Four familiar chords.'], ['Rise', 'Climbing to home.'], ['Dark', 'A moodier loop.'], ['Chops', 'Chopped melody.']],
    ['Slap house', 'Tok slap bas, karanlık minör akorlar ve sade bir radyo hook\'u.', ['Radyo', 'Dört tanıdık akor.'], ['Tırmanış', 'Ana akora çıkış.'], ['Karanlık', 'Daha kasvetli döngü.'], ['Chop', 'Kesilmiş melodi.']]);
  add('amapiano', 'Amapiano', 'afro', '#e0b24a', [110, 114, 112], 2, ['minor', 'dorian'], 12, { sev: true,
    chord: { mode: 'sustain', pats: ['long', 'whole'] }, bass: { pats: ['log'], oct: 36 },
    drums: { kick: 'x.....x...x.....', rim: '..x..x....x..x..', chat: H16, perc: '...x......x...x.', fill: 'perc' },
    mel: { mode: 'motif', dens: .3, oct: 72 }, sound: snd('rhodes', 'wf-bass-log', 'kalimba', 'afro'),
    var: [{ prog: [0, 3, 6, 2] }, { prog: [0, 5, 3, 4] }, { scale: 'dorian', prog: [0, 3, 0, 3] }, { prog: [5, 4, 0, 0], mm: 'chop', md: .8 }] },
    ['Amapiano', 'Log-drum bass, jazzy keys and shakers at a relaxed tempo.', ['Jazzy', 'A circling seventh loop.'], ['Soulful', 'Four emotional chords.'], ['Dorian', 'Two-chord groove.'], ['Chops', 'Chopped keys melody.']],
    ['Amapiano', 'Log drum bas, cazımsı tuşlular ve shaker\'lar; rahat tempoda.', ['Cazımsı', 'Dönen yedili döngü.'], ['Soul', 'Dört duygusal akor.'], ['Dorian', 'İki akorlu groove.'], ['Chop', 'Kesilmiş tuşlu melodi.']]);

  // ---- ana sahne, trance, hard
  add('electro', 'Electro House', 'main', '#ff5a3d', [126, 130, 128], 5, ['minor'], 0, {
    chord: { mode: 'stab', pats: ['rave', 'push'] }, bass: { pats: ['hook'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H16, ohat: OFF, fill: 'snare' },
    mel: { mode: 'riff', dens: .3, oct: 72 }, sound: snd('super-stab', 'fm-bass', 'sync-lead', 'club'),
    var: [{ prog: [0, 0, 5, 6] }, { prog: [0, 5, 3, 4] }, { prog: [0, 6, 5, 4], mm: 'hook', hr: ['h1', 'h4'] }, { prog: [0, 0, 0, 0], bp: 'bounce' }] },
    ['Electro house', 'Gritty synth riffs and a hard, distorted bass built for big rooms.', ['Riff', 'Riff over a late lift.'], ['Chords', 'Four-chord drop.'], ['Lead', 'A melodic lead hook.'], ['Dirty', 'One chord, bouncing bass.']],
    ['Electro house', 'Kirli synth riff\'leri ve sert, distorsiyonlu bas; büyük salonlar için.', ['Riff', 'Geç yükselen riff.'], ['Akorlar', 'Dört akorlu drop.'], ['Lead', 'Melodik lead hook\'u.'], ['Kirli', 'Tek akor, zıplayan bas.']]);
  add('psy', 'Psytrance', 'trance', '#a6ff5c', [140, 148, 145], 4, ['phrygian', 'minor', 'harmonic'], 0, {
    chord: { mode: 'sustain', pats: ['whole'] }, bass: { pats: ['trance16'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H16, ohat: OFF, fill: 'snare' },
    mel: { mode: 'riff', dens: .35, oct: 72 }, sound: snd('dark-drone', 'trance-bass', 'sync-lead', 'trance'),
    var: [{ prog: [0, 0, 0, 0] }, { scale: 'phrygian', prog: [0, 0, 1, 0] }, { prog: [0, 0, 6, 0], mm: 'arp', arp: 'skip' }, { scale: 'harmonic', prog: [0, 5, 0, 6], mm: 'motif' }] },
    ['Psytrance', 'The galloping kick-bass engine with twisting, hypnotic leads.', ['Drive', 'One note, full gallop.'], ['Phrygian', 'Eastern half-step colour.'], ['Arp', 'A skipping arp lead.'], ['Story', 'Harmonic minor motif.']],
    ['Psytrance', 'Dörtnala kick-bas motoru ve kıvrılan, hipnotik lead\'ler.', ['Sürüş', 'Tek nota, tam dörtnala.'], ['Frigyen', 'Doğulu yarım ses rengi.'], ['Arp', 'Atlayan arp lead.'], ['Hikâye', 'Armonik minör motif.']]);
  add('uplifting', 'Uplifting Trance', 'trance', '#5cc8ff', [136, 140, 138], 9, ['minor'], 0, {
    chord: { mode: 'sustain', pats: ['whole', 'long'], arp: 'trance' }, bass: { pats: ['trance16'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H16, ohat: OFF, fill: 'snare' },
    mel: { mode: 'hook', hr: ['h2', 'h1', 'h5'], dens: .4, oct: 72, rep: .5 }, sound: snd('wf-pad-festival', 'trance-bass', 'wf-lead-anthem', 'trance'),
    var: [{ prog: [0, 5, 2, 6] }, { prog: [5, 6, 0, 0] }, { prog: [0, 5, 6, 4], cm: 'arp', mm: 'arp', arp: 'trance' }, { prog: [3, 5, 0, 6], bp: 'offbeat8' }] },
    ['Uplifting trance', 'Soaring supersaw melody, rolling bass and a long emotional breakdown.', ['Anthem', 'The classic trance loop.'], ['Rise', 'Climbing to home.'], ['Arps', 'Arpeggios everywhere.'], ['Offbeat', 'Classic offbeat bass.']],
    ['Uplifting trance', 'Süzülen supersaw melodi, akan bas ve uzun, duygusal breakdown.', ['Marş', 'Klasik trance döngüsü.'], ['Tırmanış', 'Ana akora çıkış.'], ['Arp', 'Her yerde arpej.'], ['Offbeat', 'Klasik offbeat bas.']]);
  add('hardstyle', 'Hardstyle', 'hard', '#ff3d6e', [148, 155, 150], 4, ['minor', 'harmonic'], 0, {
    chord: { mode: 'stab', pats: ['quarter'] }, bass: { pats: ['offbeat8'], oct: 36 },
    drums: { kick: K4, clap: SN, ohat: OFF, fill: 'snare' },
    mel: { mode: 'hook', hr: ['h1', 'h3', 'h4'], dens: .45, oct: 72, rep: .5 }, sound: snd('super-stab', 'punch-bass', 'wf-lead-anthem', 'hard'),
    var: [{ prog: [0, 5, 2, 6] }, { scale: 'harmonic', prog: [0, 5, 3, 4] }, { prog: [5, 6, 0, 0] }, { prog: [0, 0, 5, 6], mm: 'riff', md: .7 }] },
    ['Hardstyle', 'A hard distorted kick, reverse bass and a euphoric anthem lead.', ['Euphoric', 'Big four-chord anthem.'], ['Harmonic', 'Dramatic raised seventh.'], ['Rise', 'Climbing to home.'], ['Raw', 'A rawer riff.']],
    ['Hardstyle', 'Sert distorsiyonlu kick, reverse bas ve coşkulu marş lead\'i.', ['Coşkulu', 'Büyük dört akorlu marş.'], ['Armonik', 'Dramatik yükseltilmiş yedili.'], ['Tırmanış', 'Ana akora çıkış.'], ['Raw', 'Daha ham bir riff.']]);
  add('hardcore', 'Hardcore', 'hard', '#ff2a2a', [165, 180, 170], 9, ['minor', 'phrygian'], 0, {
    chord: { mode: 'stab', pats: ['rave'] }, bass: { pats: ['pumpRoot'], oct: 36 },
    drums: { kick: K4, clap: C24, chat: H16, ohat: OFF, fill: 'snare' },
    mel: { mode: 'riff', dens: .35, oct: 72 }, sound: snd('rave-stab', 'punch-bass', 'wf-lead-screech', 'hard'),
    var: [{ prog: [0, 0, 5, 6] }, { scale: 'phrygian', prog: [0, 1, 0, 1] }, { prog: [0, 5, 3, 4], mm: 'hook', hr: ['h4', 'h1'] }, { prog: [0, 6, 5, 6] }] },
    ['Hardcore', 'Very fast and relentless: pounding kick, rave stabs and a shouting riff.', ['Rave', 'Riff with a late lift.'], ['Phrygian', 'Half-step aggression.'], ['Melodic', 'A fast melodic hook.'], ['March', 'Stepping chords.']],
    ['Hardcore', 'Çok hızlı ve acımasız: döven kick, rave stab\'leri ve bağıran riff.', ['Rave', 'Geç yükselen riff.'], ['Frigyen', 'Yarım seslik saldırganlık.'], ['Melodik', 'Hızlı melodik hook.'], ['Marş', 'Adım adım akorlar.']]);

  // ---- bass müzik
  add('liquid', 'Liquid Drum & Bass', 'bass', '#4ad6c4', [170, 176, 174], 5, ['minor', 'dorian', 'major'], 0, { sev: true,
    chord: { mode: 'sustain', pats: ['long', 'whole'] }, bass: { pats: ['dnb'], oct: 36 },
    drums: { kick: BRK, snare: SN, chat: H8, rim: GH, fill: 'snare' },
    mel: { mode: 'hook', hr: ['h2', 'h5', 'h6'], dens: .3, oct: 72, rep: .5 }, sound: snd('rhodes', 'sub-sine', 'aah-female', 'dnb'),
    var: [{ prog: [0, 5, 2, 6] }, { prog: [5, 6, 0, 0] }, { scale: 'dorian', prog: [0, 3, 0, 3] }, { prog: [0, 2, 5, 4], mm: 'arp', arp: 'pp', md: .7 }] },
    ['Liquid drum & bass', 'Fast breakbeat, deep sub and soulful seventh chords with a vocal-style hook.', ['Soul', 'Four warm chords.'], ['Rise', 'Climbing to home.'], ['Dorian', 'Two-chord roller.'], ['Arp', 'A gentle arp on top.']],
    ['Liquid drum & bass', 'Hızlı breakbeat, derin sub ve vokal tadında hook\'lu soul yedili akorlar.', ['Soul', 'Dört sıcak akor.'], ['Tırmanış', 'Ana akora çıkış.'], ['Dorian', 'İki akorlu roller.'], ['Arp', 'Üstte yumuşak arp.']]);
  add('neuro', 'Neurofunk', 'bass', '#8cff3d', [172, 176, 174], 5, ['minor', 'phrygian'], 0, {
    chord: { mode: 'sustain', pats: ['whole'] }, bass: { pats: ['reese'], oct: 36 },
    drums: { kick: BRK, snare: SN, chat: H16, rim: GH, fill: 'snare' },
    mel: { mode: 'riff', dens: .2, oct: 60 }, sound: snd('dark-drone', 'wf-bass-reese', 'fm-lead', 'dnb'),
    var: [{ prog: [0, 0, 0, 0] }, { scale: 'phrygian', prog: [0, 0, 1, 0] }, { prog: [0, 0, 5, 6], dr: { kick: 'x......x..x.....' } }, { scale: 'phrygian', prog: [0, 6, 0, 1], md: 1.4 }] },
    ['Neurofunk', 'Dark, technical drum & bass: a snarling reese bass and a tight break.', ['Reese', 'One note, all bass.'], ['Phrygian', 'Half-step dread.'], ['Stepper', 'Extra kick, late lift.'], ['Tech', 'Denser riff, darker turn.']],
    ['Neurofunk', 'Karanlık, teknik drum & bass: hırlayan reese bas ve sıkı break.', ['Reese', 'Tek nota, tam bas.'], ['Frigyen', 'Yarım seslik tedirginlik.'], ['Stepper', 'Ek kick, geç yükseliş.'], ['Tech', 'Yoğun riff, karanlık dönüş.']]);
  add('dubstep', 'Dubstep', 'bass', '#b05cff', [138, 142, 140], 5, ['minor', 'phrygian'], 0, {
    chord: { mode: 'sustain', pats: ['whole'] }, bass: { pats: ['wobble'], oct: 36 },
    drums: { kick: 'x.....x.........', snare: HALF, chat: H8, ohat: '..............x.', fill: 'snare' },
    mel: { mode: 'riff', dens: .15, oct: 60 }, sound: snd('dark-drone', 'wf-bass-wobble', 'fm-lead', 'trap'),
    var: [{ prog: [0, 0, 0, 0] }, { prog: [0, 0, 5, 6] }, { scale: 'phrygian', prog: [0, 1, 0, 1] }, { prog: [0, 5, 3, 4], mm: 'hook', hr: ['h2', 'h5'] }] },
    ['Dubstep', 'Half-time drums and a heavy, moving bass with lots of space.', ['Heavy', 'One note, big bass.'], ['Turn', 'A late two-chord lift.'], ['Phrygian', 'Half-step menace.'], ['Melodic', 'Chords and a slow hook.']],
    ['Dubstep', 'Yarı tempolu davul ve ağır, hareketli bas; bolca boşluk.', ['Ağır', 'Tek nota, büyük bas.'], ['Dönüş', 'Geç gelen iki akor.'], ['Frigyen', 'Yarım seslik tehdit.'], ['Melodik', 'Akorlar ve yavaş hook.']]);
  add('futurebass', 'Future Bass', 'bass', '#ff8ad6', [140, 160, 150], 3, ['major', 'minor'], 0, { sev: true,
    chord: { mode: 'stab', pats: ['push', 'quarter'] }, bass: { pats: ['longRoot'], oct: 36 },
    drums: { kick: 'x......x........', clap: HALF, chat: H8, fill: 'snare' },
    mel: { mode: 'hook', hr: ['h1', 'h3', 'h6'], dens: .4, oct: 72, rep: .5 }, sound: snd('wf-stab-future', 'sub-sine', 'wf-vox-chop', 'trap'),
    var: [{ scale: 'major', prog: [3, 4, 5, 0] }, { scale: 'major', prog: [0, 4, 5, 3] }, { scale: 'minor', prog: [5, 6, 0, 0] }, { scale: 'major', prog: [3, 0, 4, 5], mm: 'chop' }] },
    ['Future bass', 'Huge pumping seventh chords, half-time drums and a vocal-chop hook.', ['Bright', 'Rising major chords.'], ['Pop', 'The pop progression.'], ['Minor', 'Bittersweet lift.'], ['Chops', 'Chopped vocal melody.']],
    ['Future bass', 'Dev pompalayan yedili akorlar, yarı tempolu davul ve vokal chop hook\'u.', ['Parlak', 'Yükselen majör akorlar.'], ['Pop', 'Pop akor yürüyüşü.'], ['Minör', 'Buruk yükseliş.'], ['Chop', 'Kesilmiş vokal melodi.']]);
  add('ukg', 'UK Garage', 'bass', '#ffe45c', [128, 134, 132], 7, ['minor', 'dorian'], 18, { sev: true,
    chord: { mode: 'stab', pats: ['garage', 'house'] }, bass: { pats: ['ukg'], oct: 36 },
    drums: { kick: BRK, clap: SN, chat: H8, ohat: OFF, rim: '.......x.......x', fill: 'none' },
    mel: { mode: 'chop', dens: .4, oct: 72 }, sound: snd('wf-keys-organ', 'deep-bass', 'wf-vox-chop', 'house'),
    var: [{ prog: [0, 3, 0, 3] }, { prog: [0, 5, 3, 4] }, { scale: 'dorian', prog: [0, 6, 5, 6] }, { prog: [3, 4, 0, 0], mm: 'motif' }] },
    ['UK garage', 'Shuffled 2-step drums, organ stabs, a skipping bass and vocal chops.', ['2-step', 'Two-chord skip.'], ['Soulful', 'Four emotional chords.'], ['Dorian', 'Lighter colour.'], ['Motif', 'A short repeating motif.']],
    ['UK garage', 'Shuffle\'lı 2-step davul, org stab\'leri, seken bas ve vokal chop\'ları.', ['2-step', 'İki akorlu sekme.'], ['Soul', 'Dört duygusal akor.'], ['Dorian', 'Daha hafif renk.'], ['Motif', 'Kısa, tekrar eden motif.']]);
  add('breaks', 'Breakbeat', 'bass', '#ff9a3d', [126, 134, 130], 2, ['minor', 'dorian'], 6, {
    chord: { mode: 'stab', pats: ['sparse', 'offbeat'] }, bass: { pats: ['bounce'], oct: 36 },
    drums: { kick: BRK, snare: SN, chat: H8, rim: GH, ohat: '..............x.', fill: 'snare' },
    mel: { mode: 'riff', dens: .25, oct: 72 }, sound: snd('vapor-stab', 'reese', 'pulse-lead', 'dnb'),
    var: [{ prog: [0, 0, 5, 6] }, { prog: [0, 3, 0, 6] }, { prog: [0, 0, 0, 0], dr: { kick: 'x......x..x.....' } }, { prog: [0, 5, 3, 4], mm: 'hook', hr: ['h1', 'h6'] }] },
    ['Breakbeat', 'A broken kick-snare groove at house tempo with a rough bass and a synth riff.', ['Break', 'Riff with a late lift.'], ['Rave', 'Old-school turn.'], ['Funky', 'Busier kick, one chord.'], ['Melodic', 'A lead hook on top.']],
    ['Breakbeat', 'House temposunda kırık kick-snare groove\'u, sert bas ve synth riff\'i.', ['Break', 'Geç yükselen riff.'], ['Rave', 'Eski usul dönüş.'], ['Funky', 'Yoğun kick, tek akor.'], ['Melodik', 'Üstte lead hook\'u.']]);

  // ---- chill
  add('lofi', 'Lo-fi Hip-Hop', 'chill', '#c9a27a', [70, 90, 80], 0, ['major', 'dorian', 'minor'], 22, { sev: true,
    chord: { mode: 'stab', pats: ['boom', 'neo'] }, bass: { pats: ['boom'], oct: 36 },
    drums: { kick: 'x......x..x.....', snare: SN, chat: H8, fill: 'none' },
    mel: { mode: 'motif', dens: .25, oct: 72 }, sound: snd('wf-keys-dusty', 'wf-bass-felt', 'harp', 'lofi'),
    var: [{ scale: 'major', prog: [1, 4, 0, 5] }, { scale: 'dorian', prog: [0, 3, 0, 3] }, { scale: 'minor', prog: [0, 5, 3, 4] }, { scale: 'major', prog: [3, 2, 1, 0], mm: 'chop', md: .6 }] },
    ['Lo-fi hip-hop', 'Dusty jazz chords, a lazy swung beat and a small, sleepy melody.', ['Jazz', 'The ii–V–I turnaround.'], ['Dorian', 'Two mellow chords.'], ['Rainy', 'A sadder minor loop.'], ['Stroll', 'Chords walking down.']],
    ['Lo-fi hip-hop', 'Tozlu caz akorları, tembel swing\'li ritim ve küçük, uykulu bir melodi.', ['Caz', 'ii–V–I dönüşü.'], ['Dorian', 'İki yumuşak akor.'], ['Yağmurlu', 'Daha hüzünlü minör döngü.'], ['Yürüyüş', 'Aşağı yürüyen akorlar.']]);
  add('downtempo', 'Downtempo', 'chill', '#7ad1c0', [90, 110, 100], 9, ['minor', 'dorian'], 8, { sev: true,
    chord: { mode: 'sustain', pats: ['long', 'whole'] }, bass: { pats: ['rnb'], oct: 36 },
    drums: { kick: 'x.....x...x.....', snare: SN, chat: H8O, rim: GH, fill: 'none' },
    mel: { mode: 'hook', hr: ['h2', 'h5'], dens: .25, oct: 72, rep: .5 }, sound: snd('tape-pad', 'deep-bass', 'sine-lead', 'deep'),
    var: [{ prog: [0, 5, 2, 6] }, { scale: 'dorian', prog: [0, 3, 0, 3] }, { prog: [5, 6, 0, 0], mm: 'arp', arp: 'pp', md: .6 }, { prog: [0, 6, 5, 4] }] },
    ['Downtempo', 'Slow, wide and warm: long pads, a soft break and an unhurried lead.', ['Drift', 'Four slow chords.'], ['Dorian', 'Two-chord float.'], ['Arp', 'A sparse arp.'], ['Descend', 'Chords stepping down.']],
    ['Downtempo', 'Yavaş, geniş ve sıcak: uzun pad\'ler, yumuşak break ve acelesiz lead.', ['Süzülüş', 'Dört yavaş akor.'], ['Dorian', 'İki akorda süzülme.'], ['Arp', 'Seyrek bir arp.'], ['İniş', 'Aşağı inen akorlar.']]);
  add('synthwave', 'Synthwave', 'chill', '#ff5cc0', [84, 110, 100], 9, ['minor', 'major'], 0, {
    chord: { mode: 'sustain', pats: ['long', 'whole'], arp: 'up' }, bass: { pats: ['eighth'], oct: 36 },
    drums: { kick: 'x.......x.......', snare: SN, chat: H8, fill: 'snare' },
    mel: { mode: 'hook', hr: ['h2', 'h1', 'h3'], dens: .35, oct: 72, rep: .5 }, sound: snd('string-machine', 'pluck-bass', 'pulse-lead', 'indie'),
    var: [{ prog: [0, 5, 2, 6] }, { prog: [5, 6, 0, 0] }, { scale: 'major', prog: [0, 4, 5, 3] }, { prog: [0, 2, 5, 6], cm: 'arp', mm: 'arp', arp: 'up' }] },
    ['Synthwave', '80s nostalgia: eighth-note bass, big snare, string machine and a neon lead.', ['Night drive', 'Four minor chords.'], ['Sunrise', 'Climbing to home.'], ['Major', 'Bright and hopeful.'], ['Arps', 'Arpeggios everywhere.']],
    ['Synthwave', '80\'ler nostaljisi: sekizlik bas, büyük snare, string machine ve neon lead.', ['Gece sürüşü', 'Dört minör akor.'], ['Gün doğumu', 'Ana akora çıkış.'], ['Majör', 'Parlak ve umutlu.'], ['Arp', 'Her yerde arpej.']]);

  // ---- rap ve R&B
  add('boombap', 'Boom Bap', 'urban', '#d9a15c', [84, 96, 90], 2, ['minor', 'dorian'], 20, { sev: true,
    chord: { mode: 'stab', pats: ['boom', 'sparse'] }, bass: { pats: ['boom'], oct: 36 },
    drums: { kick: 'x......x.x......', snare: SN, chat: H8, fill: 'none' },
    mel: { mode: 'motif', dens: .25, oct: 72 }, sound: snd('rhodes', 'upright', 'muted-pluck', 'lofi'),
    var: [{ prog: [0, 3, 0, 3] }, { prog: [0, 5, 3, 4] }, { scale: 'dorian', prog: [0, 6, 0, 6] }, { prog: [5, 4, 0, 0], mm: 'chop', md: .6 }] },
    ['Boom bap', 'Classic 90s rap: a swung kick-snare, sampled-style keys and room for a verse.', ['Head nod', 'Two-chord loop.'], ['Story', 'Four-chord loop.'], ['Dorian', 'Jazzier colour.'], ['Chops', 'Chopped-sample feel.']],
    ['Boom bap', 'Klasik 90\'lar rap\'i: swing\'li kick-snare, sample tadında tuşlular ve söze yer.', ['Kafa sallatan', 'İki akorlu döngü.'], ['Hikâye', 'Dört akorlu döngü.'], ['Dorian', 'Daha cazımsı renk.'], ['Chop', 'Kesilmiş sample hissi.']]);
  add('trap', 'Trap', 'urban', '#ff4d6a', [130, 150, 140], 1, ['minor', 'harmonic', 'phrygian'], 0, {
    chord: { mode: 'sustain', pats: ['whole', 'long'] }, bass: { pats: ['sub808'], oct: 36 },
    drums: { kick: 'x......x..x.....', clap: HALF, chat: H16, ohat: '......x.........', fill: 'snare' },
    mel: { mode: 'motif', dens: .3, oct: 72 }, sound: snd('sad-pad', 'wf-808-clean', 'wf-pluck-trapbell', 'trap'),
    var: [{ prog: [0, 5, 0, 5] }, { scale: 'harmonic', prog: [0, 0, 5, 4] }, { scale: 'phrygian', prog: [0, 1, 0, 1] }, { prog: [0, 5, 3, 4], mm: 'arp', arp: 'gate', md: .7 }] },
    ['Trap', 'Long 808 bass, half-time clap, fast hats and a dark bell motif.', ['Dark', 'Two minor chords.'], ['Harmonic', 'Dramatic raised seventh.'], ['Phrygian', 'Half-step menace.'], ['Arp', 'A gated arp melody.']],
    ['Trap', 'Uzun 808 bas, yarı tempolu clap, hızlı hat\'ler ve karanlık çan motifi.', ['Karanlık', 'İki minör akor.'], ['Armonik', 'Dramatik yükseltilmiş yedili.'], ['Frigyen', 'Yarım seslik tehdit.'], ['Arp', 'Kesik arp melodisi.']]);
  add('drill', 'Drill', 'urban', '#6a7bff', [138, 146, 142], 5, ['minor', 'harmonic'], 0, {
    chord: { mode: 'sustain', pats: ['whole'] }, bass: { pats: ['drill'], oct: 36 },
    drums: { kick: 'x......x..x..x..', clap: HALF, rim: '..............x.', chat: 'x..x..x.x..x..x.', fill: 'none' },
    mel: { mode: 'motif', dens: .3, oct: 72 }, sound: snd('choir-pad', 'wf-808-slide', 'wf-pluck-pizz', 'trap'),
    var: [{ prog: [0, 5, 0, 5] }, { scale: 'harmonic', prog: [0, 5, 4, 4] }, { prog: [0, 2, 5, 4] }, { scale: 'harmonic', prog: [0, 0, 3, 4], mm: 'arp', arp: 'gate', md: .7 }] },
    ['Drill', 'Sliding 808 lines, 3-3-2 hats and cold minor strings.', ['Cold', 'Two minor chords.'], ['Harmonic', 'Tense raised seventh.'], ['Cinematic', 'Four dramatic chords.'], ['Arp', 'A gated arp melody.']],
    ['Drill', 'Kayan 808 hatları, 3-3-2 hat\'ler ve soğuk minör yaylılar.', ['Soğuk', 'İki minör akor.'], ['Armonik', 'Gergin yükseltilmiş yedili.'], ['Sinematik', 'Dört dramatik akor.'], ['Arp', 'Kesik arp melodisi.']]);
  add('rnb', 'R&B', 'urban', '#c78cff', [80, 100, 90], 3, ['major', 'dorian', 'minor'], 12, { sev: true,
    chord: { mode: 'stab', pats: ['neo', 'boom'] }, bass: { pats: ['rnb'], oct: 36 },
    drums: { kick: 'x.....x..x......', clap: SN, chat: H16, rim: '..........x.....', fill: 'none' },
    mel: { mode: 'hook', hr: ['h2', 'h5', 'h6'], dens: .3, oct: 72, rep: .5 }, sound: snd('wf-keys-tine', 'deep-bass', 'wf-lead-whistle', 'trap'),
    var: [{ scale: 'major', prog: [1, 4, 0, 5] }, { scale: 'dorian', prog: [0, 3, 0, 3] }, { scale: 'minor', prog: [5, 4, 0, 0] }, { scale: 'major', prog: [3, 2, 1, 0], cm: 'sustain', cp: 'long', mm: 'chop', md: .6 }] },
    ['R&B', 'Smooth seventh chords on electric piano, a laid-back beat and a singable hook.', ['Smooth', 'The ii–V–I turnaround.'], ['Neo soul', 'Two dorian chords.'], ['Late night', 'A moodier minor loop.'], ['Slow jam', 'Long chords walking down.']],
    ['R&B', 'Elektrikli piyanoda pürüzsüz yedili akorlar, rahat ritim ve söylenebilir hook.', ['Pürüzsüz', 'ii–V–I dönüşü.'], ['Neo soul', 'İki dorian akor.'], ['Gece yarısı', 'Daha kasvetli minör döngü.'], ['Slow jam', 'Aşağı yürüyen uzun akorlar.']]);
  add('trapsoul', 'Trap Soul', 'urban', '#8c6aff', [120, 140, 130], 8, ['minor', 'dorian'], 0, { sev: true,
    chord: { mode: 'sustain', pats: ['long', 'whole'] }, bass: { pats: ['sub808'], oct: 36 },
    drums: { kick: 'x.........x..x..', clap: HALF, chat: H16, fill: 'none' },
    mel: { mode: 'hook', hr: ['h5', 'h2'], dens: .25, oct: 72, rep: .5 }, sound: snd('wf-pad-tape', 'wf-808-clean', 'wf-vox-chop', 'trap'),
    var: [{ prog: [0, 5, 3, 4] }, { prog: [5, 4, 0, 0] }, { scale: 'dorian', prog: [0, 3, 0, 3] }, { prog: [0, 2, 5, 6], mm: 'motif' }] },
    ['Trap soul', 'R&B chords over trap drums: 808 bass, soft keys and a slow vocal-style hook.', ['Moody', 'Four minor chords.'], ['Yearning', 'Starts away from home.'], ['Dorian', 'Two-chord vibe.'], ['Motif', 'A short repeating motif.']],
    ['Trap soul', 'Trap davulu üstünde R&B akorları: 808 bas, yumuşak tuşlular ve yavaş vokal tadında hook.', ['Kasvetli', 'Dört minör akor.'], ['Özlem', 'Ana akorun dışından başlar.'], ['Dorian', 'İki akorluk hava.'], ['Motif', 'Kısa, tekrar eden motif.']]);
  add('phonk', 'Phonk', 'urban', '#ff3db0', [120, 140, 130], 0, ['phrygian', 'minor'], 0, {
    chord: { mode: 'sustain', pats: ['whole'] }, bass: { pats: ['sub808'], oct: 36 },
    drums: { kick: 'x.....x.x.....x.', clap: C24, chat: H8, ohat: OFF, fill: 'snare' },
    mel: { mode: 'riff', dens: .3, oct: 72 }, sound: snd('choir-pad', 'wf-808-dirt', 'wf-metal-cowbell', 'trap'),
    var: [{ prog: [0, 0, 0, 0] }, { scale: 'phrygian', prog: [0, 1, 0, 1] }, { scale: 'minor', prog: [0, 0, 5, 6] }, { scale: 'phrygian', prog: [0, 5, 0, 1], mm: 'motif' }] },
    ['Phonk', 'A metallic cowbell-style riff over distorted 808s and a driving beat.', ['Drift', 'One note, one riff.'], ['Phrygian', 'Half-step menace.'], ['Lift', 'A late two-chord lift.'], ['Motif', 'A short repeating motif.']],
    ['Phonk', 'Distorsiyonlu 808 ve sürükleyici ritim üstünde metalik, cowbell tadında riff.', ['Drift', 'Tek nota, tek riff.'], ['Frigyen', 'Yarım seslik tehdit.'], ['Yükseliş', 'Geç gelen iki akor.'], ['Motif', 'Kısa, tekrar eden motif.']]);

  HG.GROUPS.push('bass', 'urban', 'chill');
})();
