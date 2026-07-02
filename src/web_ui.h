#pragma once
#include <pgmspace.h>

const char PAGE_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>HID Control</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#1a1a2e;color:#e0e0e0;font-family:-apple-system,BlinkMacSystemFont,sans-serif;padding:10px;max-width:480px;margin:auto}
h2{color:#7c83fd;font-size:12px;text-transform:uppercase;letter-spacing:1px;margin-bottom:10px}
.card{background:#16213e;border-radius:10px;padding:14px;margin-bottom:10px}
button{background:#0f3460;color:#e0e0e0;border:none;border-radius:6px;padding:9px 13px;cursor:pointer;font-size:13px;transition:background .15s}
button:hover{background:#7c83fd}
button:active{transform:scale(.95)}
button.on{background:#27ae60}
button:disabled{opacity:.35;cursor:not-allowed;transform:none}
textarea{width:100%;background:#0f3460;color:#e0e0e0;border:1px solid #7c83fd44;border-radius:6px;padding:8px;font-size:13px;resize:vertical;min-height:55px;margin-bottom:8px}
.row{display:flex;flex-wrap:wrap;gap:6px;margin-bottom:6px}
.row:last-child{margin-bottom:0}

/* Status */
#statusBar{display:flex;align-items:center;gap:10px;background:#16213e;border-radius:10px;padding:10px 14px;margin-bottom:10px}
.dot{width:10px;height:10px;border-radius:50%;background:#e74c3c;flex-shrink:0}
.dot.ok{background:#2ecc71}
.dot.ok{animation:none}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}
.dot{animation:pulse 1.5s infinite}
.dot.ok{animation:none}

/* D-pad */
.dpad{display:grid;grid-template-columns:repeat(3,46px);grid-template-rows:repeat(3,46px);gap:4px;margin-bottom:10px}
.dpad button{width:46px;height:46px;padding:0;font-size:17px}
.dpad .gap{background:transparent}

/* Sliders / inputs */
.ctrl{display:flex;align-items:center;gap:8px;flex-wrap:wrap;margin-bottom:8px;font-size:13px}
.ctrl:last-child{margin-bottom:0}
input[type=range]{flex:1;min-width:80px;accent-color:#7c83fd}
input[type=number]{width:65px;background:#0f3460;color:#e0e0e0;border:1px solid #7c83fd44;border-radius:4px;padding:4px 6px;font-size:13px}
input[type=checkbox]{accent-color:#7c83fd;width:15px;height:15px}
label{font-size:13px;color:#aaa;cursor:pointer;display:flex;align-items:center;gap:6px}

/* Progress */
#prog{font-size:12px;color:#aaa;min-height:16px;margin-top:6px}
</style>
</head>
<body>

<div id="statusBar">
  <div class="dot" id="dot"></div>
  <span id="statusTxt">Connecting...</span>
</div>

<!-- KEYBOARD -->
<div class="card">
  <h2>Keyboard</h2>
  <div class="ctrl" style="margin-bottom:8px">
    <span>Layout:</span>
    <select id="kbLayout" onchange="cmd('LAYOUT '+this.value)" style="background:#2a2a2a;color:#eee;border:1px solid #444;padding:3px 8px;border-radius:4px">
      <option value="US">US</option>
      <option value="UK">UK</option>
    </select>
  </div>
  <textarea id="txt" placeholder="Type text here... (Enter to send, Alt+Enter for new line)"></textarea>
  <div class="row">
    <button id="sendBtn" onclick="sendText()">Send Text <small style="opacity:.6">[Ctrl+Shift+S]</small></button>
  </div>
  <div id="macros" style="margin-top:10px;display:flex;flex-direction:column;gap:6px"></div>
  <div class="row">
    <button class="key" onclick="cmd('KEY 40')">&#9166; Enter</button>
    <button class="key" onclick="cmd('KEY 41')">Esc</button>
    <button class="key" onclick="cmd('KEY 43')">Tab</button>
    <button class="key" onclick="cmd('KEY 42')">&#9003; Back</button>
    <button class="key" onclick="cmd('KEY 76')">Del</button>
  </div>
  <div class="row">
    <button class="key" onclick="cmd('KEY 82')">&#8593;</button>
    <button class="key" onclick="cmd('KEY 81')">&#8595;</button>
    <button class="key" onclick="cmd('KEY 80')">&#8592;</button>
    <button class="key" onclick="cmd('KEY 79')">&#8594;</button>
  </div>
  <div class="row">
    <button class="key" onclick="cmd('COMBO 1 6')">Ctrl+C</button>
    <button class="key" onclick="cmd('COMBO 1 25')">Ctrl+V</button>
    <button class="key" onclick="cmd('COMBO 1 29')">Ctrl+Z</button>
    <button class="key" onclick="cmd('COMBO 1 4')">Ctrl+A</button>
    <button class="key" onclick="cmd('COMBO 1 23')">Ctrl+W</button>
    <button class="key" onclick="cmd('COMBO 8 40')">Win+D</button>
  </div>
</div>

<!-- MOUSE -->
<div class="card">
  <h2>Mouse</h2>
  <div class="dpad">
    <button class="key" onclick="mv(-1,-1)">&#8598;</button>
    <button class="key" onclick="mv(0,-1)">&#8593;</button>
    <button class="key" onclick="mv(1,-1)">&#8599;</button>
    <button class="key" onclick="mv(-1,0)">&#8592;</button>
    <div class="gap"></div>
    <button class="key" onclick="mv(1,0)">&#8594;</button>
    <button class="key" onclick="mv(-1,1)">&#8601;</button>
    <button class="key" onclick="mv(0,1)">&#8595;</button>
    <button class="key" onclick="mv(1,1)">&#8600;</button>
  </div>
  <div class="ctrl">
    <span>Step:</span>
    <input type="range" id="stepR" min="1" max="100" value="20" oninput="step=+this.value;document.getElementById('stepV').textContent=step">
    <span id="stepV">20</span>px
  </div>
  <div class="row">
    <button class="key" onclick="cmd('CLICK 1')">Left Click</button>
    <button class="key" onclick="cmd('CLICK 2')">Right Click</button>
    <button class="key" onclick="cmd('CLICK 3')">Middle</button>
    <button class="key" onclick="cmd('SCROLL 3')">Scroll &#8593;</button>
    <button class="key" onclick="cmd('SCROLL -3')">Scroll &#8595;</button>
  </div>
  <div class="row" style="margin-top:8px">
    <button id="ptBtn" onclick="togglePassthrough()">&#127918; Mouse Passthrough <small style="opacity:.6">[RAlt+M]</small></button>
    <span id="ptStat" style="font-size:12px;color:#aaa;align-self:center"></span>
  </div>
</div>

<!-- JIGGLER -->
<div class="card">
  <h2>Mouse Jiggler</h2>
  <div class="row" style="margin-bottom:10px">
    <button id="jigBtn" onclick="toggleJig()">Enable Jiggler</button>
    <span id="jigStat" style="font-size:12px;color:#2ecc71;line-height:1;align-self:center"></span>
  </div>
  <div id="jigTimer" style="font-size:11px;color:#888;margin-bottom:6px;font-family:monospace;min-height:14px"></div>
  <div id="jigTrend" style="margin-bottom:8px;display:none">
    <div style="font-size:10px;color:#666;margin-bottom:3px">Recent intervals:</div>
    <div id="jigBars" style="display:flex;align-items:flex-end;gap:3px;height:32px"></div>
    <div id="jigLabels" style="display:flex;gap:3px;margin-top:2px"></div>
  </div>
  <div class="ctrl">
    <span>Min interval:</span>
    <input type="range" id="jigMin" min="1" max="1200" value="20"
      oninput="document.getElementById('jigMinV').textContent=this.value;cmd('JIGGLE MIN '+(this.value*1000))">
    <span id="jigMinV">20</span>s
  </div>
  <div class="ctrl">
    <span>Max interval:</span>
    <input type="range" id="jigMax" min="1" max="1200" value="60"
      oninput="document.getElementById('jigMaxV').textContent=this.value;cmd('JIGGLE MAX '+(this.value*1000))">
    <span id="jigMaxV">60</span>s
  </div>
  <div class="ctrl">
    <span>Step:</span>
    <input type="number" id="jigStep" value="10" min="1" max="100"
      onchange="cmd('JIGGLE STEP '+this.value)">px
  </div>
  <div class="ctrl">
    <label><input type="checkbox" id="jigRand" onchange="cmd('JIGGLE RAND '+(this.checked?1:0))"> Randomise direction</label>
  </div>
</div>

<!-- BLE DEVICES -->
<div class="card">
  <h2>Bluetooth Devices</h2>
  <p style="font-size:11px;color:#888;margin-bottom:8px">To switch devices: <strong style="color:#bbb">1)</strong> Turn off Bluetooth on the current device &nbsp;<strong style="color:#bbb">2)</strong> Tap Switch Device &nbsp;<strong style="color:#bbb">3)</strong> Connect from the new device's Bluetooth settings. To switch back, repeat with the other device.</p>
  <div id="bleList" style="font-size:13px;color:#aaa">Loading...</div>
  <div class="row" style="margin-top:8px">
    <button onclick="bleSwitch()">Switch Device</button>
    <button onclick="loadBleDevices()">Refresh</button>
    <button onclick="bleForgetAll()" style="background:#7a2020">Forget All</button>
  </div>
  <span id="bleStat" style="font-size:12px;color:#aaa;margin-top:4px;display:block"></span>
</div>

<!-- WIFI CONFIG -->
<div class="card">
  <h2>Home WiFi</h2>
  <p style="font-size:11px;color:#888;margin-bottom:8px">Connect to your local network — access GUI without switching WiFi</p>
  <div class="ctrl">
    <span>SSID:</span>
    <input type="text" id="wifiSSID" placeholder="Network name" style="flex:1;background:#0f3460;color:#e0e0e0;border:1px solid #7c83fd44;border-radius:6px;padding:8px;font-size:13px">
  </div>
  <div class="ctrl">
    <span>Pass:</span>
    <input type="password" id="wifiPass" placeholder="Password" style="flex:1;background:#0f3460;color:#e0e0e0;border:1px solid #7c83fd44;border-radius:6px;padding:8px;font-size:13px">
  </div>
  <div class="row" style="margin-top:8px">
    <button onclick="wifiConnect()">Connect</button>
    <button onclick="wifiClear()">Clear / Disconnect</button>
  </div>
  <div id="wifiStat" style="font-size:12px;color:#aaa;margin-top:6px"></div>
</div>

<!-- SHAPES -->
<div class="card">
  <h2>Draw Shapes</h2>
  <p style="font-size:11px;color:#888;margin-bottom:8px">Click into a drawing app first (pencil tool, pointer precision off)</p>
  <div class="row">
    <button id="shpCircle" onclick="drawCircle()">&#11044; Circle</button>
    <button id="shpSquare" onclick="drawSquare()">&#9632; Square</button>
    <button id="shpTri"    onclick="drawTriangle()">&#9650; Triangle</button>
    <button id="shpStar"   onclick="drawStar()">&#9733; Star</button>
    <button id="shpHeart"  onclick="drawHeart()">&#10084; Heart</button>
    <button id="shpSpiral" onclick="drawSpiral()">&#10007; Spiral</button>
    <button id="shpHex"    onclick="drawHexagon()">&#11041; Hexagon</button>
  </div>
  <div id="prog"></div>
</div>

<!-- TEXT DRAWING -->
<div class="card">
  <h2>Draw Block Text</h2>
  <p style="font-size:11px;color:#888;margin-bottom:8px">Draws large pixel-art letters with the mouse</p>
  <div class="ctrl">
    <input type="text" id="blockText" maxlength="6" placeholder="HI" style="flex:1;background:#0f3460;color:#e0e0e0;border:1px solid #7c83fd44;border-radius:6px;padding:8px;font-size:16px">
    <button id="shpText" onclick="drawText()">Draw</button>
  </div>
</div>

<!-- IMAGES -->
<div class="card">
  <h2>Draw Images</h2>
  <p style="font-size:11px;color:#888;margin-bottom:8px">Pre-drawn line art — click into drawing app first</p>
  <div class="row">
    <button id="shpSmiley" onclick="drawSmiley()">&#128512; Smiley</button>
    <button id="shpHouse"  onclick="drawHouse()">&#127968; House</button>
    <button id="shpTree"   onclick="drawTree()">&#127795; Tree</button>
    <button id="shpArrow"  onclick="drawArrow()">&#10144; Arrow</button>
  </div>
</div>

<script>
var step = 20;
var jigOn = false;
var drawing = false;

function sleep(ms){ return new Promise(r=>setTimeout(r,ms)); }

// WebSocket for low-latency commands
var sock = null;
var sockReady = false;

function connectWS(){
  sock = new WebSocket('ws://'+location.hostname+':81');
  sock.onopen  = function(){ sockReady=true; };
  sock.onclose = function(){ sockReady=false; setTimeout(connectWS,2000); };
  sock.onerror = function(){ sockReady=false; };
}
connectWS();

// Send over WS if available, else fall back to HTTP
async function cmd(c){
  if(sockReady){ sock.send(c); return {ok:true}; }
  try{
    var r = await fetch('/cmd',{method:'POST',body:c});
    return await r.json();
  }catch(e){ return {ok:false}; }
}

// For commands where we need the response (e.g. status checks), always use HTTP
async function cmdHttp(c){
  try{
    var r = await fetch('/cmd',{method:'POST',body:c});
    return await r.json();
  }catch(e){ return {ok:false}; }
}

async function sendText(){
  var t = document.getElementById('txt').value;
  if(!t) return;
  await cmd('TYPE '+t);
}

var INP='flex:1;min-width:0;background:#1a1a2e;color:#eee;border:1px solid #444;border-radius:6px;padding:6px 8px;font-size:12px';
(function(){
  var wrap=document.getElementById('macros');
  for(var n=1;n<=5;n++){
    var row=document.createElement('div');
    row.style.cssText='display:flex;gap:5px;align-items:center';
    row.innerHTML=
      '<input type="text" id="mname'+n+'" placeholder="Name..." autocomplete="off" oninput="saveNick('+n+')" style="width:80px;flex:0 0 80px;'+INP+'">'
      +'<input type="password" id="macro'+n+'" placeholder="Content..." autocomplete="off" oninput="saveMacro('+n+')" style="'+INP+'">'
      +'<button onclick="sendMacro('+n+')" style="white-space:nowrap;font-size:11px">Send&nbsp;<small style="opacity:.6">C+S+'+n+'</small></button>';
    wrap.appendChild(row);
  }
  for(var i=1;i<=5;i++){
    var nick=localStorage.getItem('macroname'+i), val=localStorage.getItem('macro'+i);
    if(nick) document.getElementById('mname'+i).value=nick;
    if(val)  document.getElementById('macro'+i).value=val;
  }
})();

function saveNick(n){ localStorage.setItem('macroname'+n, document.getElementById('mname'+n).value); }
function saveMacro(n){ localStorage.setItem('macro'+n, document.getElementById('macro'+n).value); }
function sendMacro(n){ var t=document.getElementById('macro'+n).value; if(t) cmd('TYPE '+t); }

document.getElementById('txt').addEventListener('keydown', function(e){
  if(e.key==='Enter' && e.altKey){
    // Alt+Enter — insert newline
    e.preventDefault();
    var t=e.target, s=t.selectionStart, v=t.value;
    t.value=v.substring(0,s)+'\n'+v.substring(t.selectionEnd);
    t.selectionStart=t.selectionEnd=s+1;
  } else if(e.key==='Enter'){
    e.preventDefault();
    sendText();
  }
});

function mv(sx,sy){ cmd('MOVE '+(sx*step)+' '+(sy*step)); }

// Jiggler
function toggleJig(){
  cmd(jigOn ? 'JIGGLE OFF' : 'JIGGLE ON').then(pollStatus);
}

// Status polling
async function pollStatus(){
  try{
    var r = await fetch('/status');
    var d = await r.json();
    var c = d.connected;
    document.getElementById('dot').className = 'dot'+(c?' ok':'');
    var bleStr = c ? 'BLE Connected' : 'BLE Advertising...';
    var staStr = d.staIP ? ' | WiFi: '+d.staSSID+' ('+d.staIP+')' : '';
    document.getElementById('statusTxt').textContent = bleStr + staStr;

    // Update WiFi status line
    var ws = document.getElementById('wifiStat');
    if (d.staIP && !ws.textContent.includes('onnect')) {
      ws.textContent = 'Connected: ' + d.staIP;
      ws.style.color = '#2ecc71';
    } else if (!d.staIP && ws.textContent.startsWith('Connected')) {
      ws.textContent = '';
    }

    // Sync keyboard layout
    if(d.layout) document.getElementById('kbLayout').value = d.layout;

    // Sync jiggler UI
    jigOn = d.jiggling;
    var jb = document.getElementById('jigBtn');
    jb.textContent = jigOn ? 'Disable Jiggler' : 'Enable Jiggler';
    jb.className = jigOn ? 'on' : '';
    document.getElementById('jigStat').textContent = jigOn ? 'Jiggling...' : '';
    if(d.jiggleMinMs){ document.getElementById('jigMin').value = Math.round(d.jiggleMinMs/1000); document.getElementById('jigMinV').textContent = Math.round(d.jiggleMinMs/1000); }
    if(d.jiggleMaxMs){ document.getElementById('jigMax').value = Math.round(d.jiggleMaxMs/1000); document.getElementById('jigMaxV').textContent = Math.round(d.jiggleMaxMs/1000); }
    if(d.jiggleStep) document.getElementById('jigStep').value = d.jiggleStep;
    if(typeof d.jiggleRand !== 'undefined') document.getElementById('jigRand').checked = d.jiggleRand;
    if(typeof d.msUntilJiggle !== 'undefined'){ window._jigMs=d.msUntilJiggle; window._jigAt=Date.now(); window._jigCnt=d.jiggleCount||0; }
    if(d.jiggleHistory) renderJigTrend(d.jiggleHistory);

    // Disable BLE-dependent buttons when disconnected or drawing
    var dis = !c || drawing;
    document.querySelectorAll('button.key, #sendBtn').forEach(function(b){ b.disabled=dis; });
    document.querySelectorAll('button[id^=shp]').forEach(function(b){ b.disabled=drawing; });
  }catch(e){}
}
setInterval(pollStatus, 2000);
pollStatus();

function renderJigTrend(hist){
  var trend=document.getElementById('jigTrend');
  var bars=document.getElementById('jigBars');
  var labs=document.getElementById('jigLabels');
  if(!hist||!hist.length){trend.style.display='none';return;}
  trend.style.display='block';
  var mx=Math.max.apply(null,hist);
  bars.innerHTML=''; labs.innerHTML='';
  hist.forEach(function(v,i){
    var h=mx?Math.max(4,Math.round((v/mx)*32)):8;
    var bar=document.createElement('div');
    bar.style.cssText='width:18px;background:#2ecc71;height:'+h+'px;border-radius:2px 2px 0 0;opacity:'+(0.4+0.6*(i+1)/hist.length).toFixed(2);
    bars.appendChild(bar);
    var lbl=document.createElement('div');
    lbl.style.cssText='width:18px;font-size:9px;color:#666;text-align:center;overflow:hidden';
    lbl.textContent=v+'s';
    labs.appendChild(lbl);
  });
}

setInterval(function(){
  var el=document.getElementById('jigTimer');
  if(!el) return;
  if(!jigOn||window._jigMs===undefined||window._jigMs<0){el.textContent='';return;}
  var left=Math.max(0,Math.round((window._jigMs-(Date.now()-window._jigAt))/1000));
  el.textContent='Next: ~'+left+'s | Fired: '+(window._jigCnt||0)+'x';
},1000);

// Mouse passthrough
var ptActive = false;
var ptAccX = 0, ptAccY = 0;
var ptTimer = null;

function togglePassthrough(){
  if(!ptActive){ document.body.requestPointerLock(); }
  else { document.exitPointerLock(); }
}

document.addEventListener('pointerlockchange', function(){
  ptActive = !!document.pointerLockElement;
  var btn = document.getElementById('ptBtn');
  var stat = document.getElementById('ptStat');
  if(ptActive){
    btn.innerHTML = 'Exit Passthrough <small style="opacity:.6">[RAlt+M or Esc]</small>';
    btn.className = 'on';
    stat.textContent = 'Active — Esc to exit';
    ptAccX = 0; ptAccY = 0;
    ptTimer = setInterval(ptFlush, 20);
  } else {
    btn.innerHTML = '\u{1F3AE} Mouse Passthrough <small style="opacity:.6">[RAlt+M]</small>';
    btn.className = '';
    stat.textContent = '';
    clearInterval(ptTimer);
    ptMods = 0;
    cmd('RELEASE');
  }
});

document.addEventListener('mousemove', function(e){
  if(!ptActive) return;
  ptAccX += e.movementX;
  ptAccY += e.movementY;
  // Send immediately over WS — don't wait for flush timer
  if(sockReady) ptFlush();
});

document.addEventListener('mousedown', function(e){
  if(!ptActive) return;
  e.preventDefault();
  var b = e.button===0?1:e.button===2?2:3;
  cmd('PRESS '+b);
});

document.addEventListener('mouseup', function(e){
  if(!ptActive) return;
  cmd('RELEASE');
});

document.addEventListener('wheel', function(e){
  if(!ptActive) return;
  e.preventDefault();
  cmd('SCROLL '+(e.deltaY>0?-3:3));
},{passive:false});

document.addEventListener('contextmenu', function(e){
  if(ptActive) e.preventDefault();
});

// Keyboard passthrough
// Browser keyCode -> HID keycode table
var KEY_MAP = {
  'KeyA':4,'KeyB':5,'KeyC':6,'KeyD':7,'KeyE':8,'KeyF':9,'KeyG':10,'KeyH':11,
  'KeyI':12,'KeyJ':13,'KeyK':14,'KeyL':15,'KeyM':16,'KeyN':17,'KeyO':18,'KeyP':19,
  'KeyQ':20,'KeyR':21,'KeyS':22,'KeyT':23,'KeyU':24,'KeyV':25,'KeyW':26,'KeyX':27,
  'KeyY':28,'KeyZ':29,
  'Digit1':30,'Digit2':31,'Digit3':32,'Digit4':33,'Digit5':34,
  'Digit6':35,'Digit7':36,'Digit8':37,'Digit9':38,'Digit0':39,
  'Enter':40,'Escape':41,'Backspace':42,'Tab':43,'Space':44,
  'Minus':45,'Equal':46,'BracketLeft':47,'BracketRight':48,'Backslash':49,
  'Semicolon':51,'Quote':52,'Backquote':53,'Comma':54,'Period':55,'Slash':56,
  'CapsLock':57,
  'F1':58,'F2':59,'F3':60,'F4':61,'F5':62,'F6':63,
  'F7':64,'F8':65,'F9':66,'F10':67,'F11':68,'F12':69,
  'Insert':73,'Home':74,'PageUp':75,'Delete':76,'End':77,'PageDown':78,
  'ArrowRight':79,'ArrowLeft':80,'ArrowDown':81,'ArrowUp':82,
  'NumLock':83,'NumpadDivide':84,'NumpadMultiply':85,'NumpadSubtract':86,
  'NumpadAdd':87,'NumpadEnter':88,
  'Numpad1':89,'Numpad2':90,'Numpad3':91,'Numpad4':92,'Numpad5':93,
  'Numpad6':94,'Numpad7':95,'Numpad8':96,'Numpad9':97,'Numpad0':98,
  'NumpadDecimal':99
};

var ptMods = 0;   // current modifier bitmask
var rightAlt = false; // track Right Alt for passthrough toggle

function modBit(code){
  return {
    'ControlLeft':1,'ControlRight':16,
    'ShiftLeft':2,'ShiftRight':32,
    'AltLeft':4,'AltRight':64,
    'MetaLeft':8,'MetaRight':128
  }[code]||0;
}

document.addEventListener('keydown', function(e){
  if(e.code==='AltRight') rightAlt = true;

  // Global shortcuts — work regardless of passthrough state
  if(e.ctrlKey && e.shiftKey && e.code==='KeyS'){
    e.preventDefault(); e.stopPropagation();
    sendText(); return;
  }
  if(rightAlt && e.code==='KeyM'){
    e.preventDefault(); e.stopPropagation();
    rightAlt = false;
    togglePassthrough(); return;
  }
  if(e.ctrlKey && e.shiftKey && /^Digit[1-5]$/.test(e.code)){
    e.preventDefault(); e.stopPropagation();
    sendMacro(parseInt(e.code[5])); return;
  }

  // Passthrough keyboard forwarding
  if(!document.pointerLockElement) return;
  if(e.code==='Escape'){ document.exitPointerLock(); return; }
  e.preventDefault();
  e.stopPropagation();

  var mod = modBit(e.code);
  if(mod){ ptMods |= mod; return; }

  var key = KEY_MAP[e.code];
  if(!key) return;

  var c = ptMods ? ('COMBO '+ptMods+' '+key) : ('KEY '+key);
  // Show last key in status for feedback
  document.getElementById('ptStat').textContent = 'Active — ' + e.code + (ptMods?' mod:'+ptMods:'') + ' — Esc to exit';
  if(sockReady) sock.send(c);
  else cmd(c);
});

document.addEventListener('keyup', function(e){
  if(e.code==='AltRight') rightAlt = false;
  if(!document.pointerLockElement) return;
  var mod = modBit(e.code);
  if(mod) ptMods &= ~mod;
});

function ptFlush(){
  if(!ptActive || (ptAccX===0 && ptAccY===0)) return;
  var dx = Math.max(-127, Math.min(127, Math.round(ptAccX)));
  var dy = Math.max(-127, Math.min(127, Math.round(ptAccY)));
  ptAccX -= dx; ptAccY -= dy;
  if(sockReady){ sock.send('MOVE '+dx+' '+dy); }
  else { fetch('/cmd',{method:'POST',body:'MOVE '+dx+' '+dy}); }
}

// BLE device management
async function loadBleDevices(){
  var el=document.getElementById('bleList');
  try{
    var r=await fetch('/ble'); var d=await r.json();
    el.innerHTML='';
    var info=document.createElement('div');
    info.style.cssText='font-size:13px;margin-bottom:8px;color:#ccc';
    info.textContent=d.count+' bonded device'+(d.count===1?'':'s')+
                     (d.active?' — currently connected':' — advertising');
    el.appendChild(info);
  }catch(e){ el.textContent='Error loading devices'; }
}

async function bleSwitch(){
  var el=document.getElementById('bleStat');
  el.textContent='Switching...';
  var r=await fetch('/ble/connect',{method:'POST'});
  var d=await r.json();
  el.textContent=d.wasConnected ? 'Disconnected — now advertising. Connect from new device.' : 'Advertising — connect from your device.';
  setTimeout(function(){ loadBleDevices(); el.textContent=''; },4000);
}

async function bleForgetAll(){
  if(!confirm('Remove all bonded devices?')) return;
  await fetch('/ble/forget',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'mac='});
  loadBleDevices();
}

loadBleDevices();

// WiFi config
async function wifiConnect(){
  var ssid=document.getElementById('wifiSSID').value;
  var pass=document.getElementById('wifiPass').value;
  if(!ssid) return;
  var ws=document.getElementById('wifiStat');
  ws.textContent='Connecting...'; ws.style.color='#aaa';
  var r=await fetch('/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass)});
  var d=await r.json();
  ws.textContent=d.msg||(d.ok?'OK':'Failed');
}
async function wifiClear(){
  var ws=document.getElementById('wifiStat');
  var r=await fetch('/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='});
  var d=await r.json();
  ws.textContent=d.msg||(d.ok?'Cleared':'Failed'); ws.style.color='#aaa';
  document.getElementById('wifiSSID').value='';
  document.getElementById('wifiPass').value='';
}

// Shapes
async function drawSeq(cmds, label){
  drawing = true;
  var prog = document.getElementById('prog');
  for(var i=0;i<cmds.length;i++){
    prog.textContent = label+': '+( i+1)+'/'+cmds.length;
    await cmd(cmds[i]);
    await sleep(40);
  }
  await cmd('RELEASE');
  prog.textContent = label+' done!';
  drawing = false;
  setTimeout(function(){ prog.textContent=''; }, 2000);
}

function drawCircle(){
  var r=50, s=60, cmds=[];
  for(var i=0;i<s;i++){
    var a0=2*Math.PI*i/s, a1=2*Math.PI*(i+1)/s;
    cmds.push('DRAG '+Math.round(r*(Math.cos(a1)-Math.cos(a0)))+' '+Math.round(r*(Math.sin(a1)-Math.sin(a0))));
  }
  drawSeq(cmds,'Circle');
}

function drawSquare(){
  var sz=100, st=10, d=Math.round(sz/st), cmds=[];
  for(var i=0;i<st;i++) cmds.push('DRAG '+d+' 0');
  for(var i=0;i<st;i++) cmds.push('DRAG 0 '+d);
  for(var i=0;i<st;i++) cmds.push('DRAG -'+d+' 0');
  for(var i=0;i<st;i++) cmds.push('DRAG 0 -'+d);
  drawSeq(cmds,'Square');
}

function drawTriangle(){
  var sz=100, st=20;
  var dx1=Math.round(sz/st);
  var dx2=Math.round(-sz/2/st);
  var dy2=Math.round(sz*Math.sqrt(3)/2/st);
  var cmds=[];
  for(var i=0;i<st;i++) cmds.push('DRAG '+dx1+' 0');
  for(var i=0;i<st;i++) cmds.push('DRAG '+dx2+' '+dy2);
  for(var i=0;i<st;i++) cmds.push('DRAG '+dx2+' -'+dy2);
  drawSeq(cmds,'Triangle');
}

function drawStar(){
  var outer=60, inner=25, pts=5, st=4, cmds=[];
  var verts=[];
  for(var i=0;i<pts*2;i++){
    var a=Math.PI*i/pts - Math.PI/2;
    var r=i%2===0?outer:inner;
    verts.push([Math.round(r*Math.cos(a)), Math.round(r*Math.sin(a))]);
  }
  verts.push(verts[0]);
  for(var i=1;i<verts.length;i++){
    var dx=verts[i][0]-verts[i-1][0], dy=verts[i][1]-verts[i-1][1];
    for(var s=0;s<st;s++) cmds.push('DRAG '+Math.round(dx/st)+' '+Math.round(dy/st));
  }
  drawSeq(cmds,'Star');
}

function drawHeart(){
  var s=40, st=72, cmds=[];
  for(var i=0;i<st;i++){
    var t0=2*Math.PI*i/st-Math.PI, t1=2*Math.PI*(i+1)/st-Math.PI;
    var x0=s*(16*Math.pow(Math.sin(t0),3));
    var y0=s*-(13*Math.cos(t0)-5*Math.cos(2*t0)-2*Math.cos(3*t0)-Math.cos(4*t0));
    var x1=s*(16*Math.pow(Math.sin(t1),3));
    var y1=s*-(13*Math.cos(t1)-5*Math.cos(2*t1)-2*Math.cos(3*t1)-Math.cos(4*t1));
    cmds.push('DRAG '+Math.round(x1-x0)+' '+Math.round(y1-y0));
  }
  drawSeq(cmds,'Heart');
}

function drawSpiral(){
  var turns=4, steps=turns*36, cmds=[];
  var px=0,py=0;
  for(var i=1;i<=steps;i++){
    var a=2*Math.PI*i/36;
    var r=i*1.8;
    var nx=Math.round(r*Math.cos(a)), ny=Math.round(r*Math.sin(a));
    cmds.push('DRAG '+(nx-px)+' '+(ny-py));
    px=nx; py=ny;
  }
  drawSeq(cmds,'Spiral');
}

function drawHexagon(){
  var r=60, sides=6, st=8, cmds=[];
  var verts=[];
  for(var i=0;i<=sides;i++){
    var a=Math.PI*i/3 - Math.PI/6;
    verts.push([Math.round(r*Math.cos(a)), Math.round(r*Math.sin(a))]);
  }
  for(var i=1;i<verts.length;i++){
    var dx=verts[i][0]-verts[i-1][0], dy=verts[i][1]-verts[i-1][1];
    for(var s=0;s<st;s++) cmds.push('DRAG '+Math.round(dx/st)+' '+Math.round(dy/st));
  }
  drawSeq(cmds,'Hexagon');
}

// Block text — 5x7 pixel font
var FONT={
  'A':[[0,6],[1,0],[2,6],[1,3],[2,3]],
  'B':[[0,0],[0,6],[1,0],[2,1],[1,3],[2,4],[1,6],[2,5]],
  'C':[[2,1],[1,0],[0,1],[0,5],[1,6],[2,5]],
  'D':[[0,0],[0,6],[1,0],[2,1],[2,5],[1,6],[0,6]],
  'E':[[2,0],[0,0],[0,6],[2,6],[0,3],[1,3]],
  'H':[[0,0],[0,6],[0,3],[2,3],[2,0],[2,6]],
  'I':[[0,0],[2,0],[1,0],[1,6],[0,6],[2,6]],
  'L':[[0,0],[0,6],[2,6]],
  'O':[[1,0],[2,1],[2,5],[1,6],[0,5],[0,1],[1,0]],
  'S':[[2,1],[1,0],[0,1],[0,3],[1,3],[2,4],[2,6],[1,6],[0,5]],
  'T':[[0,0],[2,0],[1,0],[1,6]],
  'U':[[0,0],[0,5],[1,6],[2,5],[2,0]],
  ' ':[]
};

async function drawText(){
  var txt = (document.getElementById('blockText').value||'HI').toUpperCase();
  var sc=12, gap=4; // scale and letter gap
  drawing=true;
  var prog=document.getElementById('prog');
  var cx=0;
  for(var ci=0;ci<txt.length;ci++){
    var ch=txt[ci], strokes=FONT[ch]||FONT[' '];
    // Each stroke is a list of [col,row] waypoints scaled by sc
    // We treat strokes as pairs: lift pen, move to start, draw to end
    for(var si=0;si<strokes.length;si+=2){
      if(si+1>=strokes.length) break;
      var x0=cx+strokes[si][0]*sc, y0=strokes[si][1]*sc;
      var x1=cx+strokes[si+1][0]*sc, y1=strokes[si+1][1]*sc;
      // Navigate to start (pen up)
      await cmd('RELEASE');
      await cmd('MOVE '+(x0)+' '+(y0)); await sleep(30);
      // Draw to end
      await cmd('PRESS 1');
      var ddx=x1-x0, ddy=y1-y0, steps=Math.max(Math.abs(ddx),Math.abs(ddy),1);
      for(var s=1;s<=steps;s++){
        await cmd('MOVE '+Math.round((ddx/steps))+' '+Math.round((ddy/steps)));
        await sleep(20);
      }
      prog.textContent='Text: '+txt[ci]+' stroke '+(si/2+1);
    }
    cx+=(3*sc)+gap;
  }
  await cmd('RELEASE');
  prog.textContent='Text done!';
  drawing=false;
  setTimeout(function(){prog.textContent='';},2000);
}

// Pre-baked images as stroke lists: [lift, x,y, draw, x,y, ...]
// Format: sequence of {pen: 0=up/1=down, dx, dy} relative moves
function strokesToCmds(strokes){
  var cmds=[], px=0, py=0;
  for(var i=0;i<strokes.length;i++){
    var s=strokes[i];
    if(s[0]===0){ // pen up move
      cmds.push('RELEASE');
      if(s[1]!==0||s[2]!==0) cmds.push('MOVE '+s[1]+' '+s[2]);
    } else { // pen down
      cmds.push('DRAG '+s[1]+' '+s[2]);
    }
  }
  return cmds;
}

async function drawSmiley(){
  var cmds=[], r=50, s=60;
  // Face circle
  for(var i=0;i<s;i++){
    var a0=2*Math.PI*i/s, a1=2*Math.PI*(i+1)/s;
    cmds.push('DRAG '+Math.round(r*(Math.cos(a1)-Math.cos(a0)))+' '+Math.round(r*(Math.sin(a1)-Math.sin(a0))));
  }
  // Left eye
  cmds.push('RELEASE'); cmds.push('MOVE -18 -15');
  for(var i=0;i<12;i++){
    var a0=2*Math.PI*i/12, a1=2*Math.PI*(i+1)/12, er=7;
    cmds.push('DRAG '+Math.round(er*(Math.cos(a1)-Math.cos(a0)))+' '+Math.round(er*(Math.sin(a1)-Math.sin(a0))));
  }
  // Right eye
  cmds.push('RELEASE'); cmds.push('MOVE 36 0');
  for(var i=0;i<12;i++){
    var a0=2*Math.PI*i/12, a1=2*Math.PI*(i+1)/12, er=7;
    cmds.push('DRAG '+Math.round(er*(Math.cos(a1)-Math.cos(a0)))+' '+Math.round(er*(Math.sin(a1)-Math.sin(a0))));
  }
  // Smile arc
  cmds.push('RELEASE'); cmds.push('MOVE -50 20');
  var sr=28, sa=Math.PI*0.15, ea=Math.PI*0.85, ast=20;
  for(var i=0;i<ast;i++){
    var a0=sa+(ea-sa)*i/ast, a1=sa+(ea-sa)*(i+1)/ast;
    cmds.push('DRAG '+Math.round(sr*(Math.cos(a1)-Math.cos(a0)))+' '+Math.round(sr*(Math.sin(a1)-Math.sin(a0))));
  }
  drawSeq(cmds,'Smiley');
}

async function drawHouse(){
  var cmds=[];
  var w=80, h=60, rh=40, st=10;
  // Base square
  for(var i=0;i<st;i++) cmds.push('DRAG '+Math.round(w/st)+' 0');
  for(var i=0;i<st;i++) cmds.push('DRAG 0 '+Math.round(h/st));
  for(var i=0;i<st;i++) cmds.push('DRAG -'+Math.round(w/st)+' 0');
  for(var i=0;i<st;i++) cmds.push('DRAG 0 -'+Math.round(h/st));
  // Roof
  cmds.push('RELEASE'); cmds.push('MOVE 0 0');
  for(var i=0;i<st;i++) cmds.push('DRAG '+Math.round(w/2/st)+' -'+Math.round(rh/st));
  for(var i=0;i<st;i++) cmds.push('DRAG '+Math.round(w/2/st)+' '+Math.round(rh/st));
  // Door
  cmds.push('RELEASE'); cmds.push('MOVE -'+Math.round(w*0.6)+' 0');
  var dw=16, dh=22;
  for(var i=0;i<5;i++) cmds.push('DRAG -'+Math.round(dw/5)+' 0');
  for(var i=0;i<5;i++) cmds.push('DRAG 0 -'+Math.round(dh/5));
  for(var i=0;i<5;i++) cmds.push('DRAG '+Math.round(dw/5)+' 0');
  for(var i=0;i<5;i++) cmds.push('DRAG 0 '+Math.round(dh/5));
  drawSeq(cmds,'House');
}

async function drawTree(){
  var cmds=[];
  // Trunk
  for(var i=0;i<5;i++) cmds.push('DRAG 0 -'+8);
  for(var i=0;i<3;i++) cmds.push('DRAG '+6+' 0');
  for(var i=0;i<5;i++) cmds.push('DRAG 0 '+8);
  for(var i=0;i<3;i++) cmds.push('DRAG -'+6+' 0');
  // Three layers of triangles
  var layers=[[70,50],[55,40],[40,30]];
  var baseY=0;
  for(var li=0;li<layers.length;li++){
    var lw=layers[li][0], lh=layers[li][1], lst=12;
    cmds.push('RELEASE');
    cmds.push('MOVE -'+Math.round(lw/2)+' -'+Math.round(baseY+lh*0.3));
    baseY+=lh*0.5;
    for(var i=0;i<lst;i++) cmds.push('DRAG '+Math.round(lw/lst)+' 0');
    for(var i=0;i<lst;i++) cmds.push('DRAG -'+Math.round(lw/2/lst)+' -'+Math.round(lh/lst));
    for(var i=0;i<lst;i++) cmds.push('DRAG -'+Math.round(lw/2/lst)+' '+Math.round(lh/lst));
  }
  drawSeq(cmds,'Tree');
}

async function drawArrow(){
  var cmds=[], st=10, shaft=80, hw=30, hh=40;
  // Shaft
  for(var i=0;i<st;i++) cmds.push('DRAG '+Math.round(shaft/st)+' 0');
  // Arrowhead
  for(var i=0;i<st;i++) cmds.push('DRAG -'+Math.round(hw/st)+' -'+Math.round(hh/2/st));
  cmds.push('RELEASE'); cmds.push('MOVE '+hw+' '+Math.round(hh/2));
  for(var i=0;i<st;i++) cmds.push('DRAG -'+Math.round(hw/st)+' '+Math.round(hh/2/st));
  drawSeq(cmds,'Arrow');
}
</script>
</body>
</html>
)rawhtml";
