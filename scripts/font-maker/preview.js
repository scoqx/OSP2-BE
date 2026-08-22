(function(global) {
  'use strict';

  const state = global.FontMakerTool.state;

  function parseCFG(cfgContent) {
    const lines = cfgContent.replace(/\\n/g, '\n').split(/\r?\n/);
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

      const parts = line.split(/\s+/).filter(p => p.length > 0);
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
          void colorMapType; void imageDescriptor; void xOrigin; void yOrigin;
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
      void tc;
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
    const generatedData = state.getGeneratedData();
    if (!generatedData || !generatedData.cfgContent) {
      return;
    }

    try {
      const cfgContent = generatedData.cfgContent;
      const fontData = parseCFG(cfgContent);

      // Render all available symbols in order (as in atlas)
      const codes = Object.keys(fontData.metrics).map(Number).sort((a, b) => a - b);
      const chunked = [];
      for (let i = 0; i < codes.length; i += 16) {
        const slice = codes.slice(i, i + 16).map(c => String.fromCharCode(c)).join('');
        chunked.push(slice);
      }
      const testText = chunked.join('\n');
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
    const quakeTab = document.getElementById('previewTabQuake');
    const quakeContainer = document.getElementById('previewQuakeContainer');

    if (!quakeTab || !quakeContainer) return;

    quakeTab.classList.add('active');
    quakeContainer.style.display = 'block';

    const testInputs = ['testCharWidth', 'testCharHeight', 'testMaxWidth', 'testProportional', 'testShadow', 'testShowBounds'];
    testInputs.forEach(id => {
      const el = document.getElementById(id);
      if (el) {
        el.addEventListener('input', () => {
          const generatedData = state.getGeneratedData();
          if (quakeContainer.style.display !== 'none' && generatedData && generatedData.cfgContent) {
            testFont();
          }
        });
        el.addEventListener('change', () => {
          const generatedData = state.getGeneratedData();
          if (quakeContainer.style.display !== 'none' && generatedData && generatedData.cfgContent) {
            testFont();
          }
        });
      }
    });

    // Run initial test if data already exists
    const generatedData = state.getGeneratedData();
    if (generatedData && generatedData.cfgContent) {
      testFont();
    }
  }

  function bindTestButton() {
    const testFontBtn = document.getElementById('testFontBtn');
    if (testFontBtn) {
      testFontBtn.addEventListener('click', testFont);
    }
  }

  global.FontMakerTool = global.FontMakerTool || {};
  global.FontMakerTool.preview = {
    parseCFG,
    renderTestText,
    testFont,
    initPreviewTabs,
    bindTestButton,
    loadTGAImage
  };
})(window);








