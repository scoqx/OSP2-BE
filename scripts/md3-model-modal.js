// Q3 Model Inspector — MD3 metadata and surface shaders (no 3D preview)
(function () {
    'use strict';

    const ACCENT = '#f0c040';
    const ACCENT_DIM = 'rgba(240, 192, 64, 0.35)';


    function readIdent(view, offset) {
        const u8 = new Uint8Array(view.buffer, offset, 4);
        return String.fromCharCode(u8[0], u8[1], u8[2], u8[3]);
    }

    function assertIdP3(view, offset, context) {
        const id = readIdent(view, offset);
        if (id !== 'IDP3') {
            const got = view.getInt32(offset, true);
            throw new Error(
                t(
                    (context || 'MD3') + ': expected IDP3, got "' + id + '" (0x' + (got >>> 0).toString(16) + ')',
                    (context || 'MD3') + ': ожидался IDP3, получено "' + id + '" (0x' + (got >>> 0).toString(16) + ')'
                )
            );
        }
    }

    function getLanguage() {
        return window.location.pathname.indexOf('/ru/') >= 0 ? 'ru' : 'en';
    }

    function t(en, ru) {
        return getLanguage() === 'ru' ? ru : en;
    }

    function readCString(view, offset, maxLen) {
        const u8 = new Uint8Array(view.buffer, offset, maxLen);
        let len = 0;
        while (len < maxLen && u8[len] !== 0) len++;
        return new TextDecoder('latin1').decode(u8.subarray(0, len));
    }

    function writeCString(view, offset, maxLen, str) {
        const enc = new TextEncoder().encode((str || '').slice(0, maxLen - 1));
        const u8 = new Uint8Array(view.buffer, offset, maxLen);
        u8.fill(0);
        u8.set(enc.subarray(0, maxLen - 1));
    }

    /**
     * Surface layout per io_scene_md3/fmt_md3.py (blender-md3):
     * magic, name[64], flags, nFrames, nShaders, nVerts, nTris, offTris, offShaders, offST, offVerts, offEnd
     * Shaders only in lump at offShaders (64-byte name + int index, stride 68).
     */
    function readSurfaceShaderInfo(view, surfOfs) {
        const numShaders = view.getInt32(surfOfs + 76, true);
        const ofsShaders = view.getInt32(surfOfs + 92, true);
        const lump = [];
        let shader = '';
        let shaderOffset = 0;

        if (numShaders > 0 && ofsShaders >= 0) {
            const stride = 68;
            for (let si = 0; si < numShaders; si++) {
                const off = surfOfs + ofsShaders + si * stride;
                if (off + 64 > view.buffer.byteLength) break;
                const name = readCString(view, off, 64).trim();
                lump.push({ name, offset: off });
                if (name && !shader) {
                    shader = name;
                    shaderOffset = off;
                }
            }
        }

        const displayShader =
            lump
                .map(function (x) {
                    return x.name;
                })
                .filter(Boolean)
                .join(', ') || shader;

        return { shader, displayShader, shaderOffset, numShaders, lump };
    }

    function shaderForDisplay(surf) {
        return surf.displayShader || surf.shader || '';
    }

    function parseMD3(buffer) {
        const view = new DataView(buffer);
        if (buffer.byteLength < 108) throw new Error(t('File too small for MD3', 'Файл слишком мал для MD3'));
        assertIdP3(view, 0, t('File header', 'Заголовок файла'));
        const version = view.getInt32(4, true);
        if (version !== 15) {
            console.warn('MD3 version', version, '(expected 15)');
        }
        const modelName = readCString(view, 8, 64);
        const flags = view.getInt32(72, true);
        const numFrames = view.getInt32(76, true);
        const numTags = view.getInt32(80, true);
        const numSurfaces = view.getInt32(84, true);
        const ofsTags = view.getInt32(96, true);
        const ofsSurfaces = view.getInt32(100, true);
        const ofsEnd = view.getInt32(104, true);

        const tags = [];
        let tagOfs = ofsTags;
        for (let i = 0; i < numTags; i++) {
            if (tagOfs + 68 > buffer.byteLength) break;
            const tagName = readCString(view, tagOfs, 64);
            tags.push({ name: tagName, numFrames: numFrames });
            tagOfs += 64 + numFrames * 48;
        }

        const surfaces = [];
        let surfOfs = ofsSurfaces;
        for (let i = 0; i < numSurfaces; i++) {
            if (surfOfs + 108 > buffer.byteLength) break;
            assertIdP3(view, surfOfs, t('Surface ' + i, 'Surface ' + i));
            const sh = readSurfaceShaderInfo(view, surfOfs);
            surfaces.push({
                index: i,
                name: readCString(view, surfOfs + 4, 64),
                shader: sh.shader,
                displayShader: sh.displayShader,
                shaderOffset: sh.shaderOffset,
                numShaders: sh.numShaders,
                shaderLump: sh.lump,
                numVerts: view.getInt32(surfOfs + 80, true),
                numTriangles: view.getInt32(surfOfs + 84, true)
            });
            const surfSize = view.getInt32(surfOfs + 104, true);
            if (surfSize <= 0 || surfOfs + surfSize > buffer.byteLength) {
                throw new Error(
                    t('Invalid surface size at surface ' + i, 'Некорректный размер surface ' + i)
                );
            }
            surfOfs += surfSize;
        }

        return {
            buffer,
            view,
            modelName,
            version,
            flags,
            numFrames,
            numTags,
            numSurfaces,
            ofsEnd,
            tags,
            surfaces,
            fileName: ''
        };
    }

    function getMd3ToolHTML() {
        return (
            '<style>' +
            '.md3-tool-root{color-scheme:dark;--black:#000;--white:#fff;--accent:' +
            ACCENT +
            ';--accent-dim:' +
            ACCENT_DIM +
            ';--border:rgba(255,255,255,0.22)}' +
            '.md3-tool-root *{box-sizing:border-box}' +
            '.md3-tool-app{width:100%;font-family:var(--font-family,Arial,sans-serif);color:var(--white)}' +
            '.md3-tool-app h1{font-size:1.2rem;text-transform:uppercase;letter-spacing:.08em;margin:0 0 6px}' +
            '.md3-tool-app h1 span{color:var(--accent)}' +
            '.md3-tool-app .subtitle{font-size:.78rem;color:rgba(255,255,255,.65);margin-bottom:14px;line-height:1.4}' +
            '.md3-drop{position:relative;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:10px;' +
            'border:2px dashed var(--border);border-radius:8px;padding:16px 20px;text-align:center;cursor:pointer;margin-bottom:12px;background:rgba(255,255,255,.03);min-height:80px}' +
            '.md3-drop.dragover{border-color:var(--accent);background:var(--accent-dim)}' +
            '.md3-drop-hint{margin:0;line-height:1.45;color:rgba(255,255,255,.85)}.md3-drop-hint strong{color:var(--accent)}' +
            '.md3-file-input{position:absolute;width:1px;height:1px;padding:0;margin:-1px;overflow:hidden;clip:rect(0,0,0,0);white-space:nowrap;border:0}' +
            '.md3-browse-btn{width:100%;max-width:100%;margin:0;padding:8px 14px;border-radius:6px;border:2px solid var(--accent);background:transparent;color:var(--accent);' +
            'font-weight:bold;font-size:.78rem;cursor:pointer;line-height:1.2;box-sizing:border-box}' +
            '.md3-main .md3-actions .md3-btn{width:100%;max-width:100%;box-sizing:border-box}' +
            '.md3-browse-btn:hover{background:var(--accent-dim)}' +
            '.md3-name-banner{display:none;width:100%;box-sizing:border-box;margin:0 0 12px;padding:10px 12px;' +
            'border:1px solid var(--border);border-radius:8px;background:rgba(255,255,255,.05)}' +
            '.md3-name-banner.visible{display:block}' +
            '.md3-name-banner b{display:block;color:var(--accent);font-size:.7rem;text-transform:uppercase;margin-bottom:6px}' +
            '.md3-name-value{display:block;width:100%;font-size:.8rem;line-height:1.45;word-break:break-all;overflow-wrap:anywhere;color:rgba(255,255,255,.92)}' +
            '.md3-meta{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:8px;margin-bottom:12px}' +
            '.md3-meta-item{background:rgba(255,255,255,.05);border:1px solid var(--border);border-radius:6px;padding:8px 10px;font-size:.75rem}' +
            '.md3-meta-item b{display:block;color:var(--accent);font-size:.7rem;text-transform:uppercase;margin-bottom:2px}' +
            '.md3-table-wrap{overflow:auto;max-height:42vh;border:1px solid var(--border);border-radius:8px}' +
            '.md3-table{width:100%;border-collapse:collapse;font-size:.78rem}' +
            '.md3-table th{position:sticky;top:0;background:#111;color:var(--accent);text-align:left;padding:8px;border-bottom:1px solid var(--border)}' +
            '.md3-table td{padding:6px 8px;border-bottom:1px solid rgba(255,255,255,.08);vertical-align:middle}' +
            '.md3-table input[type=text]{width:100%;min-width:120px;padding:4px 8px;background:rgba(0,0,0,.4);border:1px solid var(--border);border-radius:4px;color:var(--white);font-size:.78rem}' +
            '.md3-table input:focus{outline:none;border-color:var(--accent)}' +
            '.md3-tags{font-size:.78rem;margin-bottom:10px}.md3-tags code{color:var(--accent)}' +
            '.md3-actions{display:flex;flex-wrap:wrap;align-items:center;gap:8px;margin-top:12px}' +
            '.md3-btn{display:inline-flex;align-items:center;justify-content:center;padding:8px 16px;border-radius:6px;border:2px solid var(--accent);background:var(--accent);color:#000;font-weight:bold;cursor:pointer;font-size:.8rem;line-height:1.2}' +
            '.md3-btn:disabled{opacity:.4;cursor:not-allowed}' +
            '.md3-status{font-size:.75rem;margin-top:8px;min-height:1.2em}.md3-status.err{color:#f66}.md3-status.ok{color:var(--accent)}' +
            '.md3-empty{color:rgba(255,255,255,.45);font-size:.8rem;padding:12px 0}' +
            '.md3-tool-app .tool-credits{margin-top:10px;padding-top:6px;border-top:1px solid rgba(255,255,255,.1);text-align:center;font-size:.7rem;color:rgba(255,255,255,.5)}' +
            '</style>' +
            '<motion class="md3-tool-app">' +
            '<h1><span>Q3</span> ' +
            t('Model Inspector', 'Инспектор модели') +
            '</h1>' +
            '<p class="subtitle">' +
            t('Inspect and edit shader names inside MD3 surface lumps. No 3D preview.', 'Просмотр и правка имён шейдеров в MD3 (блок surface). Без 3D-превью.') +
            '</p>' +
            '<motion class="md3-main">' +
            '<motion class="md3-drop" id="md3DropZone">' +
            '<input type="file" id="md3FileInput" class="md3-file-input" accept=".md3"/>' +
            '<p class="md3-drop-hint">' +
            t('Drop file here or', 'Перетащите файл сюда или') +
            '</p>' +
            '<button type="button" class="md3-browse-btn" data-md3-browse="md3">' +
            t('Choose .md3…', 'Выбрать .md3…') +
            '</button></motion>' +
            '<motion id="md3NameBanner" class="md3-name-banner">' +
            '<b>' +
            t('Name (from MD3)', 'Имя (из MD3)') +
            '</b><span id="md3ModelNameValue" class="md3-name-value"></span></motion>' +
            '<motion id="md3InfoBlock" style="display:none">' +
            '<motion class="md3-meta" id="md3Meta"></motion>' +
            '<motion class="md3-tags" id="md3TagsLine"></motion>' +
            '<motion class="md3-table-wrap"><table class="md3-table"><thead><tr><th>#</th><th>' +
            t('Surface', 'Поверхность') +
            '</th><th>' +
            t('Shader', 'Шейдер') +
            '</th><th>' +
            t('Verts', 'Верш.') +
            '</th><th>' +
            t('Tris', 'Треуг.') +
            '</th></tr></thead><tbody id="md3SurfacesBody"></tbody></table></motion>' +
            '<motion class="md3-actions"><button type="button" class="md3-btn" id="md3DownloadBtn" disabled>' +
            t('Download MD3', 'Скачать MD3') +
            '</button></motion></motion>' +
            '<p class="md3-empty" id="md3EmptyHint">' +
            t('No MD3 loaded.', 'MD3 не загружен.') +
            '</p></motion>' +
            '<p class="md3-status" id="md3Status"></p>' +
            '<motion class="tool-credits">' +
            t('Created by', 'Сделано') +
            ' <strong>diwoc</strong></motion></motion>'
        ).replace(/<\/?motion\b/g, (s) => s.replace(/motion/g, 'div'));
    }

    let state = { model: null };

    function setStatus(root, msg, type) {
        const el = root.querySelector('#md3Status');
        if (!el) return;
        el.textContent = msg || '';
        el.className = 'md3-status' + (type ? ' ' + type : '');
    }

    function renderModel(root, model) {
        const info = root.querySelector('#md3InfoBlock');
        const empty = root.querySelector('#md3EmptyHint');
        const meta = root.querySelector('#md3Meta');
        const nameBanner = root.querySelector('#md3NameBanner');
        const nameValue = root.querySelector('#md3ModelNameValue');
        const tagsLine = root.querySelector('#md3TagsLine');
        const tbody = root.querySelector('#md3SurfacesBody');
        const dl = root.querySelector('#md3DownloadBtn');
        if (!info || !tbody) return;

        if (!model) {
            info.style.display = 'none';
            if (empty) empty.style.display = 'block';
            if (dl) dl.disabled = true;
            if (nameBanner) nameBanner.classList.remove('visible');
            tbody.innerHTML = '';
            return;
        }

        if (empty) empty.style.display = 'none';
        info.style.display = 'block';
        if (dl) dl.disabled = false;

        if (nameBanner && nameValue) {
            nameValue.textContent = model.modelName || '—';
            nameBanner.classList.add('visible');
        }

        const metaItems = [
            [t('File', 'Файл'), model.fileName || '—'],
            [t('Version', 'Версия'), String(model.version)],
            [t('Frames', 'Кадры'), String(model.numFrames)],
            [t('Surfaces', 'Поверхности'), String(model.numSurfaces)],
            [t('Tags', 'Теги'), String(model.numTags)],
            [t('Size', 'Размер'), (model.buffer.byteLength / 1024).toFixed(1) + ' KiB']
        ];
        meta.innerHTML = metaItems
            .map(function (pair) {
                return '<div class="md3-meta-item"><b>' + pair[0] + '</b>' + escapeHtml(pair[1]) + '</div>';
            })
            .join('');

        if (model.tags.length) {
            tagsLine.innerHTML =
                t('Tags:', 'Теги:') +
                ' ' +
                model.tags.map(function (tg) {
                    return '<code>' + escapeHtml(tg.name) + '</code> (' + tg.numFrames + ' ' + t('frames', 'кадров') + ')';
                }).join(', ');
        } else {
            tagsLine.textContent = '';
        }

        const anyMd3Shader = model.surfaces.some(function (s) {
            return s.shader;
        });
        tbody.innerHTML = model.surfaces
            .map(function (s, idx) {
                const val = shaderForDisplay(s);
                const title =
                    val || s.numShaders
                        ? ''
                        : t(
                              'Player meshes often omit shader lumps — editing may require a converter',
                              'У части моделей игрока нет shader lump — редактирование может быть недоступно'
                          );
                return (
                    '<tr data-surf="' +
                    idx +
                    '"><td>' +
                    s.index +
                    '</td><td>' +
                    escapeHtml(s.name) +
                    '</td><td><input type="text" data-shader="' +
                    idx +
                    '" value="' +
                    escapeAttr(val) +
                    '" maxlength="63" title="' +
                    escapeAttr(title) +
                    '"/></td><td>' +
                    s.numVerts +
                    '</td><td>' +
                    s.numTriangles +
                    '</td></tr>'
                );
            })
            .join('');

        if (!anyMd3Shader) {
            setStatus(
                root,
                t(
                    'Shader paths in MD3 surface lump are empty (common on player meshes). Export from your tools may add shader names.',
                    'Пути шейдеров в lump surface в MD3 пустые (часто у моделей игрока). Экспорт из редактора может добавить имена.'
                ),
                'ok'
            );
        }
    }

    function escapeHtml(s) {
        return String(s)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;');
    }

    function escapeAttr(s) {
        return escapeHtml(s).replace(/"/g, '&quot;');
    }

    function applyShadersFromUI(root) {
        if (!state.model) return false;
        const inputs = root.querySelectorAll('#md3SurfacesBody input[data-shader]');
        let ok = true;
        inputs.forEach(function (inp) {
            const idx = parseInt(inp.getAttribute('data-shader'), 10);
            const val = inp.value.trim().split(',')[0].trim();
            if (val.length > 63) {
                ok = false;
                inp.style.borderColor = '#f66';
                return;
            }
            inp.style.borderColor = '';
            const surf = state.model.surfaces[idx];
            if (!surf) return;
            if (!surf.shaderOffset || !surf.numShaders) {
                inp.style.borderColor = '#f66';
                ok = false;
                return;
            }
            writeCString(state.model.view, surf.shaderOffset, 64, val);
            surf.shader = val;
            surf.displayShader = val;
        });
        return ok;
    }

    function loadMd3File(root, file) {
        const reader = new FileReader();
        reader.onload = function () {
            try {
                const copy = reader.result.slice(0);
                const model = parseMD3(copy);
                model.fileName = file.name;
                state.model = model;
                renderModel(root, model);
                setStatus(root, t('Loaded: ', 'Загружен: ') + file.name, 'ok');
                if (window.ChangeTracker) window.ChangeTracker.markChanges('md3model');
            } catch (err) {
                state.model = null;
                renderModel(root, null);
                setStatus(root, err.message || String(err), 'err');
            }
        };
        reader.onerror = function () {
            setStatus(root, t('Failed to read file', 'Не удалось прочитать файл'), 'err');
        };
        reader.readAsArrayBuffer(file);
    }

    function downloadBlob(filename, content, mime) {
        const blob = content instanceof Blob ? content : new Blob([content], { type: mime || 'application/octet-stream' });
        const a = document.createElement('a');
        a.href = URL.createObjectURL(blob);
        a.download = filename;
        a.click();
        URL.revokeObjectURL(a.href);
    }

    function openFilePickerOnce(input) {
        if (!input) return;
        if (input._md3PickerBusy) return;
        input._md3PickerBusy = true;
        input.click();
        window.setTimeout(function () {
            input._md3PickerBusy = false;
        }, 450);
    }

    function setupFileDropZone(drop, input, onFile, isValidFile) {
        if (!drop || !input) return;

        drop.querySelectorAll('.md3-browse-btn').forEach(function (btn) {
            btn.addEventListener('click', function (e) {
                e.preventDefault();
                e.stopPropagation();
                openFilePickerOnce(input);
            });
        });

        drop.addEventListener('click', function (e) {
            if (e.target === input || e.target.closest('.md3-browse-btn')) return;
            e.preventDefault();
            openFilePickerOnce(input);
        });

        drop.addEventListener('dblclick', function (e) {
            e.preventDefault();
        });

        input.addEventListener('change', function () {
            const f = input.files && input.files[0];
            input.value = '';
            if (!f) return;
            if (isValidFile && !isValidFile(f)) return;
            onFile(f);
        });

        drop.addEventListener('dragover', function (e) {
            e.preventDefault();
            drop.classList.add('dragover');
        });
        drop.addEventListener('dragleave', function () {
            drop.classList.remove('dragover');
        });
        drop.addEventListener('drop', function (e) {
            e.preventDefault();
            drop.classList.remove('dragover');
            const f = e.dataTransfer.files && e.dataTransfer.files[0];
            if (!f) return;
            if (isValidFile && !isValidFile(f)) return;
            onFile(f);
        });
    }

    function bindMd3Tool(content) {
        const root = content;

        setupFileDropZone(
            root.querySelector('#md3DropZone'),
            root.querySelector('#md3FileInput'),
            function (f) {
                loadMd3File(root, f);
            },
            function (f) {
                if (/\.md3$/i.test(f.name)) return true;
                setStatus(root, t('Expected .md3 file', 'Нужен файл .md3'), 'err');
                return false;
            }
        );

        root.querySelector('#md3SurfacesBody') &&
            root.querySelector('#md3SurfacesBody').addEventListener('input', function () {
                if (window.ChangeTracker) window.ChangeTracker.markChanges('md3model');
            });

        const dlBtn = root.querySelector('#md3DownloadBtn');
        if (dlBtn) {
            dlBtn.addEventListener('click', function () {
                if (!state.model) return;
                if (!applyShadersFromUI(root)) {
                    setStatus(
                        root,
                        t(
                            'Cannot write shaders: missing shader lump in MD3, or shader path exceeds 63 characters.',
                            'Не удалось записать шейдеры: нет shader lump в MD3 или строка длиннее 63 символов.'
                        ),
                        'err'
                    );
                    return;
                }
                const name = state.model.fileName || 'model.md3';
                downloadBlob(name, state.model.buffer);
                setStatus(root, t('Download started', 'Загрузка начата'), 'ok');
                if (window.ChangeTracker) window.ChangeTracker.markCompleted('md3model');
            });
        }
    }

    document.addEventListener('DOMContentLoaded', function () {
        const openBtn = document.getElementById('openMd3ModelModal');
        const modal = document.getElementById('md3ModelModal');
        const closeBtn = document.getElementById('closeMd3ModelModal');
        const contentEl = document.getElementById('md3ModelToolContent');

        if (!openBtn || !modal || !closeBtn || !contentEl) return;

        let loaded = false;
        function loadTool() {
            if (loaded) return;
            contentEl.className = 'md3-tool-root';
            contentEl.innerHTML = getMd3ToolHTML();
            loaded = true;
            bindMd3Tool(contentEl);
        }

        function openModal() {
            loadTool();
            modal.classList.add('active');
            document.body.style.overflow = 'hidden';
            if (history.pushState) history.pushState(null, null, '#md3model');
            else window.location.hash = '#md3model';
        }

        function closeModal() {
            modal.classList.remove('active');
            document.body.style.overflow = '';
            if (history.pushState) {
                history.pushState(null, null, window.location.pathname + window.location.search);
            } else window.location.hash = '';
            if (window.ChangeTracker) window.ChangeTracker.resetChanges('md3model');
        }

        openBtn.addEventListener('click', function (e) {
            e.preventDefault();
            e.stopPropagation();
            openModal();
        });
        closeBtn.addEventListener('click', closeModal);

        modal.addEventListener('mousedown', function (e) {
            window._md3MouseDownBg = e.target === modal;
        });
        modal.addEventListener('mouseup', function (e) {
            if (window._md3MouseDownBg && (e.target === modal || !modal.contains(e.target))) closeModal();
            window._md3MouseDownBg = false;
        });
        document.addEventListener('keydown', function (e) {
            if (e.key === 'Escape' && modal.classList.contains('active')) closeModal();
        });

        if (window.location.hash === '#md3model') openModal();
        window.addEventListener('hashchange', function () {
            if (window.location.hash === '#md3model') openModal();
            else if (modal.classList.contains('active')) closeModal();
        });
    });
})();
