"""
Dashboard page - real-time telemetry visualization.
"""
from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel, 
                              QGridLayout, QFrame, QSplitter, QPushButton, QDoubleSpinBox)
from PyQt6.QtCore import Qt, QTimer
from ui.components import MetricCard, RealtimeChart, StackedAreaChart
from viewmodels import AppState


class DashboardPage(QWidget):
    """Main dashboard with live metrics and charts."""
    
    def __init__(self, app_state: AppState, parent=None):
        super().__init__(parent)
        self.app_state = app_state
        
        # Virtual PID state
        self.sim_active = False
        self.sim_last_time = None
        self.sim_integral = 0.0
        self.sim_last_error = 0.0
        
        # Main layout
        layout = QVBoxLayout(self)
        layout.setContentsMargins(24, 24, 24, 24)
        layout.setSpacing(24)
        
        # --- Top Header & Sim Layout ---
        header_layout = QHBoxLayout()
        title = QLabel("Live Dashboard")
        title.setObjectName("heading1")
        header_layout.addWidget(title)
        
        header_layout.addStretch()
        
        # Virtual PID controls right in the dashboard
        sim_frame = QFrame()
        sim_frame.setObjectName("panel")
        sim_layout = QHBoxLayout(sim_frame)
        sim_layout.setContentsMargins(10, 5, 10, 5)
        
        self.sim_toggle = QPushButton("🧪 Virtual PID (Tuning Mode)")
        self.sim_toggle.setCheckable(True)
        self.sim_toggle.toggled.connect(self._toggle_sim)
        self.sim_toggle.setObjectName("primaryButton")
        sim_layout.addWidget(self.sim_toggle)
        
        sim_layout.addWidget(QLabel("SP(L/min):"))
        self.sim_sp = QDoubleSpinBox()
        self.sim_sp.setRange(0, 10)
        self.sim_sp.setValue(2.0)
        self.sim_sp.setSingleStep(0.1)
        sim_layout.addWidget(self.sim_sp)

        sim_layout.addWidget(QLabel("Kp:"))
        self.sim_kp = QDoubleSpinBox()
        self.sim_kp.setRange(0, 200)
        self.sim_kp.setValue(50.0)
        sim_layout.addWidget(self.sim_kp)
        
        sim_layout.addWidget(QLabel("Ki:"))
        self.sim_ki = QDoubleSpinBox()
        self.sim_ki.setRange(0, 200)
        self.sim_ki.setValue(10.0)
        sim_layout.addWidget(self.sim_ki)
        
        sim_layout.addWidget(QLabel("Kd:"))
        self.sim_kd = QDoubleSpinBox()
        self.sim_kd.setRange(0, 200)
        self.sim_kd.setValue(0.0)
        sim_layout.addWidget(self.sim_kd)
        
        header_layout.addWidget(sim_frame)
        layout.addLayout(header_layout)
        
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
        if self.sim_active:
            # --- VIRTUAL/SHADOW PID LOGIC ---
            sp = self.sim_sp.value()
            kp = self.sim_kp.value()
            ki = self.sim_ki.value()
            kd = self.sim_kd.value()
            
            # Error = Reference - Actual variable (using Flow Rate here as example)
            # You can change data.flow_rate to data.level_tank1 if needed
            current_error = sp - data.flow_rate
            
            dt = 0.1  # default 100ms
            if self.sim_last_time is not None:
                dt = data.timestamp - self.sim_last_time
                if dt <= 0: dt = 0.1
                
            self.sim_integral += current_error * dt
            
            p_val = kp * current_error
            i_val = ki * self.sim_integral
            derivative = (current_error - self.sim_last_error) / dt if dt > 0 else 0.0
            d_val = kd * derivative
            
            self.sim_last_error = current_error
            self.sim_last_time = data.timestamp
            
            # Plot simulated values
            self.flow_chart.update_series("Reference", data.timestamp, sp)
            self.flow_chart.update_series("Actual Flow", data.timestamp, data.flow_rate)
            
            self.pid_chart.update_series("P", data.timestamp, p_val)
            self.pid_chart.update_series("I", data.timestamp, i_val)
            self.pid_chart.update_series("D", data.timestamp, d_val)
        else:
            # --- REAL ARDUINO PID LOGIC ---
            self.flow_chart.update_series("Reference", data.timestamp, data.reference)
            self.flow_chart.update_series("Actual Flow", data.timestamp, data.flow_rate)
            
            self.pid_chart.update_series("P", data.timestamp, data.pid_p)
            self.pid_chart.update_series("I", data.timestamp, data.pid_i)
            self.pid_chart.update_series("D", data.timestamp, data.pid_d)

    def _toggle_sim(self, checked: bool):
        """Toggle the virtual PID simulation overlay."""
        self.sim_active = checked
        if checked:
            # Reset simulation state when enabled
            self.sim_last_time = None
            self.sim_integral = 0.0
            self.sim_last_error = 0.0
            self.sim_toggle.setText("🟢 Virtual PID: ON")
            self.sim_toggle.setStyleSheet("background-color: #2b7a0b; color: white; border-radius: 4px; padding: 5px;")
        else:
            self.sim_toggle.setText("🧪 Virtual PID (Tuning Mode)")
            self.sim_toggle.setStyleSheet("")

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
