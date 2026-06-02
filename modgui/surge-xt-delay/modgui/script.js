function (event) {

    var ratioNames = ['1/16', '1/16.', '1/8', '1/8.', '1/4', '1/4.', '1/2', '1/2.', '1'];
    var FRAMES  = 65;
    var KNOB_W  = 48;
    var MAX_POS = KNOB_W * (FRAMES - 1);  // 3072px

    function bgToNorm(el) {
        var bp = window.getComputedStyle(el).backgroundPosition || '0px 0px';
        var x = parseFloat(bp) || 0;
        return Math.max(0, Math.min(1, -x / MAX_POS));
    }

    function updateBPM() {
        var el = event.icon.find('#knob-bpm .mod-knob-image')[0];
        if (!el) return;
        event.icon.find('.bpm-display').text(Math.round(40 + bgToNorm(el) * 120));
    }

    function updateRatio() {
        var el = event.icon.find('#knob-ratio .mod-knob-image')[0];
        if (!el) return;
        var idx = Math.min(8, Math.round(bgToNorm(el) * 8));
        event.icon.find('.ratio-display').text(ratioNames[idx]);
    }

    function observe(id, updateFn) {
        var el = event.icon.find('#' + id + ' .mod-knob-image')[0];
        if (!el) return;
        new MutationObserver(updateFn).observe(el, {
            attributes: true, attributeFilter: ['style']
        });
    }

    if (event.type === 'start') {
        setTimeout(function () {
            updateBPM();
            updateRatio();
            observe('knob-bpm',   updateBPM);
            observe('knob-ratio', updateRatio);
        }, 500);
    }
}
