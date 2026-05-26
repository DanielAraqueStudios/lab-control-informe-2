from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel,
                              QGridLayout, QFrame, QSizePolicy)
from PyQt6.QtCore import Qt
from viewmodels.app_state import AppState

class ValueCard(QFrame):
    """A styled card to display a single metric."""
    def __init__(self, title: str, unit: str = "", parent=None):
        super().__init__(parent)
        self.setObjectName("metricCard")
        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setMinimumHeight(120)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        
        layout = QVBoxLayout(self)
        layout.setAlignment(Qt.AlignmentFlag.AlignTop | Qt.AlignmentFlag.AlignLeft)
        
        lbl_title = QLabel(title)
        lbl_title.setObjectName("metricLabel")
        
        # Value container
        val_layout = QHBoxLayout()
        val_layout.setAlignment(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignBottom)
        
        self.lbl_value = QLabel("--")
        self.lbl_value.setObjectName("metricValue")
        
        lbl_unit = QLabel(unit)
        lbl_unit.setObjectName("metricUnit")
        
        val_layout.addWidget(self.lbl_value)
        val_layout.addWidget(lbl_unit)
        
        layout.addWidget(lbl_title)
        layout.addLayout(val_layout)
        
    def set_value(self, val_str: str):
        self.lbl_value.setText(val_str)


class TelemetryPage(QWidget):
    """Page for displaying real-time hardware telemetry."""
    def __init__(self, app_state: AppState, parent=None):
        super().__init__(parent)
        self.app_state = app_state
        
        layout = QVBoxLayout(self)
        layout.setContentsMargins(32, 32, 32, 32)
        layout.setSpacing(24)
        
        # Title
        lbl_title = QLabel("Telemetría en Vivo (Hardware Bruto)")
        lbl_title.setStyleSheet("font-size: 24px; font-weight: bold; color: #00e5ff;")
        layout.addWidget(lbl_title)
        
        grid = QGridLayout()
        grid.setSpacing(16)
        
        # Row 0: Ultrasónicos
        self.card_t1 = ValueCard("Nivel Tanque 1", "mm")
        self.card_t2 = ValueCard("Nivel Tanque 2", "mm")
        self.card_d1 = ValueCard("Distancia Bruta 1", "mm")
        self.card_d2 = ValueCard("Distancia Bruta 2", "mm")
        grid.addWidget(self.card_t1, 0, 0)
        grid.addWidget(self.card_t2, 0, 1)
        grid.addWidget(self.card_d1, 0, 2)
        grid.addWidget(self.card_d2, 0, 3)
        
        # Row 1: Bomba y Flujo
        self.card_flow = ValueCard("Caudal", "L/min")
        self.card_pwm = ValueCard("Bomba PWM", "/255")
        self.card_pump = ValueCard("Estado Bomba", "")
        self.card_dir = ValueCard("Dirección", "")
        grid.addWidget(self.card_flow, 1, 0)
        grid.addWidget(self.card_pwm, 1, 1)
        grid.addWidget(self.card_pump, 1, 2)
        grid.addWidget(self.card_dir, 1, 3)
        
        # Row 2: Válvulas y Volumen
        self.card_s1 = ValueCard("Ángulo Válvula 1", "°")
        self.card_s2 = ValueCard("Ángulo Válvula 2", "°")
        self.card_vol = ValueCard("Volumen Total", "L")
        grid.addWidget(self.card_s1, 2, 0)
        grid.addWidget(self.card_s2, 2, 1)
        grid.addWidget(self.card_vol, 2, 2)
        
        layout.addLayout(grid)
        layout.addStretch()
        
        # Connect signals
        self.app_state.live_data_updated.connect(self._on_live_data_updated)
        
    def _on_live_data_updated(self):
        data = self.app_state.current_live_data
        if not data: return
        
        self.card_t1.set_value(f"{data.level1_mm:.1f}")
        self.card_t2.set_value(f"{data.level2_mm:.1f}")
        self.card_d1.set_value(f"{data.dist1_mm:.1f}")
        self.card_d2.set_value(f"{data.dist2_mm:.1f}")
        
        self.card_flow.set_value(f"{data.flow_lpm:.2f}")
        self.card_pwm.set_value(f"{data.pwm}")
        self.card_pump.set_value("ENCENDIDA" if data.pump_enabled else "APAGADA")
        self.card_dir.set_value("FWD (Adelante)" if data.pump_dir == "FWD" else "REV (Atrás)")
        
        self.card_s1.set_value(f"{data.servo1_angle}")
        self.card_s2.set_value(f"{data.servo2_angle}")
        
        self.card_vol.set_value(f"{data.volume_l:.2f}")