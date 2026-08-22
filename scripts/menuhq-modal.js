// Menu Background Maker Modal — single menu background image -> pk3 (Quake accepts up to 4096×4096).
// No slicing; only resize if image exceeds 4096; optional aspect-ratio crop.

(function() {
    'use strict';

    const MAX_SIDE = 4096;  // Quake limit; scale down only if larger
    const TEX_DIR = 'textures/sfx';

    function getLanguage() {
        return window.location.pathname.includes('/ru/') ? 'ru' : 'en';
    }

    function getMenuHQToolHTML(lang) {
        const isRu = lang === 'ru';
        return `
<style>
  .menuhq-tool-root { color-scheme: dark; --black:#000; --white:#fff; --accent:#8A2BE2; }
  .menuhq-tool-root * { box-sizing: border-box; margin: 0; padding: 0; }
  .menuhq-tool-root { background: var(--black); color: var(--white); font-family: system-ui, sans-serif; }
  .menuhq-tool-app { width: 100%; max-width: 900px; border-radius: 18px; padding: 0 20px 16px; }
  .menuhq-tool-app h1 { font-size: 1.25rem; text-transform: uppercase; letter-spacing: .12em; margin-bottom: 4px; }
  .menuhq-tool-app h1 span { color: var(--accent); }
  .menuhq-tool-app .subtitle { font-size: .8rem; color: rgba(255,255,255,0.7); margin-bottom: 12px; }
  .menuhq-tool-app .layout {
    display: grid;
    grid-template-columns: minmax(0, 1.5fr) minmax(220px, 1fr);
    gap: 14px;
  }
  @media (max-width: 700px) {
    .menuhq-tool-app .layout {
      grid-template-columns: 1fr;
    }
  }
  .menuhq-tool-app .layout-preview {
    display: flex;
    flex-direction: column;
  }
  .menuhq-tool-app #menuhqDropzone {
    position: relative; min-height: 280px; border-radius: 16px;
    border: 1px dashed rgba(255,255,255,0.25); background: var(--black);
    display: flex; align-items: center; justify-content: center; overflow: hidden; cursor: pointer;
  }
  .menuhq-tool-app #menuhqDropzone.dragover { border-color: var(--accent); box-shadow: 0 0 0 1px var(--accent); }
  .menuhq-tool-app #menuhqPreviewCanvas { max-width: 100%; max-height: 100%; display: block; }
  .menuhq-tool-app .menuhq-drop-hint {
    position: absolute; inset: 0; display: flex; flex-direction: column; align-items: center; justify-content: center;
    gap: 4px; pointer-events: none; text-align: center; font-size: .82rem; color: rgba(255,255,255,0.7);
    background: radial-gradient(circle, rgba(0,0,0,0.4), rgba(0,0,0,0.9));
  }
  .menuhq-tool-app .menuhq-drop-hint strong { color: var(--white); font-size: .9rem; }
  .menuhq-tool-app .menuhq-controls {
    background: var(--black); border-radius: 16px; border: 1px solid rgba(255,255,255,0.25);
    padding: 12px; font-size: .85rem; display: flex; flex-direction: column; gap: 10px;
  }
  .menuhq-tool-app .menuhq-controls label { display: block; margin-bottom: 2px; font-size: .8rem; }
  .menuhq-tool-app .menuhq-mode-group { display: flex; gap: 8px; margin-top: 2px; }
  .menuhq-tool-app .menuhq-mode-group label { display: flex; align-items: center; gap: 4px; margin: 0; }
  .menuhq-tool-app .menuhq-mode-group input[type=radio] { accent-color: var(--accent); }
  .menuhq-tool-app .menuhq-controls small { font-size: .72rem; color: rgba(255,255,255,0.6); }
  .menuhq-tool-app .menuhq-note { font-size: .72rem; color: rgba(255,255,255,0.5); margin-left: 4px; }
  .menuhq-tool-app .menuhq-preview-header {
    display: flex;
    justify-content: stretch;
    align-items: center;
    margin-bottom: 6px;
    /* Фиксированная высота, чтобы превью не прыгало
       при появлении/скрытии кнопки загрузки */
    min-height: 30px;
  }
  .menuhq-tool-app .menuhq-upload-btn {
    padding: 4px 10px;
    border-radius: 999px;
    border: 1px solid rgba(255,255,255,0.4);
    background: rgba(255,255,255,0.04);
    color: var(--white);
    font-size: .8rem;
    cursor: pointer;
    width: 100%;
    display: none;
  }
  .menuhq-tool-app .menuhq-upload-btn.visible {
    display: block;
  }
  .menuhq-tool-app .menuhq-upload-btn:hover {
    border-color: var(--accent);
    background: rgba(138,43,226,0.25);
  }
  .menuhq-tool-app .menuhq-aspect-btn {
    padding: 4px 10px;
    border-radius: 999px;
    border: 1px solid rgba(255,255,255,0.3);
    background: transparent;
    color: var(--white);
    font-size: .8rem;
    cursor: pointer;
    min-width: 52px;
    text-align: center;
  }
  .menuhq-tool-app .menuhq-aspect-btn:hover {
    border-color: var(--accent);
    background: rgba(138,43,226,0.15);
  }
  .menuhq-tool-app .menuhq-aspect-btn.active {
    border-color: var(--accent);
    background: rgba(138,43,226,0.35);
  }
  .menuhq-tool-app #menuhqBuildBtn {
    border-radius: 999px; border: 1px solid var(--accent); background: #000; color: #fff;
    padding: 8px 14px; font-size: .8rem; cursor: pointer; text-transform: uppercase; letter-spacing: .08em;
  }
  .menuhq-tool-app #menuhqBuildBtn:hover:not(:disabled) { background: var(--accent); }
  .menuhq-tool-app #menuhqBuildBtn:disabled { opacity: .45; cursor: not-allowed; }
  .menuhq-tool-app #menuhqStatus { margin-top: 8px; font-size: .78rem; color: rgba(255,255,255,0.75); white-space: pre-wrap; }
  .menuhq-tool-app .menuhq-title-row { display: flex; align-items: baseline; gap: 12px; flex-wrap: wrap; margin-bottom: 4px; }
  .menuhq-tool-app .tool-credits {
    margin-top: 8px;
    padding-top: 6px;
    border-top: 1px solid rgba(255,255,255,0.1);
    text-align: center;
    font-size: .7rem;
    color: rgba(255,255,255,0.5);
  }
</style>

<div class="menuhq-tool-app">
  <div class="menuhq-title-row">
    <h1><span>Menu</span> Background Maker</h1>
  </div>
  <p class="subtitle">
    ${isRu ? 'Готовый пак с фоном меню (одна текстура до 4096×4096, сжатие при необходимости)' : 'Ready pack with menu background (single texture up to 4096×4096, resize when needed)'}
  </p>

  <div class="layout">
    <div class="layout-preview">
      <div class="menuhq-preview-header">
        <button id="menuhqUploadBtn" type="button" class="menuhq-upload-btn">
          ${isRu ? 'Загрузить новую' : 'Upload new'}
        </button>
      </div>
      <div id="menuhqDropzone" tabindex="0">
        <canvas id="menuhqPreviewCanvas"></canvas>
        <div class="menuhq-drop-hint" id="menuhqHint">
          <strong>${isRu ? 'Клик / Перетащить / Ctrl+V' : 'Click / Drop / Ctrl+V'}</strong>
          <div>${isRu ? 'Поддерживаются: JPG, PNG, TGA, WebP.' : 'Supported: JPG, PNG, TGA, WebP.'}</div>
        </div>
      </div>
      <div id="menuhqStatus" class="menuhq-status"></div>
    </div>
    <div class="menuhq-controls">
      <div>
        <label>${isRu ? 'Изображение' : 'Image'} <span class="menuhq-note">(ui_menuBgStyle)</span></label>
        <div class="menuhq-mode-group">
          <label><input type="radio" name="menuhqImageType" value="logo" checked> Logo</label>
          <label><input type="radio" name="menuhqImageType" value="nologo"> Nologo</label>
        </div>
        <small>${isRu
          ? 'Logo — шейдер menuback (главная страница)<br>Nologo — шейдер menubacknologo (остальные)'
          : 'Logo — shader menuback (main menu)<br>Nologo — shader menubacknologo (other pages)'}</small>
      </div>
      <div>
        <label>${isRu ? 'Соотношение сторон (необязательно)' : 'Aspect ratio (optional)'}</label>
        <div class="menuhq-mode-group">
          <button type="button" class="menuhq-aspect-btn" data-aspect="16:9">16:9</button>
          <button type="button" class="menuhq-aspect-btn" data-aspect="16:10">16:10</button>
          <button type="button" class="menuhq-aspect-btn" data-aspect="4:3">4:3</button>
          <button type="button" class="menuhq-aspect-btn" data-aspect="5:4">5:4</button>
        </div>
        <small>${isRu
          ? 'Позволяет обрезать кадр до выбранных пропорций перед обработкой.'
          : 'Crops the frame to the chosen aspect ratio before processing.'}</small>
      </div>
      <div>
        <button id="menuhqBuildBtn" type="button" disabled>${isRu ? 'Создать pk3' : 'Build pk3'}</button>
        <small style="display:block; margin-top:4px;">${isRu ? 'Создаёт zz-menuHQ-{имя}.pk3 (текстура + шейдер)' : 'Creates zz-menuHQ-{name}.pk3 (texture + shader)'}</small>
      </div>
    </div>
  </div>
  <div class="tool-credits">${isRu ? 'Сделано' : 'Created by'} <strong>diwoc</strong></div>
</div>

<canvas id="menuhqWorkCanvas" style="display:none"></canvas>
`;
    }

    const menuhqToolJS = `
(function() {
  const MAX_SIDE = ${MAX_SIDE};
  const PREVIEW_MAX_SIDE = 2048;
  const TEX_DIR = '${TEX_DIR}';

  function getLanguage() {
    return window.location.pathname.indexOf('/ru/') >= 0 ? 'ru' : 'en';
  }

  const dropzone = document.getElementById('menuhqDropzone');
  const hint = document.getElementById('menuhqHint');
  const previewCanvas = document.getElementById('menuhqPreviewCanvas');
  const previewCtx = previewCanvas.getContext('2d');
  const workCanvas = document.getElementById('menuhqWorkCanvas');
  const workCtx = workCanvas.getContext('2d');
  const buildBtn = document.getElementById('menuhqBuildBtn');
  const statusEl = document.getElementById('menuhqStatus');
  const previewWorkCanvas = document.createElement('canvas');
  const previewWorkCtx = previewWorkCanvas.getContext('2d');
  const uploadBtn = document.getElementById('menuhqUploadBtn');
  const aspectButtons = Array.from(document.querySelectorAll('.menuhq-aspect-btn'));

  const fileInput = document.createElement('input');
  fileInput.type = 'file';
  fileInput.accept = 'image/jpeg,image/png,image/webp,.jpg,.jpeg,.png,.tga,.webp';

  let currentImage = null;
  let currentFileName = '';
  let currentFileExt = '.png';

  let currentAspectKey = null; // e.g. "16:9"
  let cropRect = null; // { x, y, width, height } in normalized [0..1] coords
  let isDraggingCrop = false;
  let dragStartX = 0;
  let dragStartY = 0;
  let dragStartRectX = 0;
  let dragStartRectY = 0;

  function getImageType() {
    const r = document.querySelector('input[name="menuhqImageType"]:checked');
    return r ? r.value : 'logo';
  }

  function updateAspectButtonsUI() {
    aspectButtons.forEach(btn => {
      const key = btn.getAttribute('data-aspect');
      if (key && key === currentAspectKey) {
        btn.classList.add('active');
      } else {
        btn.classList.remove('active');
      }
    });
  }

  function recomputeCropRectForCurrentImage() {
    if (!currentImage || !currentAspectKey) {
      cropRect = null;
      return;
    }
    const parts = currentAspectKey.split(':');
    const rw = parseFloat(parts[0]);
    const rh = parseFloat(parts[1]);
    if (!rw || !rh) {
      cropRect = null;
      return;
    }
    const imgW = currentImage.w;
    const imgH = currentImage.h;
    const targetRatio = rw / rh;
    const imgRatio = imgW / imgH;

    let widthNorm;
    let heightNorm;
    let xNorm;
    let yNorm;

    if (imgRatio > targetRatio) {
      // Crop left/right
      heightNorm = 1;
      const cropWidthPx = imgH * targetRatio;
      widthNorm = cropWidthPx / imgW;
      xNorm = (1 - widthNorm) / 2;
      yNorm = 0;
    } else {
      // Crop top/bottom
      widthNorm = 1;
      const cropHeightPx = imgW / targetRatio;
      heightNorm = cropHeightPx / imgH;
      xNorm = 0;
      yNorm = (1 - heightNorm) / 2;
    }

    cropRect = {
      x: xNorm,
      y: yNorm,
      width: widthNorm,
      height: heightNorm
    };
  }

  function getExt(path) {
    const i = path.lastIndexOf('.');
    if (i < 0) return '';
    return path.slice(i).toLowerCase();
  }

  function getOutputFormat(ext) {
    if (ext === '.jpg' || ext === '.jpeg') return { mime: 'image/jpeg', ext: '.jpg', quality: 1 };
    if (ext === '.png') return { mime: 'image/png', ext: '.png', quality: 1 };
    if (ext === '.webp') return { mime: 'image/webp', ext: '.webp', quality: 1 };
    return { mime: 'image/png', ext: '.png', quality: 1 };
  }

  function baseNameNoExt(path) {
    const slash = Math.max(path.lastIndexOf('/'), path.lastIndexOf('\\\\'));
    const base = slash >= 0 ? path.slice(slash + 1) : path;
    const dot = base.lastIndexOf('.');
    return dot >= 0 ? base.slice(0, dot) : base;
  }

  if (uploadBtn) {
    uploadBtn.addEventListener('click', () => {
      fileInput.value = '';
      fileInput.click();
    });
  }

  aspectButtons.forEach(btn => {
    btn.addEventListener('click', () => {
      const key = btn.getAttribute('data-aspect');
      if (!key) return;
      if (currentAspectKey === key) {
        // Toggle off
        currentAspectKey = null;
        cropRect = null;
        updateAspectButtonsUI();
        drawPreview();
        return;
      }
      currentAspectKey = key;
      if (currentImage) {
        recomputeCropRectForCurrentImage();
      } else {
        cropRect = null;
      }
      updateAspectButtonsUI();
      drawPreview();
    });
  });

  fileInput.addEventListener('change', () => {
    const f = fileInput.files[0];
    if (f) loadImage(f);
  });

  dropzone.addEventListener('click', () => {
    // После загрузки изображения клик по превью больше не открывает диалог,
    // чтобы не мешать работе с рамкой обрезки.
    if (currentImage) return;
    fileInput.value = '';
    fileInput.click();
  });

  dropzone.addEventListener('dragover', e => {
    e.preventDefault();
    dropzone.classList.add('dragover');
  });
  dropzone.addEventListener('dragleave', () => dropzone.classList.remove('dragover'));
  dropzone.addEventListener('drop', e => {
    e.preventDefault();
    dropzone.classList.remove('dragover');
    const file = e.dataTransfer.files[0];
    if (file) loadImage(file);
  });

  const menuhqModal = document.getElementById('menuhqModal');
  if (menuhqModal) {
    menuhqModal.addEventListener('paste', e => {
      if (!menuhqModal.classList.contains('active')) return;
      const items = e.clipboardData && e.clipboardData.items;
      if (!items) return;
      for (const item of items) {
        if (item.type && item.type.startsWith('image/')) {
          const file = item.getAsFile();
          if (file) {
            loadImage(file);
            e.preventDefault();
            break;
          }
        }
      }
    });
  }

  function loadImage(file) {
    const lang = getLanguage();
    const url = URL.createObjectURL(file);
    const img = new Image();
    img.onload = () => {
      URL.revokeObjectURL(url);
      currentFileName = file.name || 'image';
      currentFileExt = getExt(currentFileName) || '.png';
      const fmt = getOutputFormat(currentFileExt);
      if (fmt.ext !== currentFileExt) currentFileExt = fmt.ext;
      workCanvas.width = img.width;
      workCanvas.height = img.height;
      workCtx.clearRect(0, 0, img.width, img.height);
      workCtx.drawImage(img, 0, 0);
      currentImage = { w: img.width, h: img.height };
      if (currentAspectKey) {
        recomputeCropRectForCurrentImage();
      } else {
        cropRect = null;
      }
      drawPreview();
      if (uploadBtn) {
        uploadBtn.classList.add('visible');
      }
      statusEl.textContent = (lang === 'ru' ? 'Загружено ' : 'Loaded ') + img.width + '×' + img.height + (lang === 'ru' ? '.' : '.');
      buildBtn.disabled = false;
      hint.style.display = 'none';
      if (window.ChangeTracker) window.ChangeTracker.markChanges('menuhq');
    };
    img.onerror = () => {
      URL.revokeObjectURL(url);
      statusEl.textContent = lang === 'ru' ? 'Ошибка загрузки изображения.' : 'Failed to load image.';
    };
    img.src = url;
  }

  function drawPreview() {
    const w = workCanvas.width;
    const h = workCanvas.height;
    const bounds = dropzone.getBoundingClientRect();
    const maxW = bounds.width - 12;
    const maxH = bounds.height - 12;
    let sourceCanvas = workCanvas;
    let sw = w, sh = h;
    if (w > PREVIEW_MAX_SIDE || h > PREVIEW_MAX_SIDE) {
      const scale = PREVIEW_MAX_SIDE / Math.max(w, h);
      sw = Math.max(1, Math.round(w * scale));
      sh = Math.max(1, Math.round(h * scale));
      previewWorkCanvas.width = sw;
      previewWorkCanvas.height = sh;
      previewWorkCtx.clearRect(0, 0, sw, sh);
      previewWorkCtx.drawImage(workCanvas, 0, 0, w, h, 0, 0, sw, sh);
      sourceCanvas = previewWorkCanvas;
    }
    const scale = Math.min(maxW / sw, maxH / sh, 1);
    previewCanvas.width = Math.round(sw * scale);
    previewCanvas.height = Math.round(sh * scale);
    previewCtx.clearRect(0, 0, previewCanvas.width, previewCanvas.height);
    previewCtx.drawImage(sourceCanvas, 0, 0, sw, sh, 0, 0, previewCanvas.width, previewCanvas.height);

    // Draw crop frame overlay (if any)
    if (cropRect && currentAspectKey) {
      const rx = cropRect.x * previewCanvas.width;
      const ry = cropRect.y * previewCanvas.height;
      const rw = cropRect.width * previewCanvas.width;
      const rh = cropRect.height * previewCanvas.height;

      // Dim outside area slightly
      previewCtx.save();
      previewCtx.fillStyle = 'rgba(0, 0, 0, 0.4)';
      previewCtx.beginPath();
      previewCtx.rect(0, 0, previewCanvas.width, previewCanvas.height);
      previewCtx.rect(rx, ry, rw, rh);
      previewCtx.fill('evenodd');

      // Frame
      previewCtx.strokeStyle = 'rgba(255, 255, 255, 0.9)';
      previewCtx.lineWidth = 2;
      previewCtx.setLineDash([6, 4]);
      previewCtx.strokeRect(rx + 1, ry + 1, rw - 2, rh - 2);
      previewCtx.restore();
    }
  }

  window.addEventListener('resize', () => {
    if (currentImage) drawPreview();
  });

  previewCanvas.addEventListener('mousedown', e => {
    if (!cropRect || !currentAspectKey) return;
    e.stopPropagation();
    const rect = previewCanvas.getBoundingClientRect();
    const px = e.clientX - rect.left;
    const py = e.clientY - rect.top;
    const rx = cropRect.x * previewCanvas.width;
    const ry = cropRect.y * previewCanvas.height;
    const rw = cropRect.width * previewCanvas.width;
    const rh = cropRect.height * previewCanvas.height;

    if (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh) {
      isDraggingCrop = true;
      dragStartX = px;
      dragStartY = py;
      dragStartRectX = cropRect.x;
      dragStartRectY = cropRect.y;
      e.preventDefault();
    }
  });

  previewCanvas.addEventListener('click', e => {
    // До загрузки изображения клики должны доходить до dropzone,
    // после загрузки — не мешать работе рамки/обрезки.
    if (!currentImage) return;
    e.stopPropagation();
  });

  window.addEventListener('mousemove', e => {
    if (!isDraggingCrop || !cropRect || !currentAspectKey) return;
    const rect = previewCanvas.getBoundingClientRect();
    const px = e.clientX - rect.left;
    const py = e.clientY - rect.top;

    const dxNorm = (px - dragStartX) / previewCanvas.width;
    const dyNorm = (py - dragStartY) / previewCanvas.height;

    let newX = dragStartRectX + dxNorm;
    let newY = dragStartRectY + dyNorm;

    // Clamp within [0, 1 - width/height]
    newX = Math.min(Math.max(newX, 0), 1 - cropRect.width);
    newY = Math.min(Math.max(newY, 0), 1 - cropRect.height);

    cropRect.x = newX;
    cropRect.y = newY;
    drawPreview();
  });

  window.addEventListener('mouseup', () => {
    isDraggingCrop = false;
  });

  previewCanvas.addEventListener('mouseleave', () => {
    isDraggingCrop = false;
  });

  function canvasPartToBlob(sourceCanvas, sx, sy, sw, sh, mime, quality) {
    const c = document.createElement('canvas');
    c.width = sw;
    c.height = sh;
    const ctx = c.getContext('2d');
    ctx.drawImage(sourceCanvas, sx, sy, sw, sh, 0, 0, sw, sh);
    return new Promise(resolve => {
      c.toBlob(blob => resolve(blob), mime || 'image/png', quality);
    });
  }

  function buildShader(ext, imageType) {
    const shaderName = imageType === 'nologo' ? 'menubacknologo' : 'menuback';
    const textureName = shaderName;
    const s = shaderName + '\\n{\\n\\tnopicmip\\n\\tnomipmaps\\n         {\\n                map ' + TEX_DIR + '/' + textureName + ext + '\\n                rgbgen identity\\n        }        \\n}\\n';
    return s.replace(/\\\\n/g, String.fromCharCode(10));
  }

  function getBuildCanvas() {
    let w = workCanvas.width;
    let h = workCanvas.height;
    if (w > MAX_SIDE || h > MAX_SIDE) {
      const scale = MAX_SIDE / Math.max(w, h);
      w = Math.max(1, Math.round(w * scale));
      h = Math.max(1, Math.round(h * scale));
    }
    if (w === workCanvas.width && h === workCanvas.height) {
      return workCanvas;
    }
    const c = document.createElement('canvas');
    c.width = w;
    c.height = h;
    const ctx = c.getContext('2d');
    ctx.drawImage(workCanvas, 0, 0, workCanvas.width, workCanvas.height, 0, 0, w, h);
    return c;
  }

  buildBtn.addEventListener('click', async () => {
    if (!currentImage || typeof JSZip === 'undefined') {
      statusEl.textContent = getLanguage() === 'ru' ? 'Ошибка: загрузите изображение.' : 'Error: load an image.';
      return;
    }
    const imageType = getImageType();
    const baseCanvas = getBuildCanvas();

    // Apply cropping (if any) in normalized coordinates
    let buildCanvas = baseCanvas;
    if (cropRect && currentAspectKey) {
      const sx = Math.round(cropRect.x * baseCanvas.width);
      const sy = Math.round(cropRect.y * baseCanvas.height);
      const sw = Math.round(cropRect.width * baseCanvas.width);
      const sh = Math.round(cropRect.height * baseCanvas.height);
      const cropped = document.createElement('canvas');
      cropped.width = sw;
      cropped.height = sh;
      const cctx = cropped.getContext('2d');
      cctx.drawImage(baseCanvas, sx, sy, sw, sh, 0, 0, sw, sh);
      buildCanvas = cropped;
    }

    const fmt = getOutputFormat(currentFileExt);
    const ext = fmt.ext;
    const shaderName = imageType === 'nologo' ? 'menubacknologo' : 'menuback';
    const base = baseNameNoExt(currentFileName) || 'image';
    const pk3Name = 'zz-menuHQ-' + base + '.pk3';
    const lang = getLanguage();
    statusEl.textContent = (lang === 'ru' ? 'Создание pk3…' : 'Building pk3…');

    const zip = new JSZip();

    const blob = await canvasPartToBlob(buildCanvas, 0, 0, buildCanvas.width, buildCanvas.height, fmt.mime, fmt.quality);
    zip.file(TEX_DIR + '/' + shaderName + ext, blob, { compression: 'DEFLATE', compressionOptions: { level: 9 } });
    const shader = buildShader(ext, imageType);
    zip.file('scripts/' + shaderName + '.shader', shader, { compression: 'DEFLATE', compressionOptions: { level: 9 } });

    try {
      const zipBlob = await zip.generateAsync({ type: 'blob', compression: 'DEFLATE', compressionOptions: { level: 9 } });
      const url = URL.createObjectURL(zipBlob);
      const a = document.createElement('a');
      a.href = url;
      a.download = pk3Name;
      document.body.appendChild(a);
      a.click();
      a.remove();
      setTimeout(() => URL.revokeObjectURL(url), 5000);
      statusEl.textContent = (lang === 'ru' ? 'Готово: ' : 'Done: ') + pk3Name;
      if (window.ChangeTracker) window.ChangeTracker.markCompleted('menuhq');
    } catch (e) {
      console.error(e);
      statusEl.textContent = (lang === 'ru' ? 'Ошибка: ' : 'Error: ') + e.message;
    }
  });
})();
`;

    document.addEventListener('DOMContentLoaded', function() {
        const openBtn = document.getElementById('openMenuHQModal');
        const modal = document.getElementById('menuhqModal');
        const closeBtn = document.getElementById('closeMenuHQModal');
        const content = document.getElementById('menuhqToolContent');

        if (!openBtn || !modal || !closeBtn || !content) return;

        let isContentLoaded = false;

        function loadTool() {
            if (isContentLoaded) return;
            content.className = 'menuhq-tool-root';
            content.innerHTML = getMenuHQToolHTML(getLanguage());
            const script = document.createElement('script');
            script.textContent = menuhqToolJS;
            content.appendChild(script);
            isContentLoaded = true;
        }

        function openModal() {
            loadTool();
            modal.classList.add('active');
            document.body.style.overflow = 'hidden';
            if (history.pushState) {
                history.pushState(null, null, '#menuhq');
            } else {
                window.location.hash = '#menuhq';
            }
            if (window.ChangeTracker) window.ChangeTracker.resetChanges('menuhq');
            setTimeout(() => {
                const dz = document.getElementById('menuhqDropzone');
                if (dz) dz.focus();
            }, 100);
        }

        function closeModal() {
            modal.classList.remove('active');
            document.body.style.overflow = '';
            if (history.pushState) {
                history.pushState(null, null, window.location.pathname + window.location.search);
            } else {
                window.location.hash = '';
            }
            if (window.ChangeTracker) window.ChangeTracker.resetChanges('menuhq');
        }

        openBtn.addEventListener('click', function(e) {
            e.preventDefault();
            e.stopPropagation();
            openModal();
        });

        closeBtn.addEventListener('click', closeModal);

        let mouseDownOnBg = false;
        modal.addEventListener('mousedown', function(e) {
            mouseDownOnBg = (e.target === modal);
        });
        const handleMouseUp = function(e) {
            if (mouseDownOnBg && (e.target === modal || !modal.contains(e.target))) {
                closeModal();
            }
            mouseDownOnBg = false;
        };
        modal.addEventListener('mouseup', handleMouseUp);
        document.addEventListener('mouseup', handleMouseUp);

        document.addEventListener('keydown', function(e) {
            if (e.key === 'Escape' && modal.classList.contains('active')) {
                e.preventDefault();
                if (window.ChangeTracker) {
                    window.ChangeTracker.handleEscPress('menuhq', closeModal);
                } else {
                    closeModal();
                }
            }
        });

        if (window.location.hash === '#menuhq') openModal();
        window.addEventListener('hashchange', function() {
            if (window.location.hash === '#menuhq') openModal();
            else if (modal.classList.contains('active')) closeModal();
        });
    });
})();
