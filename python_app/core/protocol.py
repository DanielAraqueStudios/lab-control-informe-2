"""
Protocol parser for Arduino serial communication.
"""
import re
from typing import Optional, Union
from core.models import TelemetryData, StatusMessage, MetricsData


class ProtocolParser:
    """Parse incoming serial data from ESP32-S3."""
    
    # Regex patterns for different message types
    STATUS_PATTERN = re.compile(r'\[(MODE|CTRL|PID|METRICS|ERROR|INFO|CMD|LOG|OK|READY|STEP|EXP|CAL|SERVO1|SERVO2|VALVE|LEVEL|HEIGHT)\]\s*(.+)')
    CSV_HEADER_PATTERN = re.compile(r'^Time_s,Mode,RefType')
    METRICS_OVERSHOOT = re.compile(r'Overshoot.*?:\s*([\d.]+)%')
    METRICS_RISE = re.compile(r'Rise Time.*?:\s*([\d.]+)\s*s')
    METRICS_SETTLING = re.compile(r'Settling Time.*?:\s*([\d.]+)\s*s')
    METRICS_SSE = re.compile(r'estado estable.*?:\s*([\d.]+)%')
    METRICS_PEAK = re.compile(r'pico.*?:\s*([\d.]+)')
    
    def __init__(self):
        self.csv_mode = False
        self.metrics_buffer = MetricsData()
        self.in_metrics_block = False
    
    def parse_line(self, line: str) -> Optional[Union[TelemetryData, StatusMessage, MetricsData]]:
        """Parse a single line from Arduino.
        
        Returns:
            TelemetryData if CSV data line
            StatusMessage if tagged status
            MetricsData if metrics block complete
            None if unrecognized or header
        """
        line = line.strip()
        if not line:
            return None
        
        # Check for CSV header (toggle CSV mode)
        if self.CSV_HEADER_PATTERN.match(line):
            self.csv_mode = True
            return StatusMessage("INFO", "Data logging started", )
        
        # Parse CSV telemetry data
        if self.csv_mode and ',' in line and not line.startswith('['):
            telemetry = TelemetryData.from_csv_line(line)
            if telemetry:
                return telemetry
            else:
                # Invalid CSV, exit CSV mode
                self.csv_mode = False
        
        # Parse tagged status messages
        match = self.STATUS_PATTERN.match(line)
        if match:
            level, message = match.groups()
            
            # Check if entering metrics block
            if "MÉTRICAS DE DESEMPEÑO" in line or "METRICS" in level:
                self.in_metrics_block = True
                self.metrics_buffer = MetricsData()
            
            # Extract metrics values
            if self.in_metrics_block:
                self._extract_metrics(line)
                
                # Check if metrics block complete
                if "═══╝" in line or "════════" in line:
                    self.in_metrics_block = False
                    return self.metrics_buffer
            
            return StatusMessage(level, message.strip())
        
        # Parse untagged lines (might be part of help or status output)
        if line.startswith('║') or line.startswith('╔') or line.startswith('╚'):
            # Table formatting - try to extract metrics
            if self.in_metrics_block:
                self._extract_metrics(line)
            return None  # Skip formatting lines
        
        # Unknown format - return as INFO
        if len(line) > 3:  # Avoid empty or very short debris
            return StatusMessage("INFO", line)
        
        return None
    
    def _extract_metrics(self, line: str):
        """Extract metrics values from text."""
        match = self.METRICS_OVERSHOOT.search(line)
        if match:
            self.metrics_buffer.overshoot = float(match.group(1))
        
        match = self.METRICS_RISE.search(line)
        if match:
            self.metrics_buffer.rise_time = float(match.group(1))
        
        match = self.METRICS_SETTLING.search(line)
        if match:
            self.metrics_buffer.settling_time = float(match.group(1))
        
        match = self.METRICS_SSE.search(line)
        if match:
            self.metrics_buffer.steady_state_error = float(match.group(1))
        
        match = self.METRICS_PEAK.search(line)
        if match:
            self.metrics_buffer.peak_value = float(match.group(1))
    
    def reset(self):
        """Reset parser state."""
        self.csv_mode = False
        self.in_metrics_block = False


class CommandBuilder:
    """Build commands to send to Arduino."""
    
    @staticmethod
    def set_mode(mode: str) -> str:
        """Set control mode.
        
        Args:
            mode: MANUAL, AUTO_FLOW, AUTO_LEVEL1, AUTO_LEVEL2, CASCADE
        """
        return f"SETMODE,{mode.upper()}\n"
    
    @staticmethod
    def set_reference(ref_type: str, initial: float, final: float = None, duration: float = None) -> str:
        """Set reference trajectory.
        
        Args:
            ref_type: STEP, RAMP, PARA
            initial: Initial value
            final: Final value (for STEP/RAMP/PARA)
            duration: Duration in seconds (for RAMP/PARA)
        """
        if ref_type.upper() == "STEP":
            return f"SETREF,STEP,{initial},{final},{duration}\n"
        elif ref_type.upper() == "RAMP":
            return f"SETREF,RAMP,{initial},{final},{duration}\n"
        elif ref_type.upper() == "PARA":
            return f"SETREF,PARA,{initial},{final},{duration}\n"
        else:
            return f"SETREF,STEP,{initial},{initial},0\n"
    
    @staticmethod
    def set_pid(controller: int, kp: float, ki: float, kd: float) -> str:
        """Set PID parameters.
        
        Args:
            controller: 1 (Level1) or 2 (Level2)
            kp, ki, kd: PID gains
        """
        return f"SETPID{controller},{kp},{ki},{kd}\n"
    
    @staticmethod
    def start_control() -> str:
        """Start automatic control."""
        return "STARTCTRL\n"
    
    @staticmethod
    def stop_control() -> str:
        """Stop control and motor."""
        return "STOPCTRL\n"
    
    @staticmethod
    def toggle_datalog() -> str:
        """Toggle data logging."""
        return "DATALOG\n"
    
    @staticmethod
    def toggle_metrics() -> str:
        """Toggle metrics measurement."""
        return "METRICS\n"
    
    @staticmethod
    def get_status() -> str:
        """Request status dump."""
        return "STATUS\n"
    
    @staticmethod
    def run_experiment(name: str) -> str:
        """Run predefined experiment.
        
        Args:
            name: STEP_FLOW, RAMP_LEVEL, DISTURBANCE
        """
        return f"EXPERIMENT,{name.upper()}\n"
    
    @staticmethod
    def set_pwm(value: int) -> str:
        """Set manual PWM (0-255). Only works in MANUAL mode."""
        value = max(0, min(255, int(value)))
        return f"SETPWM,{value}\n"

    @staticmethod
    def cal_mode(pwm_pct: int = 60) -> str:
        """Enter calibration mode.
        
        Args:
            pwm_pct: Motor duty cycle percentage (0-100). Default 60%.
        """
        pwm_pct = max(0, min(100, int(pwm_pct)))
        return f"CALMODE,{pwm_pct}\n"

    @staticmethod
    def cal_stop() -> str:
        """Exit calibration mode and stop motor."""
        return "CALSTOP\n"

    @staticmethod
    def set_calibration(tank: int, empty_adc: float, full_adc: float) -> str:
        """Send calibration points to Arduino.
        
        Args:
            tank:      1 or 2
            empty_adc: Raw ADC value when tank is empty
            full_adc:  Raw ADC value when tank is full
        """
        return f"SETCAL,{tank},{int(empty_adc)},{int(full_adc)}\n"

    @staticmethod
    def get_calibration() -> str:
        """Request current calibration values from Arduino."""
        return "GETCAL\n"

    @staticmethod
    def servo_angle(servo: int, angle: int) -> str:
        """Set a positional servo angle."""
        servo = 1 if int(servo) == 1 else 2
        angle = max(0, min(180, int(angle)))
        return f"S{servo},{angle}\n"

    @staticmethod
    def servo_both(angle1: int, angle2: int) -> str:
        """Set both positional servo angles."""
        angle1 = max(0, min(180, int(angle1)))
        angle2 = max(0, min(180, int(angle2)))
        return f"BOTH,{angle1},{angle2}\n"

    @staticmethod
    def servo_direction(servo: int, direction: str) -> str:
        """Move servo to LEFT, CENTER, or RIGHT position."""
        servo = 1 if int(servo) == 1 else 2
        direction = direction.upper()
        if direction not in {"LEFT", "CENTER", "RIGHT"}:
            direction = "CENTER"
        return f"DIR,{servo},{direction}\n"

    @staticmethod
    def servo_step(servo: int, direction: str) -> str:
        """Step servo LEFT or RIGHT."""
        servo = 1 if int(servo) == 1 else 2
        direction = direction.upper()
        if direction not in {"LEFT", "RIGHT"}:
            direction = "RIGHT"
        return f"STEP,{servo},{direction}\n"

    @staticmethod
    def servo_continuous(servo: int, direction: str, speed: int) -> str:
        """Run continuous-rotation servo clockwise/counterclockwise."""
        servo = 1 if int(servo) == 1 else 2
        direction = direction.upper()
        if direction not in {"CW", "CCW"}:
            direction = "CW"
        speed = max(0, min(100, int(speed)))
        return f"CR,{servo},{direction},{speed}\n"

    @staticmethod
    def servo_stop(servo: int) -> str:
        """Send neutral pulse to stop a continuous-rotation servo."""
        servo = 1 if int(servo) == 1 else 2
        return f"STOP,{servo}\n"

    @staticmethod
    def servo_disable(servo: int) -> str:
        """Disable servo PWM output."""
        servo = 1 if int(servo) == 1 else 2
        return f"DISABLE,{servo}\n"

    @staticmethod
    def set_level_target(tank: int, height_mm: float) -> str:
        """Set Sprint 09 ultrasonic height target for a tank."""
        tank = 1 if int(tank) == 1 else 2
        height_mm = max(0.0, float(height_mm))
        return f"SETLEVEL{tank},{height_mm:.1f}\n"

    @staticmethod
    def level_control(enabled: bool) -> str:
        """Enable or disable Sprint 09 automatic servo valve height control."""
        return "LEVELCTRL,ON\n" if enabled else "LEVELCTRL,OFF\n"

    @staticmethod
    def valve_status() -> str:
        """Request Sprint 09 servo valve height-control status."""
        return "VALVESTATUS\n"

    @staticmethod
    def valve_command(tank: int, action: str) -> str:
        """Open or close a Sprint 09 tank valve."""
        tank = 1 if int(tank) == 1 else 2
        action = action.upper()
        if action not in {"OPEN", "CLOSE"}:
            action = "CLOSE"
        return f"V{tank},{action}\n"

    @staticmethod
    def help_command() -> str:
        """Request help/command list."""
        return "HELP\n"
