# Sprint 4: Controlador PID

## Descripción General

Este programa implementa control PID digital en lazo cerrado para regulación de flujo hidráulico. Incluye sintonización manual por Ziegler-Nichols, logging CSV y análisis de respuesta transitoria.

## Características Principales

### 1. Control PID Digital

Implementación del algoritmo PID discreto:

```
u(k) = Kp·e(k) + Ki·∫e(τ)dτ + Kd·de(k)/dt
```

**Parámetros por defecto:**
- Kp = 50.0
- Ki = 10.0  
- Kd = 5.0
- Período de muestreo: dt = 100ms (10 Hz)

### 2. Anti-Windup

Saturación del término integral para prevenir wind-up:
- Límite superior: +200
- Límite inferior: -200

### 3. Filtrado Derivativo

El término derivativo utiliza diferencia finita con período de muestreo fijo para reducir ruido.

### 4. Sintonización Ziegler-Nichols

Método de oscilación sostenida (Gain Margin Method):

**Procedimiento:**
1. Iniciar con Ki=0, Kd=0, Kp bajo
2. Incrementar Kp gradualmente hasta obtener oscilación sostenida
3. Anotar ganancia crítica **Ku** y período de oscilación **Pu**
4. Calcular parámetros:
   - Kp = 0.6 × Ku
   - Ki = 2 × Kp / Pu  
   - Kd = Kp × Pu / 8

**Ejemplo práctico:**
```
Si Ku = 80 y Pu = 4.0s:
- Kp = 0.6 × 80 = 48.0
- Ki = 2 × 48 / 4 = 24.0
- Kd = 48 × 4 / 8 = 24.0
```

Aplicar con:
```
SETKP,48.0
SETKI,24.0
SETKD,24.0
```

## Conexiones

Sprint 9 es la referencia vigente de pinout para el sistema integrado. El control PID de flujo de este sprint usa los mismos pines de bomba y caudalímetro. La lectura SE045/ADC que aparece en este sprint queda como ruta histórica; para nivel vigente usar HC-SR04.

| Componente | Pin ESP32 | Función |
|------------|-----------|---------|
| H-Bridge ENA | GPIO 17 | PWM motor (10kHz, 8-bit) |
| H-Bridge IN1 | GPIO 15 | Dirección 1 |
| H-Bridge IN2 | GPIO 16 | Dirección 2 |
| YF-S401 Signal | GPIO 4 | Interrupción flujo |
| HC-SR04 Tank 1 TRIG | GPIO 5 | Disparo ultrasónico |
| HC-SR04 Tank 1 ECHO | GPIO 6 | Eco con divisor a 3.3V |
| HC-SR04 Tank 2 TRIG | GPIO 8 | Disparo ultrasónico |
| HC-SR04 Tank 2 ECHO | GPIO 9 | Eco con divisor a 3.3V |
| LED Status | GPIO 7 | Indicador PID activo |
| Servo válvula Tank 1 | GPIO 18 | Pin reservado por Sprint 9 |
| Servo válvula Tank 2 | GPIO 19 | Pin reservado por Sprint 9 |

## Comandos UART (115200 baud)

### Configuración PID

```
SETSP,<valor>        - Establecer setpoint (L/min)
                       Ejemplo: SETSP,1.5

SETKP,<valor>        - Ajustar ganancia proporcional
                       Ejemplo: SETKP,50.0

SETKI,<valor>        - Ajustar ganancia integral
                       Ejemplo: SETKI,10.0

SETKD,<valor>        - Ajustar ganancia derivativa
                       Ejemplo: SETKD,5.0
```

### Control

```
STARTPID             - Iniciar control PID (resetea variables)

STOPPID              - Detener control y motor

STEP,<valor>         - Ejecutar prueba de respuesta escalón
                       Duración: 60 segundos con logging
                       Ejemplo: STEP,1.2
```

### Análisis

```
LOGPID               - Activar/desactivar logging PID
                       Formato CSV: Time,Setpoint,Flow,Error,PWM,P,I,D,Level1,Level2

TUNEAUTO             - Mostrar guía sintonización Ziegler-Nichols
                       (Proceso semiautomático)

STATUS               - Ver estado completo del controlador
```

## Secuencia de Uso Típica

### 1. Primera Prueba

```
1. Conectar ESP32 al sistema hidráulico
2. Abrir Serial Monitor (115200 baud)
3. SETSP,1.0          (Setpoint 1.0 L/min)
4. STARTPID           (Activar control)
5. Observar respuesta con STATUS
6. LOGPID             (Capturar datos)
7. Copiar datos CSV a archivo .txt
8. STOPPID            (Detener después de ~1 minuto)
```

### 2. Ajuste Manual de Ganancias

Si la respuesta tiene mucho overshoot:
```
SETKP,40.0    (Reducir Kp)
SETKI,8.0     (Reducir Ki)
STARTPID
```

Si la respuesta es muy lenta:
```
SETKP,60.0    (Aumentar Kp)
SETKI,15.0    (Aumentar Ki)
STARTPID
```

Si hay oscilaciones sostenidas:
```
SETKD,10.0    (Aumentar Kd para amortiguar)
```

### 3. Prueba de Escalón Automática

```
STEP,1.5      → Aplica setpoint 1.5 L/min, registra 60s, calcula métricas
```

Datos exportables directamente a MATLAB:
```matlab
data = readtable('step_response.csv');
plot(data.Time_s, data.Flow, data.Time_s, data.Setpoint);
```

## Formato de Datos (CSV)

```
Time_s,Setpoint,Flow,Error,PWM,P,I,D,Level1,Level2
0.100,1.000,0.234,0.766,153,38.30,7.66,38.30,12.43,8.21
0.200,1.000,0.456,0.544,189,27.20,12.88,11.10,13.87,9.03
...
```

**Columnas:**
- **Time_s**: Tiempo desde inicio (segundos)
- **Setpoint**: Referencia de flujo (L/min)
- **Flow**: Flujo medido (L/min)
- **Error**: Setpoint - Flow (L/min)
- **PWM**: Señal de control (0-255)
- **P**: Término proporcional
- **I**: Término integral acumulado
- **D**: Término derivativo
- **Level1/2**: Niveles de agua (mm)

## Análisis de Desempeño

### Métricas Típicas Esperadas

Para un sistema bien sintonizado con setpoint 1.0 L/min:

| Métrica | Valor Ideal | Rango  |
|---------|-------------|-----------|
| Overshoot | 0-15% | Aceptable < 25% |
| Rise Time | 2-5 s | Depende de inercia |
| Settling Time (2%) | 5-10 s | Aceptable < 20s |
| Steady-State Error | < 2% | < 5% aceptable |

### Diagnóstico de Problemas

**Síntoma: Oscilación sostenida**
- Causa: Kp o Kd muy altos
- Solución: Reducir Kp en 20%, aumentar Kd ligeramente

**Síntoma: Respuesta lenta sin alcanzar setpoint**
- Causa: Kp muy bajo o Ki insuficiente
- Solución: Aumentar Kp y Ki gradualmente

**Síntoma: Overshoot > 30%**
- Causa: Kp muy alto
- Solución: Reducir Kp, reducir Ki

**Síntoma: Integrador crece sin control**
- Causa: Error sostenido (restricción física)
- Solución: Verificar bomba funcional, sin bloqueos

## Indicadores LED

- **Parpadeo rápido (250ms)**: PID activo
- **Parpadeo lento (1s)**: PID inactivo

## Notas Importantes

### 1. Rango de Trabajo

El control PID funciona eficientemente en:
- **Flujo**: 0.3 - 3.0 L/min (límites del YF-S401)
- **PWM útil**: 40 - 240 (evitar zona muerta del motor)

### 2. Condiciones Iniciales

Antes de `STARTPID`, asegurarse:
- [ ] Tanques con agua suficiente (> 100 ml cada uno)
- [ ] Motor funciona correctamente (probar con Sprint 1)
- [ ] Sensores leen datos (Sprint 2)
- [ ] Sin fugas en tubería

### 3. Limitaciones

- **No controla nivel** (solo flujo). Para control de nivel, ver Sprint 5
- **Sin compensación de perturbaciones** externas
- **Sin feedforward** (solo retroalimentación)

## Próximos Pasos

Una vez validado el control PID de flujo:

1. **Sprint 5** → Control dual (flujo + nivel), referencias múltiples
2. **MATLAB** → Identificación de sistema, validación de modelo
3. **Latex** → Documentación formal del proyecto

## Troubleshooting

### Error: Flujo no responde a PWM

```
1. STOPPID
2. Probar Sprint 1 (motor control directo)
3. Verificar conexiones H-Bridge
```

### Error: Integral crece indefinidamente

```
# Anti-windup debería prevenir esto
# Si ocurre, resetear con:
STOPPID
STARTPID
```

### Error: Lecturas de sensores erráticas

```
# Verificar pinout vigente:
1. Tank 1 HC-SR04: TRIG GPIO5, ECHO GPIO6 con divisor a 3.3V
2. Tank 2 HC-SR04: TRIG GPIO8, ECHO GPIO9 con divisor a 3.3V
```

## Compilación

**Requisitos:**
- Arduino IDE 2.x
- ESP32 Core >= 3.0.0 (para ledcAttach API)
- Board: ESP32S3 Dev Module

**Settings:**
```
Upload Speed: 921600
USB CDC On Boot: Enabled
Flash Mode: QIO
Flash Size: 8MB (partición default)
```

## Referencias

- Ziegler, J.G. & Nichols, N.B. (1942). "Optimum Settings for Automatic Controllers"
- Åström, K.J. & Hägglund, T. (1995). "PID Controllers: Theory, Design, and Tuning"

---

**Versión:** 1.0  
**Autores:** Daniel García Araque, David Santiago García Suarez  
**Fecha:** Febrero 2026
