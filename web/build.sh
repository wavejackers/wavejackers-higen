#!/bin/bash
cd "$(dirname "$0")"
JS="core.js p3b.js p4w.js p4z.js s1.js s2.js s3.js h-styles.js h-styles2.js h-gen.js h-synth.js h-synth2.js i18n-ui.js i18n-styles.js p3t.js u3.js u4.js u5.js"
{ cat u1.html; echo '<body>'; cat u2.html; echo '<script>'; cat $JS; echo '</script>'; } > hitgen.html
wc -c hitgen.html
