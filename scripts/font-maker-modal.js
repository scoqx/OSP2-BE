// OSP2 Font Maker Modal functionality
(function() {
    'use strict';
    
    // Determine language from URL
    function getLanguage() {
        return window.location.pathname.includes('/ru/') ? 'ru' : 'en';
    }
    
    // Font maker tool HTML content
    function getFontMakerToolHTML(lang) {
        const isRu = lang === 'ru';
        
        return `
<link rel="stylesheet" href="/scripts/font-maker/font-maker.css">

<div class="font-maker-tool-app">
  <div class="fm-header">
    <h1><span>OSP2</span> FONT MAKER</h1>
    <p class="subtitle">
      ${isRu 
        ? 'Конвертер TTF/OTF в bitmap шрифт для Quake 3 OSP2 BE'
        : 'TTF/OTF to bitmap font for Quake 3 OSP2 BE'}
    </p>
  </div>

  <div class="fm-main">
    <div class="fm-left">
      <div class="form-group">
        <label for="ttfFile">${isRu ? 'TTF/OTF или ZIP' : 'TTF/OTF or ZIP'}</label>
        <input type="file" id="ttfFile" accept=".ttf,.otf,.zip" />
        <div id="fileInfo" class="file-info" style="display: none;"></div>
      </div>

      <div class="button-group">
        <button class="btn-primary" id="generateBtn" disabled>${isRu ? 'Сгенерировать' : 'Generate'}</button>
        <button class="btn-secondary" id="testFontBtn">${isRu ? 'Тест' : 'Test'}</button>
      </div>
      <div id="downloadLinks" class="download-links" style="display: none;"></div>

      <div class="fm-row">
        <div class="form-group">
          <label for="fontName">${isRu ? 'Имя' : 'Name'}</label>
          <input type="text" id="fontName" placeholder="${isRu ? 'Авто' : 'Auto'}" />
        </div>
        <div class="form-group">
          <label for="resolution">${isRu ? 'Resolution' : 'Resolution'}</label>
          <input type="number" id="resolution" value="1.0" min="0.1" max="4.0" step="0.1" />
        </div>
        <div class="form-group">
          <label for="fontScale">Scale</label>
          <input type="number" id="fontScale" value="1.0" min="0.1" max="10.0" step="0.1" />
        </div>
      </div>

      <div class="fm-row">
        <div class="form-group">
          <label>${isRu ? 'Symbol offset' : 'Symbol offset'}</label>
          <div class="fm-two">
            <input type="number" id="horizontalOffset" value="0" min="-100" max="100" step="1" placeholder="H" />
            <input type="number" id="verticalOffset" value="0" min="-100" max="100" step="1" placeholder="V" />
          </div>
        </div>
      </div>

      <div class="fm-row">
        <div class="form-group">
          <label for="padding">${isRu ? 'Symbol box padding' : 'Symbol box padding'}</label>
          <input type="number" id="padding" value="2" min="0" max="20" />
        </div>
      </div>

      <div id="status" class="status info" style="display:none;"></div>
      <div class="info-box">
        <small><strong>ℹ️</strong> ${isRu ? 'CFG поддерживает Proportional и Monospace автоматически' : 'CFG supports Proportional and Monospace automatically'}</small>
      </div>
    </div>

    <div class="fm-right">
      <div class="fm-preview-tabs">
        <button class="preview-tab active" id="previewTabQuake" data-tab="quake">${isRu ? 'Quake 3' : 'Quake 3'}</button>
      </div>

      <div id="previewQuakeContainer">
        <div class="fm-test-grid">
          <div>
            <label>${isRu ? 'Ширина' : 'Width'}</label>
            <input type="number" id="testCharWidth" value="32" min="1" max="100" step="1" />
          </div>
          <div>
            <label>${isRu ? 'Высота' : 'Height'}</label>
            <input type="number" id="testCharHeight" value="32" min="1" max="100" step="1" />
          </div>
          <div>
            <label>${isRu ? 'Макс. ширина' : 'Max width'}</label>
            <input type="number" id="testMaxWidth" value="1920" min="1" max="2000" step="10" />
          </div>
          <div class="fm-test-flags">
            <label><input type="checkbox" id="testProportional" checked />Prop</label>
            <label><input type="checkbox" id="testShadow" />Shadow</label>
            <label><input type="checkbox" id="testShowBounds" checked />${isRu ? 'Границы' : 'Bounds'}</label>
            <label><input type="checkbox" id="testHCenter" />Center</label>
            <label><input type="checkbox" id="testHRight" />Right</label>
          </div>
        </div>
        <div class="fm-row">
          <textarea id="testText" placeholder="${isRu ? 'Текст для теста...' : 'Test text...'}">The quick brown fox jumps over the lazy dog
0123456789 !@#$%^&*()_+-=[]{}|;:,.<>?</textarea>
        </div>
        <div class="fm-test-result">
          <canvas id="testCanvas"></canvas>
        </div>
      </div>
    </div>
  </div>
</div>
`;
    }
    
    // Font maker tool JavaScript code
    const fontMakerToolJS = `
  let currentFont = null;
  let generatedData = null;
  let storedCharMetrics = [];
  
  const isRu = window.location.pathname.includes('/ru/');
  
  function showStatus(message, type) {
    const statusDiv = document.getElementById('status');
    statusDiv.className = 'status ' + type;
    statusDiv.textContent = message;
    statusDiv.style.display = 'block';
  }
  
  function getCharSet() {
    const charSetInput = document.getElementById('charSet').value.trim();
    if (charSetInput) {
      return charSetInput;
    }
    const chars = [];
    for (let i = 32; i <= 126; i++) {
      chars.push(String.fromCharCode(i));
    }
    return chars.join('');
  }
  
  function compressRLE(data, pixelSize) {
    const result = [];
    let i = 0;
    
    while (i < data.length) {
      let runLength = 1;
      const startPixel = i;
      
      while (runLength < 128 && i + runLength * pixelSize < data.length) {
        let same = true;
        for (let j = 0; j < pixelSize; j++) {
          if (data[startPixel + j] !== data[i + runLength * pixelSize + j]) {
            same = false;
            break;
          }
        }
        if (!same) break;
        runLength++;
      }
      
      if (runLength > 1) {
        result.push(0x80 | (runLength - 1));
        for (let j = 0; j < pixelSize; j++) {
          result.push(data[startPixel + j]);
        }
        i += runLength * pixelSize;
      } else {
        let rawLength = 1;
        while (rawLength < 128 && i + rawLength * pixelSize < data.length) {
          const currentIdx = i + rawLength * pixelSize;
          if (currentIdx + pixelSize < data.length) {
            let nextTwoSame = true;
            for (let j = 0; j < pixelSize; j++) {
              if (data[currentIdx + j] !== data[currentIdx + pixelSize + j]) {
                nextTwoSame = false;
                break;
              }
            }
            if (nextTwoSame) {
              break;
            }
          }
          rawLength++;
        }
        
        result.push(rawLength - 1);
        for (let k = 0; k < rawLength; k++) {
          for (let j = 0; j < pixelSize; j++) {
            result.push(data[i + k * pixelSize + j]);
          }
        }
        i += rawLength * pixelSize;
      }
    }
    
    return new Uint8Array(result);
  }
  
  function writeTGA(imageData, width, height) {
    const pixelData = [];
    const data = imageData.data;
    
    for (let y = height - 1; y >= 0; y--) {
      for (let x = 0; x < width; x++) {
        const src = (y * width + x) * 4;
        pixelData.push(data[src + 2]);
        pixelData.push(data[src + 1]);
        pixelData.push(data[src + 0]);
        pixelData.push(data[src + 3]);
      }
    }
    
    const compressedData = compressRLE(pixelData, 4);
    const buffer = new ArrayBuffer(18 + compressedData.length);
    const view = new DataView(buffer);
    let offset = 0;
    
    view.setUint8(offset++, 0);
    view.setUint8(offset++, 0);
    view.setUint8(offset++, 10);
    view.setUint16(offset, 0, true); offset += 2;
    view.setUint16(offset, 0, true); offset += 2;
    view.setUint8(offset++, 0);
    view.setUint16(offset, 0, true); offset += 2;
    view.setUint16(offset, 0, true); offset += 2;
    view.setUint16(offset, width, true); offset += 2;
    view.setUint16(offset, height, true); offset += 2;
    view.setUint8(offset++, 32);
    view.setUint8(offset++, 8);
    
    const compressedView = new Uint8Array(buffer, 18);
    compressedView.set(compressedData);
    
    return new Blob([buffer], { type: 'image/tga' });
  }
  
  let copyrightText = '';
  
  async function loadFontFromFile(fileBuffer, fileName) {
    try {
      currentFont = opentype.parse(fileBuffer);
      const fileInfo = document.getElementById('fileInfo');
      fileInfo.style.display = 'block';
      const fileNameLabel = isRu ? 'Файл:' : 'File:';
      const fileSizeLabel = isRu ? 'Размер:' : 'Size:';
      const fontNameLabel = isRu ? 'Имя шрифта:' : 'Font name:';
      const unknownLabel = isRu ? 'Неизвестно' : 'Unknown';
      fileInfo.innerHTML = 
        '<strong>' + fileNameLabel + '</strong> ' + fileName + '<br>' +
        '<strong>' + fileSizeLabel + '</strong> ' + (fileBuffer.byteLength / 1024).toFixed(2) + ' KB<br>' +
        '<strong>' + fontNameLabel + '</strong> ' + (currentFont.names.fullName ? currentFont.names.fullName.en : unknownLabel) + '<br>' +
        '<strong>Units per EM:</strong> ' + currentFont.unitsPerEm;
      
      const fontNameInput = document.getElementById('fontName');
      fontNameInput.value = currentFont.names.fullName ? currentFont.names.fullName.en : 'CustomFont';
      
      document.getElementById('generateBtn').disabled = false;
      document.getElementById('previewBtn').disabled = false;
      showStatus(isRu ? 'Шрифт загружен успешно!' : 'Font loaded successfully!', 'success');
    } catch (error) {
      showStatus((isRu ? 'Ошибка загрузки шрифта: ' : 'Error loading font: ') + error.message, 'error');
    }
  }
  
  async function handleZipFile(zipBuffer) {
    try {
      const zip = await JSZip.loadAsync(zipBuffer);
      const fontFiles = [];
      
      zip.forEach(function(relativePath, file) {
        const fileName = relativePath.split('/').pop().toLowerCase();
        if (fileName.endsWith('.ttf') || fileName.endsWith('.otf')) {
          fontFiles.push({ path: relativePath, name: fileName });
        }
      });
      
      if (fontFiles.length === 0) {
        showStatus(isRu ? 'В ZIP архиве не найдено TTF/OTF файлов' : 'No TTF/OTF files found in ZIP archive', 'error');
        return;
      }
      
      let selectedFontFile = null;
      
      if (fontFiles.length === 1) {
        selectedFontFile = fontFiles[0];
      } else {
        const fontList = fontFiles.map((f, i) => (i + 1) + '. ' + f.name).join('\\n');
        const choice = prompt((isRu ? 'Найдено несколько шрифтов. Выберите номер:\\n\\n' : 'Multiple fonts found. Select number:\\n\\n') + fontList + '\\n\\n' + (isRu ? 'Введите номер (1-' : 'Enter number (1-') + fontFiles.length + '):');
        const index = parseInt(choice) - 1;
        if (index >= 0 && index < fontFiles.length) {
          selectedFontFile = fontFiles[index];
        } else {
          showStatus(isRu ? 'Неверный выбор' : 'Invalid selection', 'error');
          return;
        }
      }
      
      if (selectedFontFile) {
        const fontData = await zip.file(selectedFontFile.path).async('arraybuffer');
        await loadFontFromFile(fontData, selectedFontFile.name);
        
        let copyrightFile = null;
        zip.forEach(function(relativePath, file) {
          const fileName = relativePath.split('/').pop().toLowerCase();
          if (fileName === 'copyright.txt' && !copyrightFile) {
            copyrightFile = file;
          }
        });
        
        if (copyrightFile) {
          copyrightText = await copyrightFile.async('string');
          showStatus((isRu ? 'Copyright.txt найден и будет добавлен в CFG' : 'Copyright.txt found and will be added to CFG'), 'info');
        } else {
          copyrightText = '';
        }
      }
    } catch (error) {
      showStatus((isRu ? 'Ошибка обработки ZIP: ' : 'Error processing ZIP: ') + error.message, 'error');
    }
  }
  
  document.getElementById('ttfFile').addEventListener('change', async function(e) {
    const file = e.target.files[0];
    if (!file) return;
    
    copyrightText = '';
    
    const reader = new FileReader();
    reader.onload = async function(e) {
      try {
        const fileBuffer = e.target.result;
        const fileName = file.name.toLowerCase();
        
        if (fileName.endsWith('.zip')) {
          if (typeof JSZip === 'undefined') {
            showStatus(isRu ? 'JSZip не загружен. Пожалуйста, подождите...' : 'JSZip not loaded. Please wait...', 'info');
            setTimeout(() => {
              reader.onload();
            }, 500);
            return;
          }
          await handleZipFile(fileBuffer);
        } else {
          await loadFontFromFile(fileBuffer, file.name);
        }
      } catch (error) {
        showStatus((isRu ? 'Ошибка загрузки файла: ' : 'Error loading file: ') + error.message, 'error');
      }
    };
    reader.readAsArrayBuffer(file);
  });
  
  
  function generateFontAtSize(fontSize, cellSize, padding, charSet, charSetMap, resolution, horizontalOffset, verticalOffset) {
    const charsPerRow = 16;
    const totalChars = 256;
    const atlasSize = charsPerRow * cellSize;
    
    const res = resolution || 1.0;
    const hOffset = horizontalOffset || 0;
    const vOffset = verticalOffset || 0;
    const scale = fontSize / currentFont.unitsPerEm;
    const charMetrics = [];
    let maxAscender = 0;
    let maxDescender = 0;
    
    for (let charCode = 0; charCode < totalChars; charCode++) {
      const char = String.fromCharCode(charCode);
      const glyph = currentFont.charToGlyph(char);
      
      if (!glyph || (charSet.length > 0 && !charSetMap.has(charCode))) {
        charMetrics.push({
          char: char, charCode: charCode, glyph: null, width: 0, height: 0,
          advanceWidth: cellSize, leftBearing: 0, bbox: null
        });
        continue;
      }
      
      const bbox = glyph.getBoundingBox();
      const advanceWidth = glyph.advanceWidth * scale;
      
      let width = 0;
      if (bbox && (bbox.x2 - bbox.x1) > 0) {
        width = (bbox.x2 - bbox.x1) * scale;
      } else {
        width = advanceWidth;
      }
      
      if (width <= 0) {
        width = advanceWidth || cellSize * 0.5;
      }
      
      const height = bbox ? (bbox.y2 - bbox.y1) * scale : fontSize;
      const leftBearing = bbox ? bbox.x1 * scale : 0;
      
      if (bbox) {
        const ascender = bbox.y2 * scale;
        const descender = Math.abs(bbox.y1 * scale);
        if (ascender > maxAscender) maxAscender = ascender;
        if (descender > maxDescender) maxDescender = descender;
      }
      
      charMetrics.push({
        char: char, charCode: charCode, glyph: glyph, width: Math.ceil(width),
        height: Math.ceil(height), advanceWidth: Math.ceil(advanceWidth),
        leftBearing: Math.ceil(leftBearing), bbox: bbox
      });
    }
    
    const availableHeight = cellSize - padding * 2;
    const totalHeight = maxAscender + maxDescender;
    
    let baselineOffset;
    if (totalHeight > 0 && totalHeight <= availableHeight) {
      baselineOffset = maxAscender;
    } else if (totalHeight > availableHeight) {
      const scaleFactor = availableHeight / totalHeight;
      maxAscender = maxAscender * scaleFactor;
      maxDescender = maxDescender * scaleFactor;
      baselineOffset = maxAscender;
    } else {
      baselineOffset = fontSize * 0.8;
    }
    
    baselineOffset = Math.max(0, Math.min(cellSize - padding, baselineOffset));
    
    const renderScale = res;
    const renderWidth = Math.ceil(atlasSize * renderScale);
    const renderHeight = Math.ceil(atlasSize * renderScale);
    
    const renderCanvas = document.createElement('canvas');
    renderCanvas.width = renderWidth;
    renderCanvas.height = renderHeight;
    const renderCtx = renderCanvas.getContext('2d', { alpha: true });
    
    if (res >= 1.0) {
      renderCtx.imageSmoothingEnabled = true;
      renderCtx.imageSmoothingQuality = res >= 2.0 ? 'high' : 'medium';
    } else {
      renderCtx.imageSmoothingEnabled = false;
      renderCtx.imageSmoothingQuality = 'low';
    }
    renderCtx.clearRect(0, 0, renderWidth, renderHeight);
    
    const canvas = document.createElement('canvas');
    canvas.width = atlasSize;
    canvas.height = atlasSize;
    const ctx = canvas.getContext('2d', { alpha: true });
    
    ctx.imageSmoothingEnabled = false;
    ctx.imageSmoothingQuality = 'low';
    ctx.clearRect(0, 0, atlasSize, atlasSize);
    
    const charData = [];
    for (let i = 0; i < charMetrics.length; i++) {
      const metric = charMetrics[i];
      const row = Math.floor(i / charsPerRow);
      const col = i % charsPerRow;
      const cellX = col * cellSize;
      const cellY = row * cellSize;
      
      const charX = cellX + padding + hOffset;
      const charY = cellY + padding + baselineOffset + vOffset;
      
      const renderCellSize = Math.ceil(cellSize * renderScale);
      const renderPadding = Math.ceil(padding * renderScale);
      const renderCharX = Math.floor((col * cellSize + padding + hOffset) * renderScale);
      const renderCharY = Math.floor((row * cellSize + padding + baselineOffset + vOffset) * renderScale);
      
      const tempCanvas = document.createElement('canvas');
      tempCanvas.width = renderCellSize;
      tempCanvas.height = renderCellSize;
      const tempCtx = tempCanvas.getContext('2d', { alpha: true });
      if (res >= 1.0) {
        tempCtx.imageSmoothingEnabled = true;
        tempCtx.imageSmoothingQuality = res >= 2.0 ? 'high' : 'medium';
      } else {
        tempCtx.imageSmoothingEnabled = false;
        tempCtx.imageSmoothingQuality = 'low';
      }
      tempCtx.clearRect(0, 0, renderCellSize, renderCellSize);
      
      let x1, w, s1, s2;
      
      if (!metric.glyph) {
        x1 = Math.floor(cellSize * 0.375);
        w = Math.floor(cellSize * 0.25);
        s1 = Math.floor(cellSize * 0.09375);
        s2 = Math.floor(cellSize * 0.09375);
      } else {
        const tempCharX = renderPadding + Math.floor(hOffset * renderScale);
        const tempCharY = renderPadding + Math.floor((baselineOffset + vOffset) * renderScale);
        const renderFontSize = fontSize * renderScale;
        const path = metric.glyph.getPath(tempCharX, tempCharY, renderFontSize);
        tempCtx.fillStyle = '#FFFFFF';
        tempCtx.beginPath();
        const commands = path.commands;
        for (let j = 0; j < commands.length; j++) {
          const cmd = commands[j];
          if (cmd.type === 'M') tempCtx.moveTo(cmd.x, cmd.y);
          else if (cmd.type === 'L') tempCtx.lineTo(cmd.x, cmd.y);
          else if (cmd.type === 'C') tempCtx.bezierCurveTo(cmd.x1, cmd.y1, cmd.x2, cmd.y2, cmd.x, cmd.y);
          else if (cmd.type === 'Q') tempCtx.quadraticCurveTo(cmd.x1, cmd.y1, cmd.x, cmd.y);
          else if (cmd.type === 'Z') tempCtx.closePath();
        }
        tempCtx.fill();
        
        const tempImageData = tempCtx.getImageData(0, 0, renderCellSize, renderCellSize);
        const tempData = tempImageData.data;
        
        let minX = renderCellSize;
        let maxX = -1;
        let foundPixel = false;
        
        for (let py = 0; py < renderCellSize; py++) {
          for (let px = 0; px < renderCellSize; px++) {
            const idx = (py * renderCellSize + px) * 4;
            const alpha = tempData[idx + 3];
            if (alpha > 0) {
              foundPixel = true;
              if (px < minX) minX = px;
              if (px > maxX) maxX = px;
            }
          }
        }
        
        if (foundPixel && minX <= maxX) {
          const actualMinX = minX - tempCharX;
          const actualMaxX = maxX - tempCharX;
          
          x1 = Math.max(0, Math.floor(actualMinX / renderScale));
          const maxXScaled = Math.ceil((actualMaxX + 1) / renderScale);
          w = Math.max(1, maxXScaled - x1);
          
          if (x1 + w > cellSize) {
            w = Math.max(1, cellSize - x1);
          }
          
          s1 = x1;
          s2 = Math.max(0, cellSize - x1 - w);
        } else {
          const scale = fontSize / currentFont.unitsPerEm;
          const charLeftEdge = padding + metric.leftBearing;
          x1 = Math.max(0, Math.ceil(charLeftEdge));
          w = Math.min(Math.ceil(metric.width), cellSize - x1);
          s1 = Math.max(0, x1);
          s2 = Math.max(0, cellSize - x1 - w);
        }
        
        const path2 = metric.glyph.getPath(renderCharX, renderCharY, fontSize * renderScale);
        renderCtx.fillStyle = 'rgba(255, 255, 255, 255)';
        renderCtx.beginPath();
        const commands2 = path2.commands;
        for (let j = 0; j < commands2.length; j++) {
          const cmd = commands2[j];
          if (cmd.type === 'M') renderCtx.moveTo(cmd.x, cmd.y);
          else if (cmd.type === 'L') renderCtx.lineTo(cmd.x, cmd.y);
          else if (cmd.type === 'C') renderCtx.bezierCurveTo(cmd.x1, cmd.y1, cmd.x2, cmd.y2, cmd.x, cmd.y);
          else if (cmd.type === 'Q') renderCtx.quadraticCurveTo(cmd.x1, cmd.y1, cmd.x, cmd.y);
          else if (cmd.type === 'Z') renderCtx.closePath();
        }
        renderCtx.fill();
      }
      
      charData.push({
        charCode: metric.charCode, x0: cellX, y0: cellY, x1: x1, w: w, s1: s1, s2: s2
      });
    }
    
    ctx.drawImage(renderCanvas, 0, 0, renderWidth, renderHeight, 0, 0, atlasSize, atlasSize);
    
    const imageData = ctx.getImageData(0, 0, atlasSize, atlasSize);
    const tgaBlob = writeTGA(imageData, atlasSize, atlasSize);
    
    const metricsWithData = charMetrics.map((metric, i) => ({
      ...metric, cfgData: charData[i]
    }));
    
    return {
      canvas: canvas, tgaBlob: tgaBlob, charData: charData,
      charMetrics: metricsWithData, cellSize: cellSize, atlasSize: atlasSize
    };
  }
  
  async function generateFont() {
    if (!currentFont) {
      showStatus(isRu ? 'Пожалуйста, загрузите TTF или OTF файл' : 'Please load a TTF or OTF file', 'error');
      return;
    }
    
    showStatus(isRu ? 'Генерация шрифта (3 размера)...' : 'Generating font (3 sizes)...', 'info');
    
    try {
      const padding = parseInt(document.getElementById('padding').value) || 2;
      const fontName = document.getElementById('fontName').value || 
        (currentFont.names.fullName ? currentFont.names.fullName.en : 'CustomFont');
      const charSet = getCharSet();
      
      const charSetMap = new Set();
      for (let i = 0; i < charSet.length; i++) {
        charSetMap.add(charSet.charCodeAt(i));
      }
      
      const fontScale = parseFloat(document.getElementById('fontScale').value) || 1.0;
      const cellSize64 = 64;
      const cellSize32 = 32;
      const cellSize16 = 16;
      const baseFontSize = 48;
      const fontSize64 = Math.round(baseFontSize * fontScale);
      const fontSize32 = Math.round((baseFontSize * fontScale) / 2);
      const fontSize16 = Math.round((baseFontSize * fontScale) / 4);
      
      const resolution = parseFloat(document.getElementById('resolution').value) || 1.0;
      const horizontalOffset = parseFloat(document.getElementById('horizontalOffset').value) || 0;
      const verticalOffset = parseFloat(document.getElementById('verticalOffset').value) || 0;
      
      showStatus((isRu ? 'Генерация размера 64 (буквы: ' : 'Generating size 64 (letters: ') + fontSize64 + 'px)...', 'info');
      const size64 = generateFontAtSize(fontSize64, cellSize64, padding, charSet, charSetMap, resolution, horizontalOffset, verticalOffset);
      
      showStatus((isRu ? 'Генерация размера 32 (буквы: ' : 'Generating size 32 (letters: ') + fontSize32 + 'px)...', 'info');
      const size32 = generateFontAtSize(fontSize32, cellSize32, padding, charSet, charSetMap, resolution, horizontalOffset, verticalOffset);
      
      showStatus((isRu ? 'Генерация размера 16 (буквы: ' : 'Generating size 16 (letters: ') + fontSize16 + 'px)...', 'info');
      const size16 = generateFontAtSize(fontSize16, cellSize16, padding, charSet, charSetMap, resolution, horizontalOffset, verticalOffset);
      
      const fontBaseName = fontName.toLowerCase().replace(/\\s+/g, '_');
      
      const cfgLines = [];
      if (copyrightText) {
        const copyrightLines = copyrightText.split(/\\r?\\n/);
        cfgLines.push('// Copyright: ' + copyrightLines[0]);
        for (let i = 1; i < copyrightLines.length; i++) {
          if (copyrightLines[i].trim()) {
            cfgLines.push('// ' + copyrightLines[i]);
          }
        }
        cfgLines.push('');
      }
      cfgLines.push('// img <font image> <threshold>, up to 4 entries allowed');
      cfgLines.push('img "gfx/2d/' + fontBaseName + '_64.tga" 48');
      cfgLines.push('img "gfx/2d/' + fontBaseName + '_32.tga" 22');
      cfgLines.push('img "gfx/2d/' + fontBaseName + '_16.tga" 0');
      cfgLines.push('// fnt <width> <height> <char width> <char height>');
      cfgLines.push('fnt ' + size64.atlasSize + ' ' + size64.atlasSize + ' ' + size64.cellSize + ' ' + size64.cellSize);
      cfgLines.push('// <ch> <x0> <y0> <x1> <w> <s1> <s2>');
      
      for (const data of size64.charData) {
        cfgLines.push(data.charCode + ' ' + data.x0 + ' ' + data.y0 + ' ' + data.x1 + ' ' + data.w + ' ' + data.s1 + ' ' + data.s2);
      }
      
      const cfgContent = cfgLines.join('\\n') + '\\n';
      const fontsCfgContent = '"' + fontName + '" "gfx/2d/' + fontBaseName + '.cfg"\\n';
      
      showStatus(isRu ? 'Создание PK3 архива...' : 'Creating PK3 archive...', 'info');
      const zip = new JSZip();
      
      zip.file('gfx/2d/' + fontBaseName + '_64.tga', size64.tgaBlob);
      zip.file('gfx/2d/' + fontBaseName + '_32.tga', size32.tgaBlob);
      zip.file('gfx/2d/' + fontBaseName + '_16.tga', size16.tgaBlob);
      zip.file('gfx/2d/' + fontBaseName + '.cfg', cfgContent);
      zip.file('fonts.cfg', fontsCfgContent);
      
      const zipBlob = await zip.generateAsync({ type: 'blob' });
      
      generatedData = {
        canvas: size64.canvas, cfgContent: cfgContent, fontsCfgContent: fontsCfgContent,
        zipBlob: zipBlob, fontName: fontName, fontBaseName: fontBaseName,
        charData: size64.charData, charMetrics: size64.charMetrics,
        tgaBlob64: size64.tgaBlob, tgaBlob32: size32.tgaBlob, tgaBlob16: size16.tgaBlob
      };
      
      storedCharMetrics = size64.charMetrics;
      displayCharInfoTable(storedCharMetrics);
      
      const previewCanvas = document.getElementById('previewCanvas');
      const sourceCanvas = size64.canvas;
      const maxDisplaySize = 1024;
      const actualCellSize = 64;
      
      if (sourceCanvas.width > maxDisplaySize) {
        const scale = maxDisplaySize / sourceCanvas.width;
        previewCanvas.width = maxDisplaySize;
        previewCanvas.height = Math.floor(sourceCanvas.height * scale);
      } else {
        previewCanvas.width = sourceCanvas.width;
        previewCanvas.height = sourceCanvas.height;
      }
      
      const previewCtx = previewCanvas.getContext('2d', { alpha: true });
      previewCtx.imageSmoothingEnabled = false;
      previewCtx.imageSmoothingQuality = 'low';
      previewCtx.drawImage(sourceCanvas, 0, 0, previewCanvas.width, previewCanvas.height);
      
      const previewInfo = document.getElementById('previewInfo');
      const displayScale = previewCanvas.width / sourceCanvas.width;
      const previewFontSize = Math.round(48 * fontScale);
      const atlasSizeLabel = isRu ? 'Размер atlas:' : 'Atlas size:';
      const cellSizeLabel = isRu ? 'Размер ячейки:' : 'Cell size:';
      const fixedLabel = isRu ? '(фиксированный)' : '(fixed)';
      const letterSizeLabel = isRu ? 'Размер буквы:' : 'Letter size:';
      const displayLabel = isRu ? 'Отображение:' : 'Display:';
      const scaleLabel = isRu ? '(масштаб ' : '(scale ';
      const percentLabel = '%)';
      const naturalLabel = isRu ? '(натуральный размер)' : '(natural size)';
      previewInfo.innerHTML = 
        '<strong>' + atlasSizeLabel + '</strong> ' + sourceCanvas.width + 'x' + sourceCanvas.height + 'px | ' +
        '<strong>' + cellSizeLabel + '</strong> ' + actualCellSize + 'x' + actualCellSize + 'px ' + fixedLabel + ' | ' +
        '<strong>' + letterSizeLabel + '</strong> ' + previewFontSize + 'px | ' +
        '<strong>Scale:</strong> ' + fontScale.toFixed(2) + ' | ' +
        '<strong>' + displayLabel + '</strong> ' + previewCanvas.width + 'x' + previewCanvas.height + 'px ' +
        (displayScale < 1.0 ? scaleLabel + (displayScale * 100).toFixed(1) + percentLabel : naturalLabel);
      
      const downloadLinks = document.getElementById('downloadLinks');
      const downloadPk3Label = isRu ? '📦 Скачать PK3' : '📦 Download PK3';
      const downloadPngLabel = isRu ? '📥 PNG' : '📥 PNG';
      downloadLinks.innerHTML = 
        '<a href="' + URL.createObjectURL(zipBlob) + '" download="' + fontBaseName + '.pk3" class="download-link">' + downloadPk3Label + '</a>' +
        '<a href="' + size64.canvas.toDataURL('image/png') + '" download="' + fontBaseName + '_64.png" class="download-link">' + downloadPngLabel + '</a>';
      downloadLinks.style.display = 'flex';
      
      previewFont();
      
      if (typeof initPreviewTabs === 'function') {
        initPreviewTabs();
      }
      
      const successMsg = isRu 
        ? 'Шрифт успешно сгенерирован! PK3 архив содержит: 3 размера TGA (16, 32, 64), CFG файл и fonts.cfg. Структура как в sansman.'
        : 'Font generated successfully! PK3 archive contains: 3 TGA sizes (16, 32, 64), CFG file and fonts.cfg.';
      showStatus(successMsg, 'success');
    } catch (error) {
      showStatus((isRu ? 'Ошибка генерации: ' : 'Generation error: ') + error.message, 'error');
      console.error(error);
    }
  }
  
  function previewFont() {
    if (generatedData && generatedData.canvas) {
      const previewCanvas = document.getElementById('previewCanvas');
      const sourceCanvas = generatedData.canvas;
      const maxDisplaySize = 1024;
      
      if (sourceCanvas.width > maxDisplaySize) {
        const scale = maxDisplaySize / sourceCanvas.width;
        previewCanvas.width = maxDisplaySize;
        previewCanvas.height = Math.floor(sourceCanvas.height * scale);
      } else {
        previewCanvas.width = sourceCanvas.width;
        previewCanvas.height = sourceCanvas.height;
      }
      
      const previewCtx = previewCanvas.getContext('2d', { alpha: true });
      previewCtx.imageSmoothingEnabled = false;
      previewCtx.imageSmoothingQuality = 'low';
      previewCtx.drawImage(sourceCanvas, 0, 0, previewCanvas.width, previewCanvas.height);
      
      showStatus(isRu ? 'Предпросмотр: показан точный atlas, который будет сохранен в TGA' : 'Preview: showing exact atlas that will be saved as TGA', 'info');
      return;
    }
    
    if (!currentFont) {
      showStatus(isRu ? 'Пожалуйста, загрузите TTF или OTF файл и сгенерируйте шрифт' : 'Please load a TTF or OTF file and generate the font', 'error');
      return;
    }
    
    showStatus(isRu ? 'Сначала сгенерируйте шрифт, чтобы увидеть предпросмотр TGA atlas' : 'First generate the font to see the TGA atlas preview', 'info');
  }
  
  function displayCharInfoTable(metrics) {
    const tableBody = document.getElementById('charInfoTableBody');
    const section = document.getElementById('charInfoSection');
    if (!tableBody || !section) return;
    
    tableBody.innerHTML = '';
    
    metrics.forEach(metric => {
      const row = document.createElement('tr');
      const char = String.fromCharCode(metric.charCode);
      const hasGlyph = metric.glyph !== null;
      const cfg = metric.cfgData || { x0: 0, y0: 0, x1: 0, w: 0, s1: 0, s2: 0 };
      
      row.style.backgroundColor = hasGlyph ? 'rgba(255,255,255,0.05)' : 'rgba(255,255,255,0.02)';
      row.innerHTML = 
        '<td style="padding: 6px; border: 1px solid rgba(255,255,255,0.25); color: rgba(255,255,255,0.8);">' + metric.charCode + '</td>' +
        '<td style="padding: 6px; border: 1px solid rgba(255,255,255,0.25); font-weight: bold; text-align: center; color: var(--white);">' + char + '</td>' +
        '<td style="padding: 6px; border: 1px solid rgba(255,255,255,0.25); color: rgba(255,255,255,0.8);">' + metric.width + '</td>' +
        '<td style="padding: 6px; border: 1px solid rgba(255,255,255,0.25); color: rgba(255,255,255,0.8);">' + metric.advanceWidth + '</td>' +
        '<td style="padding: 6px; border: 1px solid rgba(255,255,255,0.25); color: rgba(255,255,255,0.8);">' + metric.leftBearing + '</td>' +
        '<td style="padding: 6px; border: 1px solid rgba(255,255,255,0.25); color: rgba(255,255,255,0.8);">' + cfg.x0 + '</td>' +
        '<td style="padding: 6px; border: 1px solid rgba(255,255,255,0.25); color: rgba(255,255,255,0.8);">' + cfg.y0 + '</td>' +
        '<td style="padding: 6px; border: 1px solid rgba(255,255,255,0.25); color: rgba(255,255,255,0.8);">' + cfg.x1 + '</td>' +
        '<td style="padding: 6px; border: 1px solid rgba(255,255,255,0.25); color: rgba(255,255,255,0.8);">' + cfg.w + '</td>' +
        '<td style="padding: 6px; border: 1px solid rgba(255,255,255,0.25); color: rgba(255,255,255,0.8);">' + cfg.s1 + '</td>' +
        '<td style="padding: 6px; border: 1px solid rgba(255,255,255,0.25); color: rgba(255,255,255,0.8);">' + cfg.s2 + '</td>' +
        '<td style="padding: 6px; border: 1px solid rgba(255,255,255,0.25); text-align: center; color: ' + (hasGlyph ? '#00ff00' : '#ff0000') + ';">' + (hasGlyph ? '✓' : '✗') + '</td>';
      tableBody.appendChild(row);
    });
    
    section.style.display = 'block';
  }
  
  const searchInput = document.getElementById('charSearch');
  if (searchInput) {
    searchInput.addEventListener('input', function(e) {
      const searchTerm = e.target.value.toLowerCase();
      const rows = document.querySelectorAll('#charInfoTableBody tr');
      
      rows.forEach(row => {
        const cells = row.querySelectorAll('td');
        if (cells.length >= 2) {
          const code = cells[0].textContent;
          const char = cells[1].textContent;
          const match = code.includes(searchTerm) || char.toLowerCase().includes(searchTerm);
          row.style.display = match ? '' : 'none';
        }
      });
    });
  }
  
  document.getElementById('generateBtn').addEventListener('click', generateFont);
  document.getElementById('previewBtn').addEventListener('click', previewFont);
  
  function parseCFG(cfgContent) {
    const lines = cfgContent.replace(/\\\\n/g, '\\n').split(/\\r?\\n/);
    const fontData = {
      images: [],
      width: 0,
      height: 0,
      charWidth: 0,
      charHeight: 0,
      metrics: {}
    };
    
    let parsingMetrics = false;
    for (let i = 0; i < lines.length; i++) {
      const line = lines[i].trim();
      if (!line || line.startsWith('//')) continue;
      
      const parts = line.split(/\\s+/).filter(p => p.length > 0);
      if (parts[0] === 'img') {
        fontData.images.push({
          path: parts[1].replace(/"/g, ''),
          threshold: parseInt(parts[2]) || 0
        });
      } else if (parts[0] === 'fnt') {
        fontData.width = parseFloat(parts[1]);
        fontData.height = parseFloat(parts[2]);
        fontData.charWidth = parseFloat(parts[3]);
        fontData.charHeight = parseFloat(parts[4]);
        parsingMetrics = true;
      } else if (parsingMetrics && parts.length >= 7) {
        const charCode = parseInt(parts[0]);
        const x0 = parseFloat(parts[1]);
        const y0 = parseFloat(parts[2]);
        const x1 = parseFloat(parts[3]);
        const w = parseFloat(parts[4]);
        const s1 = parseFloat(parts[5]);
        const s2 = parseFloat(parts[6]);
        
        const r_width = 1.0 / fontData.width;
        const r_height = 1.0 / fontData.height;
        
        fontData.metrics[charCode] = {
          tc_mono: [
            x0 * r_width,
            y0 * r_height,
            (x0 + fontData.charWidth) * r_width,
            (y0 + fontData.charHeight) * r_height
          ],
          tc_prop: [
            (x0 + x1) * r_width,
            y0 * r_height,
            (x0 + x1 + w) * r_width,
            (y0 + fontData.charHeight) * r_height
          ],
          width: w / fontData.charWidth,
          space1: s1 / fontData.charWidth,
          space2: (s2 + w) / fontData.charWidth,
          x0: x0,
          y0: y0,
          x1: x1,
          w: w,
          s1: s1,
          s2: s2
        };
      }
    }
    
    return fontData;
  }
  
  function decompressRLE(data, width, height, pixelSize) {
    const result = new Uint8Array(width * height * pixelSize);
    let src = 0;
    let dst = 0;
    
    while (src < data.length && dst < result.length) {
      const packet = data[src++];
      const isRLE = (packet & 0x80) !== 0;
      const count = (packet & 0x7F) + 1;
      
      if (isRLE) {
        for (let i = 0; i < count && dst + pixelSize <= result.length; i++) {
          for (let j = 0; j < pixelSize && src + j < data.length; j++) {
            result[dst++] = data[src + j];
          }
        }
        src += pixelSize;
      } else {
        for (let i = 0; i < count && src + pixelSize <= data.length && dst + pixelSize <= result.length; i++) {
          for (let j = 0; j < pixelSize; j++) {
            result[dst++] = data[src++];
          }
        }
      }
    }
    
    return result;
  }
  
  async function loadTGAImage(blob) {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = function(e) {
        try {
          const buffer = e.target.result;
          const view = new DataView(buffer);
          let offset = 0;
          
          const idLength = view.getUint8(offset++);
          const colorMapType = view.getUint8(offset++);
          const imageType = view.getUint8(offset++);
          offset += 5;
          const xOrigin = view.getUint16(offset, true); offset += 2;
          const yOrigin = view.getUint16(offset, true); offset += 2;
          const width = view.getUint16(offset, true); offset += 2;
          const height = view.getUint16(offset, true); offset += 2;
          const bitsPerPixel = view.getUint8(offset++);
          const imageDescriptor = view.getUint8(offset++);
          
          offset += idLength;
          
          if (bitsPerPixel !== 32) {
            reject(new Error('Only 32-bit TGA supported'));
            return;
          }
          
          const pixelSize = bitsPerPixel / 8;
          let pixelData;
          
          if (imageType === 10) {
            const compressedData = new Uint8Array(buffer, offset);
            pixelData = decompressRLE(compressedData, width, height, pixelSize);
          } else {
            pixelData = new Uint8Array(buffer, offset, width * height * pixelSize);
          }
          
          const canvas = document.createElement('canvas');
          canvas.width = width;
          canvas.height = height;
          const ctx = canvas.getContext('2d');
          const imageData = ctx.createImageData(width, height);
          const data = imageData.data;
          
          for (let y = height - 1; y >= 0; y--) {
            for (let x = 0; x < width; x++) {
              const srcIdx = ((height - 1 - y) * width + x) * pixelSize;
              const dstIdx = (y * width + x) * 4;
              data[dstIdx] = pixelData[srcIdx + 2];
              data[dstIdx + 1] = pixelData[srcIdx + 1];
              data[dstIdx + 2] = pixelData[srcIdx];
              data[dstIdx + 3] = pixelData[srcIdx + 3];
            }
          }
          
          ctx.putImageData(imageData, 0, 0);
          const img = new Image();
          img.onload = () => resolve(img);
          img.onerror = reject;
          img.src = canvas.toDataURL();
        } catch (error) {
          reject(error);
        }
      };
      reader.onerror = reject;
      reader.readAsArrayBuffer(blob);
    });
  }
  
  function renderTestText(fontData, text, charWidth, charHeight, maxWidth, flags, shadowColor, showBounds) {
    const proportional = flags & 1;
    const shadow = flags & 2;
    const hcenter = flags & 4;
    const hright = flags & 8;
    
    const canvas = document.createElement('canvas');
    canvas.width = 800;
    canvas.height = 600;
    const ctx = canvas.getContext('2d');
    ctx.fillStyle = '#000000';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    
    let x = 50;
    let y = 50;
    let ax = x;
    let ay = y;
    const aw = charWidth;
    const ah = charHeight;
    
    const image = fontData.images[0];
    if (!image || !image.img) return canvas;
    
    let totalWidth = 0;
    for (let i = 0; i < text.length; i++) {
      const charCode = text.charCodeAt(i);
      if (charCode === 10) continue;
      const metric = fontData.metrics[charCode];
      if (!metric) continue;
      
      const tc = proportional ? metric.tc_prop : metric.tc_mono;
      const aw1 = proportional ? metric.width * aw : aw;
      const spaceAdd = proportional ? (metric.space1 + metric.space2) * aw : aw;
      totalWidth += spaceAdd;
    }
    
    if (hcenter) {
      ax -= totalWidth / 2;
    } else if (hright) {
      ax -= totalWidth;
    }
    
    x = ax;
    
    for (let i = 0; i < text.length; i++) {
      const charCode = text.charCodeAt(i);
      if (charCode === 10) {
        x = ax;
        y += ah + 5;
        continue;
      }
      
      const metric = fontData.metrics[charCode];
      if (!metric) continue;
      
      const tc = proportional ? metric.tc_prop : metric.tc_mono;
      const aw1 = proportional ? metric.width * aw : aw;
      const spaceAdd = proportional ? (metric.space1 + metric.space2) * aw : aw;
      
      const imgWidth = image.img.width;
      const imgHeight = image.img.height;
      const sx = tc[0] * imgWidth;
      const sy = tc[1] * imgHeight;
      const sw = (tc[2] - tc[0]) * imgWidth;
      const sh = (tc[3] - tc[1]) * imgHeight;
      
      if (shadow && shadowColor) {
        ctx.globalAlpha = shadowColor[3] || 0.5;
        ctx.fillStyle = 'rgba(0,0,0,' + (shadowColor[3] || 0.5) + ')';
        ctx.drawImage(image.img, sx, sy, sw, sh, x + aw / 10, y + aw / 10, aw1, ah);
      }
      
      ctx.globalAlpha = 1.0;
      ctx.drawImage(image.img, sx, sy, sw, sh, x, y, aw1, ah);
      
      if (showBounds) {
        ctx.save();
        ctx.lineWidth = 1;
        ctx.setLineDash([2, 2]);
        
        if (proportional) {
          const cellW = spaceAdd;
          const cellH = ah;
          
          ctx.strokeStyle = 'rgba(255, 0, 0, 0.5)';
          ctx.strokeRect(x, y, cellW, cellH);
          
          if (metric.w !== undefined && metric.s1 !== undefined) {
            const charW = (metric.w / fontData.charWidth) * aw;
            const charX = x + (metric.s1 / fontData.charWidth) * aw;
            
            ctx.strokeStyle = 'rgba(0, 255, 0, 0.5)';
            ctx.strokeRect(charX, y, charW, cellH);
          }
        } else {
          ctx.strokeStyle = 'rgba(255, 0, 0, 0.5)';
          ctx.strokeRect(x, y, aw, ah);
        }
        
        ctx.restore();
      }
      
      x += spaceAdd;
      
      if (x > maxWidth) {
        x = ax;
        y += ah + 5;
      }
    }
    
    return canvas;
  }
  
  async function testFont() {
    if (!generatedData || !generatedData.cfgContent) {
      return;
    }
    
    try {
      const cfgContent = generatedData.cfgContent;
      const fontData = parseCFG(cfgContent);
      
      const testText = document.getElementById('testText') ? document.getElementById('testText').value : 'The quick brown fox jumps over the lazy dog\\n0123456789 !@#$%^&*()';
      const charWidth = document.getElementById('testCharWidth') ? parseFloat(document.getElementById('testCharWidth').value) || 32 : 32;
      const charHeight = document.getElementById('testCharHeight') ? parseFloat(document.getElementById('testCharHeight').value) || 32 : 32;
      const maxWidth = document.getElementById('testMaxWidth') ? parseFloat(document.getElementById('testMaxWidth').value) || 1920 : 1920;
      const proportional = document.getElementById('testProportional') ? document.getElementById('testProportional').checked : true;
      const shadow = document.getElementById('testShadow') ? document.getElementById('testShadow').checked : false;
      const showBounds = document.getElementById('testShowBounds') ? document.getElementById('testShowBounds').checked : true;
      const hcenter = document.getElementById('testHCenter') ? document.getElementById('testHCenter').checked : false;
      const hright = document.getElementById('testHRight') ? document.getElementById('testHRight').checked : false;
      
      let flags = 0;
      if (proportional) flags |= 1;
      if (shadow) flags |= 2;
      if (hcenter) flags |= 4;
      if (hright) flags |= 8;
      
      const shadowColor = shadow ? [0, 0, 0, 0.5] : null;
      
      fontData.images[0] = { path: '', threshold: 0, img: await loadTGAImage(generatedData.tgaBlob64) };
      
      const testCanvas = renderTestText(fontData, testText, charWidth, charHeight, maxWidth, flags, shadowColor, showBounds);
      
      const canvas = document.getElementById('testCanvas');
      if (canvas) {
        const container = canvas.parentElement;
        const containerWidth = container ? container.clientWidth - 20 : maxWidth;
        const scale = Math.min(1, containerWidth / testCanvas.width);
        
        canvas.width = testCanvas.width;
        canvas.height = testCanvas.height;
        canvas.style.width = (testCanvas.width * scale) + 'px';
        canvas.style.height = (testCanvas.height * scale) + 'px';
        
        const ctx = canvas.getContext('2d');
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        ctx.drawImage(testCanvas, 0, 0);
      }
    } catch (error) {
      console.error(error);
    }
  }
  
  function initPreviewTabs() {
    const imgTab = document.getElementById('previewTabImg');
    const quakeTab = document.getElementById('previewTabQuake');
    const imgContainer = document.getElementById('previewImgContainer');
    const quakeContainer = document.getElementById('previewQuakeContainer');
    
    if (!imgTab || !quakeTab || !imgContainer || !quakeContainer) return;
    
    function switchTab(tab) {
      if (tab === 'img') {
        imgTab.style.background = 'var(--red)';
        imgTab.style.borderColor = 'var(--red)';
        imgTab.style.color = 'white';
        quakeTab.style.background = 'transparent';
        quakeTab.style.borderColor = 'var(--border-color)';
        quakeTab.style.color = 'rgba(255,255,255,0.7)';
        imgContainer.style.display = 'block';
        quakeContainer.style.display = 'none';
      } else {
        quakeTab.style.background = 'var(--red)';
        quakeTab.style.borderColor = 'var(--red)';
        quakeTab.style.color = 'white';
        imgTab.style.background = 'transparent';
        imgTab.style.borderColor = 'var(--border-color)';
        imgTab.style.color = 'rgba(255,255,255,0.7)';
        imgContainer.style.display = 'none';
        quakeContainer.style.display = 'block';
        
        if (generatedData && generatedData.cfgContent) {
          testFont();
        }
      }
    }
    
    switchTab('quake');
    
    imgTab.addEventListener('click', () => switchTab('img'));
    quakeTab.addEventListener('click', () => switchTab('quake'));
    
    const testInputs = ['testText', 'testCharWidth', 'testCharHeight', 'testMaxWidth', 'testProportional', 'testShadow', 'testShowBounds'];
    testInputs.forEach(id => {
      const el = document.getElementById(id);
      if (el) {
        el.addEventListener('input', () => {
          if (quakeContainer.style.display !== 'none' && generatedData && generatedData.cfgContent) {
            testFont();
          }
        });
        el.addEventListener('change', () => {
          if (quakeContainer.style.display !== 'none' && generatedData && generatedData.cfgContent) {
            testFont();
          }
        });
      }
    });
  }
  
  const testFontBtn = document.getElementById('testFontBtn');
  if (testFontBtn) {
    testFontBtn.addEventListener('click', testFont);
  }
`;
    
    document.addEventListener('DOMContentLoaded', function() {
        const openFontMakerBtn = document.getElementById('openFontMakerModal');
        const fontMakerModal = document.getElementById('fontMakerModal');
        const closeFontMakerModal = document.getElementById('closeFontMakerModal');
        const fontMakerToolContent = document.getElementById('fontMakerToolContent');
        
        if (!openFontMakerBtn || !fontMakerModal || !closeFontMakerModal || !fontMakerToolContent) {
            return;
        }
        
        // Load opentype.js and JSZip if not already loaded
        function loadDependencies(callback) {
            if (typeof opentype !== 'undefined' && typeof JSZip !== 'undefined') {
                callback();
                return;
            }
            
            let loaded = 0;
            const total = 2;
            
            function checkLoaded() {
                loaded++;
                if (loaded === total) {
                    callback();
                }
            }
            
            if (typeof opentype === 'undefined') {
                const opentypeScript = document.createElement('script');
                opentypeScript.src = 'https://cdn.jsdelivr.net/npm/opentype.js@1.3.4/dist/opentype.min.js';
                opentypeScript.onload = checkLoaded;
                opentypeScript.onerror = checkLoaded;
                document.head.appendChild(opentypeScript);
            } else {
                checkLoaded();
            }
            
            if (typeof JSZip === 'undefined') {
                const jszipScript = document.createElement('script');
                jszipScript.src = 'https://cdn.jsdelivr.net/npm/jszip@3.10.1/dist/jszip.min.js';
                jszipScript.onload = checkLoaded;
                jszipScript.onerror = checkLoaded;
                document.head.appendChild(jszipScript);
            } else {
                checkLoaded();
            }
        }
        
        let isContentLoaded = false;
        
        function loadFontMakerTool() {
            if (isContentLoaded) return;
            
            loadDependencies(function() {
                fontMakerToolContent.className = 'font-maker-tool-root';
                
                const lang = getLanguage();
                fontMakerToolContent.innerHTML = getFontMakerToolHTML(lang);

                setTimeout(() => {
                  const sectionTitles = document.querySelectorAll('.font-maker-tool-app .section-title');
                  sectionTitles.forEach(title => {
                    if (title.classList.contains('collapsed')) {
                      const section = title.closest('.section');
                      if (section) {
                        section.classList.add('collapsed');
                      }
                    }
                    title.addEventListener('click', function() {
                      const section = this.closest('.section');
                      if (section) {
                        section.classList.toggle('collapsed');
                        this.classList.toggle('collapsed');
                      }
                    });
                  });

                  if (window.FontMakerTool && window.FontMakerTool.app && typeof window.FontMakerTool.app.init === 'function') {
                    window.FontMakerTool.app.init();
                  }

                  setTimeout(() => {
                    if (window.FontMakerTool && window.FontMakerTool.preview && typeof window.FontMakerTool.preview.initPreviewTabs === 'function') {
                      window.FontMakerTool.preview.initPreviewTabs();
                    }
                  }, 200);
                }, 100);
                
                isContentLoaded = true;
            });
        }
        
        function openFontMakerModal() {
            loadFontMakerTool();
            fontMakerModal.classList.add('active');
            document.body.style.overflow = 'hidden';
            
            if (history.pushState) {
                history.pushState(null, null, '#fontmaker');
            } else {
                window.location.hash = '#fontmaker';
            }
            
            if (window.ChangeTracker) {
                window.ChangeTracker.resetChanges('fontmaker');
            }
            
            setTimeout(() => {
                const ttfFile = document.getElementById('ttfFile');
                if (ttfFile) {
                    ttfFile.focus();
                } else {
                    fontMakerToolContent.setAttribute('tabindex', '-1');
                    fontMakerToolContent.focus();
                }
            }, 100);
        }
        
        function closeFontMakerModalFunc() {
            fontMakerModal.classList.remove('active');
            document.body.style.overflow = '';
            if (history.pushState) {
                history.pushState(null, null, window.location.pathname + window.location.search);
            } else {
                window.location.hash = '';
            }
            if (window.ChangeTracker) {
                window.ChangeTracker.resetChanges('fontmaker');
            }
        }
        
        openFontMakerBtn.addEventListener('click', function(e) {
            e.preventDefault();
            e.stopPropagation();
            openFontMakerModal();
        });
        
        closeFontMakerModal.addEventListener('click', function() {
            closeFontMakerModalFunc();
        });
        
        var mouseDownOnBackground = false;
        fontMakerModal.addEventListener('mousedown', function(e) {
            mouseDownOnBackground = (e.target === fontMakerModal);
        });
        var handleMouseUp = function(e) {
            if (mouseDownOnBackground) {
                if (e.target === fontMakerModal || !fontMakerModal.contains(e.target)) {
                    closeFontMakerModalFunc();
                }
            }
            mouseDownOnBackground = false;
        };
        fontMakerModal.addEventListener('mouseup', handleMouseUp);
        document.addEventListener('mouseup', handleMouseUp);
        
        document.addEventListener('keydown', function(e) {
            if (e.key === 'Escape' && fontMakerModal.classList.contains('active')) {
                e.preventDefault();
                if (window.ChangeTracker) {
                    window.ChangeTracker.handleEscPress('fontmaker', function() {
                        closeFontMakerModalFunc();
                    });
                } else {
                    closeFontMakerModalFunc();
                }
            }
        });
        
        if (window.location.hash === '#fontmaker') {
            openFontMakerModal();
        }
        
        window.addEventListener('hashchange', function() {
            if (window.location.hash === '#fontmaker') {
                openFontMakerModal();
            } else if (fontMakerModal.classList.contains('active')) {
                closeFontMakerModalFunc();
            }
        });
    });
})();


