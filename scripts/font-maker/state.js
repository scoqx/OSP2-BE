(function(global) {
  'use strict';

  const state = {
    currentFont: null,
    generatedData: null,
    storedCharMetrics: [],
    copyrightText: '',
    isRu: window.location.pathname.includes('/ru/')
  };

  function showStatus(message, type) {
    const statusDiv = document.getElementById('status');
    if (!statusDiv) return;
    statusDiv.className = 'status ' + type;
    statusDiv.textContent = message;
    statusDiv.style.display = 'block';
  }

  function getCharSet() {
    const chars = [];
    for (let i = 32; i <= 126; i++) {
      chars.push(String.fromCharCode(i));
    }
    return chars.join('');
  }

  function createCharSetMap(charSet) {
    const map = new Set();
    for (let i = 0; i < charSet.length; i++) {
      map.add(charSet.charCodeAt(i));
    }
    return map;
  }

  function setCurrentFont(font) {
    state.currentFont = font;
  }

  function setGeneratedData(data) {
    state.generatedData = data;
  }

  function setStoredCharMetrics(metrics) {
    state.storedCharMetrics = metrics || [];
  }

  function setCopyrightText(text) {
    state.copyrightText = text || '';
  }

  global.FontMakerTool = global.FontMakerTool || {};
  global.FontMakerTool.state = {
    showStatus,
    getCharSet,
    createCharSetMap,
    setCurrentFont,
    setGeneratedData,
    setStoredCharMetrics,
    setCopyrightText,
    getCurrentFont: () => state.currentFont,
    getGeneratedData: () => state.generatedData,
    getStoredCharMetrics: () => state.storedCharMetrics,
    getCopyrightText: () => state.copyrightText,
    isRu: () => state.isRu
  };
})(window);








