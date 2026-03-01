"""
Dashboard page - real-time telemetry visualization.
"""
from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel, 
                              QGridLayout, QFrame, QSplitter)
from PyQt6.QtCore import Qt, QTimer
from ui.components import MetricCard, RealtimeChart, StackedAreaChart
from viewmodels import AppState


class DashboardPage(QWidget):
    """Main dashboard with live metrics and charts."""
    
    def __init__(self, app_state: AppState, parent=None):
        super().__init__(parent)
        self.app_state = app_state
        
        # Main layout
        layout = QVBoxLayout(self)
        layout.setContentsMargins(24, 24, 24, 24)
        layout.setSpacing(24)
        
        # Page title
        title = QLabel("Live Dashboard")
        title.setObjectName("heading1")
        layout.addWidget(title)
        
        # Metric cards grid
        cards_layout = QGridLayout()
        cards_layout.setSpacing(16)
        
        # Create metric cards
        self.flow_card = MetricCard("Flow Rate", "L/min", "💧")
        self.level1_card = MetricCard("Tank 1 Level", "mm", "📊")
        self.level2_card = MetricCard("Tank 2 Level", "mm", "📊")
        self.pwm_card = MetricCard("Motor PWM", "%", "⚙")
        self.error_card = MetricCard("Control Error", "", "📉")
        self.volume_card = MetricCard("Total Volume", "L", "🔄")
        
        cards_layout.addWidget(self.flow_card, 0, 0)
        cards_layout.addWidget(self.level1_card, 0, 1)
        cards_layout.addWidget(self.level2_card, 0, 2)
        cards_layout.addWidget(self.pwm_card, 1, 0)
        cards_layout.addWidget(self.error_card, 1, 1)
        cards_layout.addWidget(self.volume_card, 1, 2)
        
        layout.addLayout(cards_layout)
        
        # Charts section
        splitter = QSplitter(Qt.Orientation.Vertical)
        
        # Flow rate chart
        self.flow_chart = RealtimeChart("Flow Rate vs Reference", "Flow (L/min)", max_points=500)
        self.flow_chart.add_series("Reference", color='#ff006e', width=2)  # Red dashed
        self.flow_chart.add_series("Actual Flow", color='#00e5ff', width=3)  # Cyan solid
        self.flow_chart.setMinimumHeight(250)
        splitter.addWidget(self.flow_chart)
        
        # PID components chart
        self.pid_chart = StackedAreaChart("PID Controller Components")
        self.pid_chart.setMinimumHeight(200)
        splitter.addWidget(self.pid_chart)
        
        layout.addWidget(splitter, stretch=1)
        
        # Connect to app state signals
        self.app_state.telemetry_updated.connect(self.on_telemetry_update)
        
        # Chart update timer (30 FPS)
        self.chart_timer = QTimer()
        self.chart_timer.timeout.connect(self.update_charts)
        self.chart_timer.start(33)  # ~30 FPS
    
    def on_telemetry_update(self):
        """Handle new telemetry data."""
        data = self.app_state.current_telemetry
        if not data:
            return
        
        # Update metric cards with animation
        self.flow_card.set_value(data.flow_rate, decimals=3, animate=True)
        self.level1_card.set_value(data.level_tank1, decimals=2, animate=True)
        self.level2_card.set_value(data.level_tank2, decimals=2, animate=True)
        self.pwm_card.set_value((data.pwm / 255.0) * 100, decimals=1, animate=True)
        self.error_card.set_value(data.error, decimals=3, animate=True)
        self.volume_card.set_value(data.volume, decimals=2, animate=True)
        
        # Set status colors based on thresholds
        if abs(data.error) < 0.1:
            self.error_card.set_status_color('good')
        elif abs(data.error) < 0.5:
            self.error_card.set_status_color('warning')
        else:
            self.error_card.set_status_color('error')
        
        # Update chart data buffers (don't render yet - wait for timer)
        self.flow_chart.update_series("Reference", data.timestamp, data.reference)
        self.flow_chart.update_series("Actual Flow", data.timestamp, data.flow_rate)
        
        self.pid_chart.update_series("P", data.timestamp, data.pid_p)
        self.pid_chart.update_series("I", data.timestamp, data.pid_i)
        self.pid_chart.update_series("D", data.timestamp, data.pid_d)
    
    def update_charts(self):
        """Periodic chart rendering (controlled FPS)."""
        if self.app_state.reduce_animations:
            return  # Skip if animations disabled
        
        # Render both charts
        self.flow_chart.render()
        self.pid_chart.render()
    
    def showEvent(self, event):
        """Start chart updates when page becomes visible."""
        super().showEvent(event)
        self.chart_timer.start()
    
    def hideEvent(self, event):
        """Stop chart updates when page hidden (save CPU)."""
        super().hideEvent(event)
        self.chart_timer.stop()
