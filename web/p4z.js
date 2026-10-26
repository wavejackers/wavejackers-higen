// ---- ZIP (sıkıştırmasız)
(function () {
  const table = (() => {
    const t = new Uint32Array(256);
    for (let n = 0; n < 256; n++) {
      let c = n;
      for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1;
      t[n] = c >>> 0;
    }
    return t;
  })();
  const crc32 = bytes => {
    let c = 0xFFFFFFFF;
    for (let i = 0; i < bytes.length; i++) c = table[(c ^ bytes[i]) & 255] ^ (c >>> 8);
    return (c ^ 0xFFFFFFFF) >>> 0;
  };
  const le16 = n => [n & 255, (n >> 8) & 255];
  const le32 = n => [n & 255, (n >>> 8) & 255, (n >>> 16) & 255, (n >>> 24) & 255];
  MB.crc32 = crc32;
  MB.zip = function (files) {
    const d = new Date();
    const time = (d.getHours() << 11) | (d.getMinutes() << 5) | (d.getSeconds() >> 1);
    const date = ((d.getFullYear() - 1980) << 9) | ((d.getMonth() + 1) << 5) | d.getDate();
    const parts = [], central = [];
    let offset = 0;
    for (const f of files) {
      const name = Array.from(unescape(encodeURIComponent(f.name))).map(c => c.charCodeAt(0));
      const crc = crc32(f.data);
      const local = [0x50,0x4b,0x03,0x04].concat(le16(20), le16(0x0800), le16(0), le16(time), le16(date),
        le32(crc), le32(f.data.length), le32(f.data.length), le16(name.length), le16(0), name);
      parts.push(new Uint8Array(local), f.data);
      central.push([0x50,0x4b,0x01,0x02].concat(le16(20), le16(20), le16(0x0800), le16(0), le16(time), le16(date),
        le32(crc), le32(f.data.length), le32(f.data.length), le16(name.length), le16(0), le16(0),
        le16(0), le16(0), le32(0), le32(offset), name));
      offset += local.length + f.data.length;
    }
    let cdSize = 0;
    for (const c of central) { parts.push(new Uint8Array(c)); cdSize += c.length; }
    parts.push(new Uint8Array([0x50,0x4b,0x05,0x06].concat(le16(0), le16(0), le16(files.length), le16(files.length),
      le32(cdSize), le32(offset), le16(0))));
    let total = 0; parts.forEach(p => total += p.length);
    const out = new Uint8Array(total); let o = 0;
    parts.forEach(p => { out.set(p, o); o += p.length; });
    return out;
  };
})();
