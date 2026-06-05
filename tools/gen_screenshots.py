#!/usr/bin/env python3
"""Generate screenshot.png and thumbnail.png for each Surge XT effect pedal.

Uses Chromium headless to render the icon.html with all resources inlined as
data URIs, then copies the outputs to both the source modgui directory and the
deployed MODEP bundle.
"""

import base64
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT    = Path('/home/patch/src/surge-single-effect-lv2')
MODGUI_ROOT  = REPO_ROOT / 'modgui'
DEPLOY_ROOT  = Path('/var/modep/lv2')
LATA_PNG     = DEPLOY_ROOT / '3BandEQ.lv2/modgui/knobs/lata/lata.png'

SCREENSHOT_W, SCREENSHOT_H = 261, 430
THUMBNAIL_W,  THUMBNAIL_H  = 38,  64

EFFECTS = [
    {'name': 'flanger', 'label': 'Surge XT Flanger',       'color': 'blue'},
    {'name': 'phaser',  'label': 'Surge XT Phaser',         'color': 'blue'},
    {'name': 'reverb1', 'label': 'Surge XT Reverb 1',       'color': 'blue'},
    {'name': 'delay',   'label': 'Surge XT Delay',          'color': 'blue'},
    {'name': 'rotary',  'label': 'Surge XT Rotary Speaker', 'color': 'blue'},
    {'name': 'chorus',  'label': 'Surge XT Chorus',         'color': 'blue',
     'deploy_dir': DEPLOY_ROOT / 'Surge XT Chorus.lv2' / 'modgui'},
]


def b64url(path: Path) -> str:
    mime = 'image/svg+xml' if path.suffix == '.svg' else 'image/png'
    data = base64.b64encode(path.read_bytes()).decode()
    return f'data:{mime};base64,{data}'


def strip_mustache_blocks(text: str) -> str:
    """Remove {{#section}}...{{/section}} blocks — they have no data to render."""
    return re.sub(r'\{\{#[^}]+\}\}.*?\{\{/[^}]+\}\}', '', text, flags=re.DOTALL)


def substitute_mustache(text: str, cns='', ns='', color='blue', label='', knob='lata') -> str:
    return (text
            .replace('{{{cns}}}', cns)
            .replace('{{{ns}}}', ns)
            .replace('{{color}}', color)
            .replace('{{label}}', label)
            .replace('{{knob}}', knob))


def inline_resources(text: str, modgui_dir: Path) -> str:
    """Replace /resources/... URL references with base64 data URIs."""
    replacements = {
        '/resources/pedals/boxy/blue.png':       modgui_dir / 'pedals/boxy/blue.png',
        '/resources/pedals/footswitch.png':       modgui_dir / 'pedals/footswitch.png',
        '/resources/knobs/lata/lata.png':         LATA_PNG,
        '/resources/knobs/boxy/aluminium.png':    modgui_dir / 'knobs/boxy/aluminium.png',
        '/resources/surge_logo.svg':              modgui_dir / 'surge_logo.svg',
    }
    for url_path, file_path in replacements.items():
        if file_path.exists():
            text = text.replace(url_path, b64url(file_path))
    return text


def build_html(name: str, label: str, color: str) -> str:
    modgui_dir = MODGUI_ROOT / f'surge-xt-{name}' / 'modgui'

    icon_html = (modgui_dir / 'icon.html').read_text()
    css       = (modgui_dir / 'stylesheet.css').read_text()

    icon_html = strip_mustache_blocks(icon_html)
    icon_html = substitute_mustache(icon_html, color=color, label=label)
    css       = substitute_mustache(css)

    icon_html = inline_resources(icon_html, modgui_dir)
    css       = inline_resources(css, modgui_dir)

    return f'''<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<style>
html, body {{ margin: 0; padding: 0; width: {SCREENSHOT_W}px; height: {SCREENSHOT_H}px;
              overflow: hidden; background: transparent; }}
{css}
</style>
</head>
<body>
{icon_html}
</body>
</html>'''


def generate(name: str, label: str, color: str, deploy_dir: Path = None):
    print(f'Generating {name}...', flush=True)

    html     = build_html(name, label, color)
    tmphtml  = Path(tempfile.mktemp(suffix='.html'))
    tmphtml.write_text(html)

    screenshot_tmp = Path(f'/tmp/surge-screenshot-{name}.png')
    thumbnail_tmp  = Path(f'/tmp/surge-thumbnail-{name}.png')

    # Chromium headless screenshot
    result = subprocess.run([
        'chromium-browser',
        '--headless',
        '--no-sandbox',
        '--disable-gpu',
        '--disable-software-rasterizer',
        '--hide-scrollbars',
        f'--window-size={SCREENSHOT_W},{SCREENSHOT_H}',
        f'--screenshot={screenshot_tmp}',
        f'file://{tmphtml}',
    ], capture_output=True, timeout=30)

    tmphtml.unlink(missing_ok=True)

    if not screenshot_tmp.exists():
        print(f'  ERROR: chromium screenshot failed for {name}', file=sys.stderr)
        if result.stderr:
            print(result.stderr.decode()[:500], file=sys.stderr)
        return

    # Thumbnail via Pillow
    from PIL import Image
    with Image.open(screenshot_tmp) as img:
        thumb = img.resize((THUMBNAIL_W, THUMBNAIL_H), Image.LANCZOS)
        thumb.save(thumbnail_tmp)

    # Copy to source modgui (writable) and deployed bundle (needs sudo)
    src_dir = MODGUI_ROOT / f'surge-xt-{name}' / 'modgui'
    if deploy_dir is None:
        deploy_dir = DEPLOY_ROOT / f'surge-xt-{name}.lv2' / 'modgui'

    shutil.copy(screenshot_tmp, src_dir / 'screenshot.png')
    shutil.copy(thumbnail_tmp,  src_dir / 'thumbnail.png')

    subprocess.run(['sudo', 'cp', str(screenshot_tmp), str(deploy_dir / 'screenshot.png')], check=True)
    subprocess.run(['sudo', 'cp', str(thumbnail_tmp),  str(deploy_dir / 'thumbnail.png')],  check=True)

    screenshot_tmp.unlink(missing_ok=True)
    thumbnail_tmp.unlink(missing_ok=True)

    print(f'  Done → {src_dir}/screenshot.png ({SCREENSHOT_W}×{SCREENSHOT_H})')


if __name__ == '__main__':
    names = sys.argv[1:] or [e['name'] for e in EFFECTS]
    for effect in EFFECTS:
        if effect['name'] in names:
            generate(**effect)

    subprocess.run(['sudo', 'systemctl', 'restart', 'modep-mod-ui'])
    print('MODEP restarted.')
