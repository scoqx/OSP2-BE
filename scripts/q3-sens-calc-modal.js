// Quake 3 Sensitivity Calculator — cm/360 ↔ sensitivity ↔ DPI.
// Formula: cm/360 = 914.4 / (DPI × sensitivity × m_yaw).  (914.4 = 360 × 2.54)

(function() {
    'use strict';

    const DEFAULT_M_YAW = 0.022;
    const K_CM360 = 914.4; // 360 * 2.54
    var lastComputedFieldId = null;

    function getLanguage() {
        return window.location.pathname.indexOf('/ru/') >= 0 ? 'ru' : 'en';
    }

    function getQ3SensCalcToolHTML(lang) {
        const isRu = lang === 'ru';
        return `
<style>
  .q3sens-tool-root { color-scheme: dark; --black:#000; --white:#fff; --accent:#0a0; --border:rgba(255,255,255,0.25); }
  .q3sens-tool-root * { box-sizing: border-box; margin: 0; padding: 0; }
  .q3sens-tool-root { background: var(--black); color: var(--white); font-family: system-ui, sans-serif; }
  .q3sens-tool-app { width: 100%; max-width: 640px; border-radius: 18px; padding: 0 4px 16px; }
  .q3sens-tool-app h1 { font-size: 1.25rem; text-transform: uppercase; letter-spacing: .1em; margin-bottom: 4px; }
  .q3sens-tool-app h1 span { color: var(--accent); }
  .q3sens-tool-app .subtitle { font-size: .8rem; color: rgba(255,255,255,0.7); margin-bottom: 14px; }
  .q3sens-layout { display: flex; gap: 24px; flex-wrap: wrap; }
  .q3sens-main { flex: 1; min-width: 200px; }
  .q3sens-side { flex: 0 0 auto; min-width: 180px; }
  .q3sens-field { margin-bottom: 12px; }
  .q3sens-field label { display: block; font-size: .82rem; margin-bottom: 4px; color: rgba(255,255,255,0.85); }
  .q3sens-field input[type="number"] { width: 100%; max-width: 200px; padding: 6px 10px; background: rgba(255,255,255,0.08); border: 1px solid var(--border);
    border-radius: 6px; color: var(--white); font-size: .95rem; transition: outline .35s ease, box-shadow .35s ease; }
  .q3sens-field input:focus { outline: none; border-color: var(--accent); }
  .q3sens-field small { display: block; font-size: .72rem; color: rgba(255,255,255,0.5); margin-top: 2px; }
  .q3sens-side .q3sens-field input[type="number"] { max-width: 120px; }
  .q3sens-flash { outline: 2px solid var(--accent) !important; outline-offset: 1px; box-shadow: 0 0 14px rgba(0,170,0,0.55); }
  .q3sens-tool-app .tool-credits {
    margin-top: 12px;
    padding-top: 6px;
    border-top: 1px solid rgba(255,255,255,0.1);
    text-align: center;
    font-size: .7rem;
    color: rgba(255,255,255,0.5);
  }
</style>

<div class="q3sens-tool-app">
  <h1><span>Quake 3</span> Sensitivity Calculator</h1>
  <p class="subtitle">${isRu ? 'Укажите любые два значения — третье посчитается автоматически' : 'Enter any two values — the third is calculated automatically'}</p>

  <div class="q3sens-layout">
    <div class="q3sens-main">
      <div class="q3sens-field">
        <label>${isRu ? 'DPI/CPI мышки' : 'Mouse DPI/CPI'}</label>
        <input type="number" id="q3sensDpi" min="1" step="1" value="" placeholder="800">
      </div>
      <div class="q3sens-field">
        <label>sensitivity</label>
        <input type="number" id="q3sensSens" min="0" step="0.0001" value="" placeholder="2.5">
      </div>
      <div class="q3sens-field">
        <label>${isRu ? 'См на 360°' : 'cm per 360°'}</label>
        <input type="number" id="q3sensCm" min="0.001" step="0.001" value="" placeholder="25">
      </div>
    </div>
    <div class="q3sens-side">
      <div class="q3sens-field">
        <label>m_yaw</label>
        <input type="number" id="q3sensMyaw" step="0.001" value="0.022">
        <small>0.022 = default</small>
      </div>
    </div>
  </div>
  <div class="tool-credits">${isRu ? 'Сделано' : 'Created by'} <strong>diwoc</strong></div>
</div>
`;
    }

    function getValues(content) {
        const mYaw = parseFloat(content.querySelector('#q3sensMyaw').value) || DEFAULT_M_YAW;
        const dpiRaw = content.querySelector('#q3sensDpi').value.trim();
        const sensRaw = content.querySelector('#q3sensSens').value.trim();
        const cmRaw = content.querySelector('#q3sensCm').value.trim();
        const dpi = dpiRaw === '' ? NaN : parseFloat(dpiRaw);
        const sens = sensRaw === '' ? NaN : parseFloat(sensRaw);
        const cm = cmRaw === '' ? NaN : parseFloat(cmRaw);
        return { mYaw, dpi, sens, cm };
    }

    function setField(content, id, value) {
        const el = content.querySelector('#' + id);
        if (!el) return;
        if (el === document.activeElement) return;
        if (value === undefined || value === null || (typeof value === 'number' && (value <= 0 || !isFinite(value)))) {
            el.value = '';
            return;
        }
        if (id === 'q3sensCm') el.value = value.toFixed(3);
        else if (id === 'q3sensSens') el.value = String(Number(value.toFixed(4)));
        else if (id === 'q3sensDpi') el.value = String(Math.round(value));
        lastComputedFieldId = id;
        el.classList.remove('q3sens-flash');
        el.offsetHeight;
        el.classList.add('q3sens-flash');
        setTimeout(function() { el.classList.remove('q3sens-flash'); }, 550);
    }

    function updateResult(content, lastChangedId) {
        const { mYaw, dpi, sens, cm } = getValues(content);

        const hasDpi = typeof dpi === 'number' && dpi > 0 && isFinite(dpi);
        const hasSens = typeof sens === 'number' && sens > 0 && isFinite(sens);
        const hasCm = typeof cm === 'number' && cm > 0 && isFinite(cm);
        const filledCount = [hasDpi, hasSens, hasCm].filter(Boolean).length;

        if (filledCount < 2) return;

        // Изменили m_yaw при трёх заполненных — пересчитываем то, что считали последним
        if (lastChangedId === 'q3sensMyaw' && filledCount === 3) {
            var targetId = (lastComputedFieldId === 'q3sensSens' || lastComputedFieldId === 'q3sensCm' || lastComputedFieldId === 'q3sensDpi') ? lastComputedFieldId : 'q3sensSens';
            if (targetId === 'q3sensSens') setField(content, 'q3sensSens', K_CM360 / (dpi * mYaw * cm));
            else if (targetId === 'q3sensCm') setField(content, 'q3sensCm', K_CM360 / (dpi * sens * mYaw));
            else setField(content, 'q3sensDpi', K_CM360 / (sens * mYaw * cm));
            return;
        }

        // Три заполнены: пересчитываем то поле, которое было вычислено последним (логично при смене любого ввода)
        if (filledCount === 3 && lastChangedId) {
            if (lastChangedId === 'q3sensSens') {
                setField(content, 'q3sensCm', K_CM360 / (dpi * sens * mYaw));
                return;
            }
            if (lastChangedId === 'q3sensCm') {
                setField(content, 'q3sensSens', K_CM360 / (dpi * mYaw * cm));
                return;
            }
            if (lastChangedId === 'q3sensDpi') {
                // При смене DPI пересчитываем то, что считали последним (cm/360 или sensitivity или DPI), а не всегда sensitivity
                var targetId = (lastComputedFieldId === 'q3sensSens' || lastComputedFieldId === 'q3sensCm' || lastComputedFieldId === 'q3sensDpi') ? lastComputedFieldId : 'q3sensSens';
                if (targetId === 'q3sensSens') setField(content, 'q3sensSens', K_CM360 / (dpi * mYaw * cm));
                else if (targetId === 'q3sensCm') setField(content, 'q3sensCm', K_CM360 / (dpi * sens * mYaw));
                else setField(content, 'q3sensDpi', K_CM360 / (sens * mYaw * cm));
                return;
            }
        }

        // Два заполнены: считаем третье
        if (hasDpi && hasCm) {
            setField(content, 'q3sensSens', K_CM360 / (dpi * mYaw * cm));
            return;
        }
        if (hasDpi && hasSens && !hasCm) {
            setField(content, 'q3sensCm', K_CM360 / (dpi * sens * mYaw));
            return;
        }
        if (hasSens && hasCm && !hasDpi) {
            setField(content, 'q3sensDpi', K_CM360 / (sens * mYaw * cm));
        }
    }

    function bindQ3SensCalc() {
        const content = document.getElementById('q3SensCalcToolContent');
        if (!content) return;

        ['q3sensDpi', 'q3sensSens', 'q3sensCm', 'q3sensMyaw'].forEach(function(id) {
            const el = content.querySelector('#' + id);
            if (el) {
                el.addEventListener('input', function() { updateResult(content, this.id); });
            }
        });

        updateResult(content);
    }

    document.addEventListener('DOMContentLoaded', function() {
        const openBtn = document.getElementById('openQ3SensCalcModal');
        const modal = document.getElementById('q3SensCalcModal');
        const closeBtn = document.getElementById('closeQ3SensCalcModal');
        const contentEl = document.getElementById('q3SensCalcToolContent');

        if (!openBtn || !modal || !closeBtn || !contentEl) return;

        let loaded = false;
        function loadTool() {
            if (loaded) return;
            contentEl.className = 'q3sens-tool-root';
            contentEl.innerHTML = getQ3SensCalcToolHTML(getLanguage());
            loaded = true;
            bindQ3SensCalc();
        }

        function openModal() {
            loadTool();
            modal.classList.add('active');
            document.body.style.overflow = 'hidden';
            if (history.pushState) history.pushState(null, null, '#q3sens');
            else window.location.hash = '#q3sens';
        }

        function closeModal() {
            modal.classList.remove('active');
            document.body.style.overflow = '';
            if (history.pushState) history.pushState(null, null, window.location.pathname + window.location.search);
            else window.location.hash = '';
        }

        openBtn.addEventListener('click', function(e) {
            e.preventDefault();
            e.stopPropagation();
            openModal();
        });
        closeBtn.addEventListener('click', closeModal);

        modal.addEventListener('mousedown', function(e) {
            window._q3sensMouseDownBg = (e.target === modal);
        });
        modal.addEventListener('mouseup', function(e) {
            if (window._q3sensMouseDownBg && (e.target === modal || !modal.contains(e.target))) closeModal();
            window._q3sensMouseDownBg = false;
        });
        document.addEventListener('keydown', function(e) {
            if (e.key === 'Escape' && modal.classList.contains('active')) closeModal();
        });

        if (window.location.hash === '#q3sens') openModal();
        window.addEventListener('hashchange', function() {
            if (window.location.hash === '#q3sens') openModal();
            else if (modal.classList.contains('active')) closeModal();
        });
    });
})();
