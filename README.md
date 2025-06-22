# Smart-Surveillance-Dashboard
Full-stack smart camera system using ESP32 camera module, PIR sensor and React for the dashboard. Includes a production-grade firmware for MJPEG streaming, image capture, and flash control — paired with a sleek  dashboard for real-time monitoring and gallery access.

Ideal for IoT , home automation, and smart security.

---

## ⚙️ Features

### 🔧 ESP32-CAM Firmware (Arduino)

- 📷 MJPEG live stream at `/stream`
- 🖼️ Capture snapshots at `/capture`
- 💡 Toggle flash LED via `/flash`
- 🔒 Stream concurrency lock
- 🌐 CORS support for web UI
- 🧊 FPS delay for heat mitigation

### 🌐 Frontend Dashboard (React + Vite)

- 🎨 Modern UI with **Tailwind CSS** + **shadcn-ui**
- 🌗 Dark mode support
- 📸 Live video preview
- 🖼️ Snapshot gallery (optional Supabase integration)
- 🧪 Clean modular components (TypeScript + hooks)

---

## 🖥️ Frontend Tech Stack

- Vite
- TypeScript
- React
- Tailwind CSS
- shadcn-ui
- Heroicons / Lucide for icons

---

## 📡 Wi-Fi Configuration (Firmware)

Edit in `esp32_cam_server.ino`:

```cpp
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
