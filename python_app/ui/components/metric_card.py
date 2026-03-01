"""
Reusable metric card component for displaying telemetry values.
"""
from PyQt6.QtWidgets import QFrame, QVBoxLayout, QLabel, QHBoxLayout
from PyQt6.QtCore import Qt, QPropertyAnimation, QEasingCurve, pyqtProperty
from PyQt6.QtGui import QFont


class MetricCard(QFrame):
    """Animated card displaying a single metric value."""
    
    def __init__(self, label: str, unit: str = "", icon: str = "", parent=None):
        super().__init__(parent)
        self.setObjectName("metricCard")
        
        self._current_value = 0.0
        self._displayed_value = 0.0
        
        # Layout
        layout = QVBoxLayout(self)
        layout.setSpacing(8)
        
        # Label with optional icon
        label_layout = QHBoxLayout()
        if icon:
            icon_label = QLabel(icon)
            icon_label.setObjectName("metricIcon")
            label_layout.addWidget(icon_label)
        
        self.label_text = QLabel(label)
        self.label_text.setObjectName("metricLabel")
        label_layout.addWidget(self.label_text)
        label_layout.addStretch()
        layout.addLayout(label_layout)
        
        # Value display
        value_layout = QHBoxLayout()
        self.value_label = QLabel("--")
        self.value_label.setObjectName("metricValue")
        value_layout.addWidget(self.value_label)
        
        if unit:
            self.unit_label = QLabel(unit)
            self.unit_label.setObjectName("metricUnit")
            value_layout.addWidget(self.unit_label)
        
        value_layout.addStretch()
        layout.addLayout(value_layout)
        
        # Animation for smooth value transitions
        self.animation = QPropertyAnimation(self, b"displayedValue")
        self.animation.setDuration(300)
        self.animation.setEasingCurve(QEasingCurve.Type.OutCubic)
        
        self.setMinimumSize(180, 120)
    
    def set_value(self, value: float, decimals: int = 2, animate: bool = True):
        """Update metric value with optional animation.
        
        Args:
            value: New value to display
            decimals: Number of decimal places
            animate: Whether to animate the transition
        """
        self._current_value = value
        
        if animate:
            self.animation.stop()
            self.animation.setStartValue(self._displayed_value)
            self.animation.setEndValue(value)
            self.animation.start()
        else:
            self.displayedValue = value
    
    @pyqtProperty(float)
    def displayedValue(self):
        return self._displayed_value
    
    @displayedValue.setter
    def displayedValue(self, value: float):
        self._displayed_value = value
        # Update label with formatted value
        if abs(value) < 100:
            self.value_label.setText(f"{value:.2f}")
        elif abs(value) < 1000:
            self.value_label.setText(f"{value:.1f}")
        else:
            self.value_label.setText(f"{value:.0f}")
    
    def set_status_color(self, status: str):
        """Set card border color based on status.
        
        Args:
            status: 'normal', 'warning', 'error', 'good'
        """
        colors = {
            'normal': '#2d3548',
            'warning': '#fb5607',
            'error': '#ff006e',
            'good': '#00ff88'
        }
        color = colors.get(status, '#2d3548')
        self.setStyleSheet(f"QFrame#metricCard {{ border-color: {color}; }}")
