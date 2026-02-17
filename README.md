# LABORATORIO 2: Control de Flujo en un Sistema Hidráulico

## Información General

**Universidad:** Universidad Militar Nueva Granada  
**Facultad:** Ingeniería  
**Programa:** Ingeniería en Mecatrónica  
**Asignatura:** Control Lineal y Laboratorio  
**Semestre:** Séptimo  
**Fecha:** 2026-1  

---

## Objetivo General

Controlar el flujo de agua de entrada a un tanque considerando referencias tipo escalón, rampa, y aceleración, mediante la implementación de un control PID en electrónica análoga, sintonizado por el método de Ziegler-Nichols.

---

## Objetivos Específicos

1. **Implementar e instrumentar** un sistema hidráulico similar al mostrado en la Figura 1, incluyendo sensores y acondicionamiento de señales.

2. **Encontrar la representación matemática** de la dinámica del sistema hidráulico y compararla contra la respuesta del sistema real.

3. **Determinar un regulador PID** utilizando el método de sintonización de Ziegler-Nichols, validándolo en simulación para asegurar que el flujo de entrada siga referencias tipo:
   - Posición (escalón)
   - Velocidad (rampa)

4. **Implementar en electrónica análoga** el controlador obtenido para que el flujo de entrada haga seguimiento a referencias tipo escalón, validando el funcionamiento en el sistema real.

---

## Descripción del Sistema

El sistema hidráulico consta de:

- **Bomba peristáltica:** Elemento actuante controlado por voltaje DC
- **Tanques:** Vasos de precipitado para contención de líquido
- **Válvulas de bola:** Control de flujo entre tanques
- **Reservorio:** Almacenamiento de agua
- **Sensores:** 
  - Sensor de flujo de entrada (qin)
  - Sensores de nivel en tanques

### Configuración del Sistema

El sistema implementa un tanque hidráulico donde:
- La **entrada** es el flujo de agua qin(t) generado por una bomba peristáltica
- La **salida** es el flujo de agua que sale por gravedad
- El **objetivo de control** es regular el flujo de entrada para seguir referencias predefinidas

---

## Materiales y Equipos

### Equipos del Laboratorio

| Descripción | Cantidad |
|-------------|----------|
| Computador con MATLAB | 1 por grupo |
| Fuente de voltaje | 1 por grupo |
| Osciloscopio | 1 por grupo |
| Generador de señales | 1 por grupo |
| Multímetro | 1 por grupo |

### Materiales del Estudiante

| Descripción | Cantidad |
|-------------|----------|
| Bomba peristáltica | 1 por grupo |
| Sistema hidráulico (vasos, válvulas, reservorio) | 1 sistema completo |
| Sensores de flujo y nivel | 3 sensores |
| Protoboard | 1 por grupo |
| Amplificadores operacionales | 10 por grupo |
| Conjunto de cables | 1 conjunto |
| Resistencias y condensadores | 50 componentes |

---

## Metodología

### Fase 1: Implementación del Sistema Físico

1. **Montaje del sistema hidráulico:**
   - Ensamblar tanques, válvulas y bomba peristáltica
   - Garantizar hermeticidad para evitar derramamientos
   - Implementar medidas de seguridad para protección de equipos electrónicos

2. **Instrumentación:**
   - Instalar sensor de flujo de entrada
   - Instalar sensores de nivel en tanques
   - Implementar acondicionamiento de señales
   - Diseñar manejadores de potencia para la bomba

### Fase 2: Modelado Matemático

1. **Identificación del sistema:**
   - Obtener datos experimentales de entrada-salida
   - Aplicar leyes de conservación de masa
   - Considerar dinámica de flujo por gravedad

2. **Función de transferencia:**
   - Determinar F(s) = Qin(s) / VDC(s)
   - Donde:
     - qin(t): flujo de entrada al sistema (variable medida)
     - vDC(t): voltaje DC aplicado al motor de la bomba (variable de control)

3. **Validación del modelo:**
   - Comparar respuesta simulada vs. respuesta real
   - Ajustar parámetros si es necesario

### Fase 3: Diseño del Controlador

1. **Sintonización por Ziegler-Nichols:**
   - Método de respuesta a lazo abierto o método de oscilación sostenida
   - Obtener parámetros Kp, Ki, Kd del controlador PID
   - Objetivo: seguimiento de referencias con error de estado estable cero

2. **Referencias a seguir:**
   - **Escalón:** posición constante del flujo
   - **Rampa:** velocidad constante de cambio de flujo
   - **Parábola:** aceleración constante de flujo

3. **Simulación:**
   - Validar diseño en MATLAB/Simulink
   - Verificar especificaciones de desempeño
   - Analizar estabilidad y robustez

### Fase 4: Implementación Analógica

1. **Diseño del circuito PID:**
   - Utilizar amplificadores operacionales
   - Implementar:
     - Acción proporcional con resistencias
     - Acción integral con capacitores
     - Acción derivativa con circuito RC
   - Diseñar etapa de suma para combinar las tres acciones

2. **Validación del circuito:**
   - Simular comportamiento del circuito
   - Verificar respuesta en frecuencia
   - Comprobar saturación de amplificadores

### Fase 5: Pruebas Experimentales

1. **Implementación en lazo cerrado:**
   - Conectar sensor → controlador → actuador
   - Aplicar referencia tipo escalón
   - Registrar señales:
     - Referencia
     - Error
     - Señal de control
     - Flujo medido

2. **Análisis comparativo:**
   - Comparar resultados simulación vs. experimentales
   - Evaluar métricas de desempeño:
     - Tiempo de establecimiento
     - Sobreimpulso
     - Error en estado estacionario
     - Tiempo de subida

---

## Modelado Matemático del Sistema Hidráulico

### Balance de Masa en el Tanque

Para un tanque con área transversal constante A:

```
dh/dt = (qin - qout) / A
```

Donde:
- h(t): altura del líquido en el tanque
- qin(t): flujo de entrada (controlado por bomba)
- qout(t): flujo de salida (por gravedad)

### Flujo de Salida

El flujo de salida por orificio depende de la altura:

```
qout(t) = k√h(t)
```

Donde k es una constante que depende del área del orificio y coeficiente de descarga.

### Dinámica No Lineal

La ecuación diferencial que describe el sistema es:

```
A(dh/dt) = qin(t) - k√h(t)
```

### Linealización

Alrededor de un punto de operación h̄:

```
A(dΔh/dt) = Δqin - (k/2√h̄)Δh
```

### Función de Transferencia Tanque

En el dominio de Laplace:

```
H(s)/Qin(s) = 1/(As + k/(2√h̄))
```

### Modelo Bomba-Motor

La relación entre voltaje DC y flujo generado:

```
Qin(s)/VDC(s) = Kb/(τs + 1)
```

Donde:
- Kb: constante de la bomba
- τ: constante de tiempo del motor

### Función de Transferencia Total

```
F(s) = Qin(s)/VDC(s) = [H(s)/Qin(s)] × [Qin(s)/VDC(s)]
```

---

## Sintonización por Ziegler-Nichols

### Método de Respuesta a Lazo Abierto

1. Aplicar entrada escalón al sistema
2. Registrar curva de respuesta (curva en S)
3. Obtener parámetros:
   - L: tiempo de retardo
   - T: constante de tiempo
4. Calcular parámetros PID según tabla de Ziegler-Nichols

### Parámetros del Controlador

Para seguimiento con error de estado estable cero:

| Controlador | Kp | Ti | Td |
|-------------|----|----|-----|
| P | T/L | - | - |
| PI | 0.9T/L | L/0.3 | - |
| PID | 1.2T/L | 2L | 0.5L |

---

## Implementación Analógica del PID

### Circuito Proporcional

```
Vp(t) = Kp × e(t)
```

Implementado con amplificador inversor:
- Kp = -Rf/Rin

### Circuito Integral

```
Vi(t) = (Kp/Ti) × ∫e(t)dt
```

Implementado con integrador operacional:
- Ti = Rin × Cin

### Circuito Derivativo

```
Vd(t) = Kp × Td × de(t)/dt
```

Implementado con derivador operacional:
- Td = Rf × Cf

### Sumador

Combina las tres acciones:
```
u(t) = Vp(t) + Vi(t) + Vd(t)
```

---

## Resultados Esperados

1. **Sistema hidráulico instrumentado** completamente funcional con sensores calibrados

2. **Modelo matemático validado** que describe con precisión la dinámica del sistema

3. **Controlador PID sintonizado** capaz de realizar seguimiento a referencias tipo:
   - Escalón (error estado estable = 0)
   - Rampa (error estado estable constante)
   - Parábola (comportamiento dinámico adecuado)

4. **Implementación analógica funcional** con circuito PID operando en lazo cerrado

5. **Informe técnico en formato IEEE** documentando:
   - Metodología experimental
   - Modelado matemático
   - Diseño del controlador
   - Resultados experimentales
   - Análisis comparativo simulación vs. experimental
   - Conclusiones y recomendaciones

---

## Competencias Desarrolladas

### Competencias Técnicas

- Aplicación de conocimientos de matemáticas, ciencias e ingeniería
- Diseño y conducción de experimentos
- Análisis e interpretación de datos experimentales
- Diseño de sistemas de control que cumplan especificaciones realistas
- Utilización de técnicas y herramientas modernas de ingeniería

### Competencias Profesionales

- Comunicación técnica efectiva
- Redacción de informes con formatos estandarizados
- Uso de lenguaje técnico preciso
- Presentación oral de ideas clara y concisa

### Indicadores de Evaluación

- Correcta expresión del modelo matemático
- Aplicación adecuada de métodos de solución
- Manejo de herramientas computacionales (MATLAB/Simulink)
- Identificación de parámetros y variables del sistema
- Formulación y ejecución del protocolo experimental
- Análisis crítico de resultados
- Establecimiento de requerimientos de ingeniería
- Evaluación de alternativas de solución
- Soporte técnico de la solución propuesta
- Implementación y validación exitosa

---

## Precauciones de Seguridad

### Generales

- Uso obligatorio de bata blanca en el laboratorio
- Uso adecuado de computadores y equipos electrónicos
- Apagar elementos antes de realizar cambios en circuitos

### Eléctricas

- No exceder valores máximos de voltaje y corriente
- Consultar datasheets de componentes
- No sobrepasar potencia disipada por resistencias
- Verificar polaridad en componentes polarizados

### Sistema Hidráulico

- Garantizar hermeticidad del sistema
- Proteger equipos electrónicos de derramamientos
- Mantener área de trabajo limpia y seca
- Verificar conexiones antes de encender bomba

---

## Restricciones Importantes

⚠️ **IMPORTANTE:** El diseño mecánico de la planta debe ser **ORIGINAL** para cada grupo de estudiantes. 

- No se permite compartir plantas entre grupos del semestre 2026-1
- Los ajustes durante el semestre deben quedar registrados
- El incumplimiento se considera falta al reglamento estudiantil
- Se informará a decanatura para proceso disciplinario

---

## Referencias Técnicas

### Software

- MATLAB R2020a o superior
- Simulink
- Control System Toolbox

### Normativas

- Formato IEEE para informes técnicos
- Normas de seguridad del laboratorio UMNG

### Bibliografía Recomendada

- Ogata, K. (2010). *Ingeniería de Control Moderna*. Pearson.
- Ziegler, J. G., & Nichols, N. B. (1942). *Optimum Settings for Automatic Controllers*. Trans. ASME, 64, 759-768.
- Franklin, G. F., Powell, J. D., & Emami-Naeini, A. (2015). *Feedback Control of Dynamic Systems*. Pearson.

---

## Autores del Laboratorio

**Elaborado por:**
- Lic. Andrés Castro, Ph.D. - Docente Tiempo Completo
- Ing. Adriana Riveros, M.Sc. - Docente Tiempo Completo
- Ing. Leonardo Solaque, Ph.D. - Docente Tiempo Completo
- Ing. Angélica Nivia, M.Sc. - Docente Cátedra

**Revisado por:**
- Ing. Olga Lucía Ramos, Ph.D. - Jefe de Área (Calle 100)
- Ing. Adriana Riveros, M.Sc. - Jefe de Área (Campus)

**Aprobado por:**
- Ing. Darío Amaya, Ph.D. - Director Programa Ingeniería en Mecatrónica (Calle 100)
- Ing. José Luis Caballero, M.Sc. - Director Programa Ingeniería en Mecatrónica (Campus)

---

## Control de Versiones

| Fecha | Descripción del Cambio | Justificación |
|-------|------------------------|---------------|
| 25/06/2024 | Cambio de sistemas a trabajar | Renovación semestral de guías |
| 10/12/2024 | Cambio de sistemas a trabajar | Renovación semestral de guías |
| 02/07/2025 | Cambio de sistemas a trabajar | Renovación semestral de guías |
| 10/12/2025 | Cambio de sistemas a trabajar | Renovación semestral de guías |

---

**Última actualización:** Diciembre 2025  
**Versión:** 2.4  
**Estado:** Vigente para semestre 2026-1
