(function(global) {
  'use strict';

  const state = global.FontMakerTool.state;
  const render = global.FontMakerTool.render;
  const loader = global.FontMakerTool.loader;
  const preview = global.FontMakerTool.preview;
  let demoLoaded = false;

  async function loadDemoFont() {
    if (demoLoaded) return;
    try {
      const cfgResp = await fetch('/assets/q3gfx2/sansman.cfg');
      const tgaResp = await fetch('/assets/q3gfx2/sansman_64.tga');
      if (!cfgResp.ok || !tgaResp.ok) return;

      const cfgContent = await cfgResp.text();
      const tgaBlob64 = await tgaResp.blob();

      const generatedData = {
        cfgContent,
        fontName: 'sansman',
        fontBaseName: 'sansman',
        tgaBlob64,
        tgaBlob32: null,
        tgaBlob16: null,
        zipBlob: null
      };

      state.setGeneratedData(generatedData);

      const fontNameInput = document.getElementById('fontName');
      if (fontNameInput) fontNameInput.value = 'sansman';

      const downloadLinks = document.getElementById('downloadLinks');
      if (downloadLinks) downloadLinks.style.display = 'none';

      demoLoaded = true;
      preview.testFont();
      state.showStatus(state.isRu() ? 'Демо шрифт sansman загружен' : 'Demo font sansman loaded', 'info');
    } catch (e) {
      // silently ignore demo load errors
    }
  }

  function previewFont() {
    // Atlas preview removed; keep stub for compatibility
    return;
  }

  async function generateFont() {
    const currentFont = state.getCurrentFont();
    if (!currentFont) {
      state.showStatus(state.isRu() ? 'Пожалуйста, загрузите TTF или OTF файл' : 'Please load a TTF or OTF file', 'error');
      return;
    }

    state.showStatus(state.isRu() ? 'Генерация шрифта (3 размера)...' : 'Generating font (3 sizes)...', 'info');

    try {
      const padding = parseInt(document.getElementById('padding').value) || 2;
      const fontName = document.getElementById('fontName').value ||
        (currentFont.names.fullName ? currentFont.names.fullName.en : 'CustomFont');
      const charSet = state.getCharSet();
      const charSetMap = state.createCharSetMap(charSet);

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

      state.showStatus((state.isRu() ? 'Генерация размера 64 (буквы: ' : 'Generating size 64 (letters: ') + fontSize64 + 'px)...', 'info');
      const size64 = render.generateFontAtSize(fontSize64, cellSize64, padding, charSet, charSetMap, resolution, horizontalOffset, verticalOffset);

      state.showStatus((state.isRu() ? 'Генерация размера 32 (буквы: ' : 'Generating size 32 (letters: ') + fontSize32 + 'px)...', 'info');
      const size32 = render.generateFontAtSize(fontSize32, cellSize32, padding, charSet, charSetMap, resolution, horizontalOffset, verticalOffset);

      state.showStatus((state.isRu() ? 'Генерация размера 16 (буквы: ' : 'Generating size 16 (letters: ') + fontSize16 + 'px)...', 'info');
      const size16 = render.generateFontAtSize(fontSize16, cellSize16, padding, charSet, charSetMap, resolution, horizontalOffset, verticalOffset);

      const fontBaseName = fontName.toLowerCase().replace(/\s+/g, '_');

      const cfgLines = [];
      const copyrightText = state.getCopyrightText();
      if (copyrightText) {
        const copyrightLines = copyrightText.split(/\r?\n/);
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

      const cfgContent = cfgLines.join('\n') + '\n';
      const fontsCfgContent = '"' + fontName + '" "gfx/2d/' + fontBaseName + '.cfg"\n';

      state.showStatus(state.isRu() ? 'Создание PK3 архива...' : 'Creating PK3 archive...', 'info');
      const zip = new JSZip();

      zip.file('gfx/2d/' + fontBaseName + '_64.tga', size64.tgaBlob);
      zip.file('gfx/2d/' + fontBaseName + '_32.tga', size32.tgaBlob);
      zip.file('gfx/2d/' + fontBaseName + '_16.tga', size16.tgaBlob);
      zip.file('gfx/2d/' + fontBaseName + '.cfg', cfgContent);
      zip.file('fonts.cfg', fontsCfgContent);

      const zipBlob = await zip.generateAsync({ type: 'blob' });

      const generatedData = {
        canvas: size64.canvas, cfgContent: cfgContent, fontsCfgContent: fontsCfgContent,
        zipBlob: zipBlob, fontName: fontName, fontBaseName: fontBaseName,
        charData: size64.charData, charMetrics: size64.charMetrics,
        tgaBlob64: size64.tgaBlob, tgaBlob32: size32.tgaBlob, tgaBlob16: size16.tgaBlob
      };

      state.setGeneratedData(generatedData);
      const downloadLinks = document.getElementById('downloadLinks');
      const downloadPk3Label = state.isRu() ? '📦 Скачать PK3' : '📦 Download PK3';
      const downloadPngLabel = state.isRu() ? '📥 PNG' : '📥 PNG';
      downloadLinks.innerHTML =
        '<a href="' + URL.createObjectURL(zipBlob) + '" download="' + fontBaseName + '.pk3" class="download-link">' + downloadPk3Label + '</a>' +
        '<a href="' + size64.canvas.toDataURL('image/png') + '" download="' + fontBaseName + '_64.png" class="download-link">' + downloadPngLabel + '</a>';
      downloadLinks.style.display = 'flex';

      previewFont();

      if (typeof preview.initPreviewTabs === 'function') {
        preview.initPreviewTabs();
      }

      const successMsg = state.isRu()
        ? 'Шрифт успешно сгенерирован! PK3 архив содержит: 3 размера TGA (16, 32, 64), CFG файл и fonts.cfg. Структура как в sansman.'
        : 'Font generated successfully! PK3 archive contains: 3 TGA sizes (16, 32, 64), CFG file and fonts.cfg.';
      state.showStatus(successMsg, 'success');
    } catch (error) {
      state.showStatus((state.isRu() ? 'Ошибка генерации: ' : 'Generation error: ') + error.message, 'error');
      console.error(error);
    }
  }

  function bindButtons() {
    const generateBtn = document.getElementById('generateBtn');
    if (generateBtn) {
      generateBtn.addEventListener('click', generateFont);
    }
  }

  function init() {
    loader.bindFontFileInput();
    bindButtons();
    if (preview.bindTestButton) {
      preview.bindTestButton();
    }
    loadDemoFont();
  }

  global.FontMakerTool = global.FontMakerTool || {};
  global.FontMakerTool.app = {
    init,
    generateFont,
    previewFont
  };
})(window);








