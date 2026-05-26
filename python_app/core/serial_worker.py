"""
Serial communication worker running in separate thread.
"""
import serial
import serial.tools.list_ports
from PyQt6.QtCore import QThread, pyqtSignal, QMutex, QWaitCondition
from typing import List, Optional
import time

from core.protocol import ProtocolParser
from core.models import TelemetryData, StatusMessage, MetricsData, LiveData


class SerialWorker(QThread):
    """Non-blocking serial communication thread."""
    
    # Signals
    telemetry_received = pyqtSignal(TelemetryData)
    live_data_received = pyqtSignal(LiveData)
    status_received = pyqtSignal(StatusMessage)
    metrics_received = pyqtSignal(MetricsData)
    connection_changed = pyqtSignal(bool)  # True=connected, False=disconnected
    error_occurred = pyqtSignal(str)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.serial_port: Optional[serial.Serial] = None
        self.parser = ProtocolParser()
        
        self.port_name = ""
        self.baud_rate = 115200
        self.running = False
        self.connected = False
        
        # Thread-safe command queue
        self.command_queue = []
        self.queue_mutex = QMutex()
        self.queue_condition = QWaitCondition()
        
        # Rate limiting
        self.last_command_time = 0
        self.min_command_interval = 0.05  # 50ms between commands
    
    def connect(self, port: str, baud_rate: int = 115200):
        """Connect to serial port."""
        self.port_name = port
        self.baud_rate = baud_rate
        
        if not self.isRunning():
            self.running = True
            self.start()
        else:
            self._reconnect()
    
    def disconnect(self):
        """Disconnect from serial port."""
        self.running = False
        self._close_port()
        if self.isRunning():
            self.quit()
            self.wait(2000)  # Wait up to 2 seconds
    
    def send_command(self, command: str):
        """Queue command to send to Arduino.
        
        Args:
            command: Command string (should end with newline)
        """
        self.queue_mutex.lock()
        self.command_queue.append(command)
        self.queue_condition.wakeOne()
        self.queue_mutex.unlock()
    
    def _reconnect(self):
        """Internal reconnect logic."""
        self._close_port()
        self._open_port()
    
    def _open_port(self):
        """Open serial port."""
        try:
            self.serial_port = serial.Serial(
                port=self.port_name,
                baudrate=self.baud_rate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1,  # Non-blocking read with timeout
                write_timeout=0.5
            )
            time.sleep(2)  # Wait for Arduino reset after serial connection
            self.connected = True
            self.connection_changed.emit(True)
            self.parser.reset()
            self.status_received.emit(
                StatusMessage("INFO", f"Connected to {self.port_name} @ {self.baud_rate} baud")
            )
        except serial.SerialException as e:
            self.connected = False
            self.connection_changed.emit(False)
            self.error_occurred.emit(f"Failed to open {self.port_name}: {str(e)}")
    
    def _close_port(self):
        """Close serial port."""
        if self.serial_port and self.serial_port.is_open:
            try:
                self.serial_port.close()
            except Exception as e:
                self.error_occurred.emit(f"Error closing port: {str(e)}")
        
        if self.connected:
            self.connected = False
            self.connection_changed.emit(False)
            self.status_received.emit(StatusMessage("INFO", "Disconnected"))
    
    def run(self):
        """Main thread loop."""
        self._open_port()
        
        while self.running:
            if not self.connected:
                time.sleep(0.5)
                continue
            
            try:
                # Read incoming data
                if self.serial_port and self.serial_port.in_waiting > 0:
                    line = self.serial_port.readline().decode('utf-8', errors='ignore')
                    if line:
                        self._process_line(line)
                
                # Send queued commands (rate limited)
                self._send_queued_commands()
                
                # Small sleep to prevent CPU spinning
                time.sleep(0.001)
                
            except serial.SerialException as e:
                self.error_occurred.emit(f"Serial error: {str(e)}")
                self.connected = False
                self.connection_changed.emit(False)
                time.sleep(1)  # Wait before retry
                self._reconnect()
            
            except Exception as e:
                self.error_occurred.emit(f"Unexpected error: {str(e)}")
        
        self._close_port()
    
    def _process_line(self, line: str):
        """Parse and emit line data."""
        result = self.parser.parse_line(line)
        
        if isinstance(result, TelemetryData):
            self.telemetry_received.emit(result)
        elif isinstance(result, LiveData):
            self.live_data_received.emit(result)
        elif isinstance(result, StatusMessage):
            self.status_received.emit(result)
        elif isinstance(result, MetricsData):
            self.metrics_received.emit(result)
    
    def _send_queued_commands(self):
        """Send commands from queue with rate limiting."""
        if not self.serial_port or not self.serial_port.is_open:
            return
        
        current_time = time.time()
        
        # Rate limiting
        if current_time - self.last_command_time < self.min_command_interval:
            return
        
        self.queue_mutex.lock()
        if self.command_queue:
            command = self.command_queue.pop(0)
            self.queue_mutex.unlock()
            
            try:
                self.serial_port.write(command.encode('utf-8'))
                self.serial_port.flush()
                self.last_command_time = current_time
                
                # Echo sent command
                self.status_received.emit(
                    StatusMessage("CMD", f"→ {command.strip()}")
                )
            except Exception as e:
                self.error_occurred.emit(f"Send failed: {str(e)}")
        else:
            self.queue_mutex.unlock()
    
    @staticmethod
    def list_available_ports() -> List[str]:
        """Get list of available serial ports."""
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]
