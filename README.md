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
  <img src="https://github.com/user-attachments/assets/4191c80f-c4d9-4307-a620-5df02b7321c2" width="450" alt="Diagrama de Bloques de Arquitectura">
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

## 4. Documentación de la API (RESTful Endpoints)

El sistema expone una interfaz programática para facilitar la integración con aplicaciones móviles o tableros de control. Todas las respuestas utilizan el estándar `application/json`.

| Método | Endpoint | Descripción | Payload (JSON) |
| :--- | :--- | :--- | :--- |
| **GET** | `/api/status` | Estado de conexión, IP y señal RSSI | N/A |
| **GET** | `/api/scan` | Lista de redes WiFi detectadas | N/A |
| **POST** | `/api/connect` | Envío de SSID y Password para conexión | `{"ssid":"...","password":"..."}` |
| **POST** | `/api/reset` | Borrado de credenciales y reinicio a modo AP | `{"confirm": true}` |
| **GET** | `/api/info` | Información de hardware (Flash, MAC, ChipID) | N/A |

---

## 5. Diagramas UML (Validación de Flujo)

### 5.1. Diagrama de Secuencia: Aprovisionamiento Inicial
Describe el intercambio de mensajes entre el cliente (smartphone), el ESP32 y el Access Point externo.

<p align="center">
  <img src="https://github.com/user-attachments/assets/4a8c931e-a9e7-47d4-ad72-6f2756a31343" width="500" alt="Diagrama de Secuencia de Aprovisionamiento">
  <br>
  <b>Figura 2:</b> <i>Flujo de interacción entre Usuario, ESP32 y Almacenamiento NVS.</i>
</p>



### 5.2. Mecanismo de Reset (Hardware Interrupt)
Se implementó un manejador de interrupciones para el **GPIO 0 (Botón BOOT)**. Una pulsación prolongada (>3s) dispara la limpieza del namespace en la NVS y el reinicio del procesador, garantizando que el dispositivo nunca quede "ladrillo" (brick) por falta de red.

---

## 6. Guía de Uso e Instalación

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

## 7. Resolución de Cuestionamientos Técnicos (20%)

### 7.1. Seguridad WPA2 Enterprise (PEAP)
**¿Es posible conectarse?** Sí. 
**Requerimientos:** El ESP32 soporta protocolos Enterprise mediante la librería `esp_wpa2.h`. A diferencia de WPA2-PSK, PEAP requiere una configuración de identidad y, opcionalmente, la carga de un **Certificado CA** en formato `.pem` para validar el servidor de autenticación (RADIUS).

### 7.2. Concurrencia en WebServer
**Capacidad:** La librería `WebServer.h` estándar es síncrona y puede gestionar hasta **5 sockets** abiertos simultáneamente. Superar este límite puede causar denegación de servicio (DoS) en el portal.
**Alternativas:** Para entornos con alta concurrencia, se recomienda **ESPAsyncWebServer**, que utiliza eventos asíncronos para manejar múltiples clientes simultáneos sin bloquear la ejecución del loop principal.

---

## 8. Análisis de Viabilidad y Modelo de Negocio
Desde una perspectiva de ingeniería profesional, esta implementación:
1.  **Eficiencia Operativa:** Reduce el ciclo de vida de mantenimiento al permitir que el usuario final resuelva problemas de conectividad.
2.  **Versatilidad:** La arquitectura API REST permite que el dispositivo sea controlado por una App móvil de terceros en el futuro.
3.  **Bajo Costo:** No requiere hardware adicional (pantallas o teclados) para la configuración inicial.

---

## 9. Recursos Adicionales
*   **Postman Collection:** Ubicada en `/docs/WiFi_Provisioning.postman_collection.json`.
*   **Esquemáticos:** Ver carpeta `/hardware`.

---
**Desarrollado por:** Juan Felipe Moncada, Samuel Arcos y Juan Felipe Cardenas  
**Fecha:** Mayo 2026 | **Curso:** Internet de las Cosas (IoT)
