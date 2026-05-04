# ESP32 WiFi Provisioning System (IoT Professional Solution)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-orange.svg)](https://www.arduino.cc/)

## 1. Introducción y Justificación de Ingeniería
En entornos IoT reales, los dispositivos suelen desplegarse en lugares de difícil acceso o por usuarios sin conocimientos técnicos. La necesidad de reprogramar el código fuente para cambiar una credencial WiFi es inviable en una solución comercial. 

Este proyecto implementa una **solución de aprovisionamiento dinámico** que separa la lógica de conectividad del firmware, permitiendo que el ESP32 sea un producto "Plug & Play" mediante el uso de **Portales Cautivos** y **Memoria No Volátil (NVS)**.

---

## 2. Arquitectura del Sistema

### 2.1. Diagrama de Bloques (Hardware & Software)
La solución se basa en una arquitectura de capas para garantizar la escalabilidad.
> [!IMPORTANT]
> [INSERTAR AQUÍ: Diagrama de Bloques que muestre la relación entre el ESP32, el NVS Flash, el Transceptor WiFi y la Interfaz de Usuario Web]

### 2.2. Modelo de Estados del Sistema
El dispositivo opera bajo una máquina de estados finitos (FSM) que garantiza que el sistema siempre sea accesible, ya sea como estación (STA) o como punto de acceso (AP).

*   **UNCONFIGURED:** Estado inicial de fábrica.
*   **AP_MODE:** Servidor DNS activo y Portal Cautivo levantado.
*   **CONNECTING:** Intento de handshake con el Router.
*   **STA_MODE:** Operación normal con IP asignada.

---

## 3. Implementación Técnica

### 3.1. Gestión de Memoria No Volátil (NVS)
A diferencia de la EEPROM clásica, utilizamos la librería `Preferences.h`. 
*   **Ventaja Innovadora:** El sistema utiliza un sistema de archivos estructurado que previene la corrupción de datos y permite almacenar tipos de datos complejos de forma segura.
*   **Namespace:** `wifi_cfg`

### 3.2. Portal Cautivo y UI
La interfaz web ha sido desarrollada en **HTML5/CSS3 y JavaScript nativo**, embebida en la memoria `PROGMEM` del chip para maximizar la velocidad de carga sin depender de servidores externos (Edge Computing).

---

## 4. Documentación de la API (RESTful Endpoints)

Todas las respuestas utilizan el estándar `application/json`.

| Método | Endpoint | Descripción | Payload (JSON) |
| :--- | :--- | :--- | :--- |
| **GET** | `/api/status` | Estado de conexión y señal RSSI | N/A |
| **GET** | `/api/scan` | Escaneo de redes circundantes | N/A |
| **POST** | `/api/connect` | Envío de nuevas credenciales | `{"ssid":"...","password":"..."}` |
| **POST** | `/api/reset` | Borrado de fábrica y reinicio | `{"confirm": true}` |
| **GET** | `/api/info` | Telemetría del hardware y firmware | N/A |

---

## 5. Diagramas UML (Validación de Flujo)

### 5.1. Diagrama de Secuencia: Aprovisionamiento Inicial
Este flujo describe la interacción desde que el usuario detecta el AP hasta que el dispositivo obtiene salida a internet.
> [!TIP]
> [INSERTAR AQUÍ: Diagrama UML de Secuencia profesional]

### 5.2. Mecanismo de Reset (Hardware Interrupt)
Se implementó un manejador de interrupciones para el **GPIO 0**. Si el botón se mantiene presionado por más de 3 segundos, se dispara una rutina de borrado de NVS para evitar el bloqueo del dispositivo ante cambios de router.

---

## 6. Resolución de Cuestionamientos Técnicos (20%)

### 6.1. Seguridad WPA2 Enterprise (PEAP)
**¿Es posible conectarse?** Sí. 
**Requerimientos:** El ESP32 soporta el protocolo WPA2 Enterprise mediante la librería `esp_wpa2.h`. A diferencia de las redes domésticas, requiere una configuración de identidad (username) y el uso de métodos como **EAP-MSCHAPv2**. En entornos de alta seguridad, es indispensable cargar el **Certificado CA** para realizar la validación mutua entre el servidor RADIUS y el ESP32.

### 6.2. Concurrencia en WebServer
**Capacidad:** La librería `WebServer.h` de Arduino es síncrona y gestiona una cola limitada de sockets (típicamente hasta 5 conexiones simultáneas antes de degradar el servicio).
**Alternativas de Alto Rendimiento:** Para aplicaciones industriales con múltiples usuarios, la alternativa es **ESPAsyncWebServer**. Esta librería opera de forma asíncrona, permitiendo manejar decenas de conexiones concurrentes y WebSockets sin interrumpir el flujo principal del microcontrolador.

---

## 7. Análisis de Viabilidad y Modelo de Negocio
Desde una perspectiva de ingeniería económica, esta implementación:
1.  **Reduce Costos Operativos (OPEX):** Elimina la necesidad de soporte técnico técnico presencial para cambios de red.
2.  **Mejora la UX:** El usuario final tiene autonomía total (Self-Provisioning).
3.  **Escalabilidad:** El diseño de la API permite que esta misma lógica se integre en una App móvil personalizada en el futuro.

---

## 8. Recursos Adicionales
*   **Postman Collection:** [Link a la colección o archivo .json en la carpeta /docs]
*   **Esquemáticos:** Disponibles en la carpeta `/hardware`.
*   **Wiki Detallada:** Para instrucciones de compilación y librerías necesarias.

---
**Desarrollado por:** Juan Felipe Moncada, Samuel Arcos y Juan Felipe Cardenas  
**Fecha:** Mayo 2026 | **Curso:** Internet de las Cosas (IoT)
