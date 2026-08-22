// WAV Editor Modal functionality
(function() {
    'use strict';
    
    // Determine language from URL
    function getLanguage() {
        return window.location.pathname.includes('/ru/') ? 'ru' : 'en';
    }
    
    // Simple resampler using OfflineAudioContext
    function resampleAudioBuffer(audioBuffer, targetSampleRate) {
        const sourceSampleRate = audioBuffer.sampleRate;
        const numberOfChannels = audioBuffer.numberOfChannels;
        const length = Math.round(audioBuffer.length * targetSampleRate / sourceSampleRate);
        
        const offlineContext = new OfflineAudioContext(numberOfChannels, length, targetSampleRate);
        const source = offlineContext.createBufferSource();
        source.buffer = audioBuffer;
        source.connect(offlineContext.destination);
        source.start(0);
        
        return offlineContext.startRendering();
    }
    
    // AudioBuffer to WAV converter (inline implementation)
    function audioBufferToWav(buffer, opt) {
        opt = opt || {};
        
        const numChannels = buffer.numberOfChannels;
        const sampleRate = buffer.sampleRate;
        const format = opt.float32 ? 3 : 1;
        const bitDepth = format === 3 ? 32 : 16;
        
        let result;
        if (numChannels === 2) {
            result = interleave(buffer.getChannelData(0), buffer.getChannelData(1));
        } else {
            result = buffer.getChannelData(0);
        }
        
        return encodeWAV(result, format, sampleRate, numChannels, bitDepth);
    }
    
    function interleave(inputL, inputR) {
        const length = inputL.length + inputR.length;
        const result = new Float32Array(length);
        
        let index = 0;
        let inputIndex = 0;
        
        while (index < length) {
            result[index++] = inputL[inputIndex];
            result[index++] = inputR[inputIndex];
            inputIndex++;
        }
        return result;
    }
    
    function encodeWAV(samples, format, sampleRate, numChannels, bitDepth) {
        const bytesPerSample = bitDepth / 8;
        const blockAlign = numChannels * bytesPerSample;
        
        const buffer = new ArrayBuffer(44 + samples.length * bytesPerSample);
        const view = new DataView(buffer);
        
        /* RIFF identifier */
        writeString(view, 0, 'RIFF');
        /* RIFF chunk length */
        view.setUint32(4, 36 + samples.length * bytesPerSample, true);
        /* RIFF type */
        writeString(view, 8, 'WAVE');
        /* format chunk identifier */
        writeString(view, 12, 'fmt ');
        /* format chunk length */
        view.setUint32(16, 16, true);
        /* sample format (raw) */
        view.setUint16(20, format, true);
        /* channel count */
        view.setUint16(22, numChannels, true);
        /* sample rate */
        view.setUint32(24, sampleRate, true);
        /* byte rate (sample rate * block align) */
        view.setUint32(28, sampleRate * blockAlign, true);
        /* block align (channel count * bytes per sample) */
        view.setUint16(32, blockAlign, true);
        /* bits per sample */
        view.setUint16(34, bitDepth, true);
        /* data chunk identifier */
        writeString(view, 36, 'data');
        /* data chunk length */
        view.setUint32(40, samples.length * bytesPerSample, true);
        if (format === 1) { // Raw PCM
            floatTo16BitPCM(view, 44, samples);
        } else {
            writeFloat32(view, 44, samples);
        }
        
        return buffer;
    }
    
    function writeFloat32(output, offset, input) {
        for (let i = 0; i < input.length; i++, offset += 4) {
            output.setFloat32(offset, input[i], true);
        }
    }
    
    function floatTo16BitPCM(output, offset, input) {
        for (let i = 0; i < input.length; i++, offset += 2) {
            const s = Math.max(-1, Math.min(1, input[i]));
            output.setInt16(offset, s < 0 ? s * 0x8000 : s * 0x7FFF, true);
        }
    }
    
    function writeString(view, offset, string) {
        for (let i = 0; i < string.length; i++) {
            view.setUint8(offset + i, string.charCodeAt(i));
        }
    }
    
    // Convert stereo to mono
    function convertToMono(audioBuffer) {
        if (audioBuffer.numberOfChannels === 1) {
            return audioBuffer;
        }
        
        const numberOfChannels = audioBuffer.numberOfChannels;
        const length = audioBuffer.length;
        const sampleRate = audioBuffer.sampleRate;
        
        const monoBuffer = new AudioBuffer({
            length: length,
            numberOfChannels: 1,
            sampleRate: sampleRate
        });
        
        const monoData = monoBuffer.getChannelData(0);
        
        // Average all channels
        for (let i = 0; i < length; i++) {
            let sum = 0;
            for (let ch = 0; ch < numberOfChannels; ch++) {
                sum += audioBuffer.getChannelData(ch)[i];
            }
            monoData[i] = sum / numberOfChannels;
        }
        
        return monoBuffer;
    }
    
    // Change volume of audio buffer
    function changeVolume(audioBuffer, volumePercent) {
        const volume = volumePercent / 100; // Convert percentage to multiplier (0.0 to 2.0)
        const numberOfChannels = audioBuffer.numberOfChannels;
        const length = audioBuffer.length;
        const sampleRate = audioBuffer.sampleRate;
        
        const newBuffer = new AudioBuffer({
            length: length,
            numberOfChannels: numberOfChannels,
            sampleRate: sampleRate
        });
        
        for (let ch = 0; ch < numberOfChannels; ch++) {
            const sourceData = audioBuffer.getChannelData(ch);
            const targetData = newBuffer.getChannelData(ch);
            
            for (let i = 0; i < length; i++) {
                // Apply volume and clamp to prevent clipping
                targetData[i] = Math.max(-1, Math.min(1, sourceData[i] * volume));
            }
        }
        
        return newBuffer;
    }
    
    // Extract audio segment
    function extractSegment(audioBuffer, startTime, endTime) {
        const sampleRate = audioBuffer.sampleRate;
        const startSample = Math.floor(startTime * sampleRate);
        const endSample = Math.floor(endTime * sampleRate);
        const length = endSample - startSample;
        
        const numberOfChannels = audioBuffer.numberOfChannels;
        const newBuffer = new AudioBuffer({
            length: length,
            numberOfChannels: numberOfChannels,
            sampleRate: sampleRate
        });
        
        for (let ch = 0; ch < numberOfChannels; ch++) {
            const sourceData = audioBuffer.getChannelData(ch);
            const targetData = newBuffer.getChannelData(ch);
            for (let i = 0; i < length; i++) {
                targetData[i] = sourceData[startSample + i];
            }
        }
        
        return newBuffer;
    }
    
    // Split audio segment into parts
    function splitSegment(audioBuffer, startTime, endTime, numParts) {
        const duration = endTime - startTime;
        const partDuration = duration / numParts;
        const parts = [];
        
        for (let i = 0; i < numParts; i++) {
            const partStart = startTime + (i * partDuration);
            const partEnd = startTime + ((i + 1) * partDuration);
            const part = extractSegment(audioBuffer, partStart, partEnd);
            parts.push(part);
        }
        
        return parts;
    }
    
    function cloneAudioBuffer(buffer) {
        if (!buffer || !audioContext) return null;
        const clone = audioContext.createBuffer(
            buffer.numberOfChannels,
            buffer.length,
            buffer.sampleRate
        );
        for (let ch = 0; ch < buffer.numberOfChannels; ch++) {
            clone.getChannelData(ch).set(buffer.getChannelData(ch));
        }
        return clone;
    }
    
    // Apply fade in/out to audio buffer
    function applyFade(audioBuffer, fadeInDuration, fadeOutDuration) {
        const sampleRate = audioBuffer.sampleRate;
        const fadeInSamples = Math.floor(fadeInDuration * sampleRate);
        const fadeOutSamples = Math.floor(fadeOutDuration * sampleRate);
        const length = audioBuffer.length;
        const numberOfChannels = audioBuffer.numberOfChannels;
        
        const newBuffer = new AudioBuffer({
            length: length,
            numberOfChannels: numberOfChannels,
            sampleRate: sampleRate
        });
        
        for (let ch = 0; ch < numberOfChannels; ch++) {
            const sourceData = audioBuffer.getChannelData(ch);
            const targetData = newBuffer.getChannelData(ch);
            
            for (let i = 0; i < length; i++) {
                let gain = 1.0;
                
                // Fade in
                if (i < fadeInSamples && fadeInSamples > 0) {
                    gain *= i / fadeInSamples;
                }
                
                // Fade out
                if (i >= length - fadeOutSamples && fadeOutSamples > 0) {
                    const fadeOutProgress = (length - i) / fadeOutSamples;
                    gain *= fadeOutProgress;
                }
                
                targetData[i] = sourceData[i] * gain;
            }
        }
        
        return newBuffer;
    }
    
    // Concatenate multiple audio buffers
    function concatenateBuffers(buffers) {
        if (buffers.length === 0) return null;
        if (buffers.length === 1) return buffers[0];
        
        const sampleRate = buffers[0].sampleRate;
        const numberOfChannels = buffers[0].numberOfChannels;
        
        // Calculate total length
        let totalLength = 0;
        for (const buffer of buffers) {
            if (buffer.sampleRate !== sampleRate || buffer.numberOfChannels !== numberOfChannels) {
                throw new Error('All buffers must have the same sample rate and number of channels');
            }
            totalLength += buffer.length;
        }
        
        const resultBuffer = new AudioBuffer({
            length: totalLength,
            numberOfChannels: numberOfChannels,
            sampleRate: sampleRate
        });
        
        let offset = 0;
        for (const buffer of buffers) {
            for (let ch = 0; ch < numberOfChannels; ch++) {
                const sourceData = buffer.getChannelData(ch);
                const targetData = resultBuffer.getChannelData(ch);
                for (let i = 0; i < sourceData.length; i++) {
                    targetData[offset + i] = sourceData[i];
                }
            }
            offset += buffer.length;
        }
        
        return resultBuffer;
    }
    
    // Change pitch of audio buffer using playbackRate
    // pitchShift: positive values = higher pitch, negative = lower pitch
    // pitchShift is in semitones (e.g., 1 = one semitone up, -1 = one semitone down)
    // Note: This changes both pitch and speed. For pitch-only change, time-stretching would be needed.
    async function changePitch(audioBuffer, pitchShift) {
        // Convert semitones to playback rate multiplier
        // Each semitone is a factor of 2^(1/12) ≈ 1.059463
        const playbackRate = Math.pow(2, pitchShift / 12);
        
        const numberOfChannels = audioBuffer.numberOfChannels;
        const sampleRate = audioBuffer.sampleRate;
        const originalLength = audioBuffer.length;
        
        // Calculate new length based on playback rate
        const newLength = Math.ceil(originalLength / playbackRate);
        
        // Create offline context
        const offlineContext = new OfflineAudioContext(
            numberOfChannels,
            newLength,
            sampleRate
        );
        
        const source = offlineContext.createBufferSource();
        source.buffer = audioBuffer;
        source.playbackRate.value = playbackRate;
        source.connect(offlineContext.destination);
        source.start(0);
        
        const renderedBuffer = await offlineContext.startRendering();
        
        // If we want to maintain original duration, we need to resample
        // For now, we'll keep the new duration (which changes speed)
        // To maintain duration, we'd need time-stretching algorithm
        return renderedBuffer;
    }
    
    // WAV Editor tool HTML content
    function getWavEditorToolHTML(lang) {
        const isRu = lang === 'ru';
        
        return `
<style>
  .wav-editor-root {
    color-scheme: dark;
    --black: #000000;
    --white: #FFFFFF;
    --accent: #8B008B;
  }
  
  .wav-editor-root * {
    box-sizing: border-box;
    margin: 0;
    padding: 0;
  }
  
  .wav-editor-root {
    background: var(--black);
    color: var(--white);
    font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
  }
  
  .wav-editor-app {
    width: 100%;
    max-width: 1180px;
    background: var(--black);
    border-radius: 18px;
    padding: 0px 16px 12px;
  }
  
  .wav-editor-app h1 {
    font-size: 1.1rem;
    text-transform: uppercase;
    letter-spacing: .12em;
    margin-top: 0;
    margin-bottom: 2px;
  }
  
  .wav-editor-app h1 span {
    color: var(--accent);
  }
  
  .wav-editor-app .subtitle {
    font-size: .75rem;
    color: rgba(255, 255, 255, 0.7);
    margin-bottom: 12px;
  }
  
  .wav-editor-controls {
    display: flex;
    flex-direction: column;
    gap: 10px;
    margin-bottom: 12px;
  }
  
  .wav-editor-control-row {
    display: flex;
    gap: 8px;
    align-items: center;
    flex-wrap: wrap;
  }
  
  .wav-editor-control-row label {
    font-size: 0.8rem;
    color: rgba(255, 255, 255, 0.8);
    min-width: 90px;
  }
  
  .wav-editor-control-row input[type="file"] {
    display: none;
  }
  
  .wav-editor-btn {
    padding: 6px 12px;
    background-color: rgba(255, 255, 255, 0.1);
    border: 2px solid rgba(255, 255, 255, 0.3);
    border-radius: 6px;
    color: var(--white);
    cursor: pointer;
    font-size: 0.85rem;
    font-weight: bold;
    transition: all 0.2s;
  }
  
  .wav-editor-btn:hover {
    background-color: rgba(255, 255, 255, 0.2);
    border-color: var(--white);
  }
  
  .wav-editor-btn:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }
  
  .wav-editor-btn-primary {
    background-color: var(--white);
    border-color: var(--white);
    color: var(--black);
  }
  
  .wav-editor-btn-primary:hover:not(:disabled) {
    background-color: rgba(255, 255, 255, 0.9);
    box-shadow: 0 0 15px rgba(255, 255, 255, 0.5);
  }
  
  .wav-editor-btn-success {
    background-color: var(--white);
    border-color: var(--white);
    color: var(--black);
  }
  
  .wav-editor-btn-success:hover:not(:disabled) {
    background-color: rgba(255, 255, 255, 0.9);
    box-shadow: 0 0 15px rgba(255, 255, 255, 0.5);
  }
  
  .wav-editor-waveform-container {
    position: relative;
    width: 100%;
    height: 120px;
    background-color: rgba(255, 255, 255, 0.05);
    border: 2px solid var(--accent);
    border-radius: 6px;
    margin-bottom: 10px;
    overflow: hidden;
  }
  
  .wav-editor-waveform {
    width: 100%;
    height: 100%;
    touch-action: none;
  }
  
  .wav-editor-selection {
    position: absolute;
    top: 0;
    bottom: 0;
    background-color: rgba(139, 0, 139, 0.2);
    border-left: 2px solid var(--accent);
    border-right: 2px solid var(--accent);
    pointer-events: none;
    display: none;
  }
  
  .wav-editor-split-marker {
    position: absolute;
    top: 0;
    bottom: 0;
    width: 3px;
    background-color: rgba(255, 255, 0, 0.8);
    cursor: ew-resize;
    z-index: 10;
    pointer-events: auto;
  }
  
  .wav-editor-split-marker:hover {
    background-color: rgba(255, 255, 0, 1);
    width: 4px;
  }
  
  .wav-editor-split-marker.dragging {
    background-color: rgba(255, 255, 0, 1);
    width: 4px;
  }
  
  .wav-editor-info {
    padding: 8px 10px;
    background-color: rgba(255, 255, 255, 0.05);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 6px;
    margin-bottom: 10px;
    font-size: 0.75rem;
    color: rgba(255, 255, 255, 0.7);
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 4px 16px;
  }
  
  .wav-editor-info-item {
    margin-bottom: 0;
  }
  
  .wav-editor-info strong {
    color: var(--white);
  }
  
  .wav-editor-time-controls {
    display: flex;
    gap: 12px;
    align-items: center;
    flex-wrap: wrap;
  }
  
  .wav-editor-time-input {
    padding: 4px 8px;
    background-color: rgba(255, 255, 255, 0.1);
    border: 1px solid rgba(255, 255, 255, 0.3);
    border-radius: 4px;
    color: var(--white);
    font-size: 0.8rem;
    width: 80px;
  }
  
  .wav-editor-time-input:focus {
    outline: none;
    border-color: var(--white);
    background-color: rgba(255, 255, 255, 0.15);
  }
  
  .wav-editor-status {
    padding: 8px 10px;
    border-radius: 6px;
    margin-top: 10px;
    font-size: 0.8rem;
    display: none;
  }
  
  .wav-editor-status.success {
    background-color: rgba(255, 255, 255, 0.1);
    border: 1px solid rgba(255, 255, 255, 0.3);
    color: var(--white);
    display: block;
  }
  
  .wav-editor-status.error {
    background-color: rgba(255, 255, 255, 0.2);
    border: 1px solid rgba(255, 255, 255, 0.5);
    color: var(--white);
    display: block;
  }
  
  .wav-editor-status.info {
    background-color: rgba(255, 255, 255, 0.1);
    border: 1px solid rgba(255, 255, 255, 0.3);
    color: var(--white);
    display: block;
  }
  
  .wav-editor-playback-controls {
    display: flex;
    gap: 10px;
    align-items: center;
  }
  
  .wav-editor-playback-btn {
    padding: 4px 10px;
    background-color: rgba(255, 255, 255, 0.1);
    border: 1px solid rgba(255, 255, 255, 0.3);
    border-radius: 4px;
    color: var(--white);
    cursor: pointer;
    font-size: 0.8rem;
    transition: all 0.2s;
  }
  
  .wav-editor-playback-btn:hover {
    background-color: rgba(255, 255, 255, 0.2);
    border-color: var(--white);
  }
  
  .wav-editor-playback-btn:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }
  
  .wav-editor-export-options {
    display: flex;
    flex-direction: column;
    gap: 8px;
    padding: 10px 12px;
    background-color: rgba(255, 255, 255, 0.05);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 6px;
    margin-top: 10px;
  }
  
  .wav-editor-export-options h3 {
    font-size: 0.9rem;
    margin-bottom: 6px;
    color: var(--white);
    display: flex;
    align-items: center;
    gap: 12px;
  }
  
  .wav-editor-pk3-checkbox-wrapper {
    display: flex;
    align-items: center;
    gap: 6px;
    margin-left: auto;
  }
  
  .wav-editor-pk3-checkbox-wrapper input[type="checkbox"] {
    width: 16px;
    height: 16px;
    cursor: pointer;
    accent-color: var(--white);
    filter: brightness(0) invert(1);
  }
  
  .wav-editor-pk3-checkbox-wrapper label {
    font-size: 0.8rem;
    color: rgba(255, 255, 255, 0.8);
    cursor: pointer;
    margin: 0;
    min-width: auto;
  }
  
  .wav-editor-export-option {
    display: flex;
    align-items: center;
    gap: 8px;
  }
  
  .wav-editor-export-option input[type="checkbox"] {
    width: 16px;
    height: 16px;
    cursor: pointer;
    accent-color: var(--white);
    filter: brightness(0) invert(1);
  }
  
  .wav-editor-export-option input[type="checkbox"]:checked {
    filter: brightness(0) invert(1);
  }
  
  .wav-editor-export-option label {
    font-size: 0.8rem;
    color: rgba(255, 255, 255, 0.8);
    cursor: pointer;
  }
  
  .wav-editor-volume-control {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 10px;
    background-color: rgba(255, 255, 255, 0.05);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 6px;
    margin-bottom: 10px;
  }
  
  .wav-editor-volume-control label {
    font-size: 0.8rem;
    color: rgba(255, 255, 255, 0.8);
    min-width: 70px;
  }
  
  .wav-editor-volume-slider {
    flex: 1;
    height: 6px;
    background-color: rgba(255, 255, 255, 0.2);
    border-radius: 3px;
    outline: none;
    -webkit-appearance: none;
  }
  
  .wav-editor-volume-slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 16px;
    height: 16px;
    background-color: var(--white);
    border-radius: 50%;
    cursor: pointer;
    transition: all 0.2s;
  }
  
  .wav-editor-volume-slider::-webkit-slider-thumb:hover {
    background-color: rgba(255, 255, 255, 0.9);
    transform: scale(1.2);
  }
  
  .wav-editor-volume-slider::-moz-range-thumb {
    width: 16px;
    height: 16px;
    background-color: var(--white);
    border-radius: 50%;
    cursor: pointer;
    border: none;
    transition: all 0.2s;
  }
  
  .wav-editor-volume-slider::-moz-range-thumb:hover {
    background-color: rgba(255, 255, 255, 0.9);
    transform: scale(1.2);
  }
  
  .wav-editor-volume-value {
    font-size: 0.8rem;
    color: var(--white);
    min-width: 40px;
    text-align: right;
    font-weight: bold;
  }
  
  .wav-editor-volume-btn {
    padding: 4px 10px;
    background-color: rgba(255, 255, 255, 0.1);
    border: 1px solid rgba(255, 255, 255, 0.3);
    border-radius: 4px;
    color: var(--white);
    cursor: pointer;
    font-size: 0.8rem;
    transition: all 0.2s;
  }
  
  .wav-editor-volume-btn:hover {
    background-color: rgba(255, 255, 255, 0.2);
    border-color: var(--white);
  }
  
  .wav-editor-filename-input {
    padding: 4px 8px;
    background-color: rgba(255, 255, 255, 0.1);
    border: 1px solid rgba(255, 255, 255, 0.3);
    border-radius: 4px;
    color: var(--white);
    font-size: 0.8rem;
    flex: 1;
    min-width: 150px;
  }
  
  .wav-editor-filename-input:focus {
    outline: none;
    border-color: var(--accent);
    background-color: rgba(255, 255, 255, 0.15);
  }
  
  .wav-editor-filename-control {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 8px;
  }
  
  .wav-editor-filename-control label {
    font-size: 0.8rem;
    color: rgba(255, 255, 255, 0.8);
    min-width: 90px;
  }
  
  .wav-editor-pk3-fields {
    display: flex;
    flex-direction: column;
    gap: 8px;
    margin-top: 8px;
    padding-left: 24px;
    min-height: 80px;
    max-height: 80px;
    overflow: hidden;
    opacity: 0;
    visibility: hidden;
    transition: opacity 0.2s ease, visibility 0.2s ease;
  }
  
  .wav-editor-pk3-fields.visible {
    opacity: 1;
    visibility: visible;
  }
  
  .wav-editor-files-list {
    margin-bottom: 12px;
    max-height: 150px;
    overflow-y: auto;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 6px;
    padding: 8px;
    background-color: rgba(255, 255, 255, 0.02);
  }
  
  .wav-editor-file-item {
    padding: 6px 10px;
    margin-bottom: 4px;
    background-color: rgba(255, 255, 255, 0.05);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 4px;
    cursor: pointer;
    transition: all 0.2s;
    display: flex;
    align-items: center;
    justify-content: space-between;
    font-size: 0.8rem;
  }
  
  .wav-editor-file-item:hover {
    background-color: rgba(255, 255, 255, 0.1);
    border-color: var(--accent);
  }
  
  .wav-editor-file-item.active {
    background-color: rgba(139, 0, 139, 0.2);
    border-color: var(--accent);
  }
  
  .wav-editor-file-item-name {
    flex: 1;
    color: var(--white);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  
  .wav-editor-file-item-remove {
    padding: 2px 6px;
    background-color: rgba(255, 255, 255, 0.2);
    border: 1px solid rgba(255, 255, 255, 0.5);
    border-radius: 4px;
    color: var(--white);
    cursor: pointer;
    font-size: 0.75rem;
    margin-left: 8px;
    transition: all 0.2s;
  }
  
  .wav-editor-file-item-remove:hover {
    background-color: rgba(255, 255, 255, 0.3);
    border-color: rgba(255, 255, 255, 0.7);
  }

  .wav-editor-app .tool-credits {
    margin-top: 10px;
    padding-top: 6px;
    border-top: 1px solid rgba(255, 255, 255, 0.1);
    text-align: center;
    font-size: .7rem;
    color: rgba(255, 255, 255, 0.5);
  }
</style>

<div class="wav-editor-root">
  <div class="wav-editor-app">
    <h1><span>WAV</span> EDITOR</h1>
    <p class="subtitle">
      ${isRu 
        ? 'Редактор аудио и конвертер в формат WAV для Quake 3 Arena'
        : 'Audio editor and converter to WAV format for Quake 3 Arena'}
    </p>
    
    <div class="wav-editor-controls">
      <div class="wav-editor-control-row">
        <label>${isRu ? 'Аудио файлы:' : 'Audio files:'}</label>
        <input type="file" id="audioFileInput" accept="audio/*" multiple />
        <button class="wav-editor-btn" id="loadAudioBtn">${isRu ? 'Загрузить' : 'Load'}</button>
      </div>
      <div class="wav-editor-files-list" id="filesList" style="display: none;"></div>
    </div>
    
    <div class="wav-editor-info" id="audioInfo" style="display: none;">
      <div class="wav-editor-info-item"><strong>${isRu ? 'Файл:' : 'File:'}</strong> <span id="fileName"></span></div>
      <div class="wav-editor-info-item"><strong>${isRu ? 'Длительность:' : 'Duration:'}</strong> <span id="duration"></span></div>
      <div class="wav-editor-info-item"><strong>${isRu ? 'Частота:' : 'Sample Rate:'}</strong> <span id="sampleRate"></span></div>
      <div class="wav-editor-info-item"><strong>${isRu ? 'Каналы:' : 'Channels:'}</strong> <span id="channels"></span></div>
    </div>
    
    <div class="wav-editor-waveform-container" id="waveformContainer" style="display: none;">
      <canvas class="wav-editor-waveform" id="waveformCanvas"></canvas>
      <div class="wav-editor-selection" id="selection"></div>
    </div>
    
    <div class="wav-editor-volume-control" id="volumeControl" style="display: none;">
      <label>${isRu ? 'Громкость:' : 'Volume:'}</label>
      <input type="range" id="volumeSlider" class="wav-editor-volume-slider" min="0" max="200" value="100" step="1" />
      <span class="wav-editor-volume-value" id="volumeValue">100%</span>
      <button class="wav-editor-volume-btn" id="applyVolumeBtn">${isRu ? 'Применить' : 'Apply'}</button>
    </div>
    
    <div class="wav-editor-controls" id="trimControls" style="display: none;">
      <div class="wav-editor-control-row">
        <label>${isRu ? 'Начало (сек):' : 'Start (sec):'}</label>
        <input type="number" id="startTime" class="wav-editor-time-input" value="0" min="0" step="0.1" />
        <label>${isRu ? 'Конец (сек):' : 'End (sec):'}</label>
        <input type="number" id="endTime" class="wav-editor-time-input" value="0" min="0" step="0.1" />
        <button type="button" class="wav-editor-btn" id="selectAllBtn">${isRu ? 'Выделить всё' : 'Select All'}</button>
      </div>
      
      <div class="wav-editor-control-row">
        <div class="wav-editor-playback-controls">
          <button type="button" class="wav-editor-playback-btn" id="playBtn">${isRu ? '▶' : '▶'}</button>
          <button type="button" class="wav-editor-playback-btn" id="pauseBtn" disabled>${isRu ? '⏸' : '⏸'}</button>
          <button type="button" class="wav-editor-playback-btn" id="stopBtn" disabled>${isRu ? '⏹' : '⏹'}</button>
        </div>
      </div>
      
      <div class="wav-editor-control-row">
        <button type="button" class="wav-editor-btn" id="trimBtn">${isRu ? 'Обрезать' : 'Trim'}</button>
      </div>
      
      <div class="wav-editor-control-row">
        <button type="button" class="wav-editor-btn" id="resetBtn">${isRu ? 'Сбросить' : 'Reset'}</button>
        <button type="button" class="wav-editor-btn" id="undoBtn" disabled title="${isRu ? 'Отменить' : 'Undo'}">${isRu ? '← Назад' : '← Undo'}</button>
        <button type="button" class="wav-editor-btn" id="redoBtn" disabled title="${isRu ? 'Вернуть' : 'Redo'}">${isRu ? 'Вперёд →' : 'Redo →'}</button>
      </div>
      
      <div class="wav-editor-split-controls" style="margin-top: 12px; padding-top: 12px; border-top: 1px solid rgba(255, 255, 255, 0.2);">
        <div class="wav-editor-control-row">
          <label>${isRu ? 'Количество частей:' : 'Number of parts:'}</label>
          <input type="number" id="splitPartsCount" class="wav-editor-time-input" value="4" min="2" max="20" step="1" style="width: 80px;" />
          <button class="wav-editor-btn" id="createMarkersBtn">${isRu ? 'Создать метки' : 'Create Markers'}</button>
          <button class="wav-editor-btn" id="splitBtn" style="display: none;">${isRu ? 'SPLIT' : 'SPLIT'}</button>
        </div>
        
        <div class="wav-editor-control-row" style="margin-top: 8px;">
          <label>${isRu ? 'Затухание (мс):' : 'Fade (ms):'}</label>
          <input type="number" id="fadeDuration" class="wav-editor-time-input" value="50" min="0" max="1000" step="10" style="width: 80px;" />
          <button class="wav-editor-btn" id="previewFadeBtn">${isRu ? 'Предпрослушать' : 'Preview'}</button>
        </div>
        
        <div id="splitPartsNames" style="display: none; margin-top: 12px;">
          <div style="font-size: 0.85rem; color: rgba(255, 255, 255, 0.8); margin-bottom: 8px;">${isRu ? 'Имена частей:' : 'Part names:'}</div>
        </div>
      </div>
      
      <div class="wav-editor-pitch-controls" style="margin-top: 12px; padding-top: 12px; border-top: 1px solid rgba(255, 255, 255, 0.2);">
        <div class="wav-editor-control-row">
          <label>${isRu ? 'Питч:' : 'Pitch:'}</label>
          <button class="wav-editor-btn" id="pitchBtn">${isRu ? 'Питч' : 'Pitch'}</button>
        </div>
        
        <div id="pitchOptions" style="display: none; margin-top: 8px;">
          <div class="wav-editor-control-row">
            <label>${isRu ? 'Количество файлов:' : 'Number of files:'}</label>
            <input type="number" id="pitchFilesCount" class="wav-editor-time-input" value="4" min="1" max="20" step="1" style="width: 80px;" />
          </div>
          
          <div class="wav-editor-control-row" style="margin-top: 8px;">
            <label title="${isRu ? 'Последний файл получает этот сдвиг от оригинала; остальные — равномерно между 0 и этим значением' : 'Last file gets this shift from original; others are evenly spaced from 0 to this value'}">${isRu ? 'Финальный питч (полутонов):' : 'Final pitch (semitones):'}</label>
            <input type="number" id="pitchTarget" class="wav-editor-time-input" value="6" min="0.1" max="36" step="0.1" style="width: 80px;" title="${isRu ? 'Сдвиг последнего файла от оригинала' : 'Last file shift from original'}" />
          </div>
          
          <div class="wav-editor-control-row" style="margin-top: 8px;">
            <label>${isRu ? 'Направление:' : 'Direction:'}</label>
            <select id="pitchDirection" class="wav-editor-time-input" style="width: 120px;">
              <option value="up">${isRu ? 'Вверх' : 'Up'}</option>
              <option value="down">${isRu ? 'Вниз' : 'Down'}</option>
            </select>
          </div>
          
          <div class="wav-editor-control-row" style="margin-top: 8px;">
            <button class="wav-editor-btn wav-editor-btn-primary" id="generatePitchBtn">${isRu ? 'Сгенерировать' : 'Generate'}</button>
            <button class="wav-editor-btn" id="cancelPitchBtn">${isRu ? 'Отмена' : 'Cancel'}</button>
          </div>
          
          <div id="pitchFilesNames" style="display: none; margin-top: 12px;">
            <div style="font-size: 0.85rem; color: rgba(255, 255, 255, 0.8); margin-bottom: 8px;">${isRu ? 'Имена файлов:' : 'File names:'}</div>
          </div>
          
          <div id="pitchDownloadSection" style="display: none; margin-top: 12px;">
            <div class="wav-editor-filename-control" style="margin-top: 8px;">
              <label>${isRu ? 'Имя PK3:' : 'PK3 name:'}</label>
              <input type="text" id="pitchPk3Filename" class="wav-editor-filename-input" value="pitch_sounds.pk3" placeholder="sounds.pk3" />
            </div>
            <div class="wav-editor-filename-control" style="margin-top: 8px;">
              <label>${isRu ? 'Путь в PK3:' : 'Path in PK3:'}</label>
              <input type="text" id="pitchPk3Path" class="wav-editor-filename-input" value="sound/" placeholder="sound/" />
            </div>
            
            <div class="wav-editor-control-row" style="margin-top: 8px;">
              <button class="wav-editor-btn wav-editor-btn-success" id="downloadPitchBtn">${isRu ? 'Скачать все' : 'Download all'}</button>
            </div>
          </div>
        </div>
      </div>
    </div>
    
    <div class="wav-editor-export-options" id="exportOptions" style="display: none;">
      <h3>
        ${isRu ? 'Экспорт для Quake 3:' : 'Export for Quake 3:'}
        <div class="wav-editor-pk3-checkbox-wrapper">
          <input type="checkbox" id="exportToPk3" />
          <label for="exportToPk3">${isRu ? 'PK3' : 'PK3'}</label>
        </div>
      </h3>
      <div class="wav-editor-filename-control">
        <label>${isRu ? 'Имя файла:' : 'Filename:'}</label>
        <input type="text" id="exportFilename" class="wav-editor-filename-input" value="quake3_sound.wav" placeholder="sound.wav" />
      </div>
      <div class="wav-editor-pk3-fields" id="pk3Fields">
        <div class="wav-editor-filename-control">
          <label>${isRu ? 'Имя PK3:' : 'PK3 name:'}</label>
          <input type="text" id="pk3Filename" class="wav-editor-filename-input" value="sounds.pk3" placeholder="sounds.pk3" />
        </div>
        <div class="wav-editor-filename-control">
          <label>${isRu ? 'Путь в PK3:' : 'Path in PK3:'}</label>
          <input type="text" id="pk3Path" class="wav-editor-filename-input" value="sound/" placeholder="sound/" />
        </div>
      </div>
      <div class="wav-editor-control-row" style="margin-top: 8px;">
        <button class="wav-editor-btn wav-editor-btn-success" id="exportBtn">${isRu ? 'Экспорт WAV' : 'Export WAV'}</button>
      </div>
    </div>
    
    <div class="wav-editor-status" id="statusMessage"></div>
    <div class="tool-credits">${isRu ? 'Сделано' : 'Created by'} <strong>diwoc</strong></div>
  </div>
</div>
`;
    }
    
    let audioContext = null;
    let audioFiles = []; // Array of {id, name, originalBuffer, currentBuffer, selectionStart, selectionEnd, volume}
    let activeFileId = null;
    let audioSource = null;
    let gainNode = null;
    let isPlaying = false;
    let waveformCanvas = null;
    let waveformCtx = null;
    let demoBuffer = null; // Demo buffer for when no files are loaded
    let splitMarkers = []; // Array of split marker positions (in seconds)
    let draggedMarkerIndex = -1; // Index of marker being dragged, -1 if none
    let splitParts = []; // Array of split parts after splitting: [{buffer, name, startTime, endTime}]
    let pitchFiles = []; // Array of pitch-shifted files: [{buffer, name, pitchShift}]
    const MAX_EDIT_HISTORY = 25;
    /** @type {Record<string, { past: any[], future: any[] }>} */
    let editHistory = {};
    
    // IndexedDB for state persistence
    const DB_NAME = 'wavEditorDB';
    const DB_VERSION = 1;
    const STORE_NAME = 'audioFiles';
    
    async function initDB() {
        return new Promise((resolve, reject) => {
            const request = indexedDB.open(DB_NAME, DB_VERSION);
            
            request.onerror = () => reject(request.error);
            request.onsuccess = () => resolve(request.result);
            
            request.onupgradeneeded = (event) => {
                const db = event.target.result;
                if (!db.objectStoreNames.contains(STORE_NAME)) {
                    db.createObjectStore(STORE_NAME, { keyPath: 'id' });
                }
            };
        });
    }
    
    async function saveState() {
        try {
            const db = await initDB();
            const tx = db.transaction(STORE_NAME, 'readwrite');
            const store = tx.objectStore(STORE_NAME);
            
            if (audioFiles.length === 0) {
                // Clear saved state if no files
                return new Promise((resolve, reject) => {
                    const clearRequest = store.clear();
                    clearRequest.onsuccess = () => {
                        resolve();
                    };
                    clearRequest.onerror = () => reject(clearRequest.error);
                });
            }
            
            
            // Clear existing data first
            await new Promise((resolve, reject) => {
                const clearRequest = store.clear();
                clearRequest.onsuccess = () => resolve();
                clearRequest.onerror = () => reject(clearRequest.error);
            });
            
            // Save each file
            const savePromises = [];
            for (const file of audioFiles) {
                const savePromise = (async () => {
                    try {
                        // Convert AudioBuffer to ArrayBuffer for storage
                        const originalArrayBuffer = await audioBufferToArrayBuffer(file.originalBuffer);
                        const currentArrayBuffer = await audioBufferToArrayBuffer(file.currentBuffer);
                        
                        const fileData = {
                            id: file.id,
                            name: file.name,
                            originalArrayBuffer: originalArrayBuffer,
                            currentArrayBuffer: currentArrayBuffer,
                            selectionStart: file.selectionStart || 0,
                            selectionEnd: file.selectionEnd || 0,
                            volume: file.volume || 100,
                            originalSampleRate: file.originalBuffer.sampleRate,
                            originalChannels: file.originalBuffer.numberOfChannels,
                            currentSampleRate: file.currentBuffer.sampleRate,
                            currentChannels: file.currentBuffer.numberOfChannels,
                            originalLength: file.originalBuffer.length,
                            currentLength: file.currentBuffer.length
                        };
                        
                        return new Promise((resolve, reject) => {
                            const putRequest = store.put(fileData);
                            putRequest.onsuccess = () => resolve();
                            putRequest.onerror = () => reject(putRequest.error);
                        });
                    } catch (error) {
                        console.error(`Failed to save file ${file.name}:`, error);
                        throw error;
                    }
                })();
                savePromises.push(savePromise);
            }
            
            // Wait for all files to be saved
            await Promise.all(savePromises);
            
            // Save active file ID
            await new Promise((resolve, reject) => {
                const putRequest = store.put({
                    id: '__activeFileId__',
                    value: activeFileId
                });
                putRequest.onsuccess = () => resolve();
                putRequest.onerror = () => reject(putRequest.error);
            });
            
            // Wait for transaction to complete
            await new Promise((resolve, reject) => {
                tx.oncomplete = () => {
                    resolve();
                };
                tx.onerror = () => reject(tx.error);
            });
        } catch (error) {
            console.error('Failed to save state:', error);
            throw error;
        }
    }
    
    async function loadState() {
        try {
            if (!audioContext) {
                console.error('Cannot load state: AudioContext not initialized');
                return false;
            }
            
            const db = await initDB();
            const tx = db.transaction(STORE_NAME, 'readonly');
            const store = tx.objectStore(STORE_NAME);
            
            // Get all data
            const allData = await new Promise((resolve, reject) => {
                const getAllRequest = store.getAll();
                getAllRequest.onsuccess = () => resolve(getAllRequest.result);
                getAllRequest.onerror = () => reject(getAllRequest.error);
            });
            
            if (allData.length === 0) {
                return false; // No saved state
            }
            
            // Find active file ID
            const activeFileData = allData.find(d => d.id === '__activeFileId__');
            const savedActiveFileId = activeFileData ? activeFileData.value : null;
            
            // Filter out metadata entries
            const fileDataArray = allData.filter(d => d.id !== '__activeFileId__');
            
            if (fileDataArray.length === 0) {
                return false;
            }
            
            
            // Restore files
            audioFiles = [];
            for (const fileData of fileDataArray) {
                try {
                    // Convert ArrayBuffer back to AudioBuffer
                    const originalBuffer = await arrayBufferToAudioBuffer(
                        fileData.originalArrayBuffer,
                        fileData.originalSampleRate,
                        fileData.originalChannels
                    );
                    const currentBuffer = await arrayBufferToAudioBuffer(
                        fileData.currentArrayBuffer,
                        fileData.currentSampleRate,
                        fileData.currentChannels
                    );
                    
                    const file = {
                        id: fileData.id,
                        name: fileData.name,
                        originalBuffer: originalBuffer,
                        currentBuffer: currentBuffer,
                        selectionStart: fileData.selectionStart || 0,
                        selectionEnd: fileData.selectionEnd || 0,
                        volume: fileData.volume || 100
                    };
                    
                    audioFiles.push(file);
                } catch (error) {
                    console.error(`Failed to restore file ${fileData.name}:`, error);
                }
            }
            
            if (audioFiles.length === 0) {
                return false;
            }
            
            // Restore active file ID
            if (savedActiveFileId && audioFiles.find(f => f.id === savedActiveFileId)) {
                activeFileId = savedActiveFileId;
            } else if (audioFiles.length > 0) {
                activeFileId = audioFiles[0].id;
            }
            
            return true;
        } catch (error) {
            console.error('Failed to load state:', error);
            return false;
        }
    }
    
    // Helper: Convert AudioBuffer to ArrayBuffer
    async function audioBufferToArrayBuffer(audioBuffer) {
        const numberOfChannels = audioBuffer.numberOfChannels;
        const length = audioBuffer.length;
        const sampleRate = audioBuffer.sampleRate;
        
        // Create a WAV file from AudioBuffer
        const wavBuffer = audioBufferToWav(audioBuffer);
        return wavBuffer;
    }
    
    // Helper: Convert ArrayBuffer (WAV) back to AudioBuffer
    async function arrayBufferToAudioBuffer(arrayBuffer, sampleRate, channels) {
        if (!audioContext) {
            throw new Error('AudioContext not initialized');
        }
        // Decode the WAV file back to AudioBuffer
        const decodedBuffer = await audioContext.decodeAudioData(arrayBuffer.slice(0));
        return decodedBuffer;
    }
    
    // Helper functions
    function getActiveFile() {
        return audioFiles.find(f => f.id === activeFileId);
    }
    
    function getActiveBuffer() {
        const file = getActiveFile();
        if (file) return file.currentBuffer;
        // Return demo buffer if no files loaded
        return demoBuffer;
    }
    
    function hasActiveFile() {
        return audioFiles.length > 0 && activeFileId !== null;
    }
    
    async function loadDemoBuffer() {
        if (demoBuffer) return demoBuffer;
        
        try {
            const response = await fetch('/assets/sounds/quaddamage.wav');
            if (!response.ok) return null;
            const arrayBuffer = await response.arrayBuffer();
            demoBuffer = await audioContext.decodeAudioData(arrayBuffer);
            return demoBuffer;
        } catch (error) {
            console.error('Failed to load demo buffer:', error);
            return null;
        }
    }
    
    function showDemoMode() {
        if (!demoBuffer) return;
        
        // Show controls
        const audioInfo = document.getElementById('audioInfo');
        const waveformContainer = document.getElementById('waveformContainer');
        const trimControls = document.getElementById('trimControls');
        const exportOptions = document.getElementById('exportOptions');
        const volumeControl = document.getElementById('volumeControl');
        
        if (audioInfo) {
            const fileNameEl = document.getElementById('fileName');
            const duration = document.getElementById('duration');
            const sampleRate = document.getElementById('sampleRate');
            const channels = document.getElementById('channels');
            
            if (fileNameEl) fileNameEl.textContent = 'quaddamage.wav (demo)';
            if (duration) duration.textContent = demoBuffer.duration.toFixed(2) + 's';
            if (sampleRate) sampleRate.textContent = demoBuffer.sampleRate + ' Hz';
            if (channels) channels.textContent = demoBuffer.numberOfChannels;
            audioInfo.style.display = 'block';
        }
        
        if (waveformContainer) waveformContainer.style.display = 'block';
        if (trimControls) trimControls.style.display = 'block';
        if (exportOptions) exportOptions.style.display = 'block';
        if (volumeControl) volumeControl.style.display = 'flex';
        
        // Set default selection
        const startTimeInput = document.getElementById('startTime');
        const endTimeInput = document.getElementById('endTime');
        if (startTimeInput) {
            startTimeInput.value = '0';
            startTimeInput.max = demoBuffer.duration.toFixed(2);
        }
        if (endTimeInput) {
            endTimeInput.value = demoBuffer.duration.toFixed(2);
            endTimeInput.max = demoBuffer.duration.toFixed(2);
        }
        
        // Draw waveform
        requestAnimationFrame(() => {
            drawWaveform(demoBuffer);
            updateDemoSelectionDisplay();
        });
    }
    
    function updateDemoSelectionDisplay() {
        if (hasActiveFile()) return;
        
        const selectionEl = document.getElementById('selection');
        const waveformContainer = document.getElementById('waveformContainer');
        if (!selectionEl || !waveformContainer || !demoBuffer) return;
        
        const startTimeInput = document.getElementById('startTime');
        const endTimeInput = document.getElementById('endTime');
        const start = parseFloat(startTimeInput?.value || 0);
        const end = parseFloat(endTimeInput?.value || demoBuffer.duration);
        
        const duration = demoBuffer.duration;
        const startPercent = (start / duration) * 100;
        const endPercent = (end / duration) * 100;
        
        selectionEl.style.left = startPercent + '%';
        selectionEl.style.width = (endPercent - startPercent) + '%';
        selectionEl.style.display = 'block';
    }
    
    function generateFileId() {
        return 'file_' + Date.now() + '_' + Math.random().toString(36).substr(2, 9);
    }
    
    function updateFilesList() {
        const filesList = document.getElementById('filesList');
        if (!filesList) return;
        
        if (audioFiles.length === 0) {
            filesList.style.display = 'none';
            return;
        }
        
        filesList.style.display = 'block';
        const lang = getLanguage();
        const isRu = lang === 'ru';
        
        filesList.innerHTML = audioFiles.map(file => `
            <div class="wav-editor-file-item ${file.id === activeFileId ? 'active' : ''}" data-file-id="${file.id}">
                <span class="wav-editor-file-item-name">${file.name}</span>
                <button class="wav-editor-file-item-remove" data-file-id="${file.id}">${isRu ? '×' : '×'}</button>
            </div>
        `).join('');
        
        // Add click handlers
        filesList.querySelectorAll('.wav-editor-file-item').forEach(item => {
            item.addEventListener('click', (e) => {
                if (e.target.classList.contains('wav-editor-file-item-remove')) return;
                const fileId = item.dataset.fileId;
                switchToFile(fileId);
            });
        });
        
        // Add remove handlers
        filesList.querySelectorAll('.wav-editor-file-item-remove').forEach(btn => {
            btn.addEventListener('click', (e) => {
                e.stopPropagation();
                const fileId = btn.dataset.fileId;
                removeFile(fileId);
            });
        });
    }
    
    function switchToFile(fileId) {
        if (activeFileId === fileId) return;
        
        // Save current file state
        saveCurrentFileState();
        
        // Clear split markers when switching files
        clearSplitMarkers();
        splitParts = [];
        
        // Switch to new file
        activeFileId = fileId;
        const file = getActiveFile();
        if (!file) return;
        
        // Load file state
        loadFileState(file);
        
        // Update UI
        updateFilesList();
        updateFileInfo(file);
        drawWaveform(file.currentBuffer);
        updateSelectionDisplay();
        
        // Hide split parts names UI
        const namesContainer = document.getElementById('splitPartsNames');
        if (namesContainer) namesContainer.style.display = 'none';
        
        syncExportFilenameForActiveFile();
        updateUndoRedoButtons();
    }
    
    function saveCurrentFileState() {
        const file = getActiveFile();
        if (!file) return;
        
        const startTimeInput = document.getElementById('startTime');
        const endTimeInput = document.getElementById('endTime');
        const volumeSlider = document.getElementById('volumeSlider');
        
        if (startTimeInput) file.selectionStart = parseFloat(startTimeInput.value) || 0;
        if (endTimeInput) file.selectionEnd = parseFloat(endTimeInput.value) || 0;
        if (volumeSlider) file.volume = parseInt(volumeSlider.value) || 100;
    }
    
    async function restoreUIFromState() {
        editHistory = {};
        if (audioFiles.length === 0) return;
        
        // Update files list
        updateFilesList();
        
        // Restore active file
        const file = getActiveFile();
        if (!file) {
            if (audioFiles.length > 0) {
                activeFileId = audioFiles[0].id;
                await restoreUIFromState();
            }
            return;
        }
        
        // Show controls
        const audioInfo = document.getElementById('audioInfo');
        const waveformContainer = document.getElementById('waveformContainer');
        const trimControls = document.getElementById('trimControls');
        const exportOptions = document.getElementById('exportOptions');
        const volumeControl = document.getElementById('volumeControl');
        
        if (audioInfo) audioInfo.style.display = 'block';
        if (waveformContainer) waveformContainer.style.display = 'block';
        if (trimControls) trimControls.style.display = 'block';
        if (exportOptions) exportOptions.style.display = 'block';
        if (volumeControl) volumeControl.style.display = 'flex';
        
        // Update file info and state
        updateFileInfo(file);
        loadFileState(file);
        
        // Draw waveform
        requestAnimationFrame(() => {
            drawWaveform(file.currentBuffer);
            updateSelectionDisplay();
            syncExportFilenameForActiveFile();
            updateUndoRedoButtons();
        });
    }
    
    function loadFileState(file) {
        const startTimeInput = document.getElementById('startTime');
        const endTimeInput = document.getElementById('endTime');
        const volumeSlider = document.getElementById('volumeSlider');
        const volumeValue = document.getElementById('volumeValue');
        
        if (startTimeInput) {
            startTimeInput.value = file.selectionStart.toFixed(2);
            startTimeInput.max = file.currentBuffer.duration.toFixed(2);
        }
        if (endTimeInput) {
            endTimeInput.value = file.selectionEnd.toFixed(2);
            endTimeInput.max = file.currentBuffer.duration.toFixed(2);
        }
        if (volumeSlider) {
            volumeSlider.value = file.volume || 100;
        }
        if (volumeValue) {
            volumeValue.textContent = (file.volume || 100) + '%';
        }
    }
    
    function updateFileInfo(file) {
        const fileNameEl = document.getElementById('fileName');
        const duration = document.getElementById('duration');
        const sampleRate = document.getElementById('sampleRate');
        const channels = document.getElementById('channels');
        
        if (fileNameEl) fileNameEl.textContent = file.name;
        if (duration) duration.textContent = file.currentBuffer.duration.toFixed(2) + 's';
        if (sampleRate) sampleRate.textContent = file.currentBuffer.sampleRate + ' Hz';
        if (channels) channels.textContent = file.currentBuffer.numberOfChannels;
    }
    
    function syncExportFilenameForActiveFile() {
        const file = getActiveFile();
        const exportFilenameInput = document.getElementById('exportFilename');
        if (!file || !exportFilenameInput) return;
        const baseName = file.name.replace(/\.[^/.]+$/, '');
        exportFilenameInput.value = baseName + '.wav';
    }
    
    function removeFile(fileId) {
        const lang = getLanguage();
        const isRu = lang === 'ru';
        
        // Stop playback if removing active file
        if (fileId === activeFileId) {
            stopPlayback();
        }
        
        // Remove file
        audioFiles = audioFiles.filter(f => f.id !== fileId);
        delete editHistory[fileId];
        
        // Switch to another file if needed
        if (fileId === activeFileId) {
            if (audioFiles.length > 0) {
                switchToFile(audioFiles[0].id);
            } else {
                activeFileId = null;
                // Hide controls
                const waveformContainer = document.getElementById('waveformContainer');
                const trimControls = document.getElementById('trimControls');
                const exportOptions = document.getElementById('exportOptions');
                const volumeControl = document.getElementById('volumeControl');
                const audioInfo = document.getElementById('audioInfo');
                
                if (waveformContainer) waveformContainer.style.display = 'none';
                if (trimControls) trimControls.style.display = 'none';
                if (exportOptions) exportOptions.style.display = 'none';
                if (volumeControl) volumeControl.style.display = 'none';
                if (audioInfo) audioInfo.style.display = 'none';
                clearSplitMarkers();
                splitParts = [];
                updateUndoRedoButtons();
            }
        }
        
        // Save state after removing file
        saveState().catch(err => console.error('Failed to save state:', err));
        
        updateFilesList();
    }
    
    function showStatus(message, type) {
        const statusEl = document.getElementById('statusMessage');
        if (statusEl) {
            statusEl.textContent = message;
            statusEl.className = `wav-editor-status ${type}`;
            setTimeout(() => {
                if (statusEl.className.includes(type)) {
                    statusEl.style.display = 'none';
                }
            }, 5000);
        }
    }
    
    function drawWaveform(buffer) {
        if (!waveformCanvas || !waveformCtx || !buffer) return;
        
        const canvas = waveformCanvas;
        const ctx = waveformCtx;
        
        // Ensure canvas has dimensions
        if (canvas.offsetWidth === 0 || canvas.offsetHeight === 0) {
            // Canvas not yet visible, retry after a short delay
            setTimeout(() => drawWaveform(buffer), 50);
            return;
        }
        
        const width = canvas.width = canvas.offsetWidth;
        const height = canvas.height = canvas.offsetHeight;
        
        ctx.clearRect(0, 0, width, height);
        ctx.fillStyle = 'rgba(255, 255, 255, 0.8)';
        
        const channelData = buffer.getChannelData(0);
        if (!channelData || channelData.length === 0) return;
        
        const step = Math.ceil(channelData.length / width);
        const amp = height / 2;
        
        for (let i = 0; i < width; i++) {
            let min = 1.0;
            let max = -1.0;
            
            for (let j = 0; j < step; j++) {
                const sampleIndex = (i * step) + j;
                if (sampleIndex >= channelData.length) break;
                const sample = channelData[sampleIndex];
                if (sample < min) min = sample;
                if (sample > max) max = sample;
            }
            
            ctx.fillRect(i, (1 + min) * amp, 1, Math.max(1, (max - min) * amp));
        }
        
        // Draw split markers
        drawSplitMarkers(buffer);
    }
    
    function drawSplitMarkers(buffer) {
        if (!hasActiveFile() || splitMarkers.length === 0) return;
        
        const waveformContainer = document.getElementById('waveformContainer');
        if (!waveformContainer) return;
        
        const file = getActiveFile();
        if (!file) return;
        
        const duration = buffer.duration;
        const containerWidth = waveformContainer.offsetWidth;
        
        // Remove existing markers
        const existingMarkers = waveformContainer.querySelectorAll('.wav-editor-split-marker');
        existingMarkers.forEach(marker => marker.remove());
        
        // Draw markers
        splitMarkers.forEach((markerTime, index) => {
            if (markerTime < file.selectionStart || markerTime > file.selectionEnd) return;
            
            // Calculate position as percentage of total duration
            const left = (markerTime / duration) * 100;
            
            const marker = document.createElement('div');
            marker.className = 'wav-editor-split-marker';
            marker.style.left = left + '%';
            marker.dataset.markerIndex = index;
            marker.dataset.markerTime = markerTime;
            
            waveformContainer.appendChild(marker);
        });
    }
    
    function createSplitMarkers(numParts) {
        if (!hasActiveFile()) return;
        
        const file = getActiveFile();
        if (!file) return;
        
        const selectionDuration = file.selectionEnd - file.selectionStart;
        if (selectionDuration <= 0) return;
        
        splitMarkers = [];
        const partDuration = selectionDuration / numParts;
        
        for (let i = 1; i < numParts; i++) {
            const markerTime = file.selectionStart + (i * partDuration);
            splitMarkers.push(markerTime);
        }
        
        // Sort markers
        splitMarkers.sort((a, b) => a - b);
        
        // Update UI
        const splitBtn = document.getElementById('splitBtn');
        if (splitBtn) splitBtn.style.display = 'inline-block';
        
        // Redraw waveform with markers
        drawWaveform(file.currentBuffer);
    }
    
    function clearSplitMarkers() {
        splitMarkers = [];
        const waveformContainer = document.getElementById('waveformContainer');
        if (waveformContainer) {
            const existingMarkers = waveformContainer.querySelectorAll('.wav-editor-split-marker');
            existingMarkers.forEach(marker => marker.remove());
        }
        const splitBtn = document.getElementById('splitBtn');
        if (splitBtn) splitBtn.style.display = 'none';
    }
    
    function updateSelectionDisplay() {
        if (!hasActiveFile()) {
            updateDemoSelectionDisplay();
            return;
        }
        
        const file = getActiveFile();
        if (!file) return;
        
        const selectionEl = document.getElementById('selection');
        const waveformContainer = document.getElementById('waveformContainer');
        if (!selectionEl || !waveformContainer || !file.currentBuffer) return;
        
        const duration = file.currentBuffer.duration;
        const startPercent = (file.selectionStart / duration) * 100;
        const endPercent = (file.selectionEnd / duration) * 100;
        
        selectionEl.style.left = startPercent + '%';
        selectionEl.style.width = (endPercent - startPercent) + '%';
        selectionEl.style.display = 'block';
    }
    
    function stopPlayback() {
        if (audioSource) {
            audioSource.stop();
            audioSource = null;
        }
        isPlaying = false;
        const playBtn = document.getElementById('playBtn');
        const pauseBtn = document.getElementById('pauseBtn');
        const stopBtn = document.getElementById('stopBtn');
        if (playBtn) playBtn.disabled = false;
        if (pauseBtn) pauseBtn.disabled = true;
        if (stopBtn) stopBtn.disabled = true;
    }
    
    function playAudio(startTime = 0, endTime = null) {
        const buffer = getActiveBuffer();
        if (!buffer || !audioContext) return;
        
        stopPlayback();
        
        const source = audioContext.createBufferSource();
        const gain = audioContext.createGain();
        
        source.buffer = buffer;
        source.connect(gain);
        gain.connect(audioContext.destination);
        
        audioSource = source;
        gainNode = gain;
        
        const duration = endTime !== null ? (endTime - startTime) : (buffer.duration - startTime);
        
        source.start(0, startTime);
        if (endTime !== null) {
            source.stop(audioContext.currentTime + duration);
        }
        
        source.onended = () => {
            stopPlayback();
        };
        
        isPlaying = true;
        const playBtn = document.getElementById('playBtn');
        const pauseBtn = document.getElementById('pauseBtn');
        const stopBtn = document.getElementById('stopBtn');
        if (playBtn) playBtn.disabled = true;
        if (pauseBtn) pauseBtn.disabled = false;
        if (stopBtn) stopBtn.disabled = false;
    }
    
    function playAudioBuffer(buffer) {
        if (!buffer || !audioContext) return;
        
        stopPlayback();
        
        const source = audioContext.createBufferSource();
        const gain = audioContext.createGain();
        
        source.buffer = buffer;
        source.connect(gain);
        gain.connect(audioContext.destination);
        
        audioSource = source;
        gainNode = gain;
        
        source.start(0);
        
        source.onended = () => {
            stopPlayback();
        };
        
        isPlaying = true;
        const playBtn = document.getElementById('playBtn');
        const pauseBtn = document.getElementById('pauseBtn');
        const stopBtn = document.getElementById('stopBtn');
        if (playBtn) playBtn.disabled = true;
        if (pauseBtn) pauseBtn.disabled = false;
        if (stopBtn) stopBtn.disabled = false;
    }
    
    function showSplitPartsNamesUI() {
        const namesContainer = document.getElementById('splitPartsNames');
        if (!namesContainer) return;
        
        const lang = getLanguage();
        const isRu = lang === 'ru';
        
        namesContainer.style.display = 'block';
        namesContainer.innerHTML = '<div style="font-size: 0.85rem; color: rgba(255, 255, 255, 0.8); margin-bottom: 8px;">' +
            (isRu ? 'Имена частей:' : 'Part names:') + '</div>';
        
        splitParts.forEach((part, index) => {
            const row = document.createElement('div');
            row.className = 'wav-editor-control-row';
            row.style.marginTop = '6px';
            
            const label = document.createElement('label');
            label.textContent = `${isRu ? 'Часть' : 'Part'} ${index + 1}:`;
            label.style.minWidth = '70px';
            
            const input = document.createElement('input');
            input.type = 'text';
            input.className = 'wav-editor-filename-input';
            input.value = part.name;
            input.style.flex = '1';
            input.addEventListener('change', (e) => {
                splitParts[index].name = e.target.value || part.name;
            });
            
            const previewBtn = document.createElement('button');
            previewBtn.type = 'button';
            previewBtn.className = 'wav-editor-playback-btn';
            previewBtn.textContent = isRu ? '▶' : '▶';
            previewBtn.title = isRu ? 'Прослушать часть' : 'Preview part';
            previewBtn.style.marginLeft = '8px';
            previewBtn.addEventListener('click', () => {
                stopPlayback();
                playAudioBuffer(part.buffer);
            });
            
            row.appendChild(label);
            row.appendChild(input);
            row.appendChild(previewBtn);
            namesContainer.appendChild(row);
        });
    }
    
    function createEditSnapshot() {
        const file = getActiveFile();
        if (!file || !file.currentBuffer || !audioContext) return null;
        const bufClone = cloneAudioBuffer(file.currentBuffer);
        if (!bufClone) return null;
        return {
            currentBuffer: bufClone,
            selectionStart: file.selectionStart,
            selectionEnd: file.selectionEnd,
            volume: file.volume,
            splitMarkers: splitMarkers.slice(),
            splitParts: splitParts.map(p => ({
                name: p.name,
                startTime: p.startTime,
                endTime: p.endTime,
                buffer: cloneAudioBuffer(p.buffer)
            }))
        };
    }
    
    function applyEditSnapshot(snap) {
        if (!snap) return;
        const file = getActiveFile();
        if (!file) return;
        stopPlayback();
        file.currentBuffer = snap.currentBuffer;
        file.selectionStart = snap.selectionStart;
        file.selectionEnd = snap.selectionEnd;
        file.volume = snap.volume;
        splitMarkers = snap.splitMarkers.slice();
        splitParts = (snap.splitParts || []).map(p => ({
            name: p.name,
            startTime: p.startTime,
            endTime: p.endTime,
            buffer: p.buffer
        }));
        
        loadFileState(file);
        updateFileInfo(file);
        drawWaveform(file.currentBuffer);
        updateSelectionDisplay();
        
        const splitBtnEl = document.getElementById('splitBtn');
        if (splitBtnEl) splitBtnEl.style.display = splitMarkers.length > 0 ? 'inline-block' : 'none';
        
        const namesContainer = document.getElementById('splitPartsNames');
        if (namesContainer) {
            if (splitParts.length > 0) {
                showSplitPartsNamesUI();
            } else {
                namesContainer.style.display = 'none';
                namesContainer.innerHTML = '';
            }
        }
        
        syncExportFilenameForActiveFile();
        updateUndoRedoButtons();
        saveState().catch(err => console.error('Failed to save state:', err));
    }
    
    function pushEditHistoryBeforeChange() {
        const file = getActiveFile();
        if (!file || !audioContext) return;
        const snap = createEditSnapshot();
        if (!snap) return;
        if (!editHistory[file.id]) {
            editHistory[file.id] = { past: [], future: [] };
        }
        const h = editHistory[file.id];
        h.past.push(snap);
        if (h.past.length > MAX_EDIT_HISTORY) h.past.shift();
        h.future = [];
        updateUndoRedoButtons();
    }
    
    function updateUndoRedoButtons() {
        const undoBtn = document.getElementById('undoBtn');
        const redoBtn = document.getElementById('redoBtn');
        if (!undoBtn || !redoBtn) return;
        const file = getActiveFile();
        if (!file || !editHistory[file.id]) {
            undoBtn.disabled = true;
            redoBtn.disabled = true;
            return;
        }
        const h = editHistory[file.id];
        undoBtn.disabled = h.past.length === 0;
        redoBtn.disabled = h.future.length === 0;
    }
    
    function performUndo() {
        if (!hasActiveFile()) return;
        const file = getActiveFile();
        if (!file || !editHistory[file.id] || editHistory[file.id].past.length === 0) return;
        const h = editHistory[file.id];
        const current = createEditSnapshot();
        const prev = h.past.pop();
        if (current) h.future.push(current);
        applyEditSnapshot(prev);
        const lang = getLanguage();
        showStatus(lang === 'ru' ? 'Отменено' : 'Undone', 'info');
    }
    
    function performRedo() {
        if (!hasActiveFile()) return;
        const file = getActiveFile();
        if (!file || !editHistory[file.id] || editHistory[file.id].future.length === 0) return;
        const h = editHistory[file.id];
        const current = createEditSnapshot();
        const next = h.future.pop();
        if (current) h.past.push(current);
        if (next) applyEditSnapshot(next);
        const lang = getLanguage();
        showStatus(lang === 'ru' ? 'Возвращено' : 'Redone', 'info');
    }
    
    async function loadWavEditorTool() {
        const wavEditorToolContent = document.getElementById('wavEditorToolContent');
        if (!wavEditorToolContent) return;
        
        const lang = getLanguage();
        wavEditorToolContent.innerHTML = getWavEditorToolHTML(lang);
        
        // Initialize audio context
        try {
            audioContext = new (window.AudioContext || window.webkitAudioContext)();
        } catch (e) {
            showStatus('AudioContext not supported', 'error');
            return;
        }
        
        // Get elements
        const audioFileInput = document.getElementById('audioFileInput');
        const loadAudioBtn = document.getElementById('loadAudioBtn');
        const trimBtn = document.getElementById('trimBtn');
        const resetBtn = document.getElementById('resetBtn');
        const playBtn = document.getElementById('playBtn');
        const pauseBtn = document.getElementById('pauseBtn');
        const stopBtn = document.getElementById('stopBtn');
        const selectAllBtn = document.getElementById('selectAllBtn');
        const exportBtn = document.getElementById('exportBtn');
        const startTimeInput = document.getElementById('startTime');
        const endTimeInput = document.getElementById('endTime');
        const volumeSlider = document.getElementById('volumeSlider');
        const volumeValue = document.getElementById('volumeValue');
        const applyVolumeBtn = document.getElementById('applyVolumeBtn');
        const exportToPk3Check = document.getElementById('exportToPk3');
        const pk3Fields = document.getElementById('pk3Fields');
        const pk3FilenameInput = document.getElementById('pk3Filename');
        const pk3PathInput = document.getElementById('pk3Path');
        
        waveformCanvas = document.getElementById('waveformCanvas');
        if (waveformCanvas) {
            waveformCtx = waveformCanvas.getContext('2d');
        }
        
        // Function to load audio from file or URL
        async function loadAudioFile(fileOrUrl, fileName = null, showSuccessMessage = true) {
            try {
                let arrayBuffer;
                let actualFileName;
                
                if (fileOrUrl instanceof File) {
                    arrayBuffer = await fileOrUrl.arrayBuffer();
                    actualFileName = fileOrUrl.name;
                } else {
                    // It's a URL
                    const response = await fetch(fileOrUrl);
                    if (!response.ok) throw new Error('Failed to fetch audio file');
                    arrayBuffer = await response.arrayBuffer();
                    actualFileName = fileName || fileOrUrl.split('/').pop() || 'sample.wav';
                }
                
                const decodedBuffer = await audioContext.decodeAudioData(arrayBuffer);
                
                // Create file object
                const fileId = generateFileId();
                const file = {
                    id: fileId,
                    name: actualFileName,
                    originalBuffer: decodedBuffer,
                    currentBuffer: decodedBuffer,
                    selectionStart: 0,
                    selectionEnd: decodedBuffer.duration,
                    volume: 100
                };
                
                // Add to files array
                audioFiles.push(file);
                
                // Set as active if it's the first file (switching from demo)
                const wasDemo = !activeFileId && audioFiles.length === 1;
                if (!activeFileId) {
                    activeFileId = fileId;
                }
                
                // Update UI
                updateFilesList();
                
                // Save state after loading new file
                saveState().catch(err => console.error('Failed to save state:', err));
                
                // If this is the active file, update UI
                if (activeFileId === fileId) {
                    // Show controls
                    const audioInfo = document.getElementById('audioInfo');
                    const waveformContainer = document.getElementById('waveformContainer');
                    const trimControls = document.getElementById('trimControls');
                    const exportOptions = document.getElementById('exportOptions');
                    const volumeControl = document.getElementById('volumeControl');
                    
                    if (audioInfo) audioInfo.style.display = 'block';
                    if (waveformContainer) waveformContainer.style.display = 'block';
                    if (trimControls) trimControls.style.display = 'block';
                    if (exportOptions) exportOptions.style.display = 'block';
                    if (volumeControl) volumeControl.style.display = 'flex';
                    
                    // Update file info and state
                    updateFileInfo(file);
                    loadFileState(file);
                    
                    // Draw waveform - use requestAnimationFrame to ensure canvas is rendered
                    requestAnimationFrame(() => {
                        drawWaveform(decodedBuffer);
                        updateSelectionDisplay();
                    });
                    
                    syncExportFilenameForActiveFile();
                    updateUndoRedoButtons();
                }
                
                // Hide demo if files were loaded
                if (wasDemo && audioFiles.length > 0) {
                    demoBuffer = null; // Clear demo buffer
                }
                
                // Show success message only if requested
                if (showSuccessMessage) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Аудио загружено успешно' : 'Audio loaded successfully', 'success');
                }
                
                return true;
            } catch (error) {
                const lang = getLanguage();
                const isRu = lang === 'ru';
                showStatus((isRu ? 'Ошибка загрузки аудио: ' : 'Error loading audio: ') + error.message, 'error');
                return false;
            }
        }
        
        // Load audio file
        if (loadAudioBtn && audioFileInput) {
            loadAudioBtn.addEventListener('click', () => {
                audioFileInput.click();
            });
            
            audioFileInput.addEventListener('change', async (e) => {
                const files = Array.from(e.target.files);
                if (files.length === 0) return;
                
                // Load all files
                for (const file of files) {
                    await loadAudioFile(file);
                }
                
                // Clear input
                audioFileInput.value = '';
            });
        }
        
        // Waveform selection: new range (any direction), or drag left/right edges
        if (waveformCanvas) {
            const MIN_SEL_SEC = 0.001;
            
            let selectionPointerActive = false;
            /** @type {'new' | 'edge-left' | 'edge-right'} */
            let selectionDragMode = 'new';
            let selectionAnchorTime = 0;
            
            function edgeThresholdPx(rect) {
                return Math.max(10, Math.min(22, rect.width * 0.022));
            }
            
            function timeFromClientX(clientX, buffer, rect) {
                const x = clientX - rect.left;
                const percent = Math.max(0, Math.min(1, x / rect.width));
                return percent * buffer.duration;
            }
            
            function minSelGap(buffer) {
                return Math.max(MIN_SEL_SEC, 2 / buffer.sampleRate);
            }
            
            function pickEdgeMode(x, rect, leftPx, rightPx) {
                const EDGE = edgeThresholdPx(rect);
                const distL = Math.abs(x - leftPx);
                const distR = Math.abs(x - rightPx);
                const nearL = distL <= EDGE;
                const nearR = distR <= EDGE;
                if (nearL && nearR) {
                    return distL <= distR ? 'edge-left' : 'edge-right';
                }
                if (nearL) return 'edge-left';
                if (nearR) return 'edge-right';
                return null;
            }
            
            function teardownSelectionDocListeners() {
                document.removeEventListener('mousemove', onSelectionDocMove);
                document.removeEventListener('mouseup', onSelectionDocUp);
                document.removeEventListener('touchmove', onSelectionDocMove);
                document.removeEventListener('touchend', onSelectionDocUp);
                document.removeEventListener('touchcancel', onSelectionDocUp);
            }
            
            function onSelectionDocUp() {
                if (selectionPointerActive && hasActiveFile()) {
                    saveState().catch(err => console.error('Failed to save state:', err));
                }
                selectionPointerActive = false;
                teardownSelectionDocListeners();
                if (waveformCanvas) {
                    waveformCanvas.style.cursor = '';
                }
            }
            
            function onSelectionDocMove(e) {
                if (!selectionPointerActive || !waveformCanvas) return;
                const buffer = getActiveBuffer();
                if (!buffer) return;
                const rect = waveformCanvas.getBoundingClientRect();
                const clientX = e.touches && e.touches[0] ? e.touches[0].clientX : e.clientX;
                const t = timeFromClientX(clientX, buffer, rect);
                const dur = buffer.duration;
                const gap = minSelGap(buffer);
                
                if (hasActiveFile()) {
                    const file = getActiveFile();
                    if (!file) return;
                    if (selectionDragMode === 'new') {
                        const a = selectionAnchorTime;
                        const lo = Math.max(0, Math.min(a, t));
                        const hi = Math.min(dur, Math.max(a, t));
                        file.selectionStart = lo;
                        file.selectionEnd = hi;
                        if (startTimeInput) startTimeInput.value = lo.toFixed(2);
                        if (endTimeInput) endTimeInput.value = hi.toFixed(2);
                    } else if (selectionDragMode === 'edge-left') {
                        const newStart = Math.max(0, Math.min(t, file.selectionEnd - gap));
                        file.selectionStart = newStart;
                        if (startTimeInput) startTimeInput.value = newStart.toFixed(2);
                    } else if (selectionDragMode === 'edge-right') {
                        const newEnd = Math.min(dur, Math.max(t, file.selectionStart + gap));
                        file.selectionEnd = newEnd;
                        if (endTimeInput) endTimeInput.value = newEnd.toFixed(2);
                    }
                } else {
                    let st = parseFloat(startTimeInput?.value || 0);
                    let en = parseFloat(endTimeInput?.value || dur);
                    if (selectionDragMode === 'new') {
                        const a = selectionAnchorTime;
                        st = Math.max(0, Math.min(a, t));
                        en = Math.min(dur, Math.max(a, t));
                    } else if (selectionDragMode === 'edge-left') {
                        st = Math.max(0, Math.min(t, en - gap));
                    } else if (selectionDragMode === 'edge-right') {
                        en = Math.min(dur, Math.max(t, st + gap));
                    }
                    if (startTimeInput) {
                        startTimeInput.value = st.toFixed(2);
                        startTimeInput.max = dur.toFixed(2);
                    }
                    if (endTimeInput) {
                        endTimeInput.value = en.toFixed(2);
                        endTimeInput.max = dur.toFixed(2);
                    }
                }
                updateSelectionDisplay();
            }
            
            function beginSelectionPointer(e, clientX) {
                if (selectionPointerActive) {
                    onSelectionDocUp();
                }
                const buffer = getActiveBuffer();
                if (!buffer) return;
                const rect = waveformCanvas.getBoundingClientRect();
                const x = clientX - rect.left;
                const t = timeFromClientX(clientX, buffer, rect);
                const dur = buffer.duration;
                const gap = minSelGap(buffer);
                
                selectionAnchorTime = t;
                selectionDragMode = 'new';
                
                if (hasActiveFile()) {
                    const file = getActiveFile();
                    const leftPx = (file.selectionStart / dur) * rect.width;
                    const rightPx = (file.selectionEnd / dur) * rect.width;
                    const span = file.selectionEnd - file.selectionStart;
                    const edge = span >= gap ? pickEdgeMode(x, rect, leftPx, rightPx) : null;
                    if (edge) {
                        selectionDragMode = edge;
                    } else {
                        file.selectionStart = t;
                        file.selectionEnd = t;
                        selectionAnchorTime = t;
                        if (startTimeInput) startTimeInput.value = t.toFixed(2);
                        if (endTimeInput) endTimeInput.value = t.toFixed(2);
                    }
                } else {
                    let st = parseFloat(startTimeInput?.value || 0);
                    let en = parseFloat(endTimeInput?.value || dur);
                    const leftPx = (st / dur) * rect.width;
                    const rightPx = (en / dur) * rect.width;
                    const span = en - st;
                    const edge = span >= gap ? pickEdgeMode(x, rect, leftPx, rightPx) : null;
                    if (edge) {
                        selectionDragMode = edge;
                    } else {
                        selectionAnchorTime = t;
                        if (startTimeInput) {
                            startTimeInput.value = t.toFixed(2);
                            startTimeInput.max = dur.toFixed(2);
                        }
                        if (endTimeInput) {
                            endTimeInput.value = t.toFixed(2);
                            endTimeInput.max = dur.toFixed(2);
                        }
                    }
                }
                
                selectionPointerActive = true;
                waveformCanvas.style.cursor = selectionDragMode.startsWith('edge') ? 'grabbing' : 'crosshair';
                document.addEventListener('mousemove', onSelectionDocMove);
                document.addEventListener('mouseup', onSelectionDocUp);
                document.addEventListener('touchmove', onSelectionDocMove, { passive: true });
                document.addEventListener('touchend', onSelectionDocUp);
                document.addEventListener('touchcancel', onSelectionDocUp);
                updateSelectionDisplay();
            }
            
            waveformCanvas.addEventListener('mousedown', (e) => {
                if (e.button !== 0) return;
                beginSelectionPointer(e, e.clientX);
            });
            
            waveformCanvas.addEventListener('touchstart', (e) => {
                if (!e.touches || !e.touches[0]) return;
                beginSelectionPointer(e, e.touches[0].clientX);
            }, { passive: true });
            
            waveformCanvas.addEventListener('mousemove', (e) => {
                if (selectionPointerActive) return;
                const buffer = getActiveBuffer();
                if (!buffer || !waveformCanvas) return;
                const rect = waveformCanvas.getBoundingClientRect();
                const x = e.clientX - rect.left;
                const dur = buffer.duration;
                const gap = minSelGap(buffer);
                let showGrab = false;
                
                if (hasActiveFile()) {
                    const file = getActiveFile();
                    const leftPx = (file.selectionStart / dur) * rect.width;
                    const rightPx = (file.selectionEnd / dur) * rect.width;
                    const span = file.selectionEnd - file.selectionStart;
                    if (span >= gap && pickEdgeMode(x, rect, leftPx, rightPx)) {
                        showGrab = true;
                    }
                } else {
                    const st = parseFloat(startTimeInput?.value || 0);
                    const en = parseFloat(endTimeInput?.value || dur);
                    const leftPx = (st / dur) * rect.width;
                    const rightPx = (en / dur) * rect.width;
                    const span = en - st;
                    if (span >= gap && pickEdgeMode(x, rect, leftPx, rightPx)) {
                        showGrab = true;
                    }
                }
                
                waveformCanvas.style.cursor = showGrab ? 'grab' : '';
            });
            
            waveformCanvas.addEventListener('mouseleave', () => {
                if (!selectionPointerActive) {
                    waveformCanvas.style.cursor = '';
                }
            });
        }
        
        // Time inputs
        if (startTimeInput) {
            startTimeInput.addEventListener('change', (e) => {
                if (hasActiveFile()) {
                    const file = getActiveFile();
                    if (!file) return;
                    file.selectionStart = parseFloat(e.target.value) || 0;
                    if (file.selectionStart > file.selectionEnd) {
                        file.selectionStart = file.selectionEnd;
                        e.target.value = file.selectionStart.toFixed(2);
                    }
                    // Save state after changing selection
                    saveState().catch(err => console.error('Failed to save state:', err));
                }
                updateSelectionDisplay();
            });
        }
        
        if (endTimeInput) {
            endTimeInput.addEventListener('change', (e) => {
                const buffer = getActiveBuffer();
                if (!buffer) return;
                
                if (hasActiveFile()) {
                    const file = getActiveFile();
                    if (!file) return;
                    file.selectionEnd = parseFloat(e.target.value) || 0;
                    if (file.selectionEnd < file.selectionStart) {
                        file.selectionEnd = file.selectionStart;
                        e.target.value = file.selectionEnd.toFixed(2);
                    }
                    if (file.currentBuffer && file.selectionEnd > file.currentBuffer.duration) {
                        file.selectionEnd = file.currentBuffer.duration;
                        e.target.value = file.selectionEnd.toFixed(2);
                    }
                    // Save state after changing selection
                    saveState().catch(err => console.error('Failed to save state:', err));
                } else {
                    // Demo mode
                    const start = parseFloat(startTimeInput?.value || 0);
                    let end = parseFloat(e.target.value) || 0;
                    if (end < start) {
                        end = start;
                        e.target.value = end.toFixed(2);
                    }
                    if (end > buffer.duration) {
                        end = buffer.duration;
                        e.target.value = end.toFixed(2);
                    }
                }
                updateSelectionDisplay();
            });
        }
        
        // Trim button
        if (trimBtn) {
            trimBtn.addEventListener('click', () => {
                if (!hasActiveFile()) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Загрузите файл для редактирования' : 'Load a file to edit', 'error');
                    return;
                }
                
                const file = getActiveFile();
                if (!file || !file.currentBuffer) return;
                
                try {
                    pushEditHistoryBeforeChange();
                    file.currentBuffer = extractSegment(file.currentBuffer, file.selectionStart, file.selectionEnd);
                    drawWaveform(file.currentBuffer);
                    
                    // Update time inputs
                    if (endTimeInput) endTimeInput.value = file.currentBuffer.duration.toFixed(2);
                    if (endTimeInput) endTimeInput.max = file.currentBuffer.duration.toFixed(2);
                    
                    file.selectionStart = 0;
                    file.selectionEnd = file.currentBuffer.duration;
                    updateSelectionDisplay();
                    
                    // Update duration display
                    updateFileInfo(file);
                    
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Аудио обрезано' : 'Audio trimmed', 'success');
                } catch (error) {
                    showStatus('Error trimming audio: ' + error.message, 'error');
                }
            });
        }
        
        // Reset button
        if (resetBtn) {
            resetBtn.addEventListener('click', () => {
                const file = getActiveFile();
                if (!file || !file.originalBuffer) return;
                
                pushEditHistoryBeforeChange();
                file.currentBuffer = file.originalBuffer;
                file.selectionStart = 0;
                file.selectionEnd = file.originalBuffer.duration;
                file.volume = 100;
                
                // Clear split markers and parts
                clearSplitMarkers();
                splitParts = [];
                const namesContainer = document.getElementById('splitPartsNames');
                if (namesContainer) namesContainer.style.display = 'none';
                
                if (startTimeInput) startTimeInput.value = '0';
                if (endTimeInput) endTimeInput.value = file.originalBuffer.duration.toFixed(2);
                
                // Reset volume slider
                if (volumeSlider) {
                    volumeSlider.value = '100';
                    if (volumeValue) volumeValue.textContent = '100%';
                }
                
                loadFileState(file);
                drawWaveform(file.currentBuffer);
                updateSelectionDisplay();
                updateFileInfo(file);
                
                const lang = getLanguage();
                const isRu = lang === 'ru';
                showStatus(isRu ? 'Сброшено к оригиналу' : 'Reset to original', 'info');
            });
        }
        
        const undoBtn = document.getElementById('undoBtn');
        const redoBtn = document.getElementById('redoBtn');
        if (undoBtn) {
            undoBtn.addEventListener('click', () => {
                if (!hasActiveFile()) return;
                performUndo();
            });
        }
        if (redoBtn) {
            redoBtn.addEventListener('click', () => {
                if (!hasActiveFile()) return;
                performRedo();
            });
        }
        
        // Create markers button
        const createMarkersBtn = document.getElementById('createMarkersBtn');
        const splitPartsCountInput = document.getElementById('splitPartsCount');
        if (createMarkersBtn && splitPartsCountInput) {
            createMarkersBtn.addEventListener('click', () => {
                if (!hasActiveFile()) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Загрузите файл для редактирования' : 'Load a file to edit', 'error');
                    return;
                }
                
                const file = getActiveFile();
                if (!file || !file.currentBuffer) return;
                
                const numParts = parseInt(splitPartsCountInput.value) || 4;
                if (numParts < 2 || numParts > 20) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Количество частей должно быть от 2 до 20' : 'Number of parts must be between 2 and 20', 'error');
                    return;
                }
                
                const selectionDuration = file.selectionEnd - file.selectionStart;
                if (selectionDuration <= 0) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Выделите участок для разделения' : 'Select a segment to split', 'error');
                    return;
                }
                
                pushEditHistoryBeforeChange();
                createSplitMarkers(numParts);
                
                const lang = getLanguage();
                const isRu = lang === 'ru';
                showStatus((isRu ? 'Метки созданы. Переместите их при необходимости и нажмите SPLIT' : 'Markers created. Move them if needed and press SPLIT'), 'info');
            });
        }
        
        // Split button - split by markers
        const splitBtn = document.getElementById('splitBtn');
        if (splitBtn) {
            splitBtn.addEventListener('click', () => {
                if (!hasActiveFile()) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Загрузите файл для редактирования' : 'Load a file to edit', 'error');
                    return;
                }
                
                const file = getActiveFile();
                if (!file || !file.currentBuffer) return;
                
                if (splitMarkers.length === 0) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Создайте метки сначала' : 'Create markers first', 'error');
                    return;
                }
                
                try {
                    pushEditHistoryBeforeChange();
                    // Sort markers and add selection boundaries
                    const allMarkers = [file.selectionStart, ...splitMarkers, file.selectionEnd].sort((a, b) => a - b);
                    
                    // Split into parts
                    splitParts = [];
                    for (let i = 0; i < allMarkers.length - 1; i++) {
                        const startTime = allMarkers[i];
                        const endTime = allMarkers[i + 1];
                        const partBuffer = extractSegment(file.currentBuffer, startTime, endTime);
                        
                        // Get base filename
                        const baseName = file.name.replace(/\.[^/.]+$/, '');
                        const defaultName = `${baseName}_part${i + 1}.wav`;
                        
                        splitParts.push({
                            buffer: partBuffer,
                            name: defaultName,
                            startTime: startTime,
                            endTime: endTime
                        });
                    }
                    
                    // Show names input UI
                    showSplitPartsNamesUI();
                    
                    // Enable PK3 export automatically
                    const exportToPk3Check = document.getElementById('exportToPk3');
                    const pk3Fields = document.getElementById('pk3Fields');
                    if (exportToPk3Check && pk3Fields) {
                        exportToPk3Check.checked = true;
                        pk3Fields.classList.add('visible');
                        const exportBtn = document.getElementById('exportBtn');
                        if (exportBtn) {
                            const lang = getLanguage();
                            const isRu = lang === 'ru';
                            exportBtn.textContent = isRu ? 'Экспорт PK3' : 'Export PK3';
                        }
                    }
                    
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus((isRu ? 'Разделено на ' : 'Split into ') + splitParts.length + (isRu ? ' частей. Укажите имена и экспортируйте в PK3' : ' parts. Set names and export to PK3'), 'success');
                    
                    // Clear markers
                    clearSplitMarkers();
                } catch (error) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus((isRu ? 'Ошибка разделения: ' : 'Error splitting: ') + error.message, 'error');
                }
            });
        }
        
        // Marker dragging
        if (waveformCanvas) {
            const waveformContainer = document.getElementById('waveformContainer');
            if (waveformContainer) {
                waveformContainer.addEventListener('mousedown', (e) => {
                    if (e.target.classList.contains('wav-editor-split-marker')) {
                        draggedMarkerIndex = parseInt(e.target.dataset.markerIndex);
                        e.target.classList.add('dragging');
                        e.preventDefault();
                    }
                });
                
                document.addEventListener('mousemove', (e) => {
                    if (draggedMarkerIndex >= 0 && hasActiveFile()) {
                        const file = getActiveFile();
                        if (!file || !file.currentBuffer) return;
                        
                        const waveformContainer = document.getElementById('waveformContainer');
                        if (!waveformContainer) return;
                        
                        const rect = waveformContainer.getBoundingClientRect();
                        const x = e.clientX - rect.left;
                        const percent = x / rect.width;
                        const duration = file.currentBuffer.duration;
                        const newTime = Math.max(file.selectionStart, Math.min(file.selectionEnd, percent * duration));
                        
                        splitMarkers[draggedMarkerIndex] = newTime;
                        splitMarkers.sort((a, b) => a - b);
                        
                        // Update dragged marker index after sort
                        draggedMarkerIndex = splitMarkers.indexOf(newTime);
                        
                        drawWaveform(file.currentBuffer);
                    }
                });
                
                document.addEventListener('mouseup', () => {
                    if (draggedMarkerIndex >= 0) {
                        const markers = waveformContainer.querySelectorAll('.wav-editor-split-marker');
                        markers.forEach(m => m.classList.remove('dragging'));
                        draggedMarkerIndex = -1;
                    }
                });
            }
        }
    
        // Preview fade button
        const previewFadeBtn = document.getElementById('previewFadeBtn');
        const fadeDurationInput = document.getElementById('fadeDuration');
        if (previewFadeBtn && fadeDurationInput) {
            previewFadeBtn.addEventListener('click', async () => {
                if (!hasActiveFile()) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Загрузите файл для редактирования' : 'Load a file to edit', 'error');
                    return;
                }
                
                const file = getActiveFile();
                if (!file || !file.currentBuffer) return;
                
                try {
                    const fadeDuration = parseFloat(fadeDurationInput.value) || 0;
                    if (fadeDuration < 0 || fadeDuration > 1000) {
                        const lang = getLanguage();
                        const isRu = lang === 'ru';
                        showStatus(isRu ? 'Длительность затухания должна быть от 0 до 1000 мс' : 'Fade duration must be between 0 and 1000 ms', 'error');
                        return;
                    }
                    
                    const selectionDuration = file.selectionEnd - file.selectionStart;
                    if (selectionDuration <= 0) {
                        const lang = getLanguage();
                        const isRu = lang === 'ru';
                        showStatus(isRu ? 'Выделите участок для предпрослушивания' : 'Select a segment to preview', 'error');
                        return;
                    }
                    
                    // Stop any current playback
                    stopPlayback();
                    
                    // Get markers or create equal parts
                    let parts;
                    if (splitMarkers.length > 0) {
                        const allMarkers = [file.selectionStart, ...splitMarkers, file.selectionEnd].sort((a, b) => a - b);
                        parts = [];
                        for (let i = 0; i < allMarkers.length - 1; i++) {
                            const partBuffer = extractSegment(file.currentBuffer, allMarkers[i], allMarkers[i + 1]);
                            parts.push(partBuffer);
                        }
                    } else {
                        const splitPartsCountInput = document.getElementById('splitPartsCount');
                        const numParts = parseInt(splitPartsCountInput?.value) || 4;
                        parts = splitSegment(file.currentBuffer, file.selectionStart, file.selectionEnd, numParts);
                    }
                    
                    // Apply fade to each part
                    const fadeInSeconds = fadeDuration / 1000;
                    const processedParts = parts.map(part => {
                        return applyFade(part, fadeInSeconds, fadeInSeconds);
                    });
                    
                    // Concatenate parts
                    const previewBuffer = concatenateBuffers(processedParts);
                    
                    // Play preview
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Предпрослушивание...' : 'Previewing...', 'info');
                    
                    playAudioBuffer(previewBuffer);
                } catch (error) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus((isRu ? 'Ошибка предпрослушивания: ' : 'Error previewing: ') + error.message, 'error');
                }
            });
        }
        
        // Apply fade button (old - remove or keep for backward compatibility)
        const applyFadeBtn = document.getElementById('applyFadeBtn');
        if (applyFadeBtn && fadeDurationInput) {
            applyFadeBtn.addEventListener('click', () => {
                if (!hasActiveFile()) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Загрузите файл для редактирования' : 'Load a file to edit', 'error');
                    return;
                }
                
                const file = getActiveFile();
                if (!file || !file.currentBuffer) return;
                
                try {
                    const fadeDuration = parseFloat(fadeDurationInput.value) || 0;
                    if (fadeDuration < 0 || fadeDuration > 1000) {
                        const lang = getLanguage();
                        const isRu = lang === 'ru';
                        showStatus(isRu ? 'Длительность затухания должна быть от 0 до 1000 мс' : 'Fade duration must be between 0 and 1000 ms', 'error');
                        return;
                    }
                    
                    const selectionDuration = file.selectionEnd - file.selectionStart;
                    if (selectionDuration <= 0) {
                        const lang = getLanguage();
                        const isRu = lang === 'ru';
                        showStatus(isRu ? 'Выделите участок для применения затухания' : 'Select a segment to apply fade', 'error');
                        return;
                    }
                    
                    pushEditHistoryBeforeChange();
                    const numParts = parseInt(splitPartsCountInput?.value) || 4;
                    const parts = splitSegment(file.currentBuffer, file.selectionStart, file.selectionEnd, numParts);
                    
                    // Apply fade to each part
                    const fadeInSeconds = fadeDuration / 1000; // Convert ms to seconds
                    const processedParts = parts.map(part => {
                        return applyFade(part, fadeInSeconds, fadeInSeconds);
                    });
                    
                    // Concatenate parts back together
                    const beforeSelection = extractSegment(file.currentBuffer, 0, file.selectionStart);
                    const afterSelection = extractSegment(file.currentBuffer, file.selectionEnd, file.currentBuffer.duration);
                    
                    const allParts = [];
                    if (beforeSelection.length > 0) {
                        allParts.push(beforeSelection);
                    }
                    allParts.push(...processedParts);
                    if (afterSelection.length > 0) {
                        allParts.push(afterSelection);
                    }
                    
                    file.currentBuffer = concatenateBuffers(allParts);
                    
                    // Update UI
                    drawWaveform(file.currentBuffer);
                    updateSelectionDisplay();
                    updateFileInfo(file);
                    
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus((isRu ? 'Затухание применено к ' : 'Fade applied to ') + numParts + (isRu ? ' частям' : ' parts'), 'success');
                    
                    // Save state after applying fade
                    saveCurrentFileState();
                    saveState().catch(err => console.error('Failed to save state:', err));
                } catch (error) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus((isRu ? 'Ошибка применения затухания: ' : 'Error applying fade: ') + error.message, 'error');
                }
            });
        }
        
        // Playback controls — play always uses current selection (or demo range)
        if (playBtn) {
            playBtn.addEventListener('click', () => {
                const buffer = getActiveBuffer();
                if (!buffer) return;
                
                let start = 0;
                let end = buffer.duration;
                
                if (hasActiveFile()) {
                    const file = getActiveFile();
                    if (!file) return;
                    start = file.selectionStart;
                    end = file.selectionEnd;
                } else {
                    start = parseFloat(startTimeInput?.value || 0);
                    end = parseFloat(endTimeInput?.value || buffer.duration);
                }
                
                playAudio(start, end);
            });
        }
        
        if (pauseBtn) {
            pauseBtn.addEventListener('click', () => {
                stopPlayback();
            });
        }
        
        if (stopBtn) {
            stopBtn.addEventListener('click', () => {
                stopPlayback();
            });
        }
        
        if (selectAllBtn) {
            selectAllBtn.addEventListener('click', () => {
                const buffer = getActiveBuffer();
                if (!buffer) return;
                
                const dur = buffer.duration;
                const durStr = dur.toFixed(2);
                
                if (hasActiveFile()) {
                    const file = getActiveFile();
                    if (!file) return;
                    file.selectionStart = 0;
                    file.selectionEnd = dur;
                    if (startTimeInput) {
                        startTimeInput.value = '0';
                        startTimeInput.max = durStr;
                    }
                    if (endTimeInput) {
                        endTimeInput.value = durStr;
                        endTimeInput.max = durStr;
                    }
                    updateSelectionDisplay();
                    saveState().catch(err => console.error('Failed to save state:', err));
                } else {
                    if (startTimeInput) {
                        startTimeInput.value = '0';
                        startTimeInput.max = durStr;
                    }
                    if (endTimeInput) {
                        endTimeInput.value = durStr;
                        endTimeInput.max = durStr;
                    }
                    updateDemoSelectionDisplay();
                }
            });
        }
        
        // Volume control
        if (volumeSlider && volumeValue) {
            volumeSlider.addEventListener('input', (e) => {
                const value = parseInt(e.target.value);
                if (volumeValue) {
                    volumeValue.textContent = value + '%';
                }
            });
        }
        
        if (applyVolumeBtn) {
            applyVolumeBtn.addEventListener('click', () => {
                if (!hasActiveFile()) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Загрузите файл для редактирования' : 'Load a file to edit', 'error');
                    return;
                }
                
                const file = getActiveFile();
                if (!file || !file.currentBuffer || !volumeSlider) return;
                
                try {
                    pushEditHistoryBeforeChange();
                    const volumePercent = parseInt(volumeSlider.value);
                    file.currentBuffer = changeVolume(file.currentBuffer, volumePercent);
                    file.volume = volumePercent;
                    
                    // Update waveform
                    drawWaveform(file.currentBuffer);
                    
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Громкость изменена' : 'Volume changed', 'success');
                    
                    // Save state after volume change
                    saveCurrentFileState();
                    saveState().catch(err => console.error('Failed to save state:', err));
                } catch (error) {
                    showStatus('Error changing volume: ' + error.message, 'error');
                }
            });
        }
        
        // PK3 export toggle
        if (exportToPk3Check && pk3Fields && exportBtn) {
            exportToPk3Check.addEventListener('change', (e) => {
                const lang = getLanguage();
                const isRu = lang === 'ru';
                
                if (e.target.checked) {
                    pk3Fields.classList.add('visible');
                    exportBtn.textContent = isRu ? 'Экспорт PK3' : 'Export PK3';
                } else {
                    pk3Fields.classList.remove('visible');
                    exportBtn.textContent = isRu ? 'Экспорт WAV' : 'Export WAV';
                }
            });
        }
        
        // Export button
        if (exportBtn) {
            exportBtn.addEventListener('click', async () => {
                const lang = getLanguage();
                const isRu = lang === 'ru';
                
                // Check if we have split parts to export
                if (splitParts.length > 0) {
                    // Export split parts to PK3
                    if (typeof JSZip === 'undefined') {
                        showStatus(isRu ? 'JSZip не загружен' : 'JSZip not loaded', 'error');
                        return;
                    }
                    
                    try {
                        showStatus(isRu ? 'Экспорт частей в PK3...' : 'Exporting parts to PK3...', 'info');
                        
                        // Get PK3 filename and path
                        const pk3FilenameInput = document.getElementById('pk3Filename');
                        const pk3PathInput = document.getElementById('pk3Path');
                        
                        let pk3Filename = 'sounds.pk3';
                        if (pk3FilenameInput && pk3FilenameInput.value.trim()) {
                            pk3Filename = pk3FilenameInput.value.trim();
                            if (!pk3Filename.toLowerCase().endsWith('.pk3')) {
                                pk3Filename += '.pk3';
                            }
                        }
                        
                        let pk3Path = 'sound/';
                        if (pk3PathInput && pk3PathInput.value.trim()) {
                            pk3Path = pk3PathInput.value.trim();
                            if (!pk3Path.endsWith('/')) {
                                pk3Path += '/';
                            }
                        }
                        
                        // Create zip archive
                        const zip = new JSZip();
                        
                        // Process each split part
                        for (const part of splitParts) {
                            let exportBuffer = part.buffer;
                            
                            // Always convert to mono for Quake 3
                            exportBuffer = convertToMono(exportBuffer);
                            
                            // Always resample to 22050 Hz for Quake 3
                            if (exportBuffer.sampleRate !== 22050) {
                                exportBuffer = await resampleAudioBuffer(exportBuffer, 22050);
                            }
                            
                            // Convert to WAV
                            const wav = audioBufferToWav(exportBuffer);
                            
                            // Get filename from part name
                            let filename = part.name;
                            if (!filename.toLowerCase().endsWith('.wav')) {
                                filename += '.wav';
                            }
                            
                            zip.file(pk3Path + filename, wav);
                        }
                        
                        // Generate PK3 file
                        zip.generateAsync({ type: 'blob', compression: 'DEFLATE' }).then((pk3Blob) => {
                            // Download PK3
                            const url = URL.createObjectURL(pk3Blob);
                            const a = document.createElement('a');
                            a.href = url;
                            a.download = pk3Filename;
                            document.body.appendChild(a);
                            a.click();
                            document.body.removeChild(a);
                            URL.revokeObjectURL(url);
                            
                            showStatus((isRu ? 'PK3 экспортирован успешно с ' : 'PK3 exported successfully with ') + splitParts.length + (isRu ? ' частями' : ' parts'), 'success');
                        }).catch((error) => {
                            showStatus((isRu ? 'Ошибка создания PK3: ' : 'Error creating PK3: ') + error.message, 'error');
                            console.error(error);
                        });
                    } catch (error) {
                        showStatus((isRu ? 'Ошибка экспорта: ' : 'Error exporting: ') + error.message, 'error');
                        console.error(error);
                    }
                    return;
                }
                
                // Regular export (single file)
                if (!hasActiveFile()) {
                    showStatus(isRu ? 'Загрузите файл для экспорта' : 'Load a file to export', 'error');
                    return;
                }
                
                const file = getActiveFile();
                if (!file || !file.currentBuffer) {
                    showStatus(isRu ? 'Аудио не загружено' : 'No audio loaded', 'error');
                    return;
                }
                
                try {
                    let exportBuffer = file.currentBuffer;
                    
                    // Always convert to mono for Quake 3
                    exportBuffer = convertToMono(exportBuffer);
                    
                    // Always resample to 22050 Hz for Quake 3
                    if (exportBuffer.sampleRate !== 22050) {
                        showStatus(isRu ? 'Ресемплинг...' : 'Resampling...', 'info');
                        exportBuffer = await resampleAudioBuffer(exportBuffer, 22050);
                    }
                    
                    // Convert to WAV
                    const wav = audioBufferToWav(exportBuffer);
                    const blob = new Blob([wav], { type: 'audio/wav' });
                    
                    // Get filename from input
                    const filenameInput = document.getElementById('exportFilename');
                    let filename = 'quake3_sound.wav';
                    if (filenameInput && filenameInput.value.trim()) {
                        filename = filenameInput.value.trim();
                        // Ensure .wav extension
                        if (!filename.toLowerCase().endsWith('.wav')) {
                            filename += '.wav';
                        }
                    }
                    
                    // Check if PK3 export is enabled
                    const exportToPk3 = exportToPk3Check && exportToPk3Check.checked;
                    
                    if (exportToPk3) {
                        // Create PK3 archive
                        if (typeof JSZip === 'undefined') {
                            showStatus(isRu ? 'JSZip не загружен' : 'JSZip not loaded', 'error');
                            return;
                        }
                        
                        // Get PK3 filename and path
                        const pk3FilenameInput = document.getElementById('pk3Filename');
                        const pk3PathInput = document.getElementById('pk3Path');
                        
                        let pk3Filename = 'sounds.pk3';
                        if (pk3FilenameInput && pk3FilenameInput.value.trim()) {
                            pk3Filename = pk3FilenameInput.value.trim();
                            if (!pk3Filename.toLowerCase().endsWith('.pk3')) {
                                pk3Filename += '.pk3';
                            }
                        }
                        
                        let pk3Path = 'sound/';
                        if (pk3PathInput && pk3PathInput.value.trim()) {
                            pk3Path = pk3PathInput.value.trim();
                            // Ensure path ends with /
                            if (!pk3Path.endsWith('/')) {
                                pk3Path += '/';
                            }
                        }
                        
                        // Create zip archive
                        const zip = new JSZip();
                        zip.file(pk3Path + filename, wav);
                        
                        // Generate PK3 file
                        zip.generateAsync({ type: 'blob', compression: 'DEFLATE' }).then((pk3Blob) => {
                            // Download PK3
                            const url = URL.createObjectURL(pk3Blob);
                            const a = document.createElement('a');
                            a.href = url;
                            a.download = pk3Filename;
                            document.body.appendChild(a);
                            a.click();
                            document.body.removeChild(a);
                            URL.revokeObjectURL(url);
                            
                            showStatus(isRu ? 'PK3 экспортирован успешно' : 'PK3 exported successfully', 'success');
                        }).catch((error) => {
                            showStatus((isRu ? 'Ошибка создания PK3: ' : 'Error creating PK3: ') + error.message, 'error');
                            console.error(error);
                        });
                    } else {
                        // Download WAV directly
                        const url = URL.createObjectURL(blob);
                        const a = document.createElement('a');
                        a.href = url;
                        a.download = filename;
                        document.body.appendChild(a);
                        a.click();
                        document.body.removeChild(a);
                        URL.revokeObjectURL(url);
                        
                        showStatus(isRu ? 'WAV экспортирован успешно' : 'WAV exported successfully', 'success');
                    }
                } catch (error) {
                    showStatus('Error exporting WAV: ' + error.message, 'error');
                    console.error(error);
                }
            });
        }
        
        // Pitch controls
        const pitchBtn = document.getElementById('pitchBtn');
        const pitchOptions = document.getElementById('pitchOptions');
        const generatePitchBtn = document.getElementById('generatePitchBtn');
        const cancelPitchBtn = document.getElementById('cancelPitchBtn');
        const downloadPitchBtn = document.getElementById('downloadPitchBtn');
        const pitchFilesCountInput = document.getElementById('pitchFilesCount');
        const pitchTargetInput = document.getElementById('pitchTarget');
        const pitchDirectionSelect = document.getElementById('pitchDirection');
        const pitchPk3FilenameInput = document.getElementById('pitchPk3Filename');
        const pitchPk3PathInput = document.getElementById('pitchPk3Path');
        
        if (pitchBtn && pitchOptions) {
            pitchBtn.addEventListener('click', () => {
                if (!hasActiveFile()) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Загрузите файл для редактирования' : 'Load a file to edit', 'error');
                    return;
                }
                
                const file = getActiveFile();
                if (!file || !file.currentBuffer) return;
                
                const selectionDuration = file.selectionEnd - file.selectionStart;
                if (selectionDuration <= 0) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Выделите участок для питча' : 'Select a segment for pitch', 'error');
                    return;
                }
                
                pitchOptions.style.display = 'block';
            });
        }
        
        if (cancelPitchBtn && pitchOptions) {
            cancelPitchBtn.addEventListener('click', () => {
                pitchOptions.style.display = 'none';
                const pitchFilesNames = document.getElementById('pitchFilesNames');
                const pitchDownloadSection = document.getElementById('pitchDownloadSection');
                if (pitchFilesNames) pitchFilesNames.style.display = 'none';
                if (pitchDownloadSection) pitchDownloadSection.style.display = 'none';
                pitchFiles = [];
            });
        }
        
        if (generatePitchBtn && pitchFilesCountInput && pitchTargetInput && pitchDirectionSelect) {
            generatePitchBtn.addEventListener('click', async () => {
                if (!hasActiveFile()) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Загрузите файл для редактирования' : 'Load a file to edit', 'error');
                    return;
                }
                
                const file = getActiveFile();
                if (!file || !file.currentBuffer) return;
                
                const selectionDuration = file.selectionEnd - file.selectionStart;
                if (selectionDuration <= 0) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Выделите участок для питча' : 'Select a segment for pitch', 'error');
                    return;
                }
                
                try {
                    const numFiles = parseInt(pitchFilesCountInput.value) || 4;
                    if (numFiles < 1 || numFiles > 20) {
                        const lang = getLanguage();
                        const isRu = lang === 'ru';
                        showStatus(isRu ? 'Количество файлов должно быть от 1 до 20' : 'Number of files must be between 1 and 20', 'error');
                        return;
                    }
                    
                    const pitchFinal = parseFloat(pitchTargetInput.value);
                    if (!(pitchFinal > 0) || pitchFinal > 36) {
                        const lang = getLanguage();
                        const isRu = lang === 'ru';
                        showStatus(isRu ? 'Финальный питч: от 0.1 до 36 полутонов' : 'Final pitch must be between 0.1 and 36 semitones', 'error');
                        return;
                    }
                    
                    const direction = pitchDirectionSelect.value;
                    const sign = direction === 'up' ? 1 : -1;
                    
                    // Extract selected segment
                    const segmentBuffer = extractSegment(file.currentBuffer, file.selectionStart, file.selectionEnd);
                    
                    // Get base filename
                    const baseName = file.name.replace(/\.[^/.]+$/, '');
                    
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Генерация файлов с питчем...' : 'Generating pitch files...', 'info');
                    
                    // Generate pitch-shifted files: evenly from 0 to sign * pitchFinal (last file)
                    pitchFiles = [];
                    for (let i = 0; i < numFiles; i++) {
                        let pitchShift = 0;
                        if (numFiles === 1) {
                            pitchShift = sign * pitchFinal;
                        } else {
                            pitchShift = sign * (i / (numFiles - 1)) * pitchFinal;
                        }
                        
                        let pitchShiftedBuffer;
                        if (pitchShift === 0) {
                            pitchShiftedBuffer = segmentBuffer;
                        } else {
                            pitchShiftedBuffer = await changePitch(segmentBuffer, pitchShift);
                        }
                        
                        const defaultName = `${baseName}_pitch${i + 1}.wav`;
                        pitchFiles.push({
                            buffer: pitchShiftedBuffer,
                            name: defaultName,
                            pitchShift: pitchShift
                        });
                    }
                    
                    // Show names input UI
                    showPitchFilesNamesUI();
                    
                    // Show download section
                    const pitchDownloadSection = document.getElementById('pitchDownloadSection');
                    if (pitchDownloadSection) {
                        pitchDownloadSection.style.display = 'block';
                    }
                    
                    showStatus((isRu ? 'Сгенерировано ' : 'Generated ') + pitchFiles.length + (isRu ? ' файлов с питчем' : ' pitch files'), 'success');
                } catch (error) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus((isRu ? 'Ошибка генерации питча: ' : 'Error generating pitch: ') + error.message, 'error');
                    console.error(error);
                }
            });
        }
        
        function showPitchFilesNamesUI() {
            const namesContainer = document.getElementById('pitchFilesNames');
            if (!namesContainer) return;
            
            const lang = getLanguage();
            const isRu = lang === 'ru';
            
            namesContainer.style.display = 'block';
            namesContainer.innerHTML = '<div style="font-size: 0.85rem; color: rgba(255, 255, 255, 0.8); margin-bottom: 8px;">' + 
                (isRu ? 'Имена файлов:' : 'File names:') + '</div>';
            
            pitchFiles.forEach((pitchFile, index) => {
                const row = document.createElement('div');
                row.className = 'wav-editor-control-row';
                row.style.marginTop = '6px';
                
                const label = document.createElement('label');
                const pitchShiftText = pitchFile.pitchShift === 0 ? '0' : (pitchFile.pitchShift > 0 ? '+' : '') + pitchFile.pitchShift.toFixed(1);
                label.textContent = `${isRu ? 'Файл' : 'File'} ${index + 1} (${pitchShiftText}${isRu ? ' полутонов' : ' semitones'}):`;
                label.style.minWidth = '150px';
                
                const input = document.createElement('input');
                input.type = 'text';
                input.className = 'wav-editor-filename-input';
                input.value = pitchFile.name;
                input.style.flex = '1';
                input.addEventListener('change', (e) => {
                    pitchFiles[index].name = e.target.value || pitchFile.name;
                });
                
                // Add play button
                const playBtn = document.createElement('button');
                playBtn.className = 'wav-editor-playback-btn';
                playBtn.textContent = '▶';
                playBtn.title = isRu ? 'Прослушать' : 'Play';
                playBtn.style.marginLeft = '8px';
                playBtn.addEventListener('click', () => {
                    // Stop any current playback
                    stopPlayback();
                    // Play this pitch file
                    playAudioBuffer(pitchFile.buffer);
                });
                
                row.appendChild(label);
                row.appendChild(input);
                row.appendChild(playBtn);
                namesContainer.appendChild(row);
            });
        }
        
        if (downloadPitchBtn) {
            downloadPitchBtn.addEventListener('click', async () => {
                if (pitchFiles.length === 0) {
                    const lang = getLanguage();
                    const isRu = lang === 'ru';
                    showStatus(isRu ? 'Нет файлов для скачивания' : 'No files to download', 'error');
                    return;
                }
                
                const lang = getLanguage();
                const isRu = lang === 'ru';
                
                if (typeof JSZip === 'undefined') {
                    showStatus(isRu ? 'JSZip не загружен' : 'JSZip not loaded', 'error');
                    return;
                }
                
                try {
                    showStatus(isRu ? 'Создание PK3 архива...' : 'Creating PK3 archive...', 'info');
                    
                    // Get PK3 filename and path
                    let pk3Filename = 'pitch_sounds.pk3';
                    if (pitchPk3FilenameInput && pitchPk3FilenameInput.value.trim()) {
                        pk3Filename = pitchPk3FilenameInput.value.trim();
                        if (!pk3Filename.toLowerCase().endsWith('.pk3')) {
                            pk3Filename += '.pk3';
                        }
                    }
                    
                    let pk3Path = 'sound/';
                    if (pitchPk3PathInput && pitchPk3PathInput.value.trim()) {
                        pk3Path = pitchPk3PathInput.value.trim();
                        if (!pk3Path.endsWith('/')) {
                            pk3Path += '/';
                        }
                    }
                    
                    // Create zip archive
                    const zip = new JSZip();
                    
                    // Process each pitch file
                    for (const pitchFile of pitchFiles) {
                        let exportBuffer = pitchFile.buffer;
                        
                        // Always convert to mono for Quake 3
                        exportBuffer = convertToMono(exportBuffer);
                        
                        // Always resample to 22050 Hz for Quake 3
                        if (exportBuffer.sampleRate !== 22050) {
                            exportBuffer = await resampleAudioBuffer(exportBuffer, 22050);
                        }
                        
                        // Convert to WAV
                        const wav = audioBufferToWav(exportBuffer);
                        
                        // Get filename from pitch file name
                        let filename = pitchFile.name;
                        if (!filename.toLowerCase().endsWith('.wav')) {
                            filename += '.wav';
                        }
                        
                        zip.file(pk3Path + filename, wav);
                    }
                    
                    // Generate PK3 file
                    zip.generateAsync({ type: 'blob', compression: 'DEFLATE' }).then((pk3Blob) => {
                        // Download PK3
                        const url = URL.createObjectURL(pk3Blob);
                        const a = document.createElement('a');
                        a.href = url;
                        a.download = pk3Filename;
                        document.body.appendChild(a);
                        a.click();
                        document.body.removeChild(a);
                        URL.revokeObjectURL(url);
                        
                        showStatus((isRu ? 'PK3 экспортирован успешно с ' : 'PK3 exported successfully with ') + pitchFiles.length + (isRu ? ' файлами' : ' files'), 'success');
                    }).catch((error) => {
                        showStatus((isRu ? 'Ошибка создания PK3: ' : 'Error creating PK3: ') + error.message, 'error');
                        console.error(error);
                    });
                } catch (error) {
                    showStatus((isRu ? 'Ошибка скачивания: ' : 'Error downloading: ') + error.message, 'error');
                    console.error(error);
                }
            });
        }
        
        // Return the loadAudioFile function so it can be called from outside
        return loadAudioFile;
    }
    
    // Initialize when DOM is ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', function() {
            const wavEditorModal = document.getElementById('wavEditorModal');
            const openWavEditorBtn = document.getElementById('openWavEditorModal');
            const closeWavEditorModal = document.getElementById('closeWavEditorModal');
            
            if (!wavEditorModal || !openWavEditorBtn || !closeWavEditorModal) return;
            
            async function openWavEditorModal() {
                // Load tool first
                await loadWavEditorTool();
                
                // Show the modal first so canvas can get dimensions
                wavEditorModal.classList.add('active');
                document.body.style.overflow = 'hidden';
                
                // Small delay to ensure modal is rendered
                await new Promise(resolve => setTimeout(resolve, 10));
                
                // Try to load saved state
                const stateLoaded = await loadState();
                
                if (stateLoaded && audioFiles.length > 0) {
                    // Restore UI from saved state
                    await restoreUIFromState();
                } else {
                    // Load demo buffer if no files loaded
                    if (audioFiles.length === 0) {
                        await loadDemoBuffer();
                        if (demoBuffer) {
                            showDemoMode();
                        }
                    }
                }
                
                if (history.pushState) {
                    history.pushState(null, null, '#waveditor');
                } else {
                    window.location.hash = '#waveditor';
                }
                
                if (window.ChangeTracker) {
                    window.ChangeTracker.resetChanges('waveditor');
                }
            }
            
            async function closeWavEditorModalFunc() {
                stopPlayback();
                
                // Save current state before closing
                saveCurrentFileState();
                await saveState();
                
                // Don't clear state - it's saved in IndexedDB
                // Just clear demo buffer
                demoBuffer = null;
                
                wavEditorModal.classList.remove('active');
                document.body.style.overflow = '';
                
                if (history.pushState) {
                    history.pushState(null, null, window.location.pathname + window.location.search);
                } else {
                    window.location.hash = '';
                }
                
                if (window.ChangeTracker) {
                    window.ChangeTracker.resetChanges('waveditor');
                }
            }
            
            openWavEditorBtn.addEventListener('click', function(e) {
                e.preventDefault();
                e.stopPropagation();
                openWavEditorModal();
            });
            
            closeWavEditorModal.addEventListener('click', function() {
                closeWavEditorModalFunc();
            });
            
            var mouseDownOnBackground = false;
            wavEditorModal.addEventListener('mousedown', function(e) {
                mouseDownOnBackground = (e.target === wavEditorModal);
            });
            
            var handleMouseUp = function(e) {
                if (mouseDownOnBackground && e.target === wavEditorModal) {
                    closeWavEditorModalFunc();
                }
                mouseDownOnBackground = false;
            };
            
            wavEditorModal.addEventListener('mouseup', handleMouseUp);
            
            // Handle ESC key
            document.addEventListener('keydown', function(e) {
                if (e.key === 'Escape' && wavEditorModal.classList.contains('active')) {
                    closeWavEditorModalFunc();
                }
            });
            
            // Handle hash change
            if (window.location.hash === '#waveditor') {
                openWavEditorModal();
            }
        });
    } else {
        // DOM already loaded
        const wavEditorModal = document.getElementById('wavEditorModal');
        const openWavEditorBtn = document.getElementById('openWavEditorModal');
        const closeWavEditorModal = document.getElementById('closeWavEditorModal');
        
            if (wavEditorModal && openWavEditorBtn && closeWavEditorModal) {
            async function openWavEditorModal() {
                // Load tool first
                await loadWavEditorTool();
                
                // Show the modal first so canvas can get dimensions
                wavEditorModal.classList.add('active');
                document.body.style.overflow = 'hidden';
                
                // Small delay to ensure modal is rendered
                await new Promise(resolve => setTimeout(resolve, 10));
                
                // Try to load saved state
                const stateLoaded = await loadState();
                
                if (stateLoaded && audioFiles.length > 0) {
                    // Restore UI from saved state
                    await restoreUIFromState();
                } else {
                    // Load demo buffer if no files loaded
                    if (audioFiles.length === 0) {
                        await loadDemoBuffer();
                        if (demoBuffer) {
                            showDemoMode();
                        }
                    }
                }
                
                if (history.pushState) {
                    history.pushState(null, null, '#waveditor');
                } else {
                    window.location.hash = '#waveditor';
                }
                
                if (window.ChangeTracker) {
                    window.ChangeTracker.resetChanges('waveditor');
                }
            }
            
            async function closeWavEditorModalFunc() {
                stopPlayback();
                
                // Save current state before closing
                saveCurrentFileState();
                await saveState();
                
                // Don't clear state - it's saved in IndexedDB
                // Just clear demo buffer
                demoBuffer = null;
                
                wavEditorModal.classList.remove('active');
                document.body.style.overflow = '';
                
                if (history.pushState) {
                    history.pushState(null, null, window.location.pathname + window.location.search);
                } else {
                    window.location.hash = '';
                }
                
                if (window.ChangeTracker) {
                    window.ChangeTracker.resetChanges('waveditor');
                }
            }
            
            openWavEditorBtn.addEventListener('click', function(e) {
                e.preventDefault();
                e.stopPropagation();
                openWavEditorModal();
            });
            
            closeWavEditorModal.addEventListener('click', function() {
                closeWavEditorModalFunc();
            });
            
            var mouseDownOnBackground = false;
            wavEditorModal.addEventListener('mousedown', function(e) {
                mouseDownOnBackground = (e.target === wavEditorModal);
            });
            
            var handleMouseUp = function(e) {
                if (mouseDownOnBackground && e.target === wavEditorModal) {
                    closeWavEditorModalFunc();
                }
                mouseDownOnBackground = false;
            };
            
            wavEditorModal.addEventListener('mouseup', handleMouseUp);
            
            document.addEventListener('keydown', function(e) {
                if (e.key === 'Escape' && wavEditorModal.classList.contains('active')) {
                    closeWavEditorModalFunc();
                }
            });
            
            if (window.location.hash === '#waveditor') {
                openWavEditorModal();
            }
        }
    }
})();
