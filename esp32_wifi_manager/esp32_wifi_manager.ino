/**
 *
 * Dependencias (instalar desde Library Manager):
 *   - Arduino ESP32 Core >= 2.0.0
 *   - Preferences (incluida en ESP32 core)
 *   - WebServer    (incluida en ESP32 core)
 *   - DNSServer    (incluida en ESP32 core)
 *   - ArduinoJson  >= 6.21.0
 
 * Pinout:
 *   GPIO 0  → Botón de reset de configuración (BOOT button)
 *   GPIO 2  → LED de estado (integrado en la mayoría de placas)
 * ============================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "wifi_manager_types.h"
#include "wifi_manager_html.h"   // Páginas HTML embebidas
#include "wifi_manager_api.h"    // Manejadores de endpoints REST

// ─── Constantes de configuración ────────────────────────────
#define AP_SSID          "ESP32-Config"   // SSID del AP de configuración
#define AP_PASSWORD      ""               // Sin contraseña (portal abierto)
#define AP_CHANNEL       1
#define AP_MAX_CONN      4
#define DNS_PORT         53
#define HTTP_PORT        80
#define RESET_PIN        0                // GPIO0 = botón BOOT
#define LED_PIN          2                // LED de estado
#define RESET_HOLD_MS    3000             // ms para activar reset por botón
#define CONNECT_TIMEOUT  15000            // ms máx esperando conexión WiFi
#define NVS_NAMESPACE    "wifi_cfg"       // Namespace en NVS
#define NVS_KEY_SSID     "ssid"
#define NVS_KEY_PASS     "password"
#define NVS_KEY_VALID    "configured"

// ─── Variables globales ──────────────────────────────────────
WebServer  server(HTTP_PORT);
DNSServer  dnsServer;
Preferences prefs;

// Estado del sistema (compartido con wifi_manager_api.h)
SystemState sysState = {
  .mode          = MODE_UNCONFIGURED,
  .wifiConnected = false,
  .ssid          = "",
  .ip            = "",
  .rssi          = 0,
  .uptime        = 0,
  .resetRequested = false
};

// ─── Prototipo de funciones ──────────────────────────────────
void     startAPMode();
bool     startSTAMode(const String& ssid, const String& pass);
bool     loadCredentials(String& ssid, String& pass);
void     saveCredentials(const String& ssid, const String& pass);
void     clearCredentials();
void     setupRoutes();
void     handleResetButton();
void     blinkLED(int times, int ms);
void     setLED(bool on);
String   getStatusJSON();

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("\n========================================"));
  Serial.println(F("  ESP32 WiFi Manager v" FIRMWARE_VERSION));
  Serial.println(F("========================================"));

  // Configurar pines
  pinMode(LED_PIN, OUTPUT);
  pinMode(RESET_PIN, INPUT_PULLUP);
  setLED(false);

  // Inicializar NVS
  prefs.begin(NVS_NAMESPACE, false);

  // Intentar cargar credenciales guardadas
  String savedSSID, savedPass;
  bool hasCreds = loadCredentials(savedSSID, savedPass);

  if (hasCreds && savedSSID.length() > 0) {
    Serial.printf("[BOOT] Credenciales encontradas. SSID: %s\n", savedSSID.c_str());
    Serial.println(F("[BOOT] Intentando conectar a la red guardada..."));

    blinkLED(2, 200);

    if (startSTAMode(savedSSID, savedPass)) {
      Serial.println(F("[BOOT] Conexión exitosa. Modo STA activo."));
      sysState.mode = MODE_STA;
    } else {
      Serial.println(F("[BOOT] Falló la conexión. Iniciando modo AP..."));
      startAPMode();
    }
  } else {
    Serial.println(F("[BOOT] Sin credenciales. Iniciando modo AP..."));
    startAPMode();
  }

  // Registrar rutas HTTP
  setupRoutes();
  server.begin();
  Serial.println(F("[HTTP] Servidor web iniciado."));
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  // Atender peticiones DNS (solo en modo AP)
  if (sysState.mode == MODE_AP || sysState.mode == MODE_UNCONFIGURED) {
    dnsServer.processNextRequest();
  }

  // Atender peticiones HTTP
  server.handleClient();

  // Monitorear botón de reset
  handleResetButton();

  // Actualizar uptime
  sysState.uptime = millis() / 1000;

  // Si se solicitó reset, ejecutar
  if (sysState.resetRequested) {
    sysState.resetRequested = false;
    Serial.println(F("[RESET] Borrando credenciales y reiniciando..."));
    clearCredentials();
    blinkLED(5, 100);
    delay(500);
    ESP.restart();
  }

  // Parpadeo LED según estado
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  int blinkInterval = (sysState.mode == MODE_STA) ? 2000 : 500;

  if (millis() - lastBlink > blinkInterval) {
    lastBlink = millis();
    ledState = !ledState;
    setLED(ledState);
  }

  delay(10);
}

// ============================================================
// FUNCIONES DE MODO AP
// ============================================================
void startAPMode() {
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);

  IPAddress apIP(192, 168, 4, 1);
  IPAddress netMask(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apIP, netMask);

  bool ok = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, 0, AP_MAX_CONN);
  if (!ok) {
    Serial.println(F("[AP] ERROR: No se pudo iniciar el AP"));
    return;
  }

  Serial.printf("[AP] Modo AP iniciado.\n");
  Serial.printf("[AP] SSID    : %s\n", AP_SSID);
  Serial.printf("[AP] IP      : %s\n", WiFi.softAPIP().toString().c_str());

  // Portal cautivo: redirigir todo DNS a nuestra IP
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  Serial.println(F("[DNS] Servidor DNS para portal cautivo activo."));

  sysState.mode = MODE_AP;
  sysState.wifiConnected = false;
  sysState.ip = WiFi.softAPIP().toString();
  sysState.ssid = AP_SSID;
}

// ============================================================
// FUNCIONES DE MODO STA
// ============================================================
bool startSTAMode(const String& ssid, const String& pass) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  Serial.printf("[STA] Conectando a '%s'", ssid.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > CONNECT_TIMEOUT) {
      Serial.println(F("\n[STA] TIMEOUT — conexión fallida."));
      return false;
    }
    delay(300);
    Serial.print(".");
    // Parpadeo rápido mientras conecta
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }

  Serial.println();
  Serial.printf("[STA] Conectado exitosamente!\n");
  Serial.printf("[STA] IP     : %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[STA] Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("[STA] RSSI   : %d dBm\n", WiFi.RSSI());

  sysState.wifiConnected = true;
  sysState.ip   = WiFi.localIP().toString();
  sysState.ssid = ssid;
  sysState.rssi = WiFi.RSSI();
  return true;
}

// ============================================================
// NVS — PERSISTENCIA DE CREDENCIALES
// ============================================================
bool loadCredentials(String& ssid, String& pass) {
  bool valid = prefs.getBool(NVS_KEY_VALID, false);
  if (!valid) return false;
  ssid = prefs.getString(NVS_KEY_SSID, "");
  pass = prefs.getString(NVS_KEY_PASS, "");
  return (ssid.length() > 0);
}

void saveCredentials(const String& ssid, const String& pass) {
  prefs.putString(NVS_KEY_SSID, ssid);
  prefs.putString(NVS_KEY_PASS, pass);
  prefs.putBool(NVS_KEY_VALID, true);
  Serial.printf("[NVS] Credenciales guardadas. SSID: %s\n", ssid.c_str());
}

void clearCredentials() {
  prefs.remove(NVS_KEY_SSID);
  prefs.remove(NVS_KEY_PASS);
  prefs.putBool(NVS_KEY_VALID, false);
  Serial.println(F("[NVS] Credenciales borradas."));
}

// ============================================================
// REGISTRO DE RUTAS HTTP
// ============================================================
void setupRoutes() {
  // ── Portal Cautivo: redirigir cualquier URL al index ──────
  server.onNotFound([]() {
    String host = server.hostHeader();
    // Si no es nuestra IP, redirigir
    if (host != WiFi.softAPIP().toString() && host != "192.168.4.1") {
      server.sendHeader("Location", "http://192.168.4.1/", true);
      server.send(302, "text/plain", "");
      return;
    }
    handleNotFound();
  });

  // ── Páginas Web ──────────────────────────────────────────
  server.on("/",                HTTP_GET,  handleRoot);
  server.on("/index.html",      HTTP_GET,  handleRoot);
  server.on("/generate_204",    HTTP_GET,  handleCaptive);  // Android
  server.on("/fwlink",          HTTP_GET,  handleCaptive);  // Windows
  server.on("/hotspot-detect",  HTTP_GET,  handleCaptive);  // iOS/macOS

  // ── API REST ─────────────────────────────────────────────
  // GET  /api/status      → Estado del sistema
  server.on("/api/status",      HTTP_GET,  handleApiStatus);

  // GET  /api/scan        → Escanear redes WiFi disponibles
  server.on("/api/scan",        HTTP_GET,  handleApiScan);

  // POST /api/connect     → Configurar y conectar a red WiFi
  server.on("/api/connect",     HTTP_POST, handleApiConnect);

  // POST /api/reset       → Borrar credenciales y reiniciar AP
  server.on("/api/reset",       HTTP_POST, handleApiReset);

  // GET  /api/info        → Información del dispositivo
  server.on("/api/info",        HTTP_GET,  handleApiInfo);

  Serial.println(F("[HTTP] Rutas registradas:"));
  Serial.println(F("  GET  /api/status"));
  Serial.println(F("  GET  /api/scan"));
  Serial.println(F("  POST /api/connect"));
  Serial.println(F("  POST /api/reset"));
  Serial.println(F("  GET  /api/info"));
}

// ============================================================
// BOTÓN DE RESET FÍSICO
// ============================================================
void handleResetButton() {
  static unsigned long pressStart = 0;
  static bool pressing = false;

  bool btnPressed = (digitalRead(RESET_PIN) == LOW);

  if (btnPressed && !pressing) {
    pressing = true;
    pressStart = millis();
    Serial.println(F("[BTN] Botón presionado — mantén 3s para resetear."));
  }

  if (pressing && btnPressed) {
    unsigned long held = millis() - pressStart;
    if (held >= RESET_HOLD_MS) {
      Serial.println(F("[BTN] Reset activado por botón físico."));
      sysState.resetRequested = true;
      pressing = false;
    }
  }

  if (!btnPressed && pressing) {
    pressing = false;
    Serial.println(F("[BTN] Botón liberado (tiempo insuficiente)."));
  }
}

// ============================================================
// UTILIDADES LED
// ============================================================
void blinkLED(int times, int ms) {
  for (int i = 0; i < times; i++) {
    setLED(true);  delay(ms);
    setLED(false); delay(ms);
  }
}

void setLED(bool on) {
  digitalWrite(LED_PIN, on ? HIGH : LOW);
}
