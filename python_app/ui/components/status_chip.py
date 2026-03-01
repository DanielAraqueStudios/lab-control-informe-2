"""
Animated status chip component for connection state.
"""
from PyQt6.QtWidgets import QLabel, QHBoxLayout, QWidget
from PyQt6.QtCore import Qt, QTimer, QPropertyAnimation, QEasingCurve, pyqtProperty
from PyQt6.QtGui import QPainter, QColor, QPen, QBrush


class StatusIndicator(QWidget):
    """Animated LED-style status indicator."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(12, 12)
        self._pulse_value = 0.0
        self._active = False
        self._color = QColor("#ff006e")  # Red by default
        
        # Pulse animation
        self.pulse_animation = QPropertyAnimation(self, b"pulseValue")
        self.pulse_animation.setDuration(1000)
        self.pulse_animation.setStartValue(0.3)
        self.pulse_animation.setEndValue(1.0)
        self.pulse_animation.setEasingCurve(QEasingCurve.Type.InOutSine)
        self.pulse_animation.setLoopCount(-1)  # Infinite loop
    
    @pyqtProperty(float)
    def pulseValue(self):
        return self._pulse_value
    
    @pulseValue.setter
    def pulseValue(self, value: float):
        self._pulse_value = value
        self.update()  # Trigger repaint
    
    def set_active(self, active: bool):
        """Set indicator active state (green) or inactive (red)."""
        self._active = active
        self._color = QColor("#00ff88") if active else QColor("#ff006e")
        
        if active:
            self.pulse_animation.start()
        else:
            self.pulse_animation.stop()
            self._pulse_value = 1.0
        
        self.update()
    
    def paintEvent(self, event):
        """Custom paint for glowing LED effect."""
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Outer glow
        glow_color = QColor(self._color)
        glow_color.setAlphaF(0.3 * self._pulse_value)
        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(QBrush(glow_color))
        painter.drawEllipse(0, 0, 12, 12)
        
        # Inner solid circle
        core_color = QColor(self._color)
        core_color.setAlphaF(self._pulse_value)
        painter.setBrush(QBrush(core_color))
        painter.drawEllipse(3, 3, 6, 6)


class StatusChip(QWidget):
    """Connection status chip with animated indicator."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
        layout = QHBoxLayout(self)
        layout.setContentsMargins(12, 8, 16, 8)
        layout.setSpacing(8)
        
        # LED indicator
        self.indicator = StatusIndicator()
        layout.addWidget(self.indicator)
        
        # Status text
        self.status_label = QLabel("Disconnected")
        self.status_label.setObjectName("statusChip")
        layout.addWidget(self.status_label)
        
        self._connected = False
        self.set_connected(False)
    
    def set_connected(self, connected: bool, port: str = ""):
        """Update connection status.
        
        Args:
            connected: True if connected, False otherwise
            port: Optional port name to display
        """
        self._connected = connected
        self.indicator.set_active(connected)
        
        if connected:
            text = f"● Connected"
            if port:
                text += f" ({port})"
            self.status_label.setText(text)
            self.status_label.setObjectName("statusConnected")
        else:
            self.status_label.setText("● Disconnected")
            self.status_label.setObjectName("statusDisconnected")
        
        # Force style refresh
        self.status_label.style().unpolish(self.status_label)
        self.status_label.style().polish(self.status_label)
