# RestHandler – API REST para controle do dispositivo

Esta classe expõe um conjunto de endpoints REST baseados em `ESPAsyncWebServer` para controle e monitoramento do dispositivo. Os endpoints estão sob o prefixo `/rest`.

---

## ✩ Sumário de Endpoints

| Método | Caminho                | Descrição                          |
| ------ | ---------------------- | ---------------------------------- |
| GET    | `/rest/state`          | Retorna o estado atual do sistema  |
| GET    | `/rest/color`          | Atualiza a cor do dispositivo      |
| GET    | `/rest/brightness`     | Define brilho uniforme             |
| GET    | `/rest/bluetooth`      | Ativa ou desativa o Bluetooth      |
| GET    | `/rest/system/restart` | Reinicia o dispositivo             |
| GET    | `/rest/system/reset`   | Reseta o dispositivo para o padrão |

---

## 📘 Endpoints Detalhados

### 🔹 `GET /rest/state`

Retorna o estado atual do dispositivo em formato JSON.

#### Exemplo de resposta:

```json
{
  "deviceName": "rgbw-ctrl-of-you",
  "firmwareVersion": "3.0.0",
  "heap": 117616,
  "wifi": {
    "details": {
      "ssid": "your_wifi",
      "mac": "00:00:00:00:00:00",
      "ip": "192.168.0.2",
      "gateway": "192.168.0.1",
      "subnet": "255.255.255.0",
      "dns": "192.168.0.1"
    },
    "status": "CONNECTED"
  },
  "alexa": {
    "mode": "rgb_device",
    "names": [
      "led strip",
      "bedroom"
    ]
  },
  "output": [
    {
      "on": false,
      "value": 255
    },
    {
      "on": false,
      "value": 255
    },
    {
      "on": false,
      "value": 255
    },
    {
      "on": false,
      "value": 255
    }
  ],
  "ble": {
    "status": "OFF"
  },
  "ota": {
    "status": "Idle",
    "totalBytesExpected": 0,
    "totalBytesReceived": 0
  },
  "espNow": {
    "devices": [
      {
        "name": "bedroom light switch",
        "address": "00:00:00:00:00:00"
      }
    ]
  }
}
``` 

---

### 🎨 `GET /rest/color`

Define as cores RGBW individualmente.

#### Parâmetros:

* `r`: Valor do vermelho (0–255)
* `g`: Valor do verde (0–255)
* `b`: Valor do azul (0–255)
* `w`: Valor do branco (0–255)

#### Exemplo de requisição:

```
GET /rest/color?r=255&g=128&b=0&w=0
```

#### Resposta:

```json
{ "message": "Color updated" }
```

> **Nota:** Liga automaticamente os canais com valor > 0.

---

### 💡 `GET /rest/brightness`

Aplica o mesmo valor de brilho a todos os canais.

#### Parâmetros:

* `value`: Intensidade (0–255)

#### Exemplo:

```
GET /rest/brightness?value=150
```

#### Resposta:

```json
{ "message": "OK" }
```

> **Nota:** O canal será desligado se `value == 0`.

---

### 📶 `GET /rest/bluetooth`

Ativa ou desativa o Bluetooth.

#### Parâmetros:

* `state`: `"on"`, `"1"`, `"true"` para ligar; qualquer outro valor desliga.

#### Exemplo:

```
GET /rest/bluetooth?state=on
```

#### Respostas:

```json
{ "message": "Bluetooth enabled" }
```

```json
{ "message": "Bluetooth disabled" }
```

> Reinicia o dispositivo após desativar o Bluetooth.

---

### ↺ `GET /rest/system/restart`

Reinicia o dispositivo.

#### Resposta:

```json
{ "message": "Restarting..." }
```

---

### ⚠️ `GET /rest/system/reset`

Apaga as configurações persistentes e reinicia o dispositivo.

#### Resposta:

```json
{ "message": "Resetting to factory defaults..." }
```

---

## 📌 Observações

* Todos os endpoints retornam JSON e incluem o cabeçalho:

  ```
  Cache-Control: no-store
  ```

* Os métodos HTTP utilizados são `GET`, mas para melhor conformidade REST, recomenda-se migrar endpoints mutáveis para `POST`.
