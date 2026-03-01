"""
Application state management with reactive signals.
"""
from PyQt6.QtCore import QObject, pyqtSignal
from collections import deque
from typing import Deque, Optional
import csv
from datetime import datetime

from core.models import (
    TelemetryData,
    StatusMessage,
    MetricsData,
    PIDParameters,
    DeviceInfo,
    ControlMode
)


class AppState(QObject):
    """Central application state with reactive updates."""
    
    # Signals for state changes
    telemetry_updated = pyqtSignal()
    metrics_updated = pyqtSignal()
    connection_changed = pyqtSignal(bool)
    mode_changed = pyqtSignal(ControlMode)
    logging_changed = pyqtSignal(bool)
    
    MAX_HISTORY = 1000  # Keep last 1000 telemetry points
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
        # Current telemetry
        self.current_telemetry: Optional[TelemetryData] = None
        self.telemetry_history: Deque[TelemetryData] = deque(maxlen=self.MAX_HISTORY)
        
        # Status logs
        self.status_logs: Deque[StatusMessage] = deque(maxlen=500)
        
        # Current metrics
        self.current_metrics: Optional[MetricsData] = None
        
        # Device info
        self.device = DeviceInfo()
        
        # PID parameters
        self.pid_flow = PIDParameters(kp=50.0, ki=10.0, kd=5.0)
        self.pid_level1 = PIDParameters(kp=30.0, ki=5.0, kd=3.0)
        self.pid_level2 = PIDParameters(kp=30.0, ki=5.0, kd=3.0)
        
        # Control state
        self.current_mode = ControlMode.MANUAL
        self.is_logging = False
        self.control_active = False
        
        # Settings
        self.reduce_animations = False
        self.chart_update_rate = 30  # FPS for chart updates
        self.export_path = ""
    
    def update_telemetry(self, data: TelemetryData):
        """Update telemetry data and history."""
        self.current_telemetry = data
        self.telemetry_history.append(data)
        
        # Update mode if changed
        if data.mode != self.current_mode:
            self.current_mode = data.mode
            self.mode_changed.emit(data.mode)
        
        self.telemetry_updated.emit()
    
    def add_status(self, status: StatusMessage):
        """Add status message to logs."""
        self.status_logs.append(status)
        
        # Parse status for state updates
        if status.level == "MODE":
            try:
                mode_name = status.message.strip()
                self.current_mode = ControlMode(mode_name)
                self.mode_changed.emit(self.current_mode)
            except ValueError:
                pass
        
        elif status.level == "LOG":
            if "logging iniciado" in status.message.lower():
                self.is_logging = True
                self.logging_changed.emit(True)
            elif "logging detenido" in status.message.lower():
                self.is_logging = False
                self.logging_changed.emit(False)
        
        elif status.level == "CTRL":
            if "iniciado" in status.message.lower():
                self.control_active = True
            elif "detenido" in status.message.lower():
                self.control_active = False
    
    def update_metrics(self, metrics: MetricsData):
        """Update performance metrics."""
        self.current_metrics = metrics
        self.metrics_updated.emit()
    
    def set_connection(self, connected: bool, port: str = ""):
        """Update connection state."""
        self.device.connected = connected
        if port:
            self.device.port = port
        self.connection_changed.emit(connected)
    
    def clear_history(self):
        """Clear telemetry history."""
        self.telemetry_history.clear()
        self.telemetry_updated.emit()
    
    def clear_logs(self):
        """Clear status logs."""
        self.status_logs.clear()
    
    def export_telemetry_csv(self, filepath: str) -> bool:
        """Export telemetry history to CSV file.
        
        Returns:
            True if successful, False otherwise
        """
        try:
            with open(filepath, 'w', newline='') as f:
                writer = csv.writer(f)
                
                # Header
                writer.writerow([
                    'LocalTime', 'ArduinoTime_s', 'Mode', 'RefType', 'Reference',
                    'Flow_Lmin', 'Level1_mm', 'Level2_mm', 'PWM',
                    'PID_P', 'PID_I', 'PID_D', 'Error', 'Volume_L'
                ])
                
                # Data rows
                for data in self.telemetry_history:
                    writer.writerow([
                        data.local_time.isoformat(),
                        data.timestamp,
                        data.mode.value,
                        data.ref_type.value,
                        data.reference,
                        data.flow_rate,
                        data.level_tank1,
                        data.level_tank2,
                        data.pwm,
                        data.pid_p,
                        data.pid_i,
                        data.pid_d,
                        data.error,
                        data.volume
                    ])
            
            return True
        except Exception as e:
            print(f"Export failed: {e}")
            return False
    
    def get_telemetry_arrays(self, max_points: int = None):
        """Get telemetry data as numpy-compatible arrays for plotting.
        
        Returns:
            Dictionary of arrays: {
                'time': [...],
                'flow': [...],
                'level1': [...],
                'level2': [...],
                'pwm': [...],
                'reference': [...],
                'pid_p': [...],
                'pid_i': [...],
                'pid_d': [...]
            }
        """
        if not self.telemetry_history:
            return {key: [] for key in ['time', 'flow', 'level1', 'level2', 
                                         'pwm', 'reference', 'pid_p', 'pid_i', 'pid_d']}
        
        # Limit data points for performance
        data_slice = list(self.telemetry_history)
        if max_points and len(data_slice) > max_points:
            # Downsample by taking every nth point
            step = len(data_slice) // max_points
            data_slice = data_slice[::step]
        
        return {
            'time': [d.timestamp for d in data_slice],
            'flow': [d.flow_rate for d in data_slice],
            'level1': [d.level_tank1 for d in data_slice],
            'level2': [d.level_tank2 for d in data_slice],
            'pwm': [d.pwm for d in data_slice],
            'reference': [d.reference for d in data_slice],
            'pid_p': [d.pid_p for d in data_slice],
            'pid_i': [d.pid_i for d in data_slice],
            'pid_d': [d.pid_d for d in data_slice]
        }
