"""
Data models for hydraulic control system telemetry.
"""
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
from typing import Optional


class ControlMode(Enum):
    """Control system operating modes."""
    MANUAL = "MANUAL"
    AUTO_FLOW = "AUTO_FLOW"
    AUTO_LEVEL1 = "AUTO_LEVEL1"
    AUTO_LEVEL2 = "AUTO_LEVEL2"
    CASCADE = "CASCADE"


class ReferenceType(Enum):
    """Reference trajectory types."""
    CONSTANT = 0
    STEP = 1
    RAMP = 2
    PARABOLIC = 3
    SINE = 4


@dataclass
class TelemetryData:
    """Complete telemetry snapshot from Arduino."""
    timestamp: float
    mode: ControlMode
    ref_type: ReferenceType
    reference: float
    flow_rate: float
    level_tank1: float
    level_tank2: float
    pwm: int
    pid_p: float
    pid_i: float
    pid_d: float
    error: float
    volume: float
    local_time: datetime = field(default_factory=datetime.now)
    
    @classmethod
    def from_csv_line(cls, line: str) -> Optional['TelemetryData']:
        """Parse CSV telemetry line from Arduino.
        
        Format: Time_s,Mode,RefType,Ref,Flow,Level1,Level2,PWM,PID_P,PID_I,PID_D,Error,Volume
        """
        try:
            parts = line.strip().split(',')
            if len(parts) != 13:
                return None
            
            return cls(
                timestamp=float(parts[0]),
                mode=ControlMode(parts[1]),
                ref_type=ReferenceType(int(parts[2])),
                reference=float(parts[3]),
                flow_rate=float(parts[4]),
                level_tank1=float(parts[5]),
                level_tank2=float(parts[6]),
                pwm=int(parts[7]),
                pid_p=float(parts[8]),
                pid_i=float(parts[9]),
                pid_d=float(parts[10]),
                error=float(parts[11]),
                volume=float(parts[12])
            )
        except (ValueError, IndexError, KeyError):
            return None


@dataclass
class StatusMessage:
    """Status/log message from Arduino."""
    level: str  # INFO, ERROR, CMD, CTRL, PID, METRICS, MODE, LOG
    message: str
    timestamp: datetime = field(default_factory=datetime.now)
    
    @property
    def formatted(self) -> str:
        """Formatted message with timestamp."""
        time_str = self.timestamp.strftime("%H:%M:%S.%f")[:-3]
        return f"[{time_str}] [{self.level}] {self.message}"


@dataclass
class MetricsData:
    """Performance metrics from Arduino."""
    overshoot: Optional[float] = None
    rise_time: Optional[float] = None
    settling_time: Optional[float] = None
    steady_state_error: Optional[float] = None
    peak_value: Optional[float] = None
    timestamp: datetime = field(default_factory=datetime.now)


@dataclass
class PIDParameters:
    """PID controller parameters."""
    kp: float = 50.0
    ki: float = 10.0
    kd: float = 5.0
    setpoint: float = 1.0
    enabled: bool = False


@dataclass
class DeviceInfo:
    """ESP32 device information."""
    port: str = ""
    baud_rate: int = 115200
    connected: bool = False
    firmware_version: str = "Sprint 5 v2.0"
    uptime: float = 0.0
