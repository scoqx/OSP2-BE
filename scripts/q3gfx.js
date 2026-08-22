// Q3GFX - Adapted for BE site style
// Original: quake-gfx.js

// Font flags (from OSP2-PBE)
var DS_PROPORTIONAL = 0x4;
var DS_HCENTER = 0x8;
var DS_SHADOW = 0x1;

// Font metric structure
function FontMetric() {
    this.tc_prop = [0, 0, 0, 0];  // texture coordinates proportional
    this.tc_mono = [0, 0, 0, 0];  // texture coordinates monospace
    this.space1 = 0;              // space before character
    this.space2 = 0;              // space after character
    this.width = 0;               // character width ratio
}

// Font structure
function Font() {
    this.name = "";
    this.metrics = [];            // array of 256 FontMetric
    this.images = [];             // array of Image objects
    this.imageThresholds = [];    // array of thresholds
    this.imageCount = 0;
}

// Initialize metrics array
function InitFontMetrics(font) {
    font.metrics = [];
    for (var i = 0; i < 256; i++) {
        font.metrics[i] = new FontMetric();
    }
}

// Font manager (from Q3GFX2)
var Q3GFX_FontManager = {
    fonts: [],
    currentFont: null,
    fontsPath: "/assets/q3gfx2",
    
    // Available font names (from OSP2-PBE, in the same order as in q3gfx2-modal.js)
    fontNames: [
        "bigchars",        // 0 - id (uses bigchars.cfg)
        "numbers",         // 1 - idblock (uses numbers.cfg)
        "sansman",         // 2
        "m1rage",          // 4 (cpma uses sansman, so skip)
        "elite_emoji",     // 5
        "diablo",          // 6
        "eternal",         // 7
        "qlnumbers",       // 8
        "elite",           // 9
        "EliteBigchars",   // 10 - elitebigchars
        "EliteEternal",    // 11 - eliteeternal
        "BigcharsEliteEternal",    // 12 - bigcharseliteeternal
        "EliteStoropia",   // 13 - elitestoropia
        "BigcharsEliteStoropia",   // 14 - bigcharselitestoropia
        "BigcharsEliteOrbitron",   // 15 - bigcharseliteorbitron
        "EliteOrbitron"    // 16 - eliteorbitron
    ],
    
    // Load font from .cfg file (same as Q3GFX2)
    loadFont: function(fontName, callback) {
        var self = this;
        var fontPath = this.fontsPath + "/" + fontName + ".cfg";
        
        fetch(fontPath)
            .then(function(response) {
                if (!response.ok) {
                    throw new Error("Failed to load font: " + fontName);
                }
                return response.text();
            })
            .then(function(cfgText) {
                var font = new Font();
                font.name = fontName;
                InitFontMetrics(font);
                
                self.parseFontConfig(font, cfgText, function() {
                    self.fonts.push(font);
                    if (callback) callback(font);
                });
            })
            .catch(function(error) {
                console.error("Error loading font:", error);
                if (callback) callback(null);
            });
    },
    
    // Parse font .cfg file (same as Q3GFX2)
    parseFontConfig: function(font, cfgText, callback) {
        var self = this;
        var lines = cfgText.split('\n');
        var shaderNames = [];
        var shaderThresholds = [];
        var shaderCount = 0;
        var fontWidth = 0, fontHeight = 0;
        var charWidth = 0, charHeight = 0;
        var r_width = 0, r_height = 0;
        var i = 0;
        var inMetrics = false;
        var imagesLoaded = 0;
        
        // Parse config
        for (i = 0; i < lines.length; i++) {
            var line = lines[i].trim();
            if (!line || line.startsWith('//')) continue;
            
            var tokens = line.split(/\s+/);
            if (tokens.length === 0) continue;
            
            // Parse img directive
            if (tokens[0] === 'img' && tokens.length >= 3) {
                var imgPath = tokens[1].replace(/"/g, '');
                var threshold = parseInt(tokens[2]) || 0;
                
                // Convert path to web path
                if (imgPath.startsWith('gfx/2d/')) {
                    imgPath = self.fontsPath + "/" + imgPath.substring(7);
                } else if (!imgPath.startsWith('/')) {
                    imgPath = self.fontsPath + "/" + imgPath;
                }
                
                shaderNames.push(imgPath);
                shaderThresholds.push(threshold);
                shaderCount++;
            }
            // Parse fnt directive
            else if (tokens[0] === 'fnt' && tokens.length >= 5) {
                fontWidth = parseFloat(tokens[1]);
                fontHeight = parseFloat(tokens[2]);
                charWidth = parseFloat(tokens[3]);
                charHeight = parseFloat(tokens[4]);
                r_width = 1.0 / fontWidth;
                r_height = 1.0 / fontHeight;
                inMetrics = true;
            }
            // Parse character metrics
            else if (inMetrics && tokens.length >= 7) {
                var charCode = 0;
                
                // Parse character code (can be number or 'X')
                if (tokens[0].startsWith("'") && tokens[0].length === 3) {
                    charCode = tokens[0].charCodeAt(1) & 255;
                } else {
                    charCode = parseInt(tokens[0]) & 255;
                }
                
                if (charCode < 0 || charCode > 255) continue;
                
                var x0 = parseFloat(tokens[1]);
                var y0 = parseFloat(tokens[2]);
                var w1 = parseFloat(tokens[3]);  // x-offset
                var w2 = parseFloat(tokens[4]);  // width
                var s1 = parseFloat(tokens[5]);  // space1
                var s2 = parseFloat(tokens[6]);  // space2
                
                var fm = font.metrics[charCode];
                
                // Monospace texture coordinates
                fm.tc_mono[0] = x0 * r_width;
                fm.tc_mono[1] = y0 * r_height;
                fm.tc_mono[2] = (x0 + charWidth) * r_width;
                fm.tc_mono[3] = (y0 + charHeight) * r_height;
                
                // Proportional texture coordinates
                fm.tc_prop[1] = fm.tc_mono[1];
                fm.tc_prop[3] = fm.tc_mono[3];
                fm.tc_prop[0] = fm.tc_mono[0] + (w1 * r_width);
                fm.tc_prop[2] = fm.tc_prop[0] + (w2 * r_width);
                
                // Metrics
                fm.width = w2 / charWidth;
                fm.space1 = s1 / charWidth;
                fm.space2 = (s2 + w2) / charWidth;
            }
        }
        
        // Sort images by threshold
        for (var j = 0; j < shaderCount - 1; j++) {
            for (var k = 0; k < shaderCount - 1 - j; k++) {
                if (shaderThresholds[k] > shaderThresholds[k + 1]) {
                    var tmp = shaderThresholds[k];
                    shaderThresholds[k] = shaderThresholds[k + 1];
                    shaderThresholds[k + 1] = tmp;
                    var tmpName = shaderNames[k];
                    shaderNames[k] = shaderNames[k + 1];
                    shaderNames[k + 1] = tmpName;
                }
            }
        }
        
        // Always assume zero threshold for lowest-quality shader
        shaderThresholds[0] = 0;
        
        font.imageCount = shaderCount;
        font.imageThresholds = shaderThresholds;
        font.images = []; // Initialize images array
        
        // Load images
        var loadImage = function(index) {
            if (index >= shaderCount) {
                if (callback) callback();
                return;
            }
            
            var imgPath = shaderNames[index];
            var isTGA = imgPath.toLowerCase().endsWith('.tga');
            
            if (isTGA && typeof TGA !== 'undefined') {
                // Load TGA file using TGA loader
                var tga = new TGA();
                var req = new XMLHttpRequest();
                req.open('GET', imgPath, true);
                req.responseType = 'arraybuffer';
                
                req.onload = function() {
                    if (this.status === 200) {
                        try {
                            tga.load(new Uint8Array(req.response));
                            var canvas = tga.getCanvas();
                            var img = new Image();
                            img.onload = function() {
                                font.images[index] = img;
                                imagesLoaded++;
                                if (imagesLoaded === shaderCount) {
                                    if (callback) callback();
                                }
                            };
                            img.onerror = function() {
                                font.images[index] = null;
                                imagesLoaded++;
                                if (imagesLoaded === shaderCount) {
                                    if (callback) callback();
                                }
                            };
                            img.src = canvas.toDataURL('image/png');
                        } catch (e) {
                            font.images[index] = null;
                            imagesLoaded++;
                            if (imagesLoaded === shaderCount) {
                                if (callback) callback();
                            }
                        }
                    } else {
                        font.images[index] = null;
                        imagesLoaded++;
                        if (imagesLoaded === shaderCount) {
                            if (callback) callback();
                        }
                    }
                };
                
                req.onerror = function() {
                    font.images[index] = null;
                    imagesLoaded++;
                    if (imagesLoaded === shaderCount) {
                        if (callback) callback();
                    }
                };
                
                req.send(null);
            } else {
                // Load regular image (PNG, JPG, etc.)
                var img = new Image();
                img.onload = function() {
                    font.images[index] = img;
                    imagesLoaded++;
                    if (imagesLoaded === shaderCount) {
                        if (callback) callback();
                    }
                };
                img.onerror = function() {
                    font.images[index] = null;
                    imagesLoaded++;
                    if (imagesLoaded === shaderCount) {
                        if (callback) callback();
                    }
                };
                img.crossOrigin = "anonymous";
                img.src = imgPath;
            }
        };
        
        // Load all images
        for (var idx = 0; idx < shaderCount; idx++) {
            loadImage(idx);
        }
    },
    
    // Select font by name
    selectFont: function(fontName) {
        for (var i = 0; i < this.fonts.length; i++) {
            if (this.fonts[i].name === fontName) {
                this.currentFont = this.fonts[i];
                return true;
            }
        }
        return false;
    },
    
    // Get font image based on character height
    getFontImage: function(font, charHeight) {
        if (!font || font.imageCount === 0) return null;
        
        var img = font.images[0]; // default to lowest quality
        
        // Find the best quality image based on charHeight
        for (var i = 1; i < font.imageCount; i++) {
            if (charHeight >= font.imageThresholds[i] && font.images[i]) {
                img = font.images[i];
            }
        }
        
        return img;
    }
};

// Load fonts
function LoadFonts(context, params) {
    var fontNames = Q3GFX_FontManager.fontNames;
    var loaded = 0;
    var total = fontNames.length;
    
    for (var i = 0; i < fontNames.length; i++) {
        Q3GFX_FontManager.loadFont(fontNames[i], function(font) {
            if (font) {
                context.fonts.push(font);
                if (context.fonts.length === 1) {
                    context.currentFont = font;
                    Q3GFX_FontManager.currentFont = font;
                }
            }
            loaded++;
            if (loaded === total) {
                UpdateFontSelect(context);
                UpdateScene(context);
            }
        });
    }
}

// Update font select dropdown
function UpdateFontSelect(context) {
    if (!context.ui.font) return;
    
    // Use fontNames order, not fonts array order (fonts load asynchronously)
    context.ui.font.innerHTML = "";
    var fontNames = Q3GFX_FontManager.fontNames;
    for (var i = 0; i < fontNames.length; i++) {
        // Find font by name in loaded fonts
        var font = null;
        for (var j = 0; j < context.fonts.length; j++) {
            if (context.fonts[j].name === fontNames[i]) {
                font = context.fonts[j];
                break;
            }
        }
        // Only add option if font is loaded
        if (font) {
            var option = document.createElement("option");
            option.value = font.name;
            option.textContent = font.name;
            if (context.currentFont && context.currentFont.name === font.name) {
                option.selected = true;
            }
            context.ui.font.appendChild(option);
        }
    }
}

function OnChangeFont(context) {
    var fontName = context.ui.font.value;
    for (var i = 0; i < context.fonts.length; i++) {
        if (context.fonts[i].name === fontName) {
            context.currentFont = context.fonts[i];
            Q3GFX_FontManager.currentFont = context.fonts[i];
            break;
        }
    }
    UpdateScene(context);
    // Отметить изменения
    if (window.ChangeTracker) {
        window.ChangeTracker.markChanges('q3gfx');
    }
}

function OnChangeProportional(context) {
    if (context.ui.proportional.checked) {
        context.flags |= DS_PROPORTIONAL;
    } else {
        context.flags &= ~DS_PROPORTIONAL;
    }
    UpdateScene(context);
    // Отметить изменения
    if (window.ChangeTracker) {
        window.ChangeTracker.markChanges('q3gfx');
    }
}

function Q3GFX_Initialize(params)
{
    var context = { 
        params: params,
        gfxname: [], 
        half:1,
        nickname: params.nickname,
        fonts: [],
        currentFont: null,
        flags: DS_PROPORTIONAL | DS_HCENTER | DS_SHADOW
    };
    
    InitializeUI(context, params);
    InitializeModes(context, params, context.ui);
    LoadSymbolsMap(context, params);
    ReparseNickname(context);
    LoadFonts(context, params);

    // Set blinking timer
    StartScheduler(context);
    
    return context;
}

// ====================
//   Scheduler 

function StartScheduler(context)
{
    context.interval =  50;
    context.counter  =  0;
    context.opaque = [
        {
            step:0,
            duration:20,
            opaque:1.0,
            increament:0.05
        },
        {
            step:0,
            duration:20,
            opaque:1.0,
            increament:0.035
        }
    ];
    // Use requestAnimationFrame for smooth 60 FPS animation
    context.animationFrameId = null;
    context.lastUpdateTime = performance.now();
    TimerDispatcher(context);
}

function TimerDispatcher(context)
{    
    var now = performance.now();
    var deltaTime = now - context.lastUpdateTime;
    
    // Update blinking and layers only every 50ms (for old rendering method)
    if (deltaTime >= 50) {
        ProceedBlinking(context, context.opaque[0]);
        ProceedBlinking(context, context.opaque[1]);
        ProceedLayers(context);
        context.lastUpdateTime = now;
    }
    
    // Always update scene for smooth font rendering animation
    UpdateScene(context);
    
    // Continue animation loop
    context.animationFrameId = requestAnimationFrame(function() { TimerDispatcher(context); });
}

function ProceedLayers(context)
{
    context.counter += context.interval;
    if (context.counter < 500)
        return;
    
    context.counter = 0;
    context.half = (context.half != 1 ? 1 : 0);
}

function ProceedBlinking(context, opaque)
{
    opaque.step++;

    if (opaque.step <= opaque.duration)
    {
        opaque.opaque -= opaque.increament;
    }
    else if (opaque.step < (opaque.duration * 2))
    {
        opaque.opaque += opaque.increament;
    }
    else
    {
        opaque.step = 0;
        opaque.opaque = 1.0;
    }
}

// ====================
//   Panel view

function InitializeUI(context, params)
{
    var ui = context.ui = {};
    var root = document.getElementById(params.containerId);
    root.innerHTML = "";

    LoadUIStyles();

    CreateContainer(ui, params, root);
    CreateTopPanel(ui, params);
    CreateModePanel(ui, params);
    CreateCanvas(ui, params);
    PrepareCanvas(context, ui);
    CreateBottomPanel(ui, params);

    ui.nickname.oninput = function() { OnChangeNickname(context); };
    ui.mode.onchange = function() { OnChangeMode(context); };
    ui.background.onchange = function() { OnChangeBackground(context); };
    if (ui.font) {
        ui.font.onchange = function() { OnChangeFont(context); };
    }
    if (ui.proportional) {
        ui.proportional.onchange = function() { OnChangeProportional(context); };
    }
}

function LoadUIStyles()
{
    InjectCSS("\
        .q3gfx-panel-button\
        {\
            padding: calc(var(--spacing-sm) * 0.5) var(--spacing-md);\
            margin: 0px var(--spacing-sm) 0px 0px;\
            font-family: Consolas, monospace;\
            font-size: 0.85rem;\
            font-weight: bold;\
            border: 2px solid var(--border-color);\
            border-radius: var(--border-radius);\
            background-color: rgba(255, 255, 255, 0.05);\
            color: var(--text-color);\
            cursor: pointer;\
            transition: var(--transition);\
        }\
        .q3gfx-panel-button:hover {\
            background-color: rgba(255, 255, 255, 0.1);\
            box-shadow: 0 0 15px rgba(255, 255, 255, 0.3);\
        }\
        .q3gfx-panel-button:active {\
            transform: scale(0.98);\
        }\
    ");

    InjectCSS("\
        .q3gfx-symbols-button\
        {\
            width: 24px !important;\
            height: 24px !important;\
            min-width: 24px !important;\
            min-height: 24px !important;\
            max-width: 24px !important;\
            max-height: 24px !important;\
            padding: 2px;\
            margin: 0px var(--spacing-sm) 0px 0px;\
            border: 2px solid var(--border-color);\
            border-radius: var(--border-radius);\
            background-color: rgba(255, 255, 255, 0.1) !important;\
            background-image: none !important;\
            cursor: pointer;\
            transition: var(--transition);\
            display: inline-block !important;\
            box-sizing: border-box;\
            object-fit: contain !important;\
            vertical-align: middle;\
            image-rendering: pixelated;\
            image-rendering: -moz-crisp-edges;\
            image-rendering: crisp-edges;\
            filter: brightness(0) invert(1) !important;\
        }\
        .q3gfx-symbols-button:hover {\
            background-color: rgba(255, 255, 255, 0.2) !important;\
            transform: scale(1.1);\
        }\
        .q3gfx-symbols-button:active {\
            transform: scale(0.95);\
        }\
    ");
    
    InjectCSS("\
        select option {\
            background-color: rgba(0, 0, 0, 0.9) !important;\
            color: var(--text-color) !important;\
        }\
    ");
}

function InjectCSS(css)
{
    const style = document.createElement('style');
    style.textContent = css;
    document.getElementsByTagName('head')[0].appendChild(style);
}

function CreateContainer(ui, params, root)
{
    var container = ui.container = document.createElement("div");
    container.style.width = "100%";
    container.style.margin = "0 auto";
    container.style.color = "var(--text-color)";
    container.style.backgroundColor = "rgba(255, 255, 255, 0.05)";
    container.style.border = "2px solid var(--border-color)";
    container.style.borderRadius = "var(--border-radius)";
    container.style.padding = "var(--spacing-sm)";
    root.appendChild(container);
}

function CreateTopPanel(ui, params)
{
    var panel = ui.panel = document.createElement("div");
    panel.style.display = "flex";
    panel.style.flexWrap = "wrap";
    panel.style.gap = "var(--spacing-sm)";
    panel.style.marginBottom = "var(--spacing-sm)";
    panel.style.alignItems = "center";

    var nickname = ui.nickname = document.createElement("input");
    nickname.type = "text";
    nickname.value = params.nickname;
    nickname.selectionStart = 0;
    nickname.selectionEnd = 0;
    nickname.style.flex = "1";
    nickname.style.minWidth = "200px";
    nickname.style.height = "32px";
    nickname.style.fontFamily = "Consolas, monospace";
    nickname.style.fontSize = "0.9rem";
    nickname.style.fontWeight = "bold";
    nickname.style.backgroundColor = "rgba(255, 255, 255, 0.1)";
    nickname.style.border = "2px solid var(--border-color)";
    nickname.style.borderRadius = "var(--border-radius)";
    nickname.style.color = "var(--text-color)";
    nickname.style.padding = "0 var(--spacing-sm)";
    window.onload = function() { nickname.value = params.nickname; };
    panel.appendChild(nickname);

    var mode = ui.mode = MakeSelect();
    panel.appendChild(mode);

    var background = ui.background = MakeSelect();
    panel.appendChild(background);
    
    // Add font selector
    var font = ui.font = MakeSelect();
    font.style.minWidth = "120px";
    panel.appendChild(font);
    
    // Add proportional checkbox
    var proportionalLabel = document.createElement("label");
    proportionalLabel.style.display = "flex";
    proportionalLabel.style.alignItems = "center";
    proportionalLabel.style.cursor = "pointer";
    proportionalLabel.style.color = "var(--text-color)";
    var proportional = ui.proportional = document.createElement("input");
    proportional.type = "checkbox";
    proportional.checked = true;
    proportional.style.marginRight = "5px";
    proportionalLabel.appendChild(proportional);
    proportionalLabel.appendChild(document.createTextNode("Proportional"));
    panel.appendChild(proportionalLabel);

    ui.container.appendChild(panel);
}

function CreateModePanel(ui, params)
{
    var panel = ui.panel2 = document.createElement("div");
    panel.style.display = "block";
    panel.style.flexWrap = "wrap";
    panel.style.flexDirection = "row";
    panel.style.gap = "var(--spacing-sm)";
    panel.style.marginBottom = "var(--spacing-sm)";
    ui.container.appendChild(panel);
}

function CreateCanvas(ui, params)
{
    var canvas = ui.canvas = document.createElement("canvas");
    
    // Function to calculate and set canvas size
    var updateCanvasSize = function() {
        var containerWidth = ui.container.offsetWidth || params.width || 1000;
        var aspectRatio = params.height / params.width;
        var padding = parseInt(getComputedStyle(ui.container).paddingLeft) || 16;
        var canvasWidth = Math.max(400, containerWidth - padding * 2 - 4); // Account for padding and border, min 400px
        var canvasHeight = Math.max(200, canvasWidth * aspectRatio); // Min 200px height (reduced for better fit)
        
        canvas.width  = canvasWidth;
        canvas.height = canvasHeight;
        
        // Update params with actual canvas size
        params.width = canvasWidth;
        params.height = canvasHeight;
    };
    
    // Initial size calculation (delay to ensure container is rendered)
    setTimeout(function() {
        updateCanvasSize();
        if (ui.canvas && ui.canvas.parentNode) {
            // Trigger scene update after resize
            var context = ui.canvas._context;
            if (context) {
                UpdateScene(context);
            }
        }
    }, 0);
    
    // Set initial size
    canvas.width  = params.width || 1000;
    canvas.height = params.height || 500;
    canvas.style.width = "100%";
    canvas.style.height = "auto";
    canvas.style.display = "block";
    canvas.style.border = "2px solid var(--border-color)";
    canvas.style.borderRadius = "var(--border-radius)";
    canvas.style.backgroundColor = "rgba(0, 0, 0, 0.3)";
    ui.container.appendChild(canvas);
    
    // Store update function for later use
    canvas._updateSize = updateCanvasSize;
    
    // Update on window resize
    window.addEventListener('resize', function() {
        if (canvas._updateSize) {
            canvas._updateSize();
            var context = canvas._context;
            if (context) {
                UpdateScene(context);
            }
        }
    });
}

function PrepareCanvas(context, ui)
{
    var ctx2d = context.ctx2d = ui.canvas.getContext("2d");
    // Store context reference in canvas for resize handler
    ui.canvas._context = context;
    ctx2d.fillStyle = "rgb(100, 100, 100)";
    ctx2d.fillRect(0, 0, context.params.width, context.params.height);
}

function CreateBottomPanel(ui, params)
{
    var panel = ui.panel3 = document.createElement("div");
    panel.style.marginTop = "var(--spacing-sm)";

    var output = ui.output = document.createElement("input");
    output.type = "text";
    output.readOnly = true;
    output.style.width = "100%";
    output.style.height = "32px";
    output.style.fontFamily = "Consolas, monospace";
    output.style.fontSize = "0.9rem";
    output.style.fontWeight = "bold";
    output.style.color = "var(--text-color)";
    output.style.backgroundColor = "rgba(255, 255, 255, 0.05)";
    output.style.border = "2px solid var(--border-color)";
    output.style.borderRadius = "var(--border-radius)";
    output.style.padding = "0 var(--spacing-sm)";
    output.style.cursor = "pointer";
    output.title = "Click to copy";
    
    // Обработчик клика для копирования
    output.addEventListener("click", function() {
        this.select();
        this.setSelectionRange(0, 99999); // Для мобильных устройств
        try {
            document.execCommand("copy");
            // Отметить, что работа завершена после копирования
            if (window.ChangeTracker) {
                window.ChangeTracker.markCompleted("q3gfx");
            }
        } catch (err) {
            // Игнорировать ошибки копирования
        }
    });
    
    // Обработчик копирования через Ctrl+C
    output.addEventListener("copy", function() {
        // Отметить, что работа завершена после копирования
        if (window.ChangeTracker) {
            window.ChangeTracker.markCompleted("q3gfx");
        }
    });
    
    panel.appendChild(output);

    ui.container.appendChild(panel);
}

function MakeSelect()
{
    var select = document.createElement("select");
    select.style.minWidth = "150px";
    select.style.height = "32px";
    select.style.fontFamily = "Consolas, monospace";
    select.style.fontSize = "0.85rem";
    select.style.fontWeight = "bold";
    select.style.backgroundColor = "rgba(0, 0, 0, 0.8)";
    select.style.border = "2px solid var(--border-color)";
    select.style.borderRadius = "var(--border-radius)";
    select.style.color = "var(--text-color)";
    select.style.padding = "0 var(--spacing-sm)";
    select.style.cursor = "pointer";
    return select;
}

function MakeOption(text)
{
    var option = document.createElement("option");
    option.innerText = text;
    return option;
}

function MakeButton(text, handler)
{
    var button = document.createElement("button");
    button.type = "button";
    button.textContent = text;
    button.className = "q3gfx-panel-button";
    button.onclick = handler;
    return button;
}

function MakeColoredButton(text, font, back, handler)
{
    var color = document.createElement("button");
    color.type = "button";
    color.className = "q3gfx-panel-button";
    color.onclick = handler;
    color.style.backgroundColor = font;
    color.style.color = back;
    color.style.width = "32px";
    color.style.height = "32px";
    color.style.minWidth = "32px";
    color.style.padding = "0";
    color.style.display = "flex";
    color.style.alignItems = "center";
    color.style.justifyContent = "center";
    color.title = text; // Show text on hover
    return color;
}

function MakeRGBButton(context, text, handler)
{
    var button = MakeButton(text, handler);
    var rgb = document.createElement("input");
    rgb.type = "color";
    rgb.value = "#000000";
    rgb.style.display = "none";
    rgb.oninput = function() { handler(rgb.value.substring(1)); };

    button.appendChild(rgb);
    button.onclick = function() 
    { 
        rgb.click();
        rgb.dispatchEvent(new Event('input'));
    };

    return button;
}

function MakeSymbolsButton(context, chr)
{
    var image = new Image();
    image.className = "q3gfx-symbols-button";
    var imagePath = MakeResourcePath(context, "chr_" + chr + ".png");
    
    image.onclick = MakeSymbolHandler(context, chr);
    image.alt = "Symbol " + chr;
    image.title = "Symbol " + chr;
    
    // Set initial styles before loading
    image.style.width = "24px";
    image.style.height = "24px";
    image.style.objectFit = "contain";
    image.style.display = "inline-block";
    image.style.verticalAlign = "middle";
    image.style.backgroundColor = "rgba(255, 255, 255, 0)";
    image.style.imageRendering = "pixelated";
    image.style.imageRendering = "-moz-crisp-edges";
    image.style.imageRendering = "crisp-edges";
    image.style.filter = "brightness(0) invert(1)";
    
    image.onerror = function() {
        image.style.backgroundColor = "rgba(255, 0, 0, 0.3)";
        image.style.border = "2px solid #ff0000";
    };
    
    image.onload = function() {
        image.style.width = "24px";
        image.style.height = "24px";
        image.style.objectFit = "contain";
        image.style.display = "inline-block";
        image.style.opacity = "1";
        image.style.visibility = "visible";
    };
    
    // Load image after setting up handlers
    image.src = imagePath;
    
    return image;
}

function MakeTagHandler(context, tag) 
{ 
    return function(){ AddTag(context, tag); };  
}

function MakeSymbolHandler(context, symbol) 
{ 
    return function(){ 
        InjectTagToNickname(context, String.fromCodePoint(symbol));
        // Отметить изменения
        if (window.ChangeTracker) {
            window.ChangeTracker.markChanges('q3gfx');
        }
    };  
}

function MakePanel2Div()
{
    var div = document.createElement("div");
    div.style.display = "none";
    div.style.flexWrap = "wrap";
    div.style.gap = "var(--spacing-sm)";
    div.style.marginBottom = "var(--spacing-md)";
    div.style.flexDirection = "row";
    return div;
}

function MarkNickname(context, valid)
{
    var textbox = context.ui.nickname;
    textbox.style.borderColor = (valid ? "var(--border-color)" : "#ff0000");
}

// ====================
//   Controls

function OnChangeNickname(context)
{
    ReparseNickname(context);
    if (!context.current.validate(context.nickname))
        MarkNickname(context, false);
    UpdateScene(context);
    // Отметить изменения
    if (window.ChangeTracker) {
        window.ChangeTracker.markChanges('q3gfx');
    }
}

function OnChangeMode(context)
{
    SwitchToSelectedMode(context);
    LoadCurrentMode(context);
    // Отметить изменения
    if (window.ChangeTracker) {
        window.ChangeTracker.markChanges('q3gfx');
    }
}

function OnChangeBackground(context)
{
    SwitchToSelectedBackground(context);
    UpdateScene(context);
    // Отметить изменения
    if (window.ChangeTracker) {
        window.ChangeTracker.markChanges('q3gfx');
    }
}

function SwitchToSelectedMode(context)
{
    var selected = context.ui.mode.value;
    var modes = context.modes;
    
    for (var i = 0; i < modes.length; i++)
    {
        if (modes[i].name == selected)
        {
            if (modes[i] == context.current)
                return;

            context.current.ui.div.style.display = 'none';
            context.current = modes[i];
            break;
        }
    }
}

function SwitchToSelectedBackground(context)
{
    var selected = context.ui.background.value;
    var background = context.current.background;

    for (var i = 0; i < background.length; i++)
    {
        if (background[i].name == selected)
        {
            if (background[i].hasOwnProperty("image"))
            {
                context.background = background[i].image;
                return;
            }
        }
    }
}

function EnsureBackgroundImageLoaded(context)
{
    if (context.hasOwnProperty("background"))
        return;
    
    SwitchToSelectedBackground(context);
}

function IsValidVQ3Name(nickname)
{
    var valid = false;

    for (var i = 0; i < nickname.length; i++)
    {
        var code = nickname[i].charCodeAt(0);
        
        if (code == 0 || code == 10 || code == 13)
            return false;

        if (!valid && code >= 32)
            valid = true;

        if (IsBadQ3Char(code))
            return false;
    }

    return valid;
}

function IsValidCPMAName(nickname)
{
    for (var i = 0; i < nickname.length; i++)
    {
        var code = nickname[i].charCodeAt(0);
        if (code < 32 || code >= 127)
            return false;

        if (IsBadQ3Char(code))
            return false;
    }

    return IsValidVQ3Name(nickname);
}

function IsBadQ3Char(code)
{
    if (code == "%".charCodeAt(0) || code == "\\".charCodeAt(0) || code == ";".charCodeAt(0))
        return true;
    
    return false;
}

function LoadNickname(context)
{
    var nickname = context.ui.nickname.value;
    var shrinked = context.current.shrink(nickname);
    shrinked = TransformTopCodeChars(shrinked);
    MarkNickname(context, (shrinked == nickname));
    context.ui.output.value = "\\name \"" + shrinked + "\"";
    context.nickname = shrinked;
}

function TransformTopCodeChars(nickname)
{
    var transformed = "";
    for (var i = 0; i < nickname.length; i++)
    {
        var code = nickname[i].charCodeAt(0);
        transformed += (code > 127 ? "." : nickname[i]);

    }
    return transformed;
}

// ====================
//   Modes

function InitializeModes(context, params, ui)
{
    var index = 0;
    context.modes = [];

    for (var i = 0, a = 0; i < params.modes.length; i++)
    {
        var settings = params.modes[i];
        var mode = { ui:{div:MakePanel2Div()}, background:[], index:a };

        if (settings.mode == "vq3")
        {
            mode.name = "VQ3 (default)";
            mode.parser = ParseGFX_VQ3Style;
            mode.validate = IsValidVQ3Name;
            mode.shrink = ShrinkVQ3Name;
            CreateVQ3Panel(context, mode);
        }
        else if (settings.mode == "osp")
        {
            mode.name = "OSP Mode";
            mode.parser = ParseGFX_OSPStyle;
            mode.validate = IsValidVQ3Name;
            mode.shrink = ShrinkVQ3Name;
            CreateOSPPanel(context, mode);
        }
        else if (settings.mode == "cpma")
        {
            mode.name = "CPMA Mode";
            mode.parser = ParseGFX_CPMAStyle;
            mode.validate = IsValidCPMAName;
            mode.shrink = ShrinkCPMAName;
            CreateCPMAPanel(context, mode);
        }
        else
        {
            continue;
        }
        
        context.ui.panel2.appendChild(mode.ui.div);

        for (var b = 0; b < settings.maps.length; b++)
        {
            mode.background[b] = { 
                path: settings.maps[b].image, 
                name: settings.maps[b].name 
            };

            function OnLoadImageForMode(result)
            {
                if (result.error != 'success')
                    return;
                
                result.context.image = result.image;
                EnsureBackgroundImageLoaded(context);
            }

            AsyncLoadImage(mode.background[b], MakeResourcePath(context, settings.maps[b].image), OnLoadImageForMode);
        }
        
        context.ui.mode.appendChild(MakeOption(mode.name));

        if (settings.hasOwnProperty("default") && settings.default)
        {
            index = a;
            context.current = mode;
        }

        context.modes[a++] = mode;
    }

    if (!context.hasOwnProperty("current"))
    {
        if (!context.modes.length)
            alert("Error, modules aren't configurated");
        context.current = context.modes[0];
    }

    context.ui.mode.selectedIndex = index;
    
    CreateSymbolsPanel(context, ui);
    
    // Show panel2 container
    if (context.ui.panel2) {
        context.ui.panel2.style.display = 'block';
    }
    
    LoadCurrentMode(context);
    
    // Ensure panel2 is visible after loading
    if (context.ui.panel2) {
        context.ui.panel2.style.display = 'block';
    }
}

function LoadCurrentMode(context)
{
    var mode = context.current;
    var background = mode.background;

    // Ensure panel2 container is visible
    if (context.ui.panel2) {
        context.ui.panel2.style.display = 'block';
    }

    // Display current mode bar
    if (mode.ui && mode.ui.div) {
        mode.ui.div.style.display = 'flex';
    }
    ShowSymbolsPanel(context, false);

    // Load new background list
    context.ui.background.innerHTML = "";
    for (var i = 0; i < background.length; i++)
        context.ui.background.appendChild(MakeOption(background[i].name));

    SwitchToSelectedBackground(context);
    ReparseNickname(context);
    if (!mode.validate(context.nickname))
        MarkNickname(context, false);
}

function CreateSymbolsPanel(context, ui)
{
    var panel = ui.symbols = MakePanel2Div();

    var button = MakeButton("Back", function() { ShowSymbolsPanel(context, false); });
    panel.appendChild(button);

    panel.appendChild(MakeSymbolsButton(context, 1));
    panel.appendChild(MakeSymbolsButton(context, 2));
    panel.appendChild(MakeSymbolsButton(context, 3));
    panel.appendChild(MakeSymbolsButton(context, 4));
    panel.appendChild(MakeSymbolsButton(context, 7));
    panel.appendChild(MakeSymbolsButton(context, 8));
    panel.appendChild(MakeSymbolsButton(context, 9));
    panel.appendChild(MakeSymbolsButton(context, 11));
    panel.appendChild(MakeSymbolsButton(context, 14));
    panel.appendChild(MakeSymbolsButton(context, 16));
    panel.appendChild(MakeSymbolsButton(context, 17));
    panel.appendChild(MakeSymbolsButton(context, 18));
    panel.appendChild(MakeSymbolsButton(context, 19));
    panel.appendChild(MakeSymbolsButton(context, 20));
    panel.appendChild(MakeSymbolsButton(context, 21));
    panel.appendChild(MakeSymbolsButton(context, 23));
    panel.appendChild(MakeSymbolsButton(context, 24));
    panel.appendChild(MakeSymbolsButton(context, 25));
    panel.appendChild(MakeSymbolsButton(context, 26));
    panel.appendChild(MakeSymbolsButton(context, 27));
    panel.appendChild(MakeSymbolsButton(context, 29));
    panel.appendChild(MakeSymbolsButton(context, 30));
    panel.appendChild(MakeSymbolsButton(context, 31));
    panel.appendChild(MakeSymbolsButton(context, 127));
    
    panel.style.display = 'none';

    ui.panel2.appendChild(panel);
}

function CreateVQ3Panel(context, mode)
{
    var panel = mode.ui.div;
    
    var blink = MakeButton("Symbols", function() { ShowSymbolsPanel(context, true); });
    panel.appendChild(blink);

    panel.appendChild(MakeColoredButton("^0", "#000000", "white", MakeTagHandler(context, "^0")));
    panel.appendChild(MakeColoredButton("^1", "#ff0000", "white", MakeTagHandler(context, "^1")));
    panel.appendChild(MakeColoredButton("^2", "#00ff00", "black", MakeTagHandler(context, "^2")));
    panel.appendChild(MakeColoredButton("^3", "#ffff00", "black", MakeTagHandler(context, "^3")));
    panel.appendChild(MakeColoredButton("^4", "#0000ff", "white", MakeTagHandler(context, "^4")));
    panel.appendChild(MakeColoredButton("^5", "#00ffff", "black", MakeTagHandler(context, "^5")));
    panel.appendChild(MakeColoredButton("^6", "#ff00ff", "white", MakeTagHandler(context, "^6")));
    panel.appendChild(MakeColoredButton("^7", "#ffffff", "black", MakeTagHandler(context, "^7")));
}

function CreateOSPPanel(context, mode)
{
    var panel = mode.ui.div;

    var blink = MakeButton("Symbols", function() { ShowSymbolsPanel(context, true); });
    panel.appendChild(blink);

    panel.appendChild(MakeColoredButton("^0", "#000000", "white", MakeTagHandler(context, "^0")));
    panel.appendChild(MakeColoredButton("^1", "#ff0000", "white", MakeTagHandler(context, "^1")));
    panel.appendChild(MakeColoredButton("^2", "#00ff00", "black", MakeTagHandler(context, "^2")));
    panel.appendChild(MakeColoredButton("^3", "#ffff00", "black", MakeTagHandler(context, "^3")));
    panel.appendChild(MakeColoredButton("^4", "#0000ff", "white", MakeTagHandler(context, "^4")));
    panel.appendChild(MakeColoredButton("^5", "#00ffff", "black", MakeTagHandler(context, "^5")));
    panel.appendChild(MakeColoredButton("^6", "#ff00ff", "white", MakeTagHandler(context, "^6")));
    panel.appendChild(MakeColoredButton("^7", "#ffffff", "black", MakeTagHandler(context, "^7")));
    panel.appendChild(MakeColoredButton("^8", "#ff8800", "black", MakeTagHandler(context, "^8")));
    panel.appendChild(MakeColoredButton("^9", "#888888", "white", MakeTagHandler(context, "^9")));

    var blink = MakeButton("Fast Fading", MakeTagHandler(context, "^b"));
    panel.appendChild(blink);

    blink = MakeButton("Slow Fading", MakeTagHandler(context, "^B"));
    panel.appendChild(blink);

    var half1 = MakeButton("Layer #1", MakeTagHandler(context, "^f"));
    panel.appendChild(half1);

    var half2 = MakeButton("Layer #2", MakeTagHandler(context, "^F"));
    panel.appendChild(half2);

    var rgb = MakeRGBButton(context, "RGB Front", function(rgb) { ApplyRGBFront(context, rgb); });
    panel.appendChild(rgb);

    var rgb2 = MakeRGBButton(context, "RGB Back", function(rgb) { ApplyRGBBackground(context, rgb); });
    panel.appendChild(rgb2);

    var stop = MakeButton("Stop Effect", MakeTagHandler(context, "^N"));
    panel.appendChild(stop);
}

function CreateCPMAPanel(context, mode)
{
    var panel = mode.ui.div;
    panel.appendChild(MakeColoredButton("^0", "#000", "white", MakeTagHandler(context, "^0")));
    panel.appendChild(MakeColoredButton("^1", "#f00", "white", MakeTagHandler(context, "^1")));
    panel.appendChild(MakeColoredButton("^2", "#0f0", "black", MakeTagHandler(context, "^2")));
    panel.appendChild(MakeColoredButton("^3", "yellow", "black", MakeTagHandler(context, "^3")));
    panel.appendChild(MakeColoredButton("^4", "#00f", "white", MakeTagHandler(context, "^4")));
    panel.appendChild(MakeColoredButton("^5", "aqua", "black", MakeTagHandler(context, "^5")));
    panel.appendChild(MakeColoredButton("^6", "magenta", "white", MakeTagHandler(context, "^6")));
    panel.appendChild(MakeColoredButton("^7", "#bbb", "white", MakeTagHandler(context, "^7")));
    panel.appendChild(MakeColoredButton("^8", "#888", "black", MakeTagHandler(context, "^8")));
    panel.appendChild(MakeColoredButton("^9", "#77c", "white", MakeTagHandler(context, "^9")));
    panel.appendChild(MakeColoredButton("a", "#f00", "white", MakeTagHandler(context, "^a")));
    panel.appendChild(MakeColoredButton("b", "#f40", "white", MakeTagHandler(context, "^b")));
    panel.appendChild(MakeColoredButton("c", "#f80", "black", MakeTagHandler(context, "^c")));
    panel.appendChild(MakeColoredButton("d", "#fc0", "black", MakeTagHandler(context, "^d")));
    panel.appendChild(MakeColoredButton("e", "#ff0", "black", MakeTagHandler(context, "^e")));
    panel.appendChild(MakeColoredButton("f", "#cf0", "black", MakeTagHandler(context, "^f")));
    panel.appendChild(MakeColoredButton("g", "#8f0", "black", MakeTagHandler(context, "^g")));
    panel.appendChild(MakeColoredButton("h", "#4f0", "black", MakeTagHandler(context, "^h")));
    panel.appendChild(MakeColoredButton("i", "#0f0", "black", MakeTagHandler(context, "^i")));
    panel.appendChild(MakeColoredButton("j", "#0f4", "black", MakeTagHandler(context, "^j")));
    panel.appendChild(MakeColoredButton("k", "#0f8", "black", MakeTagHandler(context, "^k")));
    panel.appendChild(MakeColoredButton("l", "#0fc", "black", MakeTagHandler(context, "^l")));
    panel.appendChild(MakeColoredButton("m", "#0ff", "black", MakeTagHandler(context, "^m")));
    panel.appendChild(MakeColoredButton("n", "#0cf", "black", MakeTagHandler(context, "^n")));
    panel.appendChild(MakeColoredButton("o", "#08f", "white", MakeTagHandler(context, "^o")));
    panel.appendChild(MakeColoredButton("p", "#04f", "white", MakeTagHandler(context, "^p")));
    panel.appendChild(MakeColoredButton("q", "#00f", "white", MakeTagHandler(context, "^q")));
    panel.appendChild(MakeColoredButton("r", "#40f", "white", MakeTagHandler(context, "^r")));
    panel.appendChild(MakeColoredButton("s", "#80f", "black", MakeTagHandler(context, "^s")));
    panel.appendChild(MakeColoredButton("t", "#c0f", "black", MakeTagHandler(context, "^t")));
    panel.appendChild(MakeColoredButton("u", "#f0f", "white", MakeTagHandler(context, "^u")));
    panel.appendChild(MakeColoredButton("v", "#f0c", "white", MakeTagHandler(context, "^v")));
    panel.appendChild(MakeColoredButton("w", "#f08", "white", MakeTagHandler(context, "^w")));
    panel.appendChild(MakeColoredButton("x", "#f04", "white", MakeTagHandler(context, "^x")));
    panel.appendChild(MakeColoredButton("y", "#666", "white", MakeTagHandler(context, "^y")));
    panel.appendChild(MakeColoredButton("z", "#aaa", "black", MakeTagHandler(context, "^z")));
}

// ====================
//   Panel controls

function AddTag(context, tag)
{
    if (tag.length >= 1 && tag[0] != '^')
        tag = '^' + tag;
    InjectTagToNickname(context, tag);
    // Отметить изменения
    if (window.ChangeTracker) {
        window.ChangeTracker.markChanges('q3gfx');
    }
}

function AddSelectedTag(context, tag)
{
    var nickname = context.ui.nickname;
    var pos = nickname.selectionStart;
    AddTag(context, tag);
    nickname.selectionEnd = nickname.selectionStart;
    nickname.selectionStart = pos;
    nickname.focus();
}

function ApplyRGBFront(context, rgb)
{
    var nickname = context.ui.nickname;
    var value = nickname.value;
    var pos = nickname.selectionStart;
    var end = nickname.selectionEnd;

    if (end == pos)
    {
        AddSelectedTag(context, "^x" + rgb + "^n");
        // Отметить изменения
        if (window.ChangeTracker) {
            window.ChangeTracker.markChanges('q3gfx');
        }
        return;
    }
    
    nickname.value = value.slice(0, pos) + value.slice(end);
    nickname.selectionStart = pos;
    nickname.selectionEnd = pos;

    AddSelectedTag(context, "^x" + rgb + "^n");
    // Отметить изменения
    if (window.ChangeTracker) {
        window.ChangeTracker.markChanges('q3gfx');
    }
}

function ApplyRGBBackground(context, rgb)
{
    var nickname = context.ui.nickname;
    var value = nickname.value;
    var pos = nickname.selectionStart;
    var end = nickname.selectionEnd;

    if (end == pos)
    {
        AddSelectedTag(context, "^x" + rgb);
        // Отметить изменения
        if (window.ChangeTracker) {
            window.ChangeTracker.markChanges('q3gfx');
        }
        return;
    }
    
    nickname.value = value.slice(0, pos) + value.slice(end);
    nickname.selectionStart = pos;
    nickname.selectionEnd = pos;

    AddSelectedTag(context, "^x" + rgb);
    // Отметить изменения
    if (window.ChangeTracker) {
        window.ChangeTracker.markChanges('q3gfx');
    }
}

function InjectTagToNickname(context, tag)
{
    var nickname = context.ui.nickname;
    var value = nickname.value;
    var pos = nickname.selectionStart;
    nickname.value = InjectStringToString(value, pos, tag);
    nickname.selectionStart = pos + tag.length;
    nickname.selectionEnd = nickname.selectionStart;
    nickname.focus();
    nickname.dispatchEvent(new Event('input'));
}

function InjectStringToString(src, pos, str)
{
    return src.slice(0, pos) + str + src.slice(pos);
}

function GetSelectedText(elem)
{
    return elem.value.substring(elem.selectionStart, elem.selectionEnd);
}

function ShowSymbolsPanel(context, show)
{
    var mode = context.current;
    var ui = context.ui;
    if (show)
    {
        if (mode.ui && mode.ui.div) {
            mode.ui.div.style.display = 'none';
        }
        if (ui.symbols) {
            ui.symbols.style.display = 'flex';
        }
    }
    else
    {
        if (mode.ui && mode.ui.div) {
            mode.ui.div.style.display = 'flex';
        }
        if (ui.symbols) {
            ui.symbols.style.display = 'none';
        }
    }
}

// ====================
//   GFX

function LoadSymbolsMap(context, params)
{
    context.map = []; // 0 - background, 1 - front
    for (var i = 0; i < 2; i++)
    {
        var entry = {};
        entry.canvas = document.createElement("canvas");
        entry.canvas.width = entry.canvas.height = 64 * 16;
        entry.ctx2d = entry.canvas.getContext("2d");
        entry.color = "";
        context.map[i] = entry;
    }

    AsyncLoadImage(context, MakeResourcePath(context, params.symbolsMap), function(result)
        {
            if (result.error != "success")
                return;
            
            for (var i = 0; i < 2; i++)
            {
                var entry = context.map[i];
                var ctx2d = entry.ctx2d;
                ctx2d.drawImage(result.image, 0, 0, 64 * 16, 64 * 16);
                ctx2d.fillStyle = entry.color;
                ctx2d.globalCompositeOperation = "source-in";
                ctx2d.fillRect(0, 0, 64 * 16, 64 * 16);
                ctx2d.globalCompositeOperation = "source-over";
            }

            context.symbols = result.image;
        }
    );
}

// Text compiler for font rendering (from Q3GFX2)
function Q3GFX_CompileText(text) {
    var commands = [];
    var i = 0;
    var command = false;
    var color = [255, 255, 255, 255];
    var blink = 0;
    var bold = false;
    var fade = false;
    var fast = false;
    
    for (i = 0; i < text.length; i++) {
        var chr = text.charAt(i);
        
        if (command) {
            if (chr === '^') {
                // Double ^ means literal ^
                commands.push({type: 'CHAR', value: '^', color: [color[0], color[1], color[2], color[3]], blink: blink, bold: bold, fade: fade, fast: fast});
                command = false;
                continue;
            }
            
            // Parse color code first (before effects, to handle ^0 correctly)
            if (chr >= '0' && chr <= '9') {
                var colorIndex = parseInt(chr);
                color = Q3GFX_GetColor(colorIndex);
                command = false;
                continue;
            }
            // Parse effects
            else if (chr === 'b') {
                blink = 1; // Fast blinking
                fast = true;
            } else if (chr === 'B') {
                blink = 2; // Slow blinking
                bold = true;
            } else if (chr === 'f') {
                fade = true;
            } else if (chr === 'F') {
                fast = true;
            } else if (chr === 'n' || chr === 'N') {
                // Reset effects
                blink = 0;
                bold = false;
                fade = false;
                fast = false;
                color = [255, 255, 255, 255];
            } else if (chr >= 'a' && chr <= 'z') {
                color = Q3GFX_GetColorAlpha(chr);
            } else if (chr === 'x' || chr === 'X') {
                // RGB color (^xRRGGBB)
                if (i + 6 < text.length) {
                    var rgb = text.substring(i + 1, i + 7);
                    if (/^[0-9A-Fa-f]{6}$/.test(rgb)) {
                        color = [
                            parseInt(rgb.substring(0, 2), 16),
                            parseInt(rgb.substring(2, 4), 16),
                            parseInt(rgb.substring(4, 6), 16),
                            255
                        ];
                        i += 6;
                    }
                }
            }
            
            command = false;
            continue;
        }
        
        if (chr === '^') {
            command = true;
            continue;
        }
        
        commands.push({
            type: 'CHAR',
            value: chr,
            color: [color[0], color[1], color[2], color[3]],
            blink: blink,
            bold: bold,
            fade: fade,
            fast: fast
        });
    }
    
    return commands;
}

// Get color by index (VQ3/OSP style)
function Q3GFX_GetColor(index) {
    var colors = [
        [0, 0, 0, 255],       // 0 - black
        [255, 0, 0, 255],     // 1 - red
        [0, 255, 0, 255],     // 2 - green
        [255, 255, 0, 255],   // 3 - yellow
        [0, 0, 255, 255],     // 4 - blue
        [0, 255, 255, 255],   // 5 - cyan
        [255, 0, 255, 255],   // 6 - magenta
        [255, 255, 255, 255], // 7 - white
        [255, 128, 0, 255],   // 8 - orange (OSP)
        [128, 128, 128, 255]  // 9 - gray (OSP)
    ];
    
    if (index >= 0 && index < colors.length) {
        return colors[index];
    }
    return [255, 255, 255, 255];
}

// Get color by alpha code
function Q3GFX_GetColorAlpha(code) {
    return [255, 255, 255, 255];
}

// Draw string using OSP2-PBE font system (from Q3GFX2)
function Q3GFX_DrawString(ctx, x, y, text, color, shadowColor, charWidth, charHeight, maxWidth, flags, font, context) {
    if (!text || !font) return;
    
    var proportional = (flags & DS_PROPORTIONAL) ? true : false;
    var hcenter = (flags & DS_HCENTER) ? true : false;
    var shadow = (flags & DS_SHADOW) && shadowColor ? true : false;
    
    // Compile text
    var commands = Q3GFX_CompileText(text);
    if (commands.length === 0) return;
    
    // Get font image
    var fontImg = Q3GFX_FontManager.getFontImage(font, charHeight);
    if (!fontImg) return;
    
    // Calculate string length for centering
    var stringLength = 0;
    if (hcenter) {
        for (var i = 0; i < commands.length; i++) {
            if (commands[i].type === 'CHAR') {
                var charCode = commands[i].value.charCodeAt(0) & 255;
                var fm = font.metrics[charCode];
                if (proportional) {
                    stringLength += fm.space1 * charWidth;
                    stringLength += fm.space2 * charWidth;
                } else {
                    stringLength += charWidth;
                }
            }
        }
    }
    
    // Adjust x for centering
    var ax = x;
    if (hcenter) {
        ax = x - (stringLength / 2);
    }
    
    var ay = y;
    var aw = charWidth;
    var ah = charHeight;
    
    // Shadow offset
    var shadowOffsetX = charWidth / 10.0;
    var shadowOffsetY = shadowOffsetX;
    
    // Save context state
    ctx.save();
    
    // Draw characters
    for (var i = 0; i < commands.length; i++) {
        if (commands[i].type !== 'CHAR') continue;
        
        var charCode = commands[i].value.charCodeAt(0) & 255;
        var fm = font.metrics[charCode];
        if (!fm) continue;
        
        var charColor = commands[i].color || color || [255, 255, 255, 255];
        var charBlink = commands[i].blink || 0;
        var charBold = commands[i].bold || false;
        var charFade = commands[i].fade || false;
        var charFast = commands[i].fast || false;
        
        // Get texture coordinates
        var tc = proportional ? fm.tc_prop : fm.tc_mono;
        var aw1 = proportional ? (fm.width * aw) : aw;
        
        // Calculate source rectangle
        var srcX = tc[0] * fontImg.width;
        var srcY = tc[1] * fontImg.height;
        var srcW = (tc[2] - tc[0]) * fontImg.width;
        var srcH = (tc[3] - tc[1]) * fontImg.height;
        
        // Calculate alpha based on effects
        var charAlpha = charColor[3] / 255.0;
        if (charBlink > 0) {
            // Use smooth time-based blinking for better visual quality
            var now = performance.now();
            var period = charBlink === 1 ? 1000 : 2000; // Fast: 1s, Slow: 2s period
            var time = (now % period) / period;
            // Smooth sine wave: 0.1 to 0.9 alpha range (more contrast)
            var sine = Math.sin(time * Math.PI * 2);
            charAlpha *= 0.1 + 0.8 * (0.5 + 0.5 * sine);
        }
        if (charFade) {
            charAlpha *= 0.7; // Fade effect
        }
        
        // Draw shadow
        if (shadow && shadowColor) {
            var shadowCanvas = document.createElement('canvas');
            shadowCanvas.width = aw1;
            shadowCanvas.height = ah;
            var shadowCtx = shadowCanvas.getContext('2d');
            
            shadowCtx.drawImage(fontImg, srcX, srcY, srcW, srcH, 0, 0, aw1, ah);
            shadowCtx.globalCompositeOperation = 'source-atop';
            shadowCtx.fillStyle = 'rgb(' + shadowColor[0] + ',' + shadowColor[1] + ',' + shadowColor[2] + ')';
            shadowCtx.fillRect(0, 0, aw1, ah);
            
            var oldAlpha = ctx.globalAlpha;
            ctx.globalAlpha = (shadowColor[3] / 255.0) * charAlpha;
            ctx.globalCompositeOperation = 'source-over';
            ctx.drawImage(shadowCanvas, ax + shadowOffsetX, ay + shadowOffsetY);
            ctx.globalAlpha = oldAlpha;
        }
        
        // Draw character with color
        var tempCanvas = document.createElement('canvas');
        tempCanvas.width = aw1;
        tempCanvas.height = ah;
        var tempCtx = tempCanvas.getContext('2d');
        
        tempCtx.drawImage(fontImg, srcX, srcY, srcW, srcH, 0, 0, aw1, ah);
        tempCtx.globalCompositeOperation = 'source-atop';
        tempCtx.fillStyle = 'rgb(' + charColor[0] + ',' + charColor[1] + ',' + charColor[2] + ')';
        tempCtx.fillRect(0, 0, aw1, ah);
        
        var oldAlpha = ctx.globalAlpha;
        ctx.globalAlpha = charAlpha;
        ctx.globalCompositeOperation = 'source-over';
        
        // Bold effect: draw twice with small offset
        if (charBold) {
            ctx.drawImage(tempCanvas, ax - 1, ay);
            ctx.drawImage(tempCanvas, ax + 1, ay);
        }
        ctx.drawImage(tempCanvas, ax, ay);
        
        ctx.globalAlpha = oldAlpha;
        
        // Advance position
        if (proportional) {
            ax += fm.space1 * aw;
            ax += fm.space2 * aw;
        } else {
            ax += aw;
        }
    }
    
    ctx.restore();
}

function UpdateScene(context)
{
    var ctx2d  = context.ctx2d;
    var params = context.params;
    
    // Draw background image
    if (context.hasOwnProperty("background"))
    {
        ctx2d.drawImage(context.background, 0, 0, params.width, params.height);
    }
    else
    {
        ctx2d.fillStyle = "rgb(128, 128, 128)";
        ctx2d.fillRect(0, 0, params.width, params.height);
    }

    DrawNicknameBar(context, context.gfxname[context.half]);
}

function DrawNicknameBar(context, gfx)
{
    // If font is selected, use font rendering directly (like test-font.html)
    if (context.currentFont && context.currentFont.imageCount > 0 && context.currentFont.images) {
        // Check if at least one image is loaded
        var hasImage = false;
        for (var imgIdx = 0; imgIdx < context.currentFont.imageCount; imgIdx++) {
            if (context.currentFont.images[imgIdx]) {
                hasImage = true;
                break;
            }
        }
        
        if (hasImage) {
            // Convert gfx array to text string with color codes
            var text = "";
            for (var i = 0; i < gfx.length; i++) {
                var entry = gfx[i];
                // Add color code if needed
                if (entry.color && entry.color.length >= 3) {
                    // Find color index or use RGB
                    var colorStr = "";
                    if (entry.color[0] === 0 && entry.color[1] === 0 && entry.color[2] === 0) colorStr = "^0";
                    else if (entry.color[0] === 255 && entry.color[1] === 0 && entry.color[2] === 0) colorStr = "^1";
                    else if (entry.color[0] === 0 && entry.color[1] === 255 && entry.color[2] === 0) colorStr = "^2";
                    else if (entry.color[0] === 255 && entry.color[1] === 255 && entry.color[2] === 0) colorStr = "^3";
                    else if (entry.color[0] === 0 && entry.color[1] === 0 && entry.color[2] === 255) colorStr = "^4";
                    else if (entry.color[0] === 0 && entry.color[1] === 255 && entry.color[2] === 255) colorStr = "^5";
                    else if (entry.color[0] === 255 && entry.color[1] === 0 && entry.color[2] === 255) colorStr = "^6";
                    else if (entry.color[0] === 255 && entry.color[1] === 255 && entry.color[2] === 255) colorStr = "^7";
                    else if (entry.color[0] === 255 && entry.color[1] === 128 && entry.color[2] === 0) colorStr = "^8";
                    else if (entry.color[0] === 128 && entry.color[1] === 128 && entry.color[2] === 128) colorStr = "^9";
                    else {
                        // Use RGB
                        var r = entry.color[0].toString(16).padStart(2, '0');
                        var g = entry.color[1].toString(16).padStart(2, '0');
                        var b = entry.color[2].toString(16).padStart(2, '0');
                        colorStr = "^x" + r + g + b;
                    }
                    text += colorStr;
                }
                // Add special effects
                // blink: 1 = fast blink (^b), 2 = slow blink/bold (^B)
                if (entry.blink === 1) {
                    text += "^b";
                } else if (entry.blink === 2) {
                    text += "^B";
                }
                // Note: fade, fast, normal are not stored in gfx array,
                // they are handled by skip logic in ParseGFX_OSP
                text += entry.symbol;
            }
            
            var charWidth = 45;
            var charHeight = 45;
            var x = context.params.width / 2;
            var y = context.params.height / 6.0;
            var color = [255, 255, 255, 255];
            var shadowColor = [0, 0, 0, 255];
            var flags = context.flags || (DS_PROPORTIONAL | DS_HCENTER | DS_SHADOW);
            
            Q3GFX_DrawString(
                context.ctx2d, x, y, text,
                color, shadowColor,
                charWidth, charHeight,
                context.params.width, flags,
                context.currentFont,
                context
            );
            return;
        }
    }
    
    // Original rendering method
    var font = 45;
    var max = 16;

    var params = {
        x:context.params.width / 2,
        y:context.params.height / 6.0,
        size:(context.params.width - (font * 2)) / max,
        shadow:true,
        center:true
    };
    
    if (gfx.length > max)
    {
        var total = max * params.size;
        params.size = total / gfx.length;
    }

    DrawGFXText(context, gfx, params);
}

function DrawGFXText(context, gfx, params)
{
    // Original rendering method (used when font is not selected)
    var spacing = params.size;
    var offset = params.size / 12;

    if (!context.hasOwnProperty("symbols"))
        return;

    var x = params.x;
    if (params.center)
        x = (context.params.width - (spacing * gfx.length) ) / 2;

    for (var i = 0; i < gfx.length; i++)
    {
        var entry = gfx[i];
        
        var opaque = 1.0;
        if (entry.blink) {
            // Use smooth time-based blinking (same as font rendering)
            var now = performance.now();
            var period = entry.blink === 1 ? 1000 : 2000; // Fast: 1s, Slow: 2s period
            var time = (now % period) / period;
            // Smooth sine wave: 0.1 to 0.9 alpha range (more contrast)
            var sine = Math.sin(time * Math.PI * 2);
            opaque = 0.1 + 0.8 * (0.5 + 0.5 * sine);
        }
        
        var code = entry.symbol.charCodeAt(0);
        var symPosX = code % 16;
        var symPosY = (code - symPosX) / 16;

        if (params.shadow)
        {
            DrawSymbolUsingMap(
                context,
                context.map[0],
                ConvertVectorToRGBA(entry.backgroundColor, opaque),
                symPosX,
                symPosY,
                x + (i * spacing) + offset,
                params.y + offset, 
                params.size,
                params.size
            );
        }
        
        DrawSymbolUsingMap(
            context,
            context.map[1],
            ConvertVectorToRGBA(entry.color, opaque),
            symPosX,
            symPosY,
            x + (i * spacing),
            params.y, 
            params.size,
            params.size
        );
    }
}

function DrawSymbolUsingMap(context, map, color, symX, symY, x, y, w, h)
{
    if (color != map.color)
    {
        var ctx2d  = map.ctx2d;
        map.color = color;
        ctx2d.drawImage(context.symbols, 0, 0, 64 * 16, 64 * 16, 0, 0, 64 * 16, 64 * 16);
        ctx2d.fillStyle = map.color;
        ctx2d.globalCompositeOperation = "source-in";
        ctx2d.fillRect(0, 0, 64 * 16, 64 * 16);
        ctx2d.globalCompositeOperation = "source-over";
    }

    var ctx2d  = context.ctx2d;
    ctx2d.drawImage(map.canvas, 64 * symX, 64 * symY, 64, 64, x, y, w, h);
}

function ReparseNickname(context)
{
    LoadNickname(context);
    context.current.parser(context, context.nickname);
}

function ShrinkVQ3Name(nickname)
{
    if (nickname.length > 35)
        nickname = nickname.substring(0, 35);

    return nickname;
}

function ShrinkCPMAName(nickname)
{
    var length = 0;
    var skip = false;
    var newname = "";

    for (var i = 0; i < nickname.length; i++)
    {
        if (nickname[i] == '^')
            skip = true;
        else if (skip)
            skip = false;
        else
            length++;
        
        newname += nickname[i];

        if (length >= 16)
            break;
    }

    return newname;
}

function ParseGFX_VQ3Style(context, nickname)
{
    context.gfxname[0] = context.gfxname[1] = ParseGFX_VQ3(nickname);
}

function ParseGFX_OSPStyle(context, nickname)
{
    context.gfxname[0] = ParseGFX_OSP(nickname, 0);
    context.gfxname[1] = ParseGFX_OSP(nickname, 1);
}

function ParseGFX_CPMAStyle(context, nickname)
{
    context.gfxname[0] = context.gfxname[1] = ParseGFX_CPMA(nickname);
}

function ParseGFX_VQ3(text)
{
    var command = false;
    var overwrite = false;
    var color = [255, 255, 255];
    var gfxs = [];
    
    function putCharToGFXArray()
    {
        gfxs[a++] = {
            symbol: chr,
            color: color,
            backgroundColor: [0, 0, 0],
            blink: 0
        };
    }
    
    for (var i = 0, a = 0; i < text.length; i++)
    {
        var chr = text.charAt(i);
        
        if (overwrite)
        {
            overwrite = false;
            gfxs.pop(), a--;
        }
        
        if (command)
        {    
            switch (chr)
            {
                case '^':
                    putCharToGFXArray();
                    putCharToGFXArray();
                    overwrite = true;
                    continue;
                default:
                    if (/^\d+$/.test(chr))
                        color = ConvertNumberToVQ3ColorVector(chr, color);
                    else
                        color = ConvertCharToVQ3ColorVector(chr, color);
            };
            
            command = false;
            continue;
        }
        else
        {
            if (chr =='^')
            {
                command = true;
                continue;
            }
        }
        
        putCharToGFXArray();
    }
    
    return gfxs;
}

function ParseGFX_OSP(text, half)
{
    var command = false;
    var blinking = 0;
    var overwrite = false;
    var skip = false;
    var colors = { 
        front:[255, 255, 255], 
        back:[0, 0, 0], 
        custom:false 
    };
    var gfxs = [];
    
    function putCharToGFXArray()
    {
        gfxs[a++] = {
            symbol: chr,
            color: colors.front,
            backgroundColor: colors.back,
            blink: blinking
        };
    }
    
    for (var i = 0, a = 0; i < text.length; i++)
    {
        var chr = text.charAt(i);
        
        if (overwrite)
        {
            overwrite = false;
            gfxs.pop(), a--;
        }
        
        if (command)
        {
            if (skip && chr != 'N'&& chr != 'f'&& chr != 'F')
            {
                command = false;
                continue;
            }

            switch (chr)
            {
                case 'b':
                case 'B':
                    blinking = (chr == 'b' ? 1 : 2);
                    if (colors.custom)
                    {
                        colors.front = colors.back;
                        colors.custom = false;
                    }
                    break;
                case 'f':
                    skip = (half == 1);
                    break;
                case 'F':
                    skip = (half == 0);
                    break;
                case 'n':
                    blinking = 0;
                    if (colors.custom)
                    {
                        colors.front = colors.back;
                        colors.custom = false;
                    }
                case 'N':
                    blinking = 0;
                    if (colors.custom)
                    {
                        colors.front = colors.back;
                        colors.custom = false;
                    }
                    skip = false;
                    break;
                case 'x':
                case 'X':
                    var rgbtext = text.substring(i + 1, i + 7);
                    if (ValidateRGBText(rgbtext))
                    {
                        colors.back = ParseRGBToVector(rgbtext, i);
                        colors.custom = true;
                        i += 6;
                    }
                    break;
                case '^':
                    putCharToGFXArray();
                    putCharToGFXArray();
                    overwrite = true;
                    continue;
                default:
                    if (/^\d+$/.test(chr))
                    {    
                        colors.front = ConvertNumberToOSPColorVector(chr, colors.front);
                        colors.custom = false;
                    }
            };
            
            command = false;
            continue;
        }
        else
        {
            if (chr =='^')
            {
                command = true;
                continue;
            }
        }
        
        if (skip)
            continue;
        
        putCharToGFXArray();
    }
    
    return gfxs;
}

function ParseGFX_CPMA(text)
{
    var command = false;
    var overwrite = false;
    var color = [255, 255, 255];
    var gfxs = [];
    
    function putCharToGFXArray()
    {
        gfxs[a++] = {
            symbol: chr,
            color: color,
            backgroundColor: [0, 0, 0],
            blink: 0
        };
    }
    
    for (var i = 0, a = 0; i < text.length; i++)
    {
        var chr = text.charAt(i);
        
        if (overwrite)
        {
            overwrite = false;
            gfxs.pop(), a--;
        }
        
        if (command)
        {    
            switch (chr)
            {
                case '^':
                    putCharToGFXArray();
                    putCharToGFXArray();
                    overwrite = true;
                    continue;
                default:
                    if (/^[a-zA-Z\d]$/.test(chr))
                        color = ConvertCharToCPMAColorVector(chr, color);
            };
            
            command = false;
            continue;
        }
        else
        {
            if (chr =='^')
            {
                command = true;
                continue;
            }
        }
        
        putCharToGFXArray();
    }
    
    return gfxs;
}

function ConvertNumberToVQ3ColorVector(chr, dflt)
{
    var color = dflt;
    
    switch (chr)
    {
        case '0':
        case '1':
        case '9':
            return [255,   0,   0];
        case '2':
            return [  0, 255,   0];
        case '3':
            return [255, 255,   0];
        case '4':
            return [  0,   0, 255];
        case '5':
            return [  0, 255, 255];
        case '6':
            return [255,   0, 255];
        case '7':
        case '8':
            return [255, 255, 255];
        default:
            break;
    }
    
    return color;
}

function ConvertCharToVQ3ColorVector(chr, dflt)
{
    var color = dflt;
    var ascii = chr.charCodeAt(0);
    var number = 0;

    if (/^[A-Z]$/.test(chr))
    {
        number = ascii - 'A'.charCodeAt(0);
    }
    else if (/^[a-z]$/.test(chr))
    {
        number = ascii - 'a'.charCodeAt(0);
    }
    else
    {
        return dflt;
    }

    number = (number % 8) + 1;

    return ConvertNumberToVQ3ColorVector("" + number, dflt)
}

function ConvertNumberToOSPColorVector(chr, dflt)
{
    var color = dflt;
    
    switch (chr)
    {
        case '0':
            return [  0,   0,   0];
        case '1':
            return [255,   0,   0];
        case '2':
            return [  0, 255,   0];
        case '3':
            return [255, 255,   0];
        case '4':
            return [  0,   0, 255];
        case '5':
            return [  0, 255, 255];
        case '6':
            return [255,   0, 255];
        case '7':
            return [255, 255, 255];
        case '8':
            return [255, 128,   0];
        case '9':
            return [128, 128, 128];
        default:
            break;
    }
    
    return color;
}

function ConvertCharToCPMAColorVector(chr, dflt)
{
    var color = dflt;
    chr = chr.toLowerCase();

    switch (chr)
    {
        case '0': return [  0,   0,   0];
        case '1': return [255,   0,   0];
        case '2': return [  0, 255,   0];
        case '3': return [255, 255,   0];
        case '4': return [  0,   0, 255];
        case '5': return [  0, 255, 255];
        case '6': return [255,   0, 255];
        case '7': return [0xB0, 0xB0, 0xB0];
        case '8': return [128, 128, 128];
        case '9': return [112, 112, 192];
        case 'a': return [255,   0,   0];
        case 'b': return [255,  64,   0];
        case 'c': return [255, 128,   0];
        case 'd': return [255, 192,   0];
        case 'e': return [255, 255,   0];
        case 'f': return [192, 255,   0];
        case 'g': return [128, 255,   0];
        case 'h': return [ 64, 255,   0];
        case 'i': return [  0, 255,   0];
        case 'j': return [  0, 255,  64];
        case 'k': return [  0, 255, 128];
        case 'l': return [  0, 255, 192];
        case 'm': return [  0, 255, 255];
        case 'n': return [  0, 192, 255];
        case 'o': return [  0, 128, 255];
        case 'p': return [  0,  64, 255];
        case 'q': return [  0,   0, 255];
        case 'r': return [ 64,   0, 255];
        case 's': return [128,   0, 255];
        case 't': return [192,   0, 255];
        case 'u': return [255,   0, 255];
        case 'v': return [255,   0, 192];
        case 'w': return [255,   0, 128];
        case 'x': return [255,   0,  64];
        case 'y': return [196, 196, 196];
        case 'z': return [160, 160, 160];
        default:
            break;
    }
    
    return color;
}

function ValidateRGBText(text)
{
    if (text.length != 6)
        return false;
    
    var re = /[0-9A-Fa-f]{6}/g;
    return re.test(text);
}

function ParseRGBToVector(rgbtext)
{
    return [
        parseInt( rgbtext.substring(0, 2), 16 ), 
        parseInt( rgbtext.substring(2, 4), 16 ), 
        parseInt( rgbtext.substring(4, 6), 16 )
    ];
}

function ConvertVectorToRGBA(vector, alpha)
{
    return "rgba(" + vector[0] + "," + vector[1] + "," + vector[2] + "," + alpha + ")";
}

// ====================
//   Misc

function AsyncLoadImage(context, image, callback) 
{
    var seconds = 0;
    var timeout = 10;
    var completed = false;
    
    function onImageLoad()
    {
        if (completed) 
            return;
        
        callback({context:context, error:'success', image:imageObject});
        completed = true;
    }
    
    function onTimeout()
    {
        if (completed) 
            return;
        
        if (seconds >= timeout)
        {
            callback({context:context, error:'timeout'});
            completed = true;
            return;
        }
        
        seconds++;
        callback.onTimeout = setTimeout(onTimeout, 1000);
    }
    
    var imageObject = new Image();
    imageObject.onload = onImageLoad;
    imageObject.src = image;
}

function MakeResourcePath(context, name)
{
    return context.params.resources + "/" + name;
}

