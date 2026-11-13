// ---- Wavejackers Factory 26: Techno, Melodic Techno ve Progressive House için fabrika bankası (özgün sentez, sample yok)
// Alanlar s2.js ile aynı; 3.1 ekleri: gl (glide sn), fold (dalga katlayıcı 0..1), ring [oran, karışım], vf (hız -> filtre zarfı),
// mono (Hz: bunun altı tek kanal), pr (faz sıfırlama: her nota aynı atakla başlar), demo (ön dinleme cümlesi), info {dil: [karakter, kullanım]}
(function () {
  const F = (id, name, cat, r, g, en, tr, o) => {
    MB.SOUNDS.push(Object.assign({ id, name, cat, r, g: g.split(' '), vol: .2, bank: 'wf', info: { en, tr } }, o));
  };
  const BASS = { mono: 150, pr: 1, kt: .3 };
  const T = 'techno mtechno', TP = 'techno mtechno house trance', P = 'mtechno house trance';

  // ---------- SUB: temiz, kontrollü, mono
  F('wf-sub-anchor', 'Anchor Sub', 'Sub', 'b', T + ' house',
    ['Pure sine sub with a hint of 2nd harmonic. Steady, clean low end.', 'Under any kick. Keep notes in C1-G1.'],
    ['Hafif 2. harmonikli saf sinüs sub. Sabit, temiz alt bölge.', 'Her kick altında. Notaları C1-G1 arasında tut.'],
    Object.assign({ o: [['s', 0, 0, 1], ['t', 0, 1, .07]], f: ['lowpass', 520, .5, 0, 0, .2], a: [.004, .3, .95, .08] }, BASS));
  F('wf-sub-felt', 'Felt Sub', 'Sub', 'b', P,
    ['Warm triangle sub with soft saturation. Audible on small speakers.', 'Progressive house drops, long notes.'],
    ['Yumuşak doygunluklu sıcak üçgen sub. Küçük hoparlörde de duyulur.', 'Progressive house drop, uzun notalar.'],
    Object.assign({ o: [['t', 0, 0, .9], ['s', 0, 0, .6]], f: ['lowpass', 260, .7, 0, 0, .2], fd: .18, a: [.006, .4, .85, .12] }, BASS));
  F('wf-sub-pressure', 'Pressure Sub', 'Sub', 'b', T,
    ['Sine sub with a fast pitch drop at the start. Punchy, kick-like front.', 'Rumble and off-beat techno bass notes.'],
    ['Başında hızlı perde düşüşü olan sinüs sub. Kick gibi vurucu atak.', 'Rumble ve off-beat techno bas notaları.'],
    Object.assign({ o: [['s', 0, 0, 1]], pe: [12, .03], f: ['lowpass', 900, .5, 0, 0, .2], a: [.002, .22, .7, .07] }, BASS));
  F('wf-sub-glide', 'Long Tone Sub', 'Sub', 'b', P,
    ['Round sine sub with portamento between notes and a soft tail.', 'Sustained prog house lines with slides.'],
    ['Notalar arası kaymalı (glide) yuvarlak sinüs sub, yumuşak kuyruk.', 'Kaymalı, uzun progressive house hatları.'],
    Object.assign({ o: [['s', 0, 0, 1], ['t', 0, 0, .12]], gl: .07, f: ['lowpass', 600, .5, 0, 0, .2], a: [.008, .5, .9, .35] }, BASS));
  F('wf-sub-harmonic', 'Harmonic Sub', 'Sub', 'b', TP,
    ['Sine with a filtered square an octave up. The sub stays mono, the edge reads on phones.', 'When the sub must be heard in a dense mix.'],
    ['Sinüs ve bir oktav üstte filtreli kare. Sub mono kalır, üst kenar telefonda bile okunur.', 'Yoğun mikste sub duyulmalıysa.'],
    Object.assign({ o: [['s', 0, 0, 1], ['q', 0, 1, .14]], f: ['lowpass', 900, 1, 0, 0, .2, 1], a: [.004, .3, .85, .1] }, BASS));

  // ---------- ROLLING: tok, agresif, ritmik
  F('wf-roll-tunnel', 'Tunnel Roller', 'Rolling', 'b', T,
    ['Saw and sine through a 24 dB low-pass with a short envelope. Tight and dark; accents open it.', '1/16 rolling bass in the drop, velocity for accents.'],
    ['24 dB alçak geçiren ve kısa zarflı testere + sinüs. Sıkı ve karanlık; aksanlar filtreyi açar.', 'Drop\'ta 1/16 rolling bas, aksan için velocity.'],
    Object.assign({ o: [['w', 0, 0, .8], ['s', 0, 0, .7]], f: ['lowpass', 300, 4, 3.5, 0, .09, 1], vf: .5, a: [.002, .12, .25, .04], fd: .25 }, BASS));
  F('wf-roll-rubber', 'Rubber Knock', 'Rolling', 'b', T + ' house',
    ['Square and triangle with a resonant knock and a tiny pitch blip. Bouncy.', 'Off-beat and triplet rolling patterns.'],
    ['Rezonanslı vuruş ve küçük perde sıçramalı kare + üçgen. Esnek, zıplayan.', 'Off-beat ve üçleme rolling desenleri.'],
    Object.assign({ o: [['q', 0, 0, .7], ['t', 0, -1, .6]], f: ['lowpass', 380, 8, 4, 0, .07], pe: [5, .015], a: [.002, .1, .3, .05] }, BASS));
  F('wf-roll-fm', 'FM Knock', 'Rolling', 'b', T,
    ['FM bass whose brightness decays in 60 ms over a solid sine. Hard, percussive attack.', 'Fast 1/16 techno bass, works without a filter.'],
    ['Parlaklığı 60 ms\'de sönen FM bas, altında sağlam sinüs. Sert, perküsif atak.', 'Hızlı 1/16 techno bas, filtresiz çalışır.'],
    Object.assign({ o: [['s', 0, 0, .55]], fm: [1, 3.2, .06, .05, .8], f: ['lowpass', 2400, .7, 0, 0, .2], a: [.002, .16, .35, .05] }, BASS));
  F('wf-roll-hypno', 'Hypno Roller', 'Rolling', 'b', T,
    ['Digital wavetable bass with a square LFO gating the filter on 16ths. Moves by itself.', 'Hypnotic techno, long held notes.'],
    ['Filtresi 1/16\'da kare LFO ile kapanıp açılan dijital wavetable bas. Kendi kendine hareket eder.', 'Hipnotik techno, uzun tutulan notalar.'],
    Object.assign({ o: [['x', 0, 0, .9]], wt: [8, 10, .4, .5], f: ['lowpass', 420, 6, 1, 0, .1, 1], l: [[4, .45, 'cut', 'square', 1]], a: [.003, .2, .8, .06] }, BASS));
  F('wf-roll-grit', 'Grit Roller', 'Rolling', 'b', T,
    ['Two detuned saws through a wavefolder and a resonant 12 dB filter. Gnarly industrial bite, sub kept mono.', 'Peak-time and industrial techno.'],
    ['Dalga katlayıcı ve rezonanslı 12 dB filtreden iki detune testere. Hırçın endüstriyel ısırık, sub mono.', 'Peak-time ve endüstriyel techno.'],
    Object.assign({ o: [['w', -8, 0, .6], ['w', 8, 0, .6], ['s', 0, -1, .5]], fold: .45, f: ['lowpass', 520, 6, 3, 0, .15], a: [.002, .16, .45, .05] }, BASS));
  F('wf-roll-velvet', 'Velvet Drive', 'Rolling', 'b', P,
    ['Warm saw stack with a slower filter decay and gentle glide. Smooth, driving.', 'Progressive house off-beat bass.'],
    ['Daha yavaş filtre sönümlü, hafif glide\'lı sıcak testere katmanı. Yumuşak ama iten.', 'Progressive house off-beat bas.'],
    Object.assign({ o: [['w', 0, 0, .7], ['w', 0, -1, .5]], f: ['lowpass', 520, 2, 1.5, 0, .25, 1], gl: .03, a: [.003, .3, .6, .08] }, BASS));

  // ---------- ACID: rezonans, glide, aksan
  F('wf-acid-line', 'Acid Line One', 'Acid', 'bm', TP,
    ['Saw through a screaming 24 dB resonant filter; velocity drives the envelope, notes slide.', 'Write accents with velocity 110+, overlap notes for slides.'],
    ['Çığlık atan rezonanslı 24 dB filtreden testere; velocity zarfı sürer, notalar kayar.', 'Aksan için velocity 110+, kayma için notaları üst üste bindir.'],
    { o: [['w', 0, 0, 1]], f: ['lowpass', 420, 16, 5, 0, .18, 1], vf: .8, gl: .06, fd: .35, kt: .4, dl: .12, a: [.002, .2, .7, .05], mono: 150 });
  F('wf-acid-square', 'Square Acid', 'Acid', 'bm', T,
    ['Hollow square acid through a 12 dB filter: brighter squelch, longer decay, more drive.', 'Squelchy acid basslines that need to cut.'],
    ['12 dB filtreden kof kare acid: daha parlak cıvıltı, daha uzun sönüm, daha çok sürüş.', 'Mikste öne çıkması gereken cıvıltılı acid baslar.'],
    { o: [['q', 0, 0, 1]], f: ['lowpass', 260, 22, 7, 0, .3], vf: .7, gl: .05, fd: .6, kt: .4, a: [.002, .25, .55, .05], mono: 150 });
  F('wf-acid-sour', 'Sour Acid', 'Acid', 'bm', T,
    ['Folded saw with a bar-long filter LFO. The tone keeps mutating.', 'Long acid jams where the line repeats.'],
    ['Bir ölçülük filtre LFO\'lu katlanmış testere. Tını sürekli değişir.', 'Hat tekrar ederken uzun acid bölümleri.'],
    { o: [['w', 0, 0, .9]], fold: .35, f: ['lowpass', 500, 14, 4, 0, .22], l: [[.25, .4, 'cut', 'triangle', 1]], vf: .6, gl: .05, kt: .4, a: [.002, .2, .6, .05], mono: 150 });
  F('wf-acid-rubber', 'Rubber 303', 'Acid', 'b', TP,
    ['Acid with a sine sub underneath and a slower slide. Heavier low end.', 'When acid is the only bass.'],
    ['Altında sinüs sub, daha yavaş kaymalı acid. Daha dolu alt bölge.', 'Acid tek bas olduğunda.'],
    { o: [['w', 0, 0, .9]], sub: .4, f: ['lowpass', 480, 12, 3, 0, .1, 1], vf: .7, gl: .09, kt: .4, fd: .2, a: [.002, .14, .65, .05], mono: 150 });
  F('wf-acid-scream', 'Scream Acid', 'Acid', 'm', T,
    ['Wide acid lead: two saws, high resonance, drive and a dotted delay.', 'Top-line acid riffs, C3-C5.'],
    ['Geniş acid lead: iki testere, yüksek rezonans, sürüş ve noktalı delay.', 'Üst acid riff\'leri, C3-C5.'],
    { u: [2, 6], o: [['w', 0, 0, .8]], f: ['lowpass', 900, 18, 4, 0, .16, 1], vf: .8, gl: .05, dr: .4, dl: .25, a: [.002, .2, .6, .1], mono: 150 });

  // ---------- LEAD: karanlık, etkileyici
  F('wf-lead-obsidian', 'Obsidian Lead', 'Lead', 'm', T,
    ['Hollow-to-digital wavetable, three voices, a slow filter bloom and delayed vibrato.', 'Melodic techno main hooks, long notes.'],
    ['Kof-dijital wavetable, üç ses, yavaş açılan filtre ve gecikmeli vibrato.', 'Melodic techno ana hook\'ları, uzun notalar.'],
    { u: [3, 10], o: [['x', 0, 0, .8]], wt: [10, 8, .45, .7], f: ['lowpass', 1500, 2, 1.2, .02, .6, 1], vib: [5.5, 6, .35], a: [.01, .5, .8, .35], rv: .3, dl: .25 });
  F('wf-lead-cathedral', 'Cathedral Lead', 'Lead', 'm', P,
    ['Big supersaw-square stack with chorus. Emotional and wide; mono sum stays solid.', 'Euphoric breakdown melodies.'],
    ['Chorus\'lu büyük süper testere + kare katmanı. Duygusal ve geniş; mono toplamı sağlam.', 'Coşkulu breakdown melodileri.'],
    { u: [5, 14], o: [['w', 0, 0, .6], ['q', 5, 0, .25]], f: ['lowpass', 2600, 1, .6, .05, .8], ch: .4, rv: .35, dl: .2, a: [.02, .5, .85, .45] });
  F('wf-lead-ghost', 'Ghost Lead', 'Lead', 'm', T,
    ['Sine and soft triangle an octave up, glide and vibrato. Haunting and thin on purpose.', 'Sparse melodies over dark pads.'],
    ['Sinüs ve bir oktav üstte yumuşak üçgen, glide ve vibrato. Bilerek ince ve hayaletimsi.', 'Karanlık pad\'ler üstünde seyrek melodiler.'],
    { o: [['s', 0, 0, .9], ['t', 0, 1, .25]], gl: .08, fd: .2, vib: [5, 9, .4], a: [.02, .4, .85, .5], dl: .35, rv: .35 });
  F('wf-lead-razor', 'Razor Lead', 'Lead', 'm', T,
    ['Narrow pulse and saw, resonant and folded. Cuts through anything.', 'Aggressive techno riffs and stabs.'],
    ['Dar darbe ve testere; rezonanslı ve katlanmış. Her şeyin içinden geçer.', 'Agresif techno riff ve stab\'ları.'],
    { o: [['x', 0, 0, .7], ['w', 7, 0, .5]], wt: [9, 0, .3, 1], fold: .15, fd: .4, f: ['lowpass', 3200, 4, .8, 0, .2], a: [.003, .25, .7, .12], dl: .18 });
  F('wf-lead-shard', 'FM Shard', 'Lead', 'cm', TP,
    ['FM bell lead with a non-integer ratio. Glassy attack, dark body.', 'Hooks that must stay readable at low volume.'],
    ['Tam sayı olmayan oranlı FM çan lead. Camsı atak, karanlık gövde.', 'Düşük seste bile okunması gereken hook\'lar.'],
    { o: [['s', 0, 0, .4]], fm: [3.5, 2, .3, .3, .9], f: ['lowpass', 5000, .7, 0, 0, .2], a: [.003, .7, .4, .4], dl: .3, rv: .25 });
  F('wf-lead-mourning', 'Mourning Saw', 'Lead', 'm', 'mtechno trance',
    ['Three saws with a slow attack and a filter that swells over a second.', 'Slow, emotional lines; let notes ring.'],
    ['Yavaş ataklı üç testere; filtre bir saniyede açılır.', 'Yavaş, duygusal hatlar; notaları uzun tut.'],
    { u: [3, 9], o: [['w', 0, 0, .7]], f: ['lowpass', 700, 1.5, 2, .9, 1.2], a: [.09, .8, .8, .7], ch: .2, rv: .4, dl: .2 });
  F('wf-lead-chant', 'Chant Lead', 'Lead', 'm', 'mtechno trance afro',
    ['Formant-filtered saw that sings an "ah" with vibrato. Vocal colour without samples.', 'Hooks that should feel human.'],
    ['"Aa" diye söyleyen, vibratolu formant filtreli testere. Sample\'sız vokal renk.', 'İnsani hissettirmesi gereken hook\'lar.'],
    { u: [2, 8], o: [['w', 0, 0, .7]], vow: 'a', vib: [5.2, 10, .3], a: [.04, .4, .85, .4], rv: .35, vol: .45 });

  // ---------- PLUCK: analog karakter
  F('wf-pluck-analog', 'Analog Pluck', 'Pluck', 'cm', TP,
    ['Saw and square, 24 dB filter snapping shut in 120 ms, velocity-sensitive.', 'Chord plucks and 1/8 arps.'],
    ['Testere + kare, 24 dB filtre 120 ms\'de kapanır, velocity\'ye duyarlı.', 'Akor pluck\'ları ve 1/8 arpejler.'],
    { o: [['w', 0, 0, .6], ['q', 4, 0, .3]], f: ['lowpass', 500, 2, 6, 0, .12, 1], vf: .5, a: [.001, .25, 0, .15], dl: .2, rv: .15 });
  F('wf-pluck-wood', 'Hollow Wood', 'Pluck', 'cm', 'mtechno afro deep',
    ['Triangle and noise through a band-pass. Short, woody and dark.', 'Percussive melodic patterns.'],
    ['Bant geçirenden geçen üçgen ve gürültü. Kısa, ahşap, karanlık.', 'Perküsif melodik desenler.'],
    { o: [['t', 0, 0, .9], ['s', 0, 1, .3]], noi: .05, f: ['bandpass', 900, 2, 2, 0, .08], a: [.001, .2, 0, .12], rv: .2 });
  F('wf-pluck-crystal', 'Crystal Tick', 'Pluck', 'cm', 'mtechno trance',
    ['Very short FM tick with a sine body an octave up. Bright, tiny transient.', 'Fast 1/16 arps, high register.'],
    ['Çok kısa FM tık, bir oktav üstte sinüs gövde. Parlak, küçük geçici.', 'Hızlı 1/16 arpejler, tiz bölge.'],
    { o: [['s', 0, 1, .5]], fm: [7, 1.5, .05, 0, .7], a: [.001, .18, 0, .1], dl: .3, rv: .25 });
  F('wf-pluck-dub', 'Dub Pluck', 'Pluck', 'cm', 'techno deep',
    ['Resonant square pluck sent into a long dotted delay and a dark room.', 'Dub techno chords, sparse hits.'],
    ['Uzun noktalı delay ve karanlık odaya gönderilen rezonanslı kare pluck.', 'Dub techno akorları, seyrek vuruşlar.'],
    { o: [['q', 0, 0, .8]], f: ['lowpass', 420, 8, 4, 0, .1], a: [.001, .22, 0, .15], dl: .45, rv: .25 });
  F('wf-pluck-prog', 'Prog Pluck', 'Pluck', 'cm', P,
    ['Five-voice detuned saw pluck with a bit of chorus. Wide and bright.', 'Progressive house chord plucks.'],
    ['Biraz chorus\'lu beş sesli detune testere pluck. Geniş ve parlak.', 'Progressive house akor pluck\'ları.'],
    { u: [5, 16], o: [['w', 0, 0, .6]], f: ['lowpass', 700, 1.5, 5, 0, .2], a: [.001, .35, .05, .25], ch: .3, dl: .3, rv: .2 });
  F('wf-pluck-mallet', 'Glass Mallet', 'Pluck', 'cm', 'mtechno trance',
    ['Glass and bell wavetables with a soft mallet decay.', 'Melodic patterns and counter-melodies.'],
    ['Yumuşak tokmak sönümlü cam ve çan wavetable\'ları.', 'Melodik desenler ve karşı melodiler.'],
    { o: [['x', 0, 0, .9]], wt: [3, 5, .5, .8], a: [.001, .4, .05, .3], rv: .3, dl: .15 });

  // ---------- STAB: kısa synth stab
  F('wf-stab-basement', 'Basement Chord', 'Stab', 'c', 'techno deep',
    ['Square-saw chord through a resonant filter into delay and room. Classic dub chord.', 'Off-beat chord stabs.'],
    ['Rezonanslı filtreden delay ve odaya giden kare-testere akor. Klasik dub akor.', 'Off-beat akor stab\'ları.'],
    { o: [['q', 0, 0, .5], ['w', 3, 0, .4]], f: ['lowpass', 600, 6, 3, 0, .14], a: [.002, .25, .1, .2], dl: .5, rv: .3, ch: .2 });
  F('wf-stab-warehouse', 'Warehouse Stab', 'Stab', 'c', 'techno house',
    ['Wide four-voice saw plus square with a fast filter snap. 90s rave energy.', 'Short chord hits in drops.'],
    ['Hızlı filtre kapanışlı geniş dört sesli testere + kare. 90\'lar rave enerjisi.', 'Drop\'ta kısa akor vuruşları.'],
    { u: [4, 22], o: [['w', 0, 0, .55], ['q', 7, 0, .3]], f: ['lowpass', 1400, 2, 3, 0, .1], a: [.002, .18, .25, .1], rv: .2, dl: .2 });
  F('wf-stab-organ', 'Prog Organ', 'Stab', 'c', 'house trance',
    ['Organ wavetable with a sine octave. Short, round house stab.', 'Piano-house style chord rhythms.'],
    ['Sinüs oktavlı org wavetable. Kısa, yuvarlak house stab.', 'Piano-house tarzı akor ritimleri.'],
    { o: [['x', 0, 0, .7], ['s', 0, 1, .3]], wt: [4, 4, 0, 1], a: [.003, .2, .3, .12], rv: .2 });
  F('wf-stab-hammer', 'Hammer Stab', 'Stab', 'cm', T,
    ['Inharmonic FM over a square, slightly folded. Metallic and hard.', 'Industrial one-shot chords.'],
    ['Kare üstünde uyumsuz FM, hafif katlanmış. Metalik ve sert.', 'Endüstriyel tek vuruşluk akorlar.'],
    { o: [['q', 0, -1, .4]], fm: [1.41, 4, .08, .1, .8], fold: .2, a: [.001, .2, .05, .15], rv: .2 });
  F('wf-stab-brass', 'Dark Brass', 'Stab', 'c', 'mtechno trance',
    ['Two detuned saws with a slow 30 ms filter rise. Brassy swell.', 'Dramatic chord hits.'],
    ['30 ms\'de açılan filtreli iki detune testere. Bakır üflemeli şişkinlik.', 'Dramatik akor vuruşları.'],
    { o: [['w', -6, 0, .55], ['w', 6, 0, .55]], f: ['lowpass', 700, 1, 2.5, .03, .25], a: [.02, .3, .55, .2], rv: .25 });
  F('wf-stab-chop', 'Chopped Chord', 'Stab', 'c', 'techno house',
    ['Triangle-saw chord through a band-pass that a 1/8 square LFO chops.', 'Long chords that sound rhythmic.'],
    ['1/8 kare LFO ile doğranan bant geçirenden triangle-testere akor.', 'Ritmik duyulan uzun akorlar.'],
    { o: [['t', 0, 0, .6], ['w', 0, 0, .4]], f: ['bandpass', 1100, 2, 1, 0, .2], l: [[2, .8, 'amp', 'square', 1]], a: [.005, .4, .7, .2], dl: .25, rv: .2 });

  // ---------- SEQ: sekans ve arpej
  F('wf-seq-pulse', 'Pulse Sequence', 'Seq', 'm', TP,
    ['Narrow pulse with a one-bar triangle LFO on the filter over a short envelope.', '1/16 sequences that evolve over the bar.'],
    ['Kısa zarf üstünde bir ölçülük üçgen filtre LFO\'lu dar darbe.', 'Ölçü boyunca gelişen 1/16 sekanslar.'],
    { o: [['x', 0, 0, .8]], wt: [9, 9, 0, .9], f: ['lowpass', 900, 3, 3, 0, .09, 1], l: [[.25, .5, 'cut', 'triangle', 1]], a: [.001, .15, .2, .08], dl: .2 });
  F('wf-seq-bounce', 'Bounce Sequence', 'Seq', 'm', T,
    ['Resonant saw with strong velocity response into a dotted delay. Echoes bounce back.', 'Sparse patterns; the delay fills the gaps.'],
    ['Güçlü velocity tepkili rezonanslı testere, noktalı delay. Yankılar geri seker.', 'Seyrek desenler; boşlukları delay doldurur.'],
    { o: [['w', 0, 0, .9]], f: ['lowpass', 600, 10, 4, 0, .07], vf: .8, a: [.001, .12, .1, .08], dl: .35 });
  F('wf-seq-hypno', 'Hypnotic Arp', 'Seq', 'cm', T,
    ['Digital wavetable with a half-bar filter LFO. Repeats without sounding static.', 'Long hypnotic arpeggios.'],
    ['Yarım ölçülük filtre LFO\'lu dijital wavetable. Tekrarlar ama durağan duyulmaz.', 'Uzun hipnotik arpejler.'],
    { o: [['x', 0, 0, .8]], wt: [8, 3, .5, .7], f: ['lowpass', 1200, 4, 2, 0, .12], l: [[.5, .6, 'cut', 'sine', 1]], a: [.001, .2, .15, .1], dl: .25, rv: .15 });
  F('wf-seq-sparkle', 'Sparkle Arp', 'Seq', 'cm', 'mtechno trance house',
    ['Light FM and triangle an octave up, short with delay and room. Glittering.', 'High arps above pads.'],
    ['Hafif FM ve bir oktav üstte üçgen; kısa, delay ve odalı. Parıltılı.', 'Pad\'lerin üstünde tiz arpejler.'],
    { o: [['t', 0, 1, .4]], fm: [2, 1.2, .1, .1, .6], a: [.001, .25, .05, .15], dl: .4, rv: .3 });
  F('wf-seq-ring', 'Ring Sequence', 'Seq', 'm', T,
    ['Square ring-modulated at 1.5x pitch. Metallic but in tune.', 'Industrial sequences.'],
    ['1,5 kat perdede halka modülasyonlu kare. Metalik ama akortlu.', 'Endüstriyel sekanslar.'],
    { o: [['q', 0, 0, .8]], ring: [1.5, .5], f: ['lowpass', 2000, 2, 1.5, 0, .1], a: [.001, .16, .1, .08], dl: .2 });
  F('wf-seq-cine', 'Cinematic Arp', 'Seq', 'cm', 'mtechno trance',
    ['Three-voice saw with chorus, delay and room. Big and cinematic.', 'Breakdown arpeggios.'],
    ['Chorus, delay ve odalı üç sesli testere. Büyük ve sinematik.', 'Breakdown arpejleri.'],
    { u: [3, 8], o: [['w', 0, 0, .7]], f: ['lowpass', 1200, 2, 3, 0, .15], a: [.001, .3, .1, .2], ch: .3, dl: .3, rv: .25 });

  // ---------- PAD
  F('wf-pad-horizon', 'Horizon Pad', 'Pad', 'c', P,
    ['Six-voice saw pad, slow attack, a slow filter LFO and chorus. Open and wide.', 'Breakdowns and intros.'],
    ['Altı sesli testere pad, yavaş atak, yavaş filtre LFO\'su ve chorus. Açık ve geniş.', 'Breakdown ve intro.'],
    { u: [6, 18], o: [['w', 0, 0, .55]], f: ['lowpass', 1600, .7, 0, 0, .2], lf: [.1, .25], a: [1.2, 1, .85, 2], ch: .5, rv: .45 });
  F('wf-pad-nocturne', 'Nocturne Pad', 'Pad', 'c', T,
    ['Hollow and vowel wavetables blended, dark low-pass, long release.', 'Dark melodic techno beds.'],
    ['Kof ve vokal wavetable karışımı, karanlık alçak geçiren, uzun bırakma.', 'Karanlık melodic techno zeminleri.'],
    { u: [3, 10], o: [['x', 0, 0, .8]], wt: [10, 7, .4, .6], f: ['lowpass', 1100, 1, 0, 0, .2], a: [.9, 1, .85, 2.2], rv: .5, ch: .3 });
  F('wf-pad-glass', 'Glass Cathedral', 'Pad', 'c', 'mtechno trance',
    ['Sines stacked in octaves with a triangle on top. Clear, airy, almost no low end.', 'Above a busy bass.'],
    ['Oktav oktav sinüsler, üstte üçgen. Berrak, havadar, neredeyse altı yok.', 'Kalabalık bas üstünde.'],
    { o: [['s', 0, 0, .5], ['s', 3, 1, .4], ['t', -3, 2, .15]], a: [1.5, 1, .85, 2.4], ch: .4, rv: .55 });
  F('wf-pad-warm', 'Analog Warmth', 'Pad', 'c', 'house deep mtechno',
    ['Triangle and saw with a filter that slowly swells open over 1.2 s.', 'Warm chord beds in prog house.'],
    ['1,2 saniyede açılan filtreli üçgen ve testere.', 'Progressive house\'ta sıcak akor zemini.'],
    { o: [['t', 0, 0, .6], ['w', 6, 0, .4]], f: ['lowpass', 700, 1, 1.2, 1.2, 2], a: [.8, 1, .9, 1.6], ch: .4, rv: .35 });
  F('wf-pad-choir', 'Distant Choir', 'Pad', 'c', 'mtechno trance afro',
    ['Four-voice formant "oh" with vibrato and a long room. Distant choir colour.', 'Big moments; layer under leads.'],
    ['Vibratolu, uzun odalı dört sesli formant "oo". Uzaktan koro rengi.', 'Büyük anlar; lead\'lerin altına katman.'],
    { u: [4, 12], o: [['w', 0, 0, .6]], vow: 'o', male: true, vib: [4.8, 7, .5], a: [.9, .8, .9, 1.8], rv: .5, ch: .3, vol: .5 });
  F('wf-pad-strings', 'Tension Strings', 'Pad', 'c', 'mtechno trance',
    ['Five-voice saw strings through a soft band-pass with vibrato. Tense, cinematic.', 'Build-ups and dramatic sections.'],
    ['Vibratolu, yumuşak bant geçirenli beş sesli testere yaylı. Gergin, sinematik.', 'Build-up ve dramatik bölümler.'],
    { u: [5, 12], o: [['w', 0, 0, .6]], f: ['bandpass', 1800, 1, 0, 0, .2], vib: [5.4, 7, .6], a: [.5, .6, .9, 1.4], ch: .4, rv: .4 });

  // ---------- TEXTURE: atmosfer
  F('wf-tex-air', 'Air Bed', 'Texture', 'c', TP,
    ['Breathy noise and sine through a slowly moving band-pass. Air without pitch focus.', 'Hold a chord under the whole breakdown.'],
    ['Yavaş hareket eden bant geçirenden nefesli gürültü ve sinüs. Perde odağı olmayan hava.', 'Tüm breakdown boyunca bir akor tut.'],
    { o: [['s', 0, 0, .3]], noi: .5, f: ['bandpass', 2500, 1.5, 0, 0, .2], lf: [.07, .6], a: [2, 1, .9, 3], rv: .6 });
  F('wf-tex-drone', 'Iron Drone', 'Texture', 'c', T,
    ['Low saw and square, folded, through a resonant filter that drifts slowly.', 'Industrial intros and tension.'],
    ['Katlanmış alçak testere ve kare; yavaşça kayan rezonanslı filtre.', 'Endüstriyel intro ve gerilim.'],
    { o: [['w', 0, -1, .6], ['q', 9, 0, .3]], fold: .2, f: ['lowpass', 350, 6, 0, 0, .2], lf: [.05, .8], a: [2, 1, .9, 2.5], rv: .35 });
  F('wf-tex-shimmer', 'Shimmer Veil', 'Texture', 'c', 'mtechno trance',
    ['Bell wavetable with a sine two octaves up, four voices, chorus, delay and long room.', 'Sparkle over pads in breakdowns.'],
    ['İki oktav üstte sinüslü çan wavetable, dört ses, chorus, delay ve uzun oda.', 'Breakdown\'da pad üstüne parıltı.'],
    { u: [4, 10], o: [['x', 0, 0, .5], ['s', 0, 2, .2]], wt: [5, 3, .3, .6], a: [1.5, 1, .8, 2.5], ch: .7, rv: .6, dl: .3 });
  F('wf-tex-breath', 'Breathing Pulse', 'Texture', 'c', P,
    ['Soft triangle pad whose level pulses on half notes. Breathes with the tempo.', 'Rhythmic background in prog house.'],
    ['Seviyesi yarım notalarla nefes alan yumuşak üçgen pad. Tempoyla nefes alır.', 'Progressive house\'ta ritmik arka plan.'],
    { u: [3, 8], o: [['t', 0, 0, .8]], l: [[.5, .8, 'amp', 'sine', 1]], a: [1, 1, .9, 1.8], rv: .4 });
  F('wf-tex-dust', 'Static Dust', 'Texture', 'c', T,
    ['High band-passed noise with a fast square tremolo. Crackle and grit, no tone.', 'Quiet layer for tension; keep it low.'],
    ['Hızlı kare tremololu tiz bant geçiren gürültü. Çıtırtı ve kum; tonu yok.', 'Gerilim için sessiz katman; düşük tut.'],
    { o: [['s', 0, 0, .05]], noi: .7, f: ['bandpass', 6000, 2, 0, 0, .2], l: [[13, .5, 'amp', 'square', 0]], a: [.5, 1, .9, 1.5], rv: .5 });

  // ---------- METAL: metalik, endüstriyel
  F('wf-metal-anvil', 'Anvil', 'Metal', 'cm', T,
    ['Inharmonic FM hit with a fast pitch drop. Pitched industrial strike.', 'Accents on the 4 or the off-beat.'],
    ['Hızlı perde düşüşlü uyumsuz FM vuruşu. Perdeli endüstriyel darbe.', '4. vuruşta ya da off-beat\'te aksan.'],
    { o: [['s', 0, 0, .2]], fm: [1.414, 6, .12, .05, .8], pe: [24, .01], a: [.001, .3, 0, .2], rv: .2 });
  F('wf-metal-clang', 'Clang Bell', 'Metal', 'cm', T,
    ['FM bell at a 3.73 ratio, ring-modulated. Cold and metallic with a long tail.', 'Sparse hits and call-and-response.'],
    ['3,73 oranlı halka modülasyonlu FM çan. Soğuk, metalik, uzun kuyruk.', 'Seyrek vuruşlar ve soru-cevap.'],
    { o: [['s', 0, 0, .2]], fm: [3.73, 3, .5, .2, .8], ring: [2.17, .4], a: [.001, .9, .1, .6], rv: .35 });
  F('wf-metal-grinder', 'Grinder', 'Metal', 'bm', T,
    ['Detuned saw and square, heavily folded and driven. Abrasive industrial tone.', 'Industrial basslines or lead riffs; sub kept mono.'],
    ['Detune testere ve kare, fazlaca katlanmış ve sürülmüş. Aşındırıcı endüstriyel tını.', 'Endüstriyel bas ya da lead riff; sub mono.'],
    { o: [['w', 0, 0, .6], ['q', 30, 0, .4]], fold: .6, fd: .4, f: ['lowpass', 1500, 3, 2, 0, .15, 1], a: [.002, .2, .6, .1], mono: 150, kt: .4 });
  F('wf-metal-pipe', 'Steel Pipe', 'Metal', 'cm', T,
    ['Sine ring-modulated at 2.76x. Hollow pipe knock with a quick decay.', 'Percussive melodic patterns.'],
    ['2,76 katta halka modülasyonlu sinüs. Hızlı sönümlü kof boru vuruşu.', 'Perküsif melodik desenler.'],
    { o: [['s', 0, 0, .9]], ring: [2.76, .7], a: [.001, .6, 0, .3], dl: .3, rv: .2 });
  F('wf-metal-rust', 'Rust Sequence', 'Metal', 'm', T,
    ['Digital wavetable ring-modulated at 1.33x with a snappy filter. Corroded arp.', 'Industrial 1/16 sequences.'],
    ['1,33 katta halka modülasyonlu, hızlı filtreli dijital wavetable. Paslı arpej.', 'Endüstriyel 1/16 sekanslar.'],
    { o: [['x', 0, 0, .8]], wt: [8, 8, 0, .9], ring: [1.33, .5], f: ['lowpass', 2500, 3, 2, 0, .06], a: [.001, .14, .1, .08], dl: .2 });

  // ---------- FX: riser, impact, geçiş (uzun nota çalınır)
  F('wf-fx-riser', 'Noise Riser', 'FX', 'cm', TP,
    ['Noise and saws rising two octaves while the filter opens over 6 s.', 'Hold one note for 4-8 bars before the drop.'],
    ['Filtre 6 saniyede açılırken iki oktav yükselen gürültü ve testereler.', 'Drop\'tan önce 4-8 ölçü tek nota tut.'],
    { u: [4, 20], o: [['w', 0, 0, .4]], noi: .8, pe: [-24, 4], f: ['lowpass', 300, 1, 30, 6, 1], a: [3, 1, 1, 1], rv: .5, demo: 'fx' });
  F('wf-fx-impact', 'Sub Impact', 'FX', 'cm', TP,
    ['Sine dropping three octaves with a noise burst and room. Weight for the downbeat.', 'First beat of a drop.'],
    ['Gürültü patlaması ve odayla üç oktav düşen sinüs. İlk vuruşa ağırlık.', 'Drop\'un ilk vuruşu.'],
    { o: [['s', 0, 0, 1]], noi: .3, pe: [36, .25], f: ['lowpass', 2000, .7, 4, 0, .3], a: [.001, 1.2, 0, .6], rv: .4, demo: 'hit' });
  F('wf-fx-down', 'Downlifter', 'FX', 'cm', TP,
    ['Noise and saw falling two octaves while the filter closes. Releases tension.', 'After a drop or into a breakdown.'],
    ['Filtre kapanırken iki oktav düşen gürültü ve testere. Gerilimi boşaltır.', 'Drop sonrası ya da breakdown\'a girerken.'],
    { o: [['w', 0, 0, .4]], noi: .6, pe: [24, 2], f: ['lowpass', 400, 1, 12, 0, 2.5], a: [.01, 3, 0, 1], rv: .5, demo: 'fx' });
  F('wf-fx-sweep', 'Tension Sweep', 'FX', 'c', TP,
    ['Wall-of-sound wavetable, five voices, filter opening over 8 s.', 'Hold a chord through the build-up.'],
    ['Beş sesli "ses duvarı" wavetable, filtre 8 saniyede açılır.', 'Build-up boyunca bir akor tut.'],
    { u: [5, 16], o: [['x', 0, 0, .6]], wt: [11, 0, .3, .8], f: ['lowpass', 200, 2, 40, 8, 1], a: [2, 1, 1, 1.5], rv: .4, demo: 'fx' });
  F('wf-fx-zap', 'Laser Zap', 'FX', 'cm', T,
    ['Sine falling four octaves in 80 ms into a delay. Short transition zap.', 'Fills and section edges.'],
    ['80 ms\'de dört oktav düşen sinüs ve delay. Kısa geçiş zap\'ı.', 'Dolgu ve bölüm geçişleri.'],
    { o: [['s', 0, 0, .9]], pe: [48, .08], a: [.001, .25, 0, .1], dl: .4, demo: 'hit' });

  MB.SOUNDS.forEach(s => { MB.byId[s.id] = s; });
  MB.CATS = MB.CATS.concat(['Sub', 'Rolling', 'Acid', 'Seq', 'Texture', 'Metal', 'FX']);
})();

// ---- Factory 26 genişlemesi (3.3): rap, R&B, bass müzik, house ve hard türleri için 24 yeni ses (özgün sentez, sample yok)
(function () {
  const F = (id, name, cat, r, g, en, tr, o) => {
    const s = Object.assign({ id, name, cat, r, g: g.split(' '), vol: .2, bank: 'wf', info: { en, tr } }, o);
    MB.SOUNDS.push(s); MB.byId[id] = s;
  };
  const BASS = { mono: 150, pr: 1, kt: .3 };
  const U = 'trap lofi', B = 'dnb trap', H = 'house deep';

  // ---------- 808 ve bas
  F('wf-808-clean', 'Clean 808', 'Sub', 'b', U + ' dnb',
    ['Sine 808 with a short pitch drop and a long, even tail.', 'Trap and R&B bass lines. Let notes ring.'],
    ['Kısa perde düşüşlü, uzun ve düzgün kuyruklu sinüs 808.', 'Trap ve R&B bas hatları. Notaları uzun bırak.'],
    Object.assign({ o: [['s', 0, 0, 1]], pe: [9, .04], f: ['lowpass', 700, .5, 0, 0, .2], a: [.002, 1.6, .5, .5] }, BASS));
  F('wf-808-dirt', 'Dirt 808', 'Sub', 'b', U,
    ['Overdriven 808: the saturation adds harmonics that cut through phone speakers.', 'Drill, phonk and hard trap.'],
    ['Doyurulmuş 808: eklenen harmonikler telefon hoparlöründe de duyulur.', 'Drill, phonk ve sert trap.'],
    Object.assign({ o: [['s', 0, 0, 1], ['t', 0, 1, .15]], pe: [12, .03], fold: .25, fd: .7, f: ['lowpass', 1400, .6, 0, 0, .2], a: [.002, 1.1, .45, .35] }, BASS));
  F('wf-808-slide', 'Slide 808', 'Sub', 'b', U,
    ['808 that glides between overlapping notes.', 'Drill slides: overlap two notes to bend.'],
    ['Üst üste binen notalar arasında kayan 808.', 'Drill kaymaları: iki notayı üst üste bindir.'],
    Object.assign({ o: [['s', 0, 0, 1], ['s', 0, 1, .1]], gl: .11, pe: [5, .05], fd: .3, f: ['lowpass', 900, .5, 0, 0, .2], a: [.003, 1.3, .7, .3] }, BASS));
  F('wf-bass-reese', 'Night Reese', 'Bas', 'b', B + ' techno',
    ['Two detuned saws beating against each other, mono below 150 Hz.', 'Drum & bass and breakbeat long notes.'],
    ['Birbirine vuran iki detune testere, 150 Hz altı mono.', 'Drum & bass ve breakbeat uzun notaları.'],
    Object.assign({ o: [['w', -19, 0, .6], ['w', 19, 0, .6], ['s', 0, -1, .7]], f: ['lowpass', 640, 2, 1.5, .02, .6], lf: [.35, .35], a: [.012, .4, .9, .22], fd: .3 }, BASS));
  F('wf-bass-wobble', 'Wobble', 'Bas', 'b', B,
    ['Saw and square through a filter that opens and closes three times a second.', 'Dubstep and bass house drops.'],
    ['Saniyede üç kez açılıp kapanan filtreden geçen testere ve kare.', 'Dubstep ve bass house drop\'ları.'],
    Object.assign({ o: [['w', 0, 0, .7], ['q', 0, -1, .5]], f: ['lowpass', 420, 6, 2, 0, .5], lf: [3, .8], a: [.004, .3, .9, .12], fd: .5 }, BASS));
  F('wf-bass-log', 'Log Drum', 'Bas', 'b', 'afro deep house',
    ['Knocking FM tone with a fast pitch drop: the amapiano log drum.', 'Short syncopated bass hits.'],
    ['Hızlı perde düşüşlü, tok FM tonu: amapiano log drum\'ı.', 'Kısa, senkoplu bas vuruşları.'],
    Object.assign({ fm: [2, 1.4, .12, 0], o: [['s', 0, 0, .9]], pe: [7, .05], f: ['lowpass', 1800, .5, 0, 0, .2], a: [.001, .45, 0, .2] }, BASS));
  F('wf-bass-slap', 'Slap Pluck', 'Bas', 'b', H,
    ['Square bass with a very fast filter snap: the slap house bounce.', 'Bouncing off-beat patterns.'],
    ['Çok hızlı filtre vuruşlu kare bas: slap house zıplaması.', 'Zıplayan off-beat kalıpları.'],
    Object.assign({ o: [['q', 0, 0, .8], ['w', 7, 0, .3]], f: ['lowpass', 300, 4, 5, 0, .09], vf: .5, a: [.001, .22, .25, .08] }, BASS));
  F('wf-bass-felt', 'Felt Upright', 'Bas', 'b', 'lofi deep',
    ['Soft triangle with a rounded attack, like a muted upright.', 'Lo-fi and boom bap walking lines.'],
    ['Yuvarlak ataklı yumuşak üçgen; susturulmuş kontrbas gibi.', 'Lo-fi ve boom bap yürüyen hatları.'],
    Object.assign({ o: [['t', 0, 0, .9], ['s', 0, 1, .25]], f: ['lowpass', 520, 1, 1.5, 0, .15], a: [.012, .5, .2, .14] }, BASS));

  // ---------- tuşlular
  F('wf-keys-tine', 'Tine Keys', 'Keys', 'cm', U + ' ' + H,
    ['FM electric piano: bell-like attack that mellows as the note holds.', 'R&B and deep house seventh chords.'],
    ['FM elektrikli piyano: çan gibi atak, nota sürdükçe yumuşar.', 'R&B ve deep house yedili akorları.'],
    { fm: [1, 1.1, .5, .15], o: [['s', 0, 0, .3]], f: ['lowpass', 5200, .5, 0, 0, .3], vf: .4, a: [.003, 1.2, .25, .5], ch: .5, rv: .25 });
  F('wf-keys-dusty', 'Dusty Keys', 'Keys', 'cm', 'lofi trap',
    ['Dark, slightly wobbly keys with tape hiss.', 'Lo-fi chords and sleepy melodies.'],
    ['Karanlık, hafif dalgalanan, bant hışırtılı tuşlu.', 'Lo-fi akorlar ve uykulu melodiler.'],
    { o: [['t', 0, 0, .8], ['s', 0, 1, .2]], f: ['lowpass', 1500, .7, 1, 0, .4], noi: .03, vib: [.7, 14, 0], a: [.006, .9, .3, .5], rv: .3 });
  F('wf-keys-house', 'House Piano', 'Keys', 'cm', H,
    ['Bright, hard-hitting piano-style stack that cuts through a full mix.', 'Piano house chord stabs.'],
    ['Dolu mix\'i delen parlak, sert vuruşlu piyano tadında katman.', 'Piano house akor vuruşları.'],
    { o: [['w', 0, 0, .5], ['t', 0, 1, .4], ['s', 0, 0, .5]], f: ['lowpass', 2600, 1, 4, 0, .5], vf: .5, a: [.002, 1, .15, .4], rv: .3 });
  F('wf-keys-organ', 'Garage Organ', 'Keys', 'c', H + ' dnb',
    ['Percussive drawbar organ with a hollow top.', 'UK garage and house off-beat stabs.'],
    ['Oyuk tınılı, vurmalı drawbar org.', 'UK garage ve house off-beat vuruşları.'],
    { o: [['s', 0, 0, .8], ['s', 0, 1, .6], ['s', 0, 1.58, .4]], f: ['lowpass', 5000, .5, 0, 0, .3], a: [.002, .25, .5, .07], ch: .4, rv: .2 });

  // ---------- pad ve akor
  F('wf-pad-tape', 'Tape Pad', 'Pad', 'c', 'lofi trap deep',
    ['Soft detuned pad with slow tape wobble.', 'Trap soul, downtempo and intros.'],
    ['Yavaş bant dalgalanmalı, yumuşak detune pad.', 'Trap soul, downtempo ve girişler.'],
    { u: [3, 14], o: [['t', 0, 0, .7], ['w', 0, 0, .25]], f: ['lowpass', 1300, .7, 1, 1.2, 1], vib: [.5, 16, 0], a: [.5, 1, .85, 1.4], rv: .5 });
  F('wf-pad-festival', 'Festival Saw', 'Pad', 'c', 'trance house',
    ['Seven-voice supersaw wall, wide and bright.', 'Trance, hardstyle and future bass breakdowns.'],
    ['Yedi sesli supersaw duvarı; geniş ve parlak.', 'Trance, hardstyle ve future bass breakdown\'ları.'],
    { u: [7, 30], o: [['w', 0, 0, .6], ['w', 0, 1, .25]], f: ['lowpass', 5200, .7, 1, .3, 1], a: [.12, .6, .9, .9], rv: .45 });
  F('wf-stab-future', 'Future Chord', 'Stab', 'c', 'house trance trap',
    ['Wide supersaw chord that decays quickly, made to pump.', 'Future bass and future house chords.'],
    ['Hızlı sönen geniş supersaw akoru; pompalamak için.', 'Future bass ve future house akorları.'],
    { u: [7, 24], o: [['w', 0, 0, .6], ['q', 0, 1, .2]], f: ['lowpass', 3800, 1, 3, 0, .6], a: [.004, .7, .35, .3], rv: .3 });
  F('wf-stab-tape', 'Tape Chord', 'Stab', 'c', 'deep lofi dnb',
    ['Muffled chord hit thrown into a long delay.', 'Dub techno, liquid and lo-fi stabs.'],
    ['Uzun delay\'e gönderilen boğuk akor vuruşu.', 'Dub techno, liquid ve lo-fi vuruşları.'],
    { o: [['w', 0, 0, .6], ['t', 8, 0, .5]], f: ['lowpass', 850, 2, 2, 0, .25], a: [.004, .3, 0, .25], dl: .55, rv: .35 });

  // ---------- lead ve pluck
  F('wf-lead-whistle', 'Night Whistle', 'Lead', 'm', U + ' deep',
    ['Breathy sine whistle with late vibrato and a slide between notes.', 'Trap, R&B and slap house hooks.'],
    ['Geç gelen vibratolu, notalar arası kayan nefesli sinüs ıslık.', 'Trap, R&B ve slap house hook\'ları.'],
    { o: [['s', 0, 1, .9]], gl: .05, noi: .04, vib: [5.5, 18, .3], f: ['lowpass', 5000, .5, 0, 0, .3], a: [.05, .2, .9, .25], rv: .35, dl: .2 });
  F('wf-lead-screech', 'Hard Screech', 'Lead', 'm', 'techno trance',
    ['Folded saw with a screaming resonant filter.', 'Hardstyle, hardcore and psy leads.'],
    ['Çığlık atan rezonanslı filtreden geçen katlanmış testere.', 'Hardstyle, hardcore ve psy lead\'leri.'],
    { o: [['w', 0, 0, .8], ['q', 0, 1, .3]], fold: .55, f: ['bandpass', 1500, 6, 3, 0, .35], fd: .6, a: [.003, .3, .8, .12], dl: .2 });
  F('wf-lead-anthem', 'Anthem Saw', 'Lead', 'm', 'trance house',
    ['Seven-voice supersaw lead with a fast attack and wide stereo.', 'Uplifting trance and hardstyle melodies.'],
    ['Hızlı ataklı, geniş stereolu yedi sesli supersaw lead.', 'Uplifting trance ve hardstyle melodileri.'],
    { u: [7, 20], o: [['w', 0, 0, .7]], f: ['lowpass', 6500, .7, 1, 0, .4], a: [.006, .4, .8, .3], rv: .4, dl: .25 });
  F('wf-pluck-trapbell', 'Trap Bell', 'Pluck', 'cm', U + ' dnb',
    ['Glassy FM bell with a long ringing tail.', 'Dark trap and drill motifs.'],
    ['Uzun çınlayan kuyruklu, camsı FM çan.', 'Karanlık trap ve drill motifleri.'],
    { fm: [3.5, 1.2, .5, 0], o: [['s', 0, 0, .3]], f: ['lowpass', 8000, .5, 0, 0, .3], a: [.001, 1.4, 0, 1.1], rv: .5, dl: .3 });
  F('wf-pluck-pizz', 'Cold Pizz', 'Pluck', 'cm', U,
    ['Very short string pluck, dry and tight.', 'Drill and trap staccato lines.'],
    ['Çok kısa, kuru ve sıkı yaylı çekmesi.', 'Drill ve trap staccato hatları.'],
    { o: [['w', 0, 0, .7], ['t', 0, 0, .4]], f: ['lowpass', 900, 1.5, 5, 0, .07], a: [.001, .16, 0, .12], rv: .3 });
  F('wf-pluck-future', 'Future Pluck', 'Pluck', 'cm', 'house trance trap',
    ['Five-voice saw pluck, bright and wide.', 'Future house chops and melodic arps.'],
    ['Parlak ve geniş, beş sesli testere pluck.', 'Future house chop\'ları ve melodik arp\'lar.'],
    { u: [5, 16], o: [['w', 0, 0, .7]], f: ['lowpass', 1200, 2, 5, 0, .16], a: [.001, .3, 0, .22], rv: .3, dl: .3 });
  F('wf-metal-cowbell', 'Phonk Bell', 'Metal', 'cm', U,
    ['Two clashing squares through a band-pass: a tuned cowbell.', 'Phonk riffs. Play it high.'],
    ['Band geçirenden geçen iki çatışan kare: akortlu cowbell.', 'Phonk riff\'leri. Tiz çal.'],
    { o: [['q', 0, 0, .7], ['q', 680, 0, .6]], f: ['bandpass', 2200, 3, 0, 0, .2], kt: 1, fd: .4, a: [.001, .28, 0, .18], rv: .2 });
  F('wf-vox-chop', 'Vocal Chop', 'Vox', 'cm', 'house deep trap dnb',
    ['Short vowel-shaped saw: a pitched vocal slice.', 'Future bass and UK garage chops.'],
    ['Sesli harf biçimli kısa testere: perdeli vokal dilimi.', 'Future bass ve UK garage chop\'ları.'],
    { u: [3, 10], o: [['w', 0, 0, .7]], vow: 'e', gl: .03, a: [.006, .18, .3, .12], dl: .3, rv: .3, vol: .5 });
})();
