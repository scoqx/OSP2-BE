(function(global) {
  'use strict';

  const state = global.FontMakerTool.state;

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

  function generateFontAtSize(fontSize, cellSize, padding, charSet, charSetMap, resolution, horizontalOffset, verticalOffset) {
    const currentFont = state.getCurrentFont();
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
          const scaleValue = fontSize / currentFont.unitsPerEm;
          const charLeftEdge = padding + metric.leftBearing;
          x1 = Math.max(0, Math.ceil(charLeftEdge));
          w = Math.min(Math.ceil(metric.width), cellSize - x1);
          s1 = Math.max(0, x1);
          s2 = Math.max(0, cellSize - x1 - w);
          void scaleValue; // avoid lint unused
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

  global.FontMakerTool = global.FontMakerTool || {};
  global.FontMakerTool.render = {
    compressRLE,
    writeTGA,
    generateFontAtSize
  };
})(window);








