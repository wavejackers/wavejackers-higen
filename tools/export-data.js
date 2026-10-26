// Web sürümünün veri dosyalarını eklenti için tek bir JSON'a çevirir.
// Kullanım: node tools/export-data.js <web-kaynak-klasörü> Resources/hitgen-data.json
const fs = require('fs'), path = require('path');
const dir = process.argv[2], out = process.argv[3];
const files = ['core.js', 'p3b.js', 'p4w.js', 's2.js', 's3.js', 'h-styles.js', 'h-styles2.js', 'h-gen.js', 'h-synth.js', 'h-synth2.js', 'i18n-ui.js', 'i18n-styles.js', 'p3t.js'];
const src = files.map(f => fs.readFileSync(path.join(dir, f), 'utf8')).join('\n');
const data = new Function(src + `
  const wt = HG.WT.map(t => { const a = []; for (let n = 1; n <= 48; n++) a.push(+t.f(n).toFixed(6)); return a; });
  return {
    version: 2, notes: MB.NOTES, scales: MB.SCALES, scaleOrder: Object.keys(MB.SCALES), gm: MB.GM, ch: MB.CH, tname: MB.TNAME,
    cp: HG.CP, bp: HG.BP, hr: HG.HR, arp: HG.ARP, styles: HG.STYLES, groups: HG.GROUPS,
    kits: MB.KITS_LIST, sounds: MB.SOUNDS, trim: MB.TRIM, wt, en: HG.EN, sig: HG.SIG.source,
    langs: HG.LANGS, rtl: HG.RTL, i18n: HG.I18N, st: HG.ST, cats: MB.CATS
  };`)();
fs.writeFileSync(out, JSON.stringify(data));
console.log('wrote', out, (fs.statSync(out).size / 1024).toFixed(1) + ' KB', data.styles.length, 'styles', data.sounds.length, 'sounds');
