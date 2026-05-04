# Aprovisionamiento de red WiFi con ESP32

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-orange.svg)](https://www.arduino.cc/)

## 1. Introducción y Justificación de Ingeniería
En entornos IoT reales, los dispositivos suelen desplegarse en lugares de difícil acceso o por usuarios sin conocimientos técnicos. La necesidad de reprogramar el código fuente para cambiar una credencial WiFi es inviable en una solución comercial. 

Este proyecto implementa una **solución de aprovisionamiento dinámico** que separa la lógica de conectividad del firmware, permitiendo que el ESP32 sea un producto "Plug & Play" mediante el uso de **Portales Cautivos** y **Memoria No Volátil (NVS)**.

---

## 2. Arquitectura del Sistema

### 2.1. Diagrama de Bloques (Hardware & Software)
La solución se basa en una arquitectura de capas para garantizar la escalabilidad y el desacoplamiento entre el hardware y la interfaz de usuario.

<p align="center">
  <img width="450" alt="Diagrama de Bloques de Arquitectura" src="https://github.com/user-attachments/assets/6df1131f-dd51-4aaf-be2e-f5c9e7078460" />
  <br>
  <b>Figura 1:</b> <i>Diagrama de arquitectura modular (Hardware, Lógica y Aplicación).</i>
</p>


### 2.2. Modelo de Estados del Sistema
El dispositivo opera bajo una máquina de estados finitos (FSM) que garantiza que el sistema siempre sea accesible:

*   **UNCONFIGURED:** Estado inicial de fábrica o tras un reset.
*   **AP_MODE:** Modo Punto de Acceso activo. Servidor DNS y Portal Cautivo levantados para recibir credenciales.
*   **CONNECTING:** Intento de handshake con el Router utilizando credenciales almacenadas.
*   **STA_MODE:** Operación normal como estación. API REST disponible para monitoreo.

---

## 3. Implementación Técnica

### 3.1. Gestión de Memoria No Volátil (NVS)
A diferencia de la EEPROM clásica, utilizamos la librería `Preferences.h` del framework de Espressif. 
*   **Ventaja Innovadora:** Utiliza una partición de la flash con un sistema de archivos estructurado que previene la corrupción de datos ante cortes de energía y permite el almacenamiento de pares llave-valor.
*   **Namespace:** `wifi_cfg`

### 3.2. Portal Cautivo y UI
La interfaz web ha sido desarrollada en **HTML5/CSS3 y JavaScript nativo**, embebida en la memoria `PROGMEM` del chip. Esto permite que el portal sea responsivo y funcional sin necesidad de conexión a internet o servidores externos (Edge Computing).

---

## 4. Especificación de la API REST (Endpoints)

El sistema expone una interfaz programática detallada para la interacción con el frontend y herramientas de diagnóstico. Todas las peticiones deben manejar el encabezado `Content-Type: application/json`.

### 4.1. Obtener Estado del Sistema
Retorna el estado operativo actual del microcontrolador y su conectividad.
*   **Endpoint:** `GET /api/status`
*   **Respuesta Exitosa (200 OK):**
    ```json
    {
      "status": "ok",
      "mode": "sta", 
      "connected": true,
      "ssid": "MiRedWiFi",
      "ip": "192.168.1.50",
      "rssi": -65,
      "uptime": 1234
    }
    ```
    *Nota: El campo `mode` puede reportar: `sta`, `ap`, `connecting` o `unconfigured`.*

### 4.2. Escanear Redes WiFi
Inicia un escaneo de radiofrecuencia para detectar puntos de acceso cercanos.
*   **Endpoint:** `GET /api/scan`
*   **Respuesta Exitosa (200 OK):**
    ```json
    {
      "status": "ok",
      "count": 2,
      "networks": [
        {
          "ssid": "MiRed",
          "rssi": -45,
          "encrypted": true,
          "channel": 6,
          "bssid": "AA:BB:CC:DD:EE:FF"
        }
      ]
    }
    ```
*   **Respuesta de Error (500):** Error de hardware en el transceptor WiFi.

### 4.3. Conectar a Nueva Red
Configura credenciales, intenta el handshake y persiste los datos en NVS si la validación es exitosa.
*   **Endpoint:** `POST /api/connect`
*   **Cuerpo de la Petición (JSON):**
    ```json
    {
      "ssid": "NombreDeMiRed", 
      "password": "MiPasswordSeguro"
    }
    ```
*   **Reglas de Validación:** `ssid` obligatorio (máx. 32 caracteres). `password` opcional para redes abiertas.
*   **Respuesta Exitosa (200 OK):** Conexión establecida y datos guardados.
*   **Respuesta de Error (400/500):** SSID faltante, formato inválido o fallo crítico de conexión con el AP.

### 4.4. Restablecer Configuración (Factory Reset)
Lógica de borrado seguro de la memoria persistente y retorno al estado de fábrica.
*   **Endpoint:** `POST /api/reset`
*   **Cuerpo (Opcional):** `{"confirm": true}`
*   **Respuesta Exitosa (200 OK):**
    ```json
    {
      "status": "ok",
      "message": "Configuración borrada. Reiniciando en modo AP..."
    }
    ```

### 4.5. Información Técnica del Dispositivo
Telemetría de bajo nivel para auditoría de hardware.
*   **Endpoint:** `GET /api/info`
*   **Respuesta Exitosa (200 OK):** Incluye `chipId`, `chipModel`, `flashSize`, `macAddress` y versión del SDK.

---

## 5. Validación y Pruebas
El sistema ha sido validado mediante pruebas funcionales de caja negra, asegurando que:
1. El portal cautivo se activa en menos de 2 segundos tras un fallo de red.
2. Las credenciales sobreviven a cortes de energía (Persistencia NVS).
3. Los endpoints responden correctamente a herramientas de cliente (probado y verificado por el equipo).

---

## 6. Diagramas UML (Validación de Flujo)

### 6.1. Diagrama de Secuencia: Aprovisionamiento Inicial
Describe el intercambio de mensajes entre el cliente (smartphone), el ESP32 y el Access Point externo.

<p align="center">
  <img src="https://github.com/user-attachments/assets/4a8c931e-a9e7-47d4-ad72-6f2756a31343" width="500" alt="Diagrama de Secuencia de Aprovisionamiento">
  <br>
  <b>Figura 2:</b> <i>Flujo de interacción entre Usuario, ESP32 y Almacenamiento NVS.</i>
</p>



### 6.2. Mecanismo de Reset (Hardware Interrupt)
Se implementó un manejador de interrupciones para el **GPIO 0 (Botón BOOT)**. Una pulsación prolongada (>3s) dispara la limpieza del namespace en la NVS y el reinicio del procesador, garantizando que el dispositivo nunca quede "ladrillo" (brick) por falta de red.

---

## 7. Guía de Uso e Instalación

### Requisitos Previos
*   **Arduino IDE** (versión 2.0 o superior) o **VS Code + PlatformIO**.
*   **Librerías necesarias:**
    *   `ArduinoJson` (v6.21.0 o superior).
    *   `WiFi`, `WebServer`, `DNSServer`, `Preferences` (incluidas en el core de ESP32).

### Instalación
1.  Clonar el repositorio: `git clone https://github.com/tu-usuario/tu-repo.git`
2.  Abrir el archivo `.ino` en el IDE de Arduino.
3.  Seleccionar la placa **ESP32 Dev Module**.
4.  Compilar y subir al dispositivo.

### Instrucciones de Configuración
1.  Al encender por primera vez, conectarse a la red WiFi llamada **"ESP32-Config"**.
2.  El portal cautivo debería abrirse automáticamente. Si no, ir a `http://192.168.4.1`.
3.  Seleccionar la red WiFi, ingresar la contraseña y guardar.
4.  El dispositivo se reiniciará y se conectará automáticamente.

---

## 8. Resolución de Cuestionamientos Técnicos (20%)

### 8.1. Seguridad WPA2 Enterprise (PEAP)
**¿Es posible conectarse?** Sí. 
**Requerimientos:** El ESP32 soporta protocolos Enterprise mediante la librería `esp_wpa2.h`. A diferencia de WPA2-PSK, PEAP requiere una configuración de identidad y, opcionalmente, la carga de un **Certificado CA** en formato `.pem` para validar el servidor de autenticación (RADIUS).

### 8.2. Concurrencia en WebServer
**Capacidad:** La librería `WebServer.h` estándar es síncrona y puede gestionar hasta **5 sockets** abiertos simultáneamente. Superar este límite puede causar denegación de servicio (DoS) en el portal.
**Alternativas:** Para entornos con alta concurrencia, se recomienda **ESPAsyncWebServer**, que utiliza eventos asíncronos para manejar múltiples clientes simultáneos sin bloquear la ejecución del loop principal.

### 8.3. Análisis de Consumo de Memoria Flash
Se comparó el uso de recursos contra el ejemplo "Basic" de la librería WiFiManager.

| Implementación | % de Memoria Flash Usada | Justificación |
| :--- | :--- | :--- |
| **WiFiManager (Basic)** | ~25% | Funcionalidad mínima sin API REST avanzada. |
| **Nuestra Solución** | **77%** | Incluye motor JSON, API completa e interfaz UI en PROGMEM. |

**Análisis:** El uso del 77% de la Flash se justifica por la robustez de la solución y la eliminación de dependencias externas, garantizando una arquitectura profesional.

---

## 9. Análisis de Viabilidad y Modelo de Negocio
Desde una perspectiva de ingeniería profesional, esta implementación:
1.  **Eficiencia Operativa:** Reduce el ciclo de vida de mantenimiento al permitir que el usuario final resuelva problemas de conectividad.
2.  **Versatilidad:** La arquitectura API REST permite que el dispositivo sea controlado por una App móvil de terceros en el futuro.
3.  **Bajo Costo:** No requiere hardware adicional (pantallas o teclados) para la configuración inicial.

---
**Desarrollado por:** Juan Felipe Moncada, Samuel Arcos y Juan Felipe Cardenas  
**Fecha:** Mayo 2026 | **Curso:** Internet de las Cosas (IoT)
