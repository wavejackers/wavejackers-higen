// GenTest.cpp ile aynı listeyi web motoruyla üretir.
const fs = require('fs'), path = require('path'), dir = process.argv[2];
const files = ['core.js', 'p3b.js', 'p4w.js', 's2.js', 's3.js', 'h-styles.js', 'h-styles2.js', 'h-gen.js', 'h-synth.js', 'h-synth2.js', 'h-synth3.js', 'h-synth4.js'];
const src = files.map(f => fs.readFileSync(path.join(dir, f), 'utf8')).join('\n');
new Function(src + `
const fnv = b => { let h = 2166136261 >>> 0; for (const x of b) { h ^= x; h = Math.imul(h, 16777619) >>> 0; } return h >>> 0; };
const K = ['chords','bass','drums','melody'], out = [];
for (const S of HG.STYLES) for (let vi = 0; vi < 4; vi++) for (const seed of [1,7,42,999,54321]) for (const sev of [0,1]) for (const thick of [0,70,100]) {
  const P = { style: S.id, vi, seed, bars: 8, key: S.key, orig: 30, energy: 60, human: 20, sevenths: !!sev, thick, bpm: S.bpmDef, swing: S.swing };
  const r = HG.generate(P), a = HG.assemble(r);
  out.push([S.id, vi, seed, sev, thick, fnv(MB.midiFile(r, K)), fnv(MB.midiFile(r, K, true)), fnv(MB.midiFile(a, K))].join(' '));
}
const ALTS = [[3,0,0,0],[0,5,0,0],[0,0,11,0],[0,0,0,17],[3,5,11,17]];
for (const S of HG.STYLES) for (let vi = 0; vi < 4; vi++) ALTS.forEach((alt, ai) => {
  const P = { style: S.id, vi, seed: 7, bars: 8, key: S.key, orig: 30, energy: 60, human: 20, sevenths: false, thick: 70, bpm: S.bpmDef, swing: S.swing, alt };
  const r = HG.generate(P), a = HG.assemble(r);
  out.push([S.id, 'alt' + ai, vi, fnv(MB.midiFile(r, K)), fnv(MB.midiFile(a, K))].join(' '));
});
// Proje iskeleti (yapı x süre) ve uç durumlar
const str = s => fnv(Array.from(s, c => c.charCodeAt(0)));
const SKX = HG.SK_STRUCT.concat([['build', 'build', 'drop'], ['intro'], ['verse', 'drop', 'break', 'outro'], ['break', 'build', 'groove', 'build']]);
for (const S of HG.STYLES) for (let st = 0; st < SKX.length; st++) for (const sec of [60, 360, 480]) {
  const P = { style: S.id, vi: st % 4, seed: 7, bars: 8, key: S.key, orig: 30, energy: 60, human: 20, sevenths: false, thick: 70, bpm: S.bpmDef, swing: S.swing };
  const r = HG.generate(P), sk = HG.skeleton(r, SKX[st], sec);
  out.push([S.id, 'sk' + st, sec, sk.profile, sk.secs.map(x => x.bars).join(','), str(sk.cues.map(c => c.bar + ':' + c.len + ':' + c.type + ':' + c.sec).join(';')), fnv(MB.midiFile(sk.song, K))].join(' '));
}
process.stdout.write(out.join('\\n') + '\\n');`)();
