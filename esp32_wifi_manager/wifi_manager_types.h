/**
 * wifi_manager_types.h
 * ─────────────────────────────────────────────
 * Tipos y estructuras compartidas del sistema.
 * Incluir en todos los módulos que necesiten
 * acceder al estado global del dispositivo.
 */

#pragma once
#include <Arduino.h>

// ── Modos de operación del sistema ──────────────
enum SystemMode {
  MODE_UNCONFIGURED = 0,   // Sin credenciales, esperando config
  MODE_AP           = 1,   // Modo Access Point (portal cautivo)
  MODE_STA          = 2,   // Conectado a red WiFi externa
  MODE_CONNECTING   = 3    // En proceso de conexión
};

// ── Estado global del sistema ─────────────────
struct SystemState {
  SystemMode mode;
  bool       wifiConnected;
  String     ssid;
  String     ip;
  int        rssi;
  unsigned long uptime;
  bool       resetRequested;
};

// ── Códigos de respuesta de la API ───────────
#define API_OK            200
#define API_BAD_REQUEST   400
#define API_NOT_FOUND     404
#define API_SERVER_ERROR  500

// ── Etiquetas de respuesta JSON ───────────────
#define JSON_STATUS_OK    "ok"
#define JSON_STATUS_ERR   "error"

// ── Versión del Firmware ──────────────────────
#define FIRMWARE_VERSION  "1.0.0"
