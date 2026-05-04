/**
 * wifi_manager_api.h
 * ─────────────────────────────────────────────────────────────
 * Manejadores de los endpoints REST del WiFi Manager.
 *
 * Endpoints implementados:
 *   GET  /api/status      → Estado del sistema
 *   GET  /api/scan        → Redes WiFi disponibles
 *   POST /api/connect     → Configurar credenciales y conectar
 *   POST /api/reset       → Borrar configuración y reiniciar
 *   GET  /api/info        → Información del firmware/hardware
 *
 * Convención de respuestas:
 *   { "status": "ok"|"error", "message": "...", ...datos... }
 *
 * CORS habilitado para facilitar pruebas desde Postman/browser.
 * ─────────────────────────────────────────────────────────────
 */

#pragma once
#include <DNSServer.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "wifi_manager_types.h"
#include "wifi_manager_html.h"

// ── Referencias externas ──────────────────────────────────────
extern WebServer    server;
extern SystemState  sysState;
extern Preferences  prefs;
extern DNSServer dnsServer;

// ── Declaraciones de funciones del sketch principal ───────────
extern void saveCredentials(const String& ssid, const String& pass);
extern void clearCredentials();
extern bool startSTAMode(const String& ssid, const String& pass);
extern void startAPMode();
extern void blinkLED(int times, int ms);

// ─────────────────────────────────────────────────────────────
// UTILIDADES: Headers CORS y Content-Type
// ─────────────────────────────────────────────────────────────
void sendCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin",  "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
}

void sendJSONResponse(int code, const String& json) {
  sendCORSHeaders();
  server.send(code, "application/json; charset=utf-8", json);
}

// ─────────────────────────────────────────────────────────────
// HANDLER: Página raíz (portal de configuración)
// ─────────────────────────────────────────────────────────────
void handleRoot() {
  sendCORSHeaders();
  server.send(200, "text/html; charset=utf-8", getIndexHTML());
}

// ─────────────────────────────────────────────────────────────
// HANDLER: Portal cautivo (Android/iOS/Windows redirect)
// ─────────────────────────────────────────────────────────────
void handleCaptive() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}

// ─────────────────────────────────────────────────────────────
// HANDLER: 404
// ─────────────────────────────────────────────────────────────
void handleNotFound() {
  sendCORSHeaders();
  StaticJsonDocument<128> doc;
  doc["status"]  = JSON_STATUS_ERR;
  doc["message"] = "Recurso no encontrado";
  doc["path"]    = server.uri();
  String out;
  serializeJson(doc, out);
  sendJSONResponse(API_NOT_FOUND, out);
}

// ─────────────────────────────────────────────────────────────
// GET /api/status
// ─────────────────────────────────────────────────────────────
/**
 * Devuelve el estado actual del dispositivo.
 *
 * Respuesta 200:
 * {
 *   "status": "ok",
 *   "mode": "sta"|"ap"|"unconfigured",
 *   "connected": true|false,
 *   "ssid": "NombreRed",
 *   "ip": "192.168.1.x",
 *   "rssi": -65,
 *   "uptime": 1234
 * }
 */
void handleApiStatus() {
  if (server.method() == HTTP_OPTIONS) { sendCORSHeaders(); server.send(204); return; }

  StaticJsonDocument<256> doc;
  doc["status"]    = JSON_STATUS_OK;

  String modeStr;
  switch (sysState.mode) {
    case MODE_STA:          modeStr = "sta";          break;
    case MODE_AP:           modeStr = "ap";           break;
    case MODE_CONNECTING:   modeStr = "connecting";   break;
    default:                modeStr = "unconfigured"; break;
  }
  doc["mode"]      = modeStr;
  doc["connected"] = sysState.wifiConnected;
  doc["ssid"]      = sysState.ssid;
  doc["ip"]        = sysState.ip;
  doc["rssi"]      = sysState.wifiConnected ? WiFi.RSSI() : 0;
  doc["uptime"]    = sysState.uptime;

  String out;
  serializeJson(doc, out);
  sendJSONResponse(API_OK, out);
}

// ─────────────────────────────────────────────────────────────
// GET /api/scan
// ─────────────────────────────────────────────────────────────
/**
 * Escanea y retorna las redes WiFi disponibles.
 *
 * Query params:
 *   ?sort=rssi   (opcional) → ordena por señal descendente
 *
 * Respuesta 200:
 * {
 *   "status": "ok",
 *   "count": 3,
 *   "networks": [
 *     { "ssid": "MiRed", "rssi": -45, "encrypted": true, "channel": 6 },
 *     ...
 *   ]
 * }
 *
 * Respuesta 500: Error de escaneo
 */
void handleApiScan() {
  if (server.method() == HTTP_OPTIONS) { sendCORSHeaders(); server.send(204); return; }

  Serial.println(F("[API] GET /api/scan — iniciando escaneo WiFi..."));

  int n = WiFi.scanNetworks(false, true);  // no async, show hidden

  if (n == WIFI_SCAN_FAILED) {
    StaticJsonDocument<128> err;
    err["status"]  = JSON_STATUS_ERR;
    err["message"] = "Escaneo WiFi fallido";
    String out; serializeJson(err, out);
    sendJSONResponse(API_SERVER_ERROR, out);
    return;
  }

  // Construir respuesta
  DynamicJsonDocument doc(2048);
  doc["status"] = JSON_STATUS_OK;
  doc["count"]  = n;
  JsonArray nets = doc.createNestedArray("networks");

  for (int i = 0; i < n; i++) {
    JsonObject net = nets.createNestedObject();
    net["ssid"]      = WiFi.SSID(i);
    net["rssi"]      = WiFi.RSSI(i);
    net["encrypted"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    net["channel"]   = WiFi.channel(i);
    net["bssid"]     = WiFi.BSSIDstr(i);
  }

  WiFi.scanDelete();

  String out;
  serializeJson(doc, out);
  sendJSONResponse(API_OK, out);
  Serial.printf("[API] Escaneo completado: %d redes encontradas.\n", n);
}

// ─────────────────────────────────────────────────────────────
// POST /api/connect
// ─────────────────────────────────────────────────────────────
/**
 * Configura y conecta a una red WiFi. Guarda las credenciales
 * en NVS y reinicia en modo STA. Si la conexión falla,
 * permanece en modo AP y responde con error.
 *
 * Body (application/json):
 * {
 *   "ssid": "NombreRed",        ← requerido, max 32 chars
 *   "password": "contraseña"    ← requerido (vacío si abierta)
 * }
 *
 * Respuesta 200 (éxito):
 * {
 *   "status": "ok",
 *   "message": "Conectado exitosamente",
 *   "ip": "192.168.1.50",
 *   "ssid": "NombreRed",
 *   "rssi": -58
 * }
 *
 * Respuesta 400 (datos inválidos):
 * { "status": "error", "message": "SSID requerido" }
 *
 * Respuesta 500 (fallo de conexión):
 * { "status": "error", "message": "No se pudo conectar a 'NombreRed'" }
 */
void handleApiConnect() {
  if (server.method() == HTTP_OPTIONS) { sendCORSHeaders(); server.send(204); return; }

  Serial.println(F("[API] POST /api/connect"));

  // ── Parsear body JSON ────────────────────────────────────
  StaticJsonDocument<256> req;
  String body = server.arg("plain");

  Serial.printf("[API] Body recibido: %s\n", body.c_str());

  DeserializationError err = deserializeJson(req, body);
  if (err) {
    StaticJsonDocument<128> errDoc;
    errDoc["status"]  = JSON_STATUS_ERR;
    errDoc["message"] = "JSON inválido: " + String(err.c_str());
    String out; serializeJson(errDoc, out);
    sendJSONResponse(API_BAD_REQUEST, out);
    return;
  }

  // ── Validar campos ────────────────────────────────────────
  String ssid = req["ssid"] | "";
  String pass = req["password"] | "";
  ssid.trim();

  if (ssid.length() == 0) {
    StaticJsonDocument<128> errDoc;
    errDoc["status"]  = JSON_STATUS_ERR;
    errDoc["message"] = "El campo 'ssid' es requerido y no puede estar vacío";
    String out; serializeJson(errDoc, out);
    sendJSONResponse(API_BAD_REQUEST, out);
    return;
  }

  if (ssid.length() > 32) {
    StaticJsonDocument<128> errDoc;
    errDoc["status"]  = JSON_STATUS_ERR;
    errDoc["message"] = "SSID demasiado largo (máximo 32 caracteres)";
    String out; serializeJson(errDoc, out);
    sendJSONResponse(API_BAD_REQUEST, out);
    return;
  }

  // ── Respuesta inmediata antes de intentar conexión ───────
  // (El cliente ya sabe que procesamos la solicitud)
  sysState.mode = MODE_CONNECTING;

  // ── Intentar conexión ────────────────────────────────────
  Serial.printf("[API] Intentando conectar a '%s'...\n", ssid.c_str());
  dnsServer.stop();

  bool connected = startSTAMode(ssid, pass);

  if (connected) {
    saveCredentials(ssid, pass);
    sysState.mode = MODE_STA;

    StaticJsonDocument<256> resp;
    resp["status"]  = JSON_STATUS_OK;
    resp["message"] = "Conectado exitosamente";
    resp["ip"]      = WiFi.localIP().toString();
    resp["ssid"]    = ssid;
    resp["rssi"]    = WiFi.RSSI();
    String out; serializeJson(resp, out);
    sendJSONResponse(API_OK, out);
    blinkLED(3, 150);

    Serial.printf("[API] Conectado. Nueva IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    // Fallo → volver a AP mode
    startAPMode();

    StaticJsonDocument<128> errDoc;
    errDoc["status"]  = JSON_STATUS_ERR;
    errDoc["message"] = "No se pudo conectar a '" + ssid + "'. Verifica las credenciales.";
    String out; serializeJson(errDoc, out);
    sendJSONResponse(API_SERVER_ERROR, out);
    Serial.println(F("[API] Conexión fallida. Volviendo a modo AP."));
  }
}

// ─────────────────────────────────────────────────────────────
// POST /api/reset
// ─────────────────────────────────────────────────────────────
/**
 * Borra las credenciales guardadas en NVS y reinicia el ESP32
 * en modo AP para recibir nueva configuración.
 *
 * Body (opcional, application/json):
 * { "confirm": true }   ← por seguridad
 *
 * Respuesta 200:
 * {
 *   "status": "ok",
 *   "message": "Configuración borrada. El dispositivo reiniciará en modo AP."
 * }
 *
 * Respuesta 400:
 * { "status": "error", "message": "Se requiere confirm: true" }
 */
void handleApiReset() {
  if (server.method() == HTTP_OPTIONS) { sendCORSHeaders(); server.send(204); return; }

  Serial.println(F("[API] POST /api/reset"));

  // Parsear body (opcional)
  String body = server.arg("plain");
  bool confirmed = true;  // Permitir sin body también

  if (body.length() > 0) {
    StaticJsonDocument<64> req;
    if (!deserializeJson(req, body)) {
      confirmed = req["confirm"] | false;
    }
  }

  if (!confirmed) {
    StaticJsonDocument<128> errDoc;
    errDoc["status"]  = JSON_STATUS_ERR;
    errDoc["message"] = "Se requiere { \"confirm\": true } para ejecutar el reset";
    String out; serializeJson(errDoc, out);
    sendJSONResponse(API_BAD_REQUEST, out);
    return;
  }

  StaticJsonDocument<128> resp;
  resp["status"]  = JSON_STATUS_OK;
  resp["message"] = "Configuración borrada. El dispositivo reiniciará en modo AP en 2 segundos.";
  String out; serializeJson(resp, out);
  sendJSONResponse(API_OK, out);

  sysState.resetRequested = true;
}

// ─────────────────────────────────────────────────────────────
// GET /api/info
// ─────────────────────────────────────────────────────────────
/**
 * Retorna información estática del dispositivo y firmware.
 *
 * Respuesta 200:
 * {
 *   "status": "ok",
 *   "firmware": "1.0.0",
 *   "chipId": "ABC123",
 *   "chipModel": "ESP32-D0WDQ6",
 *   "flashSize": 4194304,
 *   "freeHeap": 234512,
 *   "macAddress": "AA:BB:CC:DD:EE:FF",
 *   "sdkVersion": "v4.4.x"
 * }
 */
void handleApiInfo() {
  if (server.method() == HTTP_OPTIONS) { sendCORSHeaders(); server.send(204); return; }

  StaticJsonDocument<384> doc;
  doc["status"]      = JSON_STATUS_OK;
  doc["firmware"]    = FIRMWARE_VERSION;
  doc["chipId"]      = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX) +
                       String((uint32_t) ESP.getEfuseMac(),        HEX);
  doc["chipModel"]   = ESP.getChipModel();
  doc["chipCores"]   = ESP.getChipCores();
  doc["cpuFreqMHz"]  = ESP.getCpuFreqMHz();
  doc["flashSize"]   = ESP.getFlashChipSize();
  doc["freeHeap"]    = ESP.getFreeHeap();
  doc["macAddress"]  = WiFi.macAddress();
  doc["sdkVersion"]  = ESP.getSdkVersion();

  String out;
  serializeJson(doc, out);
  sendJSONResponse(API_OK, out);
}
