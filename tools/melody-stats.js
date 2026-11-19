// Melodi kalitesi ölçümü (en üst ses): ölçü başına nota, aralık, adım/sıçrama oranı, ölçü tekrarı, akor sesi oranı.
// Kullanım: node tools/melody-stats.js web   (eşiklerin altında kalırsa çıkış kodu 1)
const fs=require('fs'),path=require('path'),dir=process.argv[2];
const files=['core.js','p3b.js','p4w.js','s2.js','s3.js','s4.js','h-styles.js','h-styles2.js','h-gen.js','h-synth.js','h-synth2.js','h-synth3.js','h-synth4.js','i18n-ui.js','i18n-styles.js','h-genres.js'];
const src=files.map(f=>fs.readFileSync(path.join(dir,f),'utf8')).join('\n');
const out=new Function(src+`
const rows=[];
for (const S of HG.STYLES) for (let seed=1;seed<=6;seed++) {
  const r=HG.generate({style:S.id,vi:0,seed,bars:8,key:S.key,orig:30,energy:60,human:20,sevenths:false,thick:70,bpm:S.bpmDef,swing:S.swing});
  const raw=r.tracks.melody; if(!raw.length){rows.push({id:S.id,mode:'empty'});continue;} const byS={}; raw.forEach(n=>{ if(!byS[n.s]||byS[n.s].n<n.n) byS[n.s]=n; }); const m=Object.values(byS).sort((a,b)=>a.s-b.s); const layers=raw.length/m.length;
  const p=m.map(n=>n.n), ints=p.slice(1).map((x,i)=>Math.abs(x-p[i]));
  const bars=[]; for(let b=0;b<r.bars;b++) bars.push(JSON.stringify(m.filter(n=>Math.floor(n.s/16)===b).map(n=>[n.s%16,n.n])));
  let rep=0; for(let b=1;b<bars.length;b++) if(bars.slice(0,b).includes(bars[b])) rep++;
  const dur=m.reduce((a,n)=>a+n.l,0)/m.length;
  const chordTones=m.filter(n=>{const c=r.chords[Math.floor(n.s/16)]; const pc=((n.n-c.pc)%12+12)%12; return pc===0||pc===c.third||pc===c.fifth;}).length/m.length;
  rows.push({id:S.id,mode:String(S.melody||S.mm||''),perbar:m.length/r.bars,rng:Math.max(...p)-Math.min(...p),lo:Math.min(...p),hi:Math.max(...p),dur,step:ints.filter(x=>x<=2).length/Math.max(1,ints.length),leap:ints.filter(x=>x>=5).length/Math.max(1,ints.length),rep:rep/(r.bars-1),uniq:new Set(p).size,ct:chordTones,vel:m.reduce((a,n)=>a+n.v,0)/m.length,layers});
}
return rows;`)();
const by={}; out.forEach(r=>{(by[r.id]=by[r.id]||[]).push(r)});
const mean=(l,k)=>(l.reduce((a,x)=>a+(x[k]||0),0)/l.length);
const all=Object.entries(by).map(([id,l])=>({id,perbar:mean(l,'perbar'),rng:mean(l,'rng'),lo:mean(l,'lo'),dur:mean(l,'dur'),step:mean(l,'step'),leap:mean(l,'leap'),rep:mean(l,'rep'),uniq:mean(l,'uniq'),ct:mean(l,'ct'),vel:mean(l,'vel'),layers:mean(l,'layers'),empty:l.filter(x=>x.mode==='empty').length}));
all.sort((a,b)=>a.perbar-b.perbar);
for(const r of all) console.log(r.id.padEnd(18),'n/bar',r.perbar.toFixed(1).padStart(4),'rng',r.rng.toFixed(0).padStart(3),'lo',r.lo.toFixed(0),'dur',r.dur.toFixed(1),'step',r.step.toFixed(2),'leap',r.leap.toFixed(2),'rep',r.rep.toFixed(2),'uniq',r.uniq.toFixed(1),'ct',r.ct.toFixed(2),'vel',r.vel.toFixed(0),'lay',r.layers.toFixed(1),'empty',r.empty);
const g=k=>(all.reduce((a,x)=>a+x[k],0)/all.length); console.log('ALL n/bar',g('perbar').toFixed(2),'rng',g('rng').toFixed(2),'step',g('step').toFixed(2),'leap',g('leap').toFixed(2),'rep',g('rep').toFixed(2),'uniq',g('uniq').toFixed(2),'ct',g('ct').toFixed(2));
const bad=[]; if(g('step')<.5) bad.push('step'); if(g('leap')>.3) bad.push('leap'); if(g('rng')>14) bad.push('range'); if(g('rep')<.5) bad.push('repetition');
all.forEach(r=>{ if(r.perbar<3) bad.push(r.id+' sparse '+r.perbar.toFixed(1)); });
if(bad.length){ console.log('MELODY QUALITY FAIL: '+bad.join(', ')); process.exit(1); } else console.log('melody quality: ok');
