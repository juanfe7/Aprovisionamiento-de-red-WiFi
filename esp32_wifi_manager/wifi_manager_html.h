/**
 * wifi_manager_html.h
 * ─────────────────────────────────────────────────────────────
 * Portal web del WiFi Manager almacenado en memoria de programa
 * (PROGMEM) para minimizar uso de RAM en el ESP32.
 *
 * Características:
 *   - Diseño responsive (móvil y escritorio)
 *   - Escaneo de redes en tiempo real via /api/scan
 *   - Formulario de configuración con validación
 *   - Indicador de estado de conexión
 *   - Botón de reset de configuración
 *   - Animaciones CSS sin dependencias externas
 * ─────────────────────────────────────────────────────────────
 */

#pragma once
#include <Arduino.h>

String getIndexHTML() {
  return R"rawhtml(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 · WiFi Manager</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Space+Mono:wght@400;700&family=Syne:wght@400;600;800&display=swap');

  :root {
    --bg:       #0a0e17;
    --surface:  #111827;
    --border:   #1e293b;
    --accent:   #22d3ee;
    --accent2:  #f59e0b;
    --success:  #10b981;
    --error:    #ef4444;
    --text:     #e2e8f0;
    --muted:    #64748b;
    --radius:   12px;
  }

  * { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    font-family: 'Syne', sans-serif;
    background: var(--bg);
    color: var(--text);
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 24px 16px;
    background-image:
      radial-gradient(ellipse at 20% 10%, rgba(34,211,238,0.08) 0%, transparent 50%),
      radial-gradient(ellipse at 80% 80%, rgba(245,158,11,0.06) 0%, transparent 50%);
  }

  .card {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    width: 100%;
    max-width: 480px;
    overflow: hidden;
  }

  /* ── Header ────────────────────────────────── */
  .header {
    padding: 28px 28px 20px;
    border-bottom: 1px solid var(--border);
    position: relative;
    overflow: hidden;
  }
  .header::before {
    content: '';
    position: absolute;
    top: -30px; left: -30px;
    width: 120px; height: 120px;
    background: radial-gradient(circle, rgba(34,211,238,0.15), transparent 70%);
    pointer-events: none;
  }
  .badge {
    font-family: 'Space Mono', monospace;
    font-size: 10px;
    color: var(--accent);
    letter-spacing: 3px;
    text-transform: uppercase;
    margin-bottom: 8px;
  }
  h1 {
    font-size: 24px;
    font-weight: 800;
    letter-spacing: -0.5px;
    line-height: 1.2;
  }
  h1 span { color: var(--accent); }
  .subtitle { font-size: 13px; color: var(--muted); margin-top: 6px; }

  /* ── Status bar ────────────────────────────── */
  #statusBar {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 14px 28px;
    background: rgba(34,211,238,0.04);
    border-bottom: 1px solid var(--border);
    font-size: 13px;
  }
  .dot {
    width: 8px; height: 8px;
    border-radius: 50%;
    background: var(--muted);
    flex-shrink: 0;
    transition: background 0.3s;
  }
  .dot.connected { background: var(--success); box-shadow: 0 0 6px var(--success); animation: pulse 2s infinite; }
  .dot.ap        { background: var(--accent2); box-shadow: 0 0 6px var(--accent2); }
  .dot.error     { background: var(--error); }

  @keyframes pulse {
    0%, 100% { opacity: 1; }
    50%       { opacity: 0.4; }
  }

  /* ── Scan section ──────────────────────────── */
  .section { padding: 20px 28px; border-bottom: 1px solid var(--border); }
  .section-title {
    font-size: 11px;
    font-weight: 600;
    letter-spacing: 2px;
    text-transform: uppercase;
    color: var(--muted);
    margin-bottom: 14px;
  }

  #scanBtn {
    display: flex;
    align-items: center;
    gap: 8px;
    background: transparent;
    border: 1px solid var(--border);
    color: var(--text);
    font-family: 'Syne', sans-serif;
    font-size: 13px;
    font-weight: 600;
    padding: 10px 18px;
    border-radius: 8px;
    cursor: pointer;
    transition: all 0.2s;
    width: 100%;
    justify-content: center;
  }
  #scanBtn:hover { border-color: var(--accent); color: var(--accent); }
  #scanBtn:disabled { opacity: 0.4; cursor: not-allowed; }

  #networkList { margin-top: 14px; display: flex; flex-direction: column; gap: 8px; }

  .net-item {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 12px 14px;
    background: rgba(255,255,255,0.03);
    border: 1px solid var(--border);
    border-radius: 8px;
    cursor: pointer;
    transition: all 0.2s;
    animation: fadeIn 0.3s ease;
  }
  .net-item:hover { border-color: var(--accent); background: rgba(34,211,238,0.06); }
  .net-item.selected { border-color: var(--accent); background: rgba(34,211,238,0.08); }

  @keyframes fadeIn { from { opacity:0; transform:translateY(4px); } to { opacity:1; transform:none; } }

  .net-name { font-size: 14px; font-weight: 600; }
  .net-meta {
    font-family: 'Space Mono', monospace;
    font-size: 10px;
    color: var(--muted);
    margin-top: 2px;
  }
  .net-signal { display: flex; align-items: center; gap: 6px; }
  .signal-bars { display: flex; align-items: flex-end; gap: 2px; height: 14px; }
  .signal-bars span {
    width: 3px;
    background: var(--border);
    border-radius: 2px;
    transition: background 0.3s;
  }
  .signal-bars span.active { background: var(--accent); }
  .lock-icon { font-size: 11px; color: var(--muted); }

  /* ── Form ──────────────────────────────────── */
  .form-group { margin-bottom: 16px; }
  label { display: block; font-size: 12px; font-weight: 600; color: var(--muted); margin-bottom: 6px; letter-spacing: 0.5px; }

  input[type="text"], input[type="password"] {
    width: 100%;
    background: rgba(255,255,255,0.04);
    border: 1px solid var(--border);
    border-radius: 8px;
    color: var(--text);
    font-family: 'Space Mono', monospace;
    font-size: 13px;
    padding: 11px 14px;
    outline: none;
    transition: border-color 0.2s;
  }
  input:focus { border-color: var(--accent); }
  input::placeholder { color: var(--muted); }

  .pass-wrap { position: relative; }
  .pass-wrap input { padding-right: 44px; }
  .toggle-pass {
    position: absolute;
    right: 12px; top: 50%;
    transform: translateY(-50%);
    background: none; border: none;
    color: var(--muted); cursor: pointer;
    font-size: 16px; padding: 4px;
  }
  .toggle-pass:hover { color: var(--text); }

  #connectBtn {
    width: 100%;
    background: var(--accent);
    color: #000;
    border: none;
    border-radius: 8px;
    font-family: 'Syne', sans-serif;
    font-size: 14px;
    font-weight: 800;
    padding: 13px;
    cursor: pointer;
    letter-spacing: 0.5px;
    transition: all 0.2s;
    position: relative;
    overflow: hidden;
  }
  #connectBtn:hover { background: #67e8f9; transform: translateY(-1px); }
  #connectBtn:disabled { opacity: 0.5; cursor: not-allowed; transform: none; }
  #connectBtn .spinner {
    display: none;
    width: 16px; height: 16px;
    border: 2px solid rgba(0,0,0,0.3);
    border-top-color: #000;
    border-radius: 50%;
    animation: spin 0.6s linear infinite;
    margin: 0 auto;
  }
  @keyframes spin { to { transform: rotate(360deg); } }

  /* ── Toast message ─────────────────────────── */
  #toast {
    position: fixed;
    bottom: 24px; left: 50%;
    transform: translateX(-50%) translateY(80px);
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 10px;
    padding: 12px 20px;
    font-size: 13px;
    font-weight: 600;
    max-width: 340px;
    text-align: center;
    transition: transform 0.3s ease;
    z-index: 100;
    display: flex;
    align-items: center;
    gap: 10px;
  }
  #toast.show { transform: translateX(-50%) translateY(0); }
  #toast.success { border-color: var(--success); color: var(--success); }
  #toast.error   { border-color: var(--error);   color: var(--error); }
  #toast.info    { border-color: var(--accent);  color: var(--accent); }

  /* ── Reset section ─────────────────────────── */
  .reset-section {
    padding: 20px 28px;
    display: flex;
    align-items: center;
    justify-content: space-between;
  }
  .reset-text { font-size: 12px; color: var(--muted); }
  #resetBtn {
    background: transparent;
    border: 1px solid rgba(239,68,68,0.3);
    color: var(--error);
    font-family: 'Syne', sans-serif;
    font-size: 12px;
    font-weight: 600;
    padding: 8px 16px;
    border-radius: 8px;
    cursor: pointer;
    transition: all 0.2s;
  }
  #resetBtn:hover { background: rgba(239,68,68,0.1); border-color: var(--error); }

  /* ── Footer ────────────────────────────────── */
  .footer {
    margin-top: 20px;
    font-family: 'Space Mono', monospace;
    font-size: 10px;
    color: var(--muted);
    text-align: center;
    letter-spacing: 1px;
  }
</style>
</head>
<body>

<div class="card">
  <!-- Header -->
  <div class="header">
    <div class="badge">&#9632; IoT Device</div>
    <h1>WiFi <span>Manager</span></h1>
    <p class="subtitle">Configure su conexión de red inalámbrica</p>
  </div>

  <!-- Status -->
  <div id="statusBar">
    <div class="dot" id="statusDot"></div>
    <span id="statusText">Cargando estado...</span>
  </div>

  <!-- Scan -->
  <div class="section">
    <div class="section-title">Redes disponibles</div>
    <button id="scanBtn" onclick="scanNetworks()">
      <span id="scanIcon">&#8635;</span> Escanear redes WiFi
    </button>
    <div id="networkList"></div>
  </div>

  <!-- Form -->
  <div class="section">
    <div class="section-title">Credenciales</div>
    <div class="form-group">
      <label>SSID (Nombre de Red)</label>
      <input type="text" id="ssidInput" placeholder="Ej: MiRedWiFi" maxlength="32" autocomplete="off">
    </div>
    <div class="form-group">
      <label>Contraseña</label>
      <div class="pass-wrap">
        <input type="password" id="passInput" placeholder="Contraseña de red" autocomplete="off">
        <button class="toggle-pass" onclick="togglePass()" title="Mostrar/ocultar">&#128065;</button>
      </div>
    </div>
    <button id="connectBtn" onclick="doConnect()">
      <span id="btnText">Conectar y Guardar</span>
      <div class="spinner" id="btnSpinner"></div>
    </button>
  </div>

  <!-- Reset -->
  <div class="reset-section">
    <div class="reset-text">&#9888; Borrar configuración y reiniciar en modo AP</div>
    <button id="resetBtn" onclick="doReset()">Resetear</button>
  </div>
</div>

<div id="toast"></div>

<div class="footer">ESP32 · WIFI MANAGER v1.0.0 · ARDUINO FRAMEWORK</div>

<script>
// ── Helpers ──────────────────────────────────────────────────
function toast(msg, type='info', duration=3500) {
  const t = document.getElementById('toast');
  const icons = { success:'✓', error:'✗', info:'ℹ' };
  t.className = 'show ' + type;
  t.innerHTML = `<span>${icons[type]||'•'}</span><span>${msg}</span>`;
  clearTimeout(t._timer);
  t._timer = setTimeout(() => { t.className = ''; }, duration);
}

function signalBars(rssi) {
  const level = rssi > -50 ? 4 : rssi > -65 ? 3 : rssi > -75 ? 2 : 1;
  let html = '<div class="signal-bars">';
  for (let i=1; i<=4; i++) {
    const h = 4 + i*2;
    html += `<span style="height:${h}px" class="${i<=level?'active':''}"></span>`;
  }
  return html + '</div>';
}

// ── Estado del sistema ───────────────────────────────────────
async function loadStatus() {
  try {
    const r = await fetch('/api/status');
    const d = await r.json();
    const dot  = document.getElementById('statusDot');
    const text = document.getElementById('statusText');
    if (d.connected) {
      dot.className = 'dot connected';
      text.textContent = `Conectado a "${d.ssid}" — IP: ${d.ip} (${d.rssi} dBm)`;
    } else if (d.mode === 'ap') {
      dot.className = 'dot ap';
      text.textContent = 'Modo AP activo — esperando configuración';
    } else {
      dot.className = 'dot';
      text.textContent = 'Sin conexión';
    }
  } catch(e) {
    document.getElementById('statusText').textContent = 'Error al cargar estado';
  }
}

// ── Escaneo de redes ─────────────────────────────────────────
async function scanNetworks() {
  const btn = document.getElementById('scanBtn');
  const list = document.getElementById('networkList');
  btn.disabled = true;
  document.getElementById('scanIcon').textContent = '⟳';
  list.innerHTML = '<div style="color:var(--muted);font-size:13px;padding:8px 0">Escaneando...</div>';

  try {
    const r = await fetch('/api/scan');
    const d = await r.json();
    list.innerHTML = '';

    if (!d.networks || d.networks.length === 0) {
      list.innerHTML = '<div style="color:var(--muted);font-size:13px">No se encontraron redes.</div>';
      return;
    }

    // Ordenar por señal
    d.networks.sort((a,b) => b.rssi - a.rssi);

    d.networks.forEach(net => {
      const div = document.createElement('div');
      div.className = 'net-item';
      div.innerHTML = `
        <div>
          <div class="net-name">${escHtml(net.ssid) || '<em style="color:var(--muted)">(oculta)</em>'}</div>
          <div class="net-meta">CH ${net.channel} · ${net.bssid}</div>
        </div>
        <div class="net-signal">
          ${signalBars(net.rssi)}
          <span class="lock-icon">${net.encrypted ? '🔒' : '🔓'}</span>
        </div>`;
      div.onclick = () => selectNetwork(net.ssid, div);
      list.appendChild(div);
    });

  } catch(e) {
    list.innerHTML = '<div style="color:var(--error);font-size:13px">Error en escaneo.</div>';
  } finally {
    btn.disabled = false;
    document.getElementById('scanIcon').textContent = '↻';
  }
}

function selectNetwork(ssid, el) {
  document.querySelectorAll('.net-item').forEach(e => e.classList.remove('selected'));
  el.classList.add('selected');
  document.getElementById('ssidInput').value = ssid;
  document.getElementById('passInput').focus();
}

// ── Conectar ─────────────────────────────────────────────────
async function doConnect() {
  const ssid = document.getElementById('ssidInput').value.trim();
  const pass = document.getElementById('passInput').value;

  if (!ssid) { toast('El SSID es obligatorio.', 'error'); return; }

  const btn = document.getElementById('connectBtn');
  document.getElementById('btnText').style.display = 'none';
  document.getElementById('btnSpinner').style.display = 'block';
  btn.disabled = true;

  try {
    const r = await fetch('/api/connect', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ ssid, password: pass })
    });
    const d = await r.json();

    if (d.status === 'ok') {
      toast(`✓ Conectado. IP: ${d.ip}`, 'success', 5000);
      setTimeout(loadStatus, 1500);
    } else {
      toast(d.message || 'Error de conexión', 'error');
    }
  } catch(e) {
    toast('El dispositivo está reconectando...', 'info', 5000);
    setTimeout(loadStatus, 4000);
  } finally {
    btn.disabled = false;
    document.getElementById('btnText').style.display = '';
    document.getElementById('btnSpinner').style.display = 'none';
  }
}

// ── Reset ────────────────────────────────────────────────────
async function doReset() {
  if (!confirm('¿Borrar la configuración WiFi y reiniciar en modo AP?')) return;
  try {
    const r = await fetch('/api/reset', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ confirm: true })
    });
    const d = await r.json();
    toast(d.message, 'info', 6000);
  } catch(e) {
    toast('Dispositivo reiniciando...', 'info', 6000);
  }
}

// ── Misc ─────────────────────────────────────────────────────
function togglePass() {
  const inp = document.getElementById('passInput');
  inp.type = inp.type === 'password' ? 'text' : 'password';
}

function escHtml(str) {
  return (str||'').replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}

// ── Init ─────────────────────────────────────────────────────
loadStatus();
setInterval(loadStatus, 8000);
</script>
</body>
</html>)rawhtml";
}
