// Q3 Skybox Tool Modal functionality
(function() {
    'use strict';
    
    // Determine language from URL
    function getLanguage() {
        return window.location.pathname.includes('/ru/') ? 'ru' : 'en';
    }
    
    // Skybox tool HTML content (adapted from Skybox.html)
    function getSkyboxToolHTML(lang) {
        const isRu = lang === 'ru';
        
        return `
<style>
  .skybox-tool-root{
    color-scheme: dark;
    --black:#000000;
    --white:#FFFFFF;
    --red:#FF0000;
  }
  .skybox-tool-root *{box-sizing:border-box;margin:0;padding:0}
  .skybox-tool-root{
    background:var(--black);
    color:var(--white);
    font-family:system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;
  }
  .skybox-tool-app{
    width:100%;
    max-width:1180px;
    background:var(--black);
    border-radius:18px;
    padding:0px 20px 16px;
  }
  .skybox-tool-app h1{
    font-size:1.25rem;
    text-transform:uppercase;
    letter-spacing:.12em;
    margin-top:0;
    margin-bottom:4px;
  }
  .skybox-tool-app h1 span{color:var(--red);}
  .skybox-tool-app .subtitle{
    font-size:.8rem;
    color:rgba(255,255,255,0.7);
    margin-bottom:10px;
  }
  .skybox-tool-app .subtitle strong{color:var(--white);}
  .skybox-tool-app .subtitle code{
    background:rgba(255,255,255,0.05);
    padding:1px 5px;
    border-radius:4px;
    border:1px solid rgba(255,255,255,0.25);
  }

  .skybox-tool-app .layout{
    display:grid;
    grid-template-columns:minmax(0,2.1fr) minmax(260px,1.2fr);
    gap:14px;
  }
  @media(max-width:900px){
    .skybox-tool-app .layout{grid-template-columns:1fr}
  }

  /* dropzone */
  .skybox-tool-app #dropzone{
    position:relative;
    min-height:400px;
    border-radius:16px;
    border:1px dashed rgba(255,255,255,0.25);
    background:var(--black);
    display:flex;
    align-items:center;
    justify-content:center;
    overflow:hidden;
    cursor:pointer;
    transition:border-color .2s,box-shadow .2s,background .2s;
  }
  .skybox-tool-app #dropzone.dragover{
    border-color:var(--red);
    box-shadow:0 0 0 1px var(--red);
    background:#000000;
  }
  .skybox-tool-app #previewCanvas{
    max-width:100%;
    max-height:100%;
    display:block;
  }
  .skybox-tool-app .drop-hint{
    position:absolute;
    inset:0;
    display:flex;
    flex-direction:column;
    align-items:center;
    justify-content:center;
    gap:4px;
    pointer-events:none;
    text-align:center;
    font-size:.82rem;
    color:rgba(255,255,255,0.7);
    background:radial-gradient(circle at center,rgba(0,0,0,0.4),rgba(0,0,0,0.9));
  }
  .skybox-tool-app .drop-hint strong{
    color:var(--white);
    font-size:.9rem;
  }
  .skybox-tool-app .chip{
    margin-top:4px;
    display:inline-flex;
    align-items:center;
    gap:5px;
    padding:2px 8px;
    border-radius:999px;
    background:rgba(0,0,0,0.7);
    border:1px solid rgba(255,255,255,0.25);
    font-size:.7rem;
    color:rgba(255,255,255,0.8);
  }
  .skybox-tool-app .chip span{
    width:6px;
    height:6px;
    border-radius:50%;
    background:var(--red);
  }

  /* controls */
  .skybox-tool-app .controls{
    background:var(--black);
    border-radius:16px;
    border:1px solid rgba(255,255,255,0.25);
    padding:10px 11px 10px;
    font-size:.85rem;
    display:flex;
    flex-direction:column;
    gap:9px;
  }
  .skybox-tool-app .controls label{
    display:block;
    margin-bottom:3px;
    font-size:.8rem;
    color:var(--white);
  }
  .skybox-tool-app .controls input[type=text],
  .skybox-tool-app .controls input[type=number]{
    width:100%;
    padding:4px 7px;
    border-radius:8px;
    border:1px solid rgba(255,255,255,0.4);
    background:var(--black);
    color:var(--white);
    font-size:.8rem;
    outline:none;
    transition:border-color .15s,box-shadow .15s,background .15s;
  }
  .skybox-tool-app .controls input[type=text]:focus,
  .skybox-tool-app .controls input[type=number]:focus{
    border-color:var(--red);
    box-shadow:0 0 0 1px rgba(255,0,0,0.5);
    background:#000000;
  }
  .skybox-tool-app .controls small{
    font-size:.72rem;
    color:rgba(255,255,255,0.6);
  }

  .skybox-tool-app .format-group{
    display:flex;
    gap:8px;
    margin-top:3px;
    font-size:.8rem;
  }
  .skybox-tool-app .format-group label{
    display:flex;
    align-items:center;
    gap:4px;
    margin:0;
    color:var(--white);
  }
  .skybox-tool-app .format-group input[type=radio]{accent-color:var(--red);}

  .skybox-tool-app button{
    border-radius:999px;
    border:1px solid var(--red);
    background:#000000;
    color:var(--white);
    padding:7px 12px;
    font-size:.78rem;
    cursor:pointer;
    display:inline-flex;
    align-items:center;
    justify-content:center;
    gap:6px;
    text-transform:uppercase;
    letter-spacing:.12em;
    transition:background .15s,border-color .15s,transform .08s,box-shadow .15s;
  }
  .skybox-tool-app button:disabled{
    opacity:.45;
    cursor:not-allowed;
    background:#000000;
    border-color:rgba(255,255,255,0.25);
    box-shadow:none;
  }
  .skybox-tool-app button:hover:not(:disabled){
    background:var(--red);
    border-color:var(--red);
    box-shadow:0 0 16px rgba(255,0,0,0.6);
  }
  .skybox-tool-app button:active:not(:disabled){
    transform:scale(.97);
    box-shadow:0 0 10px rgba(0,0,0,0.8) inset;
  }

  .skybox-tool-app #status{
    margin-top:8px;
    font-size:.78rem;
    color:rgba(255,255,255,0.75);
    white-space:pre-wrap;
  }

  /* cube net preview – 3 rows, 4 cols middle row */
  .skybox-tool-app #previewFaces{
    margin-top:8px;
    display:grid;
    grid-template-columns:repeat(4,auto);
    row-gap:6px;
    column-gap:6px;
    justify-content:center;
    align-items:center;
    justify-items:center;
  }
  .skybox-tool-app #previewFaces .cell{
    text-align:center;
    font-size:11px;
    color:rgba(255,255,255,0.7);
  }
  .skybox-tool-app #previewFaces canvas{
    background:#000000;
    border-radius:12px;
    border:1px solid rgba(255,255,255,0.35);
    transform-origin:center center;
  }

  /* preview-only rotations: 1st cell = _up, 6th cell = _dn */
  .skybox-tool-app #previewFaces .cell:nth-child(1) canvas{
    transform:rotate(90deg);      /* _up */
  }
  .skybox-tool-app #previewFaces .cell:nth-child(6) canvas{
    transform:rotate(-90deg);      /* _dn 90° left */
  }
  
  .skybox-tool-app .credits{
    margin-top:8px;
    padding-top:6px;
    border-top:1px solid rgba(255,255,255,0.1);
    text-align:center;
    font-size:.7rem;
    color:rgba(255,255,255,0.5);
    line-height:1.2;
  }
  .skybox-tool-app .credits a{
    color:rgba(255,255,255,0.7);
    text-decoration:none;
    transition:color .2s;
  }
  .skybox-tool-app .credits a:hover{
    color:var(--white);
  }
</style>

<div class="skybox-tool-app">
  <h1><span>Q3</span> SKYBOX TOOL</h1>
  <p class="subtitle">
    ${isRu 
      ? 'Конвертация 360° панорамы (equirectangular) в Quake 3 skybox faces:'
      : 'Convert an equirectangular 360° panorama into Quake 3 skybox faces:'}
    <code>_ft _bk _lf _rt _up _dn</code>.<br>
    <strong>${isRu ? 'Вход:' : 'Input:'}</strong> ${isRu 
      ? 'только панорамные изображения в формате equirectangular с соотношением сторон ~'
      : 'only equirectangular panoramic images with ~'}<strong>2:1</strong>${isRu ? '.' : ' aspect ratio.'}
  </p>

  <div class="layout">
    <!-- left: input / preview -->
    <div id="dropzone" tabindex="0">
      <canvas id="previewCanvas"></canvas>
      <div class="drop-hint" id="hint">
        <strong>${isRu ? 'Клик / Перетащить / Ctrl+V изображение' : 'Click / Drop / Ctrl+V image'}</strong>
        <div>${isRu ? 'Поддерживается: 360° панорамы в формате equirectangular (~2:1).' : 'Supported: 360° equirectangular panoramas (~2:1).'}</div>
        <div>${isRu ? 'Выход: 6 граней cubemap готовых для Quake 3.' : 'Output: 6 cubemap walls ready for Quake 3.'}</div>
        <div class="chip"><span></span> ${isRu ? 'Используйте высококачественные HDRIs космоса/неба для лучшего результата' : 'Use high-res space / sky HDRIs for best result'}</div>
      </div>
    </div>

    <!-- right: controls -->
    <div class="controls">
      <div>
        <label for="nameInput">${isRu ? 'Базовое имя skybox' : 'Skybox base name'}</label>
        <input id="nameInput" type="text" value="skybox">
        <small>${isRu ? 'Файлы:' : 'Files:'} <code>name_ft.ext</code>, <code>name_bk.ext</code>, <code>name_lf.ext</code>, <code>name_rt.ext</code>, <code>name_up.ext</code>, <code>name_dn.ext</code></small>
      </div>

      <div>
        <label for="sizeInput">${isRu ? 'Размер грани (px)' : 'Wall size (px)'}</label>
        <input id="sizeInput" type="number" min="16" step="1" value="2048">
        <small>${isRu ? 'Разрешение одной грани. Обычно: 512–2048.' : 'Single face resolution. Common: 512–2048.'}</small>
      </div>

      <div>
        <label for="angleInput">${isRu ? 'Yaw (горизонтальный поворот, °)' : 'Yaw (horizontal rotation, °)'}</label>
        <input id="angleInput" type="number" step="1" value="0">
        <small>${isRu ? 'Повернуть небо влево/вправо.' : 'Rotate sky left/right.'}</small>
      </div>

      <div>
        <label for="pitchInput">${isRu ? 'Pitch (вертикальный поворот, °)' : 'Pitch (vertical rotation, °)'}</label>
        <input id="pitchInput" type="number" step="1" value="0">
        <small>${isRu ? 'Повернуть небо вверх/вниз. 180° ≈ перевернуть вверх/вниз.' : 'Rotate sky up/down. 180° ≈ flip up/down.'}</small>
      </div>

      <div>
        <label>${isRu ? 'Формат вывода' : 'Output format'}</label>
        <div class="format-group">
          <label><input type="radio" name="fmt" value="jpg" checked> JPG</label>
          <label><input type="radio" name="fmt" value="png"> PNG</label>
        </div>
        <small>${isRu ? 'JPG – меньше размер; PNG – без потерь.' : 'JPG – smaller; PNG – lossless.'}</small>
      </div>

      <div>
        <button id="zipBtn" type="button" disabled>${isRu ? 'Создать и скачать ZIP' : 'Generate &amp; Download ZIP'}</button>
        <small style="display: block; margin-top: 4px;">${isRu ? 'Создаёт' : 'Creates'} <code>name_q3_skybox.zip</code> ${isRu ? 'с 6 гранями.' : 'with 6 faces.'}</small>
      </div>
    </div>
  </div>

  <div id="status"></div>
  <div id="previewFaces"></div>
  
  <div class="credits">
    ${isRu ? 'Сделано' : 'Made by'} <a href="https://t.me/Mus1nQ3" target="_blank" rel="noopener noreferrer"><strong>Musin</strong></a>
  </div>
</div>

<!-- hidden canvases -->
<canvas id="srcCanvas" style="display:none"></canvas>
<canvas id="panoCanvas" style="display:none"></canvas>
`;
    }
    
    // Skybox tool JavaScript code
    const skyboxToolJS = `
  /* ================= BASIC REFS ================= */
  const dropzone      = document.getElementById('dropzone');
  const hint          = document.getElementById('hint');
  const previewCanvas = document.getElementById('previewCanvas');
  const previewCtx    = previewCanvas.getContext('2d');
  const srcCanvas     = document.getElementById('srcCanvas');
  const srcCtx        = srcCanvas.getContext('2d');
  const panoCanvas    = document.getElementById('panoCanvas');
  const panoCtx       = panoCanvas.getContext('2d');

  const nameInput   = document.getElementById('nameInput');
  const sizeInput   = document.getElementById('sizeInput');
  const angleInput  = document.getElementById('angleInput'); // yaw
  const pitchInput  = document.getElementById('pitchInput'); // pitch
  const zipBtn      = document.getElementById('zipBtn');
  const statusEl    = document.getElementById('status');
  const facesWrap   = document.getElementById('previewFaces');

  const fmtRadios   = document.querySelectorAll('input[name="fmt"]');
  
  // Отслеживание изменений параметров
  function markSkyboxChanges() {
    if (window.ChangeTracker) {
      window.ChangeTracker.markChanges('skybox');
    }
  }
  
  if (nameInput) nameInput.addEventListener('input', markSkyboxChanges);
  if (sizeInput) sizeInput.addEventListener('input', markSkyboxChanges);
  if (angleInput) angleInput.addEventListener('input', markSkyboxChanges);
  if (pitchInput) pitchInput.addEventListener('input', markSkyboxChanges);
  if (fmtRadios) {
    fmtRadios.forEach(radio => {
      radio.addEventListener('change', markSkyboxChanges);
    });
  }

  const fileInputFake = document.createElement('input');
  fileInputFake.type = 'file';
  fileInputFake.accept = 'image/*';

  let hasImage = false;

  function getOutputFormat(){
    for (const r of fmtRadios){
      if (r.checked) return r.value;
    }
    return 'jpg';
  }

  /* ========== image load ========== */
  fileInputFake.addEventListener('change', () => {
    const f = fileInputFake.files[0];
    if (f) loadImage(f);
  });

  dropzone.addEventListener('click', () => {
    fileInputFake.value = '';
    fileInputFake.click();
  });

  dropzone.addEventListener('dragover', e => {
    e.preventDefault();
    dropzone.classList.add('dragover');
  });
  dropzone.addEventListener('dragleave', () => {
    dropzone.classList.remove('dragover');
  });
  dropzone.addEventListener('drop', e => {
    e.preventDefault();
    dropzone.classList.remove('dragover');
    const file = e.dataTransfer.files[0];
    if (file) loadImage(file);
  });

  // Only listen to paste events within the modal
  const skyboxModal = document.getElementById('skyboxModal');
  if (skyboxModal) {
    skyboxModal.addEventListener('paste', e => {
      if (!skyboxModal.classList.contains('active')) return;
      const items = e.clipboardData && e.clipboardData.items;
      if (!items) return;
      for (const item of items){
        if (item.type && item.type.startsWith('image/')){
          const file = item.getAsFile();
          if (file){
            loadImage(file);
            e.preventDefault();
            break;
          }
        }
      }
    });
  }

  function loadImage(file){
    const url = URL.createObjectURL(file);
    const img = new Image();
    img.onload = () => {
      URL.revokeObjectURL(url);
      preparePano(img);
      // Отметить изменения
      if (window.ChangeTracker) {
        window.ChangeTracker.markChanges('skybox');
      }
    };
    img.onerror = () => {
      URL.revokeObjectURL(url);
      const lang = window.location.pathname.includes('/ru/') ? 'ru' : 'en';
      statusEl.textContent = lang === 'ru' ? 'Ошибка загрузки изображения.' : 'Failed to load image.';
    };
    img.src = url;
  }

  function preparePano(img){
    const imgW = img.width;
    const imgH = img.height;

    srcCanvas.width  = imgW;
    srcCanvas.height = imgH;
    srcCtx.clearRect(0,0,imgW,imgH);
    srcCtx.drawImage(img,0,0);

    const panoW = imgW;
    const panoH = Math.floor(imgW / 2);
    panoCanvas.width  = panoW;
    panoCanvas.height = panoH;
    panoCtx.clearRect(0,0,panoW,panoH);
    panoCtx.drawImage(img, 0,0, panoW,panoH);

    const ratio = imgW / imgH;
    const lang = window.location.pathname.includes('/ru/') ? 'ru' : 'en';
    let warn = '';
    if (Math.abs(ratio - 2.0) > 0.2){
      warn = lang === 'ru'
        ? \`⚠ Предупреждение: соотношение сторон входного изображения \${ratio.toFixed(2)} (не ~2.00). Только панорамы в формате equirectangular с соотношением 2:1 полностью поддерживаются.\`
        : \`⚠ Warning: input aspect ratio is \${ratio.toFixed(2)} (not ~2.00). Only equirectangular panoramas with 2:1 ratio are fully supported.\`;
    }

    drawPreview();

    const loadedText = lang === 'ru' ? 'Загружено' : 'Loaded';
    const imageText = lang === 'ru' ? 'изображение.' : 'image.';
    statusEl.textContent =
      \`\${loadedText} \${imgW}×\${imgH} \${imageText}\\n\` +
       (warn ? warn : '');

    hasImage = true;
    zipBtn.disabled = false;
    hint.style.display = 'none';
    facesWrap.innerHTML = '';
  }

  function drawPreview(){
    const bounds = dropzone.getBoundingClientRect();
    const maxW = bounds.width  - 12;
    const maxH = bounds.height - 12;
    const pw = panoCanvas.width;
    const ph = panoCanvas.height;
    const scale = Math.min(maxW / pw, maxH / ph, 1);

    previewCanvas.width  = pw * scale;
    previewCanvas.height = ph * scale;
    previewCtx.clearRect(0,0,previewCanvas.width,previewCanvas.height);
    previewCtx.drawImage(panoCanvas, 0,0,pw,ph, 0,0,previewCanvas.width,previewCanvas.height);
  }

  window.addEventListener('resize', () => {
    if (hasImage) drawPreview();
  });

  /* ===== helper: rotate canvas 90° clockwise ===== */
  function rotateCanvas90CW(src){
    const w = src.width;
    const h = src.height;
    const out = document.createElement('canvas');
    out.width = w;
    out.height = h;
    const ctx = out.getContext('2d');
    ctx.translate(w / 2, h / 2);
    ctx.rotate(Math.PI / 2); // 90° clockwise
    ctx.drawImage(src, -w / 2, -h / 2);
    return out;
  }

  /* ========== Equirect → Cube with yaw + pitch (optimized) ========== */
  function buildQ3Faces(faceSize, yawDeg, pitchDeg){
    const panoW = panoCanvas.width;
    const panoH = panoCanvas.height;

    const srcData = panoCtx.getImageData(0,0,panoW,panoH);
    const src = srcData.data;

    const yaw   = yawDeg   * Math.PI / 180;
    const pitch = pitchDeg * Math.PI / 180;

    const cosYaw   = Math.cos(yaw);
    const sinYaw   = Math.sin(yaw);
    const cosPitch = Math.cos(pitch);
    const sinPitch = Math.sin(pitch);

    function sample(px, py){
      px = ((px % panoW) + panoW) % panoW;
      py = Math.max(0, Math.min(panoH - 1, py));
      const x = px | 0;
      const y = py | 0;
      const idx = (y * panoW + x) * 4;
      return [
        src[idx    ],
        src[idx + 1],
        src[idx + 2],
        src[idx + 3]
      ];
    }

    function renderFace(face){
      const canvas = document.createElement('canvas');
      canvas.width  = faceSize;
      canvas.height = faceSize;
      const ctx = canvas.getContext('2d');
      const imgData = ctx.createImageData(faceSize, faceSize);
      const dst = imgData.data;

      for (let y = 0; y < faceSize; y++){
        const v = (y + 0.5) / faceSize * 2 - 1;
        for (let x = 0; x < faceSize; x++){
          const u = (x + 0.5) / faceSize * 2 - 1;

          let X, Y, Z;
          switch (face){
            case 'F': X =  u; Y = -v; Z =  1; break;
            case 'B': X = -u; Y = -v; Z = -1; break;
            case 'R': X =  1; Y = -v; Z = -u; break;
            case 'L': X = -1; Y = -v; Z =  u; break;
            case 'U': X =  u; Y =  1; Z =  v; break;
            case 'D': X =  u; Y = -1; Z = -v; break;
            default:  X = u;  Y = -v; Z = 1;
          }

          /* ==== NEW ORDER: yaw → pitch (so they combine properly) ==== */

          // yaw around Y (horizontal)
          let X1 = X * cosYaw - Z * sinYaw;
          let Z1 = X * sinYaw + Z * cosYaw;
          X = X1;
          Z = Z1;

          // pitch around X (vertical)
          let Y1 = Y * cosPitch - Z * sinPitch;
          let Z2 = Y * sinPitch + Z * cosPitch;
          Y = Y1;
          Z = Z2;

          /* ========================================================== */

          const len = Math.sqrt(X*X + Y*Y + Z*Z);
          X /= len; Y /= len; Z /= len;

          const theta = Math.atan2(X, Z);
          const phi   = Math.asin(Y);

          const uTex = (theta / (2 * Math.PI) + 0.5) * panoW;
          const vTex = (0.5 - phi / Math.PI) * panoH;

          const [r,g,b,a] = sample(uTex, vTex);
          const idx = (y * faceSize + x) * 4;
          dst[idx    ] = r;
          dst[idx + 1] = g;
          dst[idx + 2] = b;
          dst[idx + 3] = a;
        }
      }

      ctx.putImageData(imgData, 0, 0);
      return canvas;
    }

    // render faces
    const ft = renderFace('F');
    const bk = renderFace('B');
    const lf = renderFace('R');
    const rt = renderFace('L');
    let up  = renderFace('U');
    let dn  = renderFace('D');

    // dn: 90° right
    dn = rotateCanvas90CW(dn);

    // up: 270° right (three times 90°)
    up = rotateCanvas90CW(up);
    up = rotateCanvas90CW(up);
    up = rotateCanvas90CW(up);
	
    return { ft, bk, lf, rt, up, dn };
  }

  /* ========== cube net preview: 3 rows, 4 cols middle row ========== */
  function renderCubeNetPreview(faces){
    facesWrap.innerHTML = '';

    const PREV = 96;

    function addCell(row, col, key, label){
      if (!faces[key]) return;
      const cell = document.createElement('div');
      cell.className = 'cell';
      cell.style.gridRow = String(row);
      cell.style.gridColumn = String(col);

      const c = document.createElement('canvas');
      c.width = PREV;
      c.height = PREV;
      const ctx = c.getContext('2d');
      ctx.drawImage(faces[key], 0,0,faces[key].width,faces[key].height, 0,0,PREV,PREV);

      const cap = document.createElement('div');
      cap.textContent = label;

      cell.appendChild(c);
      cell.appendChild(cap);
      facesWrap.appendChild(cell);
    }

    //  row1:      col2 -> _up
    //  row2:  col1..4 -> _rt _ft _lf _bk
    //  row3:      col2 -> _dn
    addCell(1, 2, 'up', '_up');
    addCell(2, 1, 'rt', '_rt');
    addCell(2, 2, 'ft', '_ft');
    addCell(2, 3, 'lf', '_lf');
    addCell(2, 4, 'bk', '_bk');
    addCell(3, 2, 'dn', '_dn');
  }

  /* ========== ZIP EXPORT ========== */
  zipBtn.addEventListener('click', async () => {
    if (!hasImage) return;
    const lang = window.location.pathname.includes('/ru/') ? 'ru' : 'en';
    if (typeof JSZip === 'undefined'){
      statusEl.textContent = lang === 'ru' ? 'Ошибка: JSZip не загружен.' : 'Error: JSZip not loaded.';
      return;
    }

    let faceSize = parseInt(sizeInput.value, 10);
    if (!Number.isFinite(faceSize) || faceSize <= 0){
      faceSize = 2048;
      sizeInput.value = String(faceSize);
    }

    const yawDeg   = parseFloat(angleInput.value);
    const pitchDeg = parseFloat(pitchInput.value);
    const yaw   = Number.isFinite(yawDeg)   ? yawDeg   : 0;
    const pitch = Number.isFinite(pitchDeg) ? pitchDeg : 0;

    const baseName = (nameInput.value || 'skybox').trim() || 'skybox';
    const fmt = getOutputFormat();
    const ext = fmt === 'png' ? 'png' : 'jpg';
    const convertingText = lang === 'ru' ? 'Конвертация панорамы → грани cubemap' : 'Converting panorama → cubemap faces';
    const yawText = lang === 'ru' ? 'Yaw (горизонтальный)' : 'Yaw (horizontal)';
    const pitchText = lang === 'ru' ? 'Pitch (вертикальный)' : 'Pitch (vertical)';
    const buildingText = lang === 'ru' ? 'Создание .zip…' : 'Building .zip…';
    const doneText = lang === 'ru' ? 'Готово:' : 'Done:';
    const errorText = lang === 'ru' ? 'Ошибка создания zip:' : 'Error creating zip:';
    
    statusEl.textContent =
      \`\${convertingText} (\${faceSize}×\${faceSize})…\\n\` +
      \`\${yawText}: \${yaw.toFixed(1)}°\\n\` +
      \`\${pitchText}: \${pitch.toFixed(1)}°.\`;

    const faces = buildQ3Faces(faceSize, yaw, pitch);

    renderCubeNetPreview(faces);

    statusEl.textContent += \`\\n\${buildingText}\`;

    const zip = new JSZip();

    async function canvasToBlob(canvas, fmt){
      return new Promise(resolve => {
        if (fmt === 'png'){
          canvas.toBlob(blob => resolve(blob), 'image/png');
        } else {
          canvas.toBlob(blob => resolve(blob), 'image/jpeg', 1.0);
        }
      });
    }

    const order = ['ft','bk','lf','rt','up','dn'];
    for (const suf of order){
      const canvas = faces[suf];
      if (!canvas) continue;
      const blob = await canvasToBlob(canvas, fmt);
      const filename = \`\${baseName}_\${suf}.\${ext}\`;
      zip.file(filename, blob);
    }

    try{
      const zipBlob = await zip.generateAsync({type:'blob'});
      const url = URL.createObjectURL(zipBlob);
      const a = document.createElement('a');
      a.href = url;
      a.download = \`\${baseName}_q3_skybox.zip\`;
      document.body.appendChild(a);
      a.click();
      a.remove();
      setTimeout(() => URL.revokeObjectURL(url), 5000);
      statusEl.textContent += \`\\n\${doneText} \${baseName}_q3_skybox.zip\`;
      
      // Отметить, что работа завершена
      if (window.ChangeTracker) {
        window.ChangeTracker.markCompleted('skybox');
      }
    }catch(e){
      console.error(e);
      statusEl.textContent += \`\\n\${errorText} \${e}\`;
    }
  });
`;

    document.addEventListener('DOMContentLoaded', function() {
        const openSkyboxBtn = document.getElementById('openSkyboxModal');
        const skyboxModal = document.getElementById('skyboxModal');
        const closeSkyboxModal = document.getElementById('closeSkyboxModal');
        const skyboxToolContent = document.getElementById('skyboxToolContent');
        
        if (!openSkyboxBtn || !skyboxModal || !closeSkyboxModal || !skyboxToolContent) {
            return;
        }
        
        let isContentLoaded = false;
        
        function loadSkyboxTool() {
            if (isContentLoaded) return;
            
            // Add root class to content div
            skyboxToolContent.className = 'skybox-tool-root';
            
            // Get language and insert HTML
            const lang = getLanguage();
            skyboxToolContent.innerHTML = getSkyboxToolHTML(lang);
            
            // Execute JavaScript
            const script = document.createElement('script');
            script.textContent = skyboxToolJS;
            skyboxToolContent.appendChild(script);
            
            isContentLoaded = true;
        }
        
        function openSkyboxModal() {
            loadSkyboxTool();
            skyboxModal.classList.add('active');
            document.body.style.overflow = 'hidden';
            
            // Update URL hash
            if (history.pushState) {
                history.pushState(null, null, '#skybox');
            } else {
                window.location.hash = '#skybox';
            }
            
            // Сбросить изменения при открытии
            if (window.ChangeTracker) {
                window.ChangeTracker.resetChanges('skybox');
            }
            
            // Set focus on dropzone after a short delay to ensure content is loaded
            setTimeout(() => {
                const dropzone = document.getElementById('dropzone');
                if (dropzone) {
                    dropzone.focus();
                } else {
                    // Fallback: focus on modal content if dropzone not ready
                    skyboxToolContent.setAttribute('tabindex', '-1');
                    skyboxToolContent.focus();
                }
            }, 100);
        }
        
        function closeSkyboxModalFunc() {
            doCloseSkyboxModal();
        }
        
        function doCloseSkyboxModal() {
            skyboxModal.classList.remove('active');
            document.body.style.overflow = '';
            // Remove URL hash
            if (history.pushState) {
                history.pushState(null, null, window.location.pathname + window.location.search);
            } else {
                window.location.hash = '';
            }
            // Сбросить изменения при закрытии
            if (window.ChangeTracker) {
                window.ChangeTracker.resetChanges('skybox');
            }
        }
        
        openSkyboxBtn.addEventListener('click', function(e) {
            e.preventDefault();
            e.stopPropagation();
            openSkyboxModal();
        });
        
        closeSkyboxModal.addEventListener('click', function() {
            closeSkyboxModalFunc();
        });
        
        // Close on background click (only if both mousedown and mouseup were outside content)
        var mouseDownOnBackground = false;
        skyboxModal.addEventListener('mousedown', function(e) {
            mouseDownOnBackground = (e.target === skyboxModal);
        });
        var handleMouseUp = function(e) {
            if (mouseDownOnBackground) {
                if (e.target === skyboxModal || !skyboxModal.contains(e.target)) {
                    closeSkyboxModalFunc();
                }
            }
            mouseDownOnBackground = false;
        };
        skyboxModal.addEventListener('mouseup', handleMouseUp);
        document.addEventListener('mouseup', handleMouseUp);
        
        // Close on Escape key
        document.addEventListener('keydown', function(e) {
            if (e.key === 'Escape' && skyboxModal.classList.contains('active')) {
                e.preventDefault();
                if (window.ChangeTracker) {
                    window.ChangeTracker.handleEscPress('skybox', function() {
                        closeSkyboxModalFunc();
                    });
                } else {
                    closeSkyboxModalFunc();
                }
            }
        });
        
        // Check URL hash on page load
        if (window.location.hash === '#skybox') {
            openSkyboxModal();
        }
        
        // Listen for hash changes (back/forward buttons)
        window.addEventListener('hashchange', function() {
            if (window.location.hash === '#skybox') {
                openSkyboxModal();
            } else if (skyboxModal.classList.contains('active')) {
                closeSkyboxModalFunc();
            }
        });
    });
})();

