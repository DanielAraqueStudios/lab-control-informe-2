"""
Control panel page for sending commands and adjusting parameters.
"""
import re
from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton,
                              QComboBox, QGroupBox, QFormLayout, QDoubleSpinBox,
                              QSlider, QSpinBox, QRadioButton, QButtonGroup, QTabWidget,
                              QFrame, QScrollArea, QGridLayout)
from PyQt6.QtCore import Qt
from core import CommandBuilder, ControlMode
from viewmodels import AppState
from core.serial_worker import SerialWorker


class ControlsPage(QWidget):
    """Main control panel for system operation."""
    
    def __init__(self, app_state: AppState, serial_worker: SerialWorker, parent=None):
        super().__init__(parent)
        self.app_state = app_state
        self.serial_worker = serial_worker

        # --- Calibration state ---
        self.cal_t1_empty_adc: float = 0.0
        self.cal_t1_full_adc:  float = 4095.0
        self.cal_t2_empty_adc: float = 0.0
        self.cal_t2_full_adc:  float = 4095.0
        self._cur_t1_adc: float = 0.0
        self._cur_t2_adc: float = 0.0
        self.servo_angle_sliders = {}
        self.servo_angle_spins = {}
        self.servo_speed_spins = {}

        # Main layout
        layout = QVBoxLayout(self)
        layout.setContentsMargins(24, 24, 24, 24)
        layout.setSpacing(24)
        
        # Page title
        title = QLabel("Control Panel")
        title.setObjectName("heading1")
        layout.addWidget(title)
        
        # Tab widget for organized controls
        tabs = QTabWidget()
        
        # Tab 1: Mode & Control
        tabs.addTab(self.create_mode_control_tab(), "Mode & Control")
        
        # Tab 2: PID Tuning
        tabs.addTab(self.create_pid_tuning_tab(), "PID Tuning")
        
        # Tab 3: References
        tabs.addTab(self.create_reference_tab(), "References")
        
        # Tab 4: Experiments
        tabs.addTab(self.create_experiments_tab(), "Experiments")

        # Tab 5: Calibration
        tabs.addTab(self.create_calibration_tab(), "🔧 Calibration")

        # Tab 6: Servo control
        tabs.addTab(self.create_servo_control_tab(), "Servo Control")

        # Connect status messages to calibration live display
        self.serial_worker.status_received.connect(self._parse_cal_message)

        layout.addWidget(tabs, stretch=1)
        
        # Emergency stop button (always visible)
        estop_btn = QPushButton("🛑 EMERGENCY STOP")
        estop_btn.setObjectName("dangerButton")
        estop_btn.setMinimumHeight(50)
        estop_btn.clicked.connect(self.emergency_stop)
        layout.addWidget(estop_btn)
    
    def create_mode_control_tab(self) -> QWidget:
        """Create mode selection and control tab."""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setSpacing(16)
        
        # Mode selection group
        mode_group = QGroupBox("Control Mode")
        mode_layout = QVBoxLayout()
        
        self.mode_buttons = QButtonGroup()
        modes = [
            ("MANUAL", "Manual PWM control (no feedback)"),
            ("AUTO_FLOW", "PID control of flow rate"),
            ("AUTO_LEVEL1", "PID control of Tank 1 level"),
            ("AUTO_LEVEL2", "PID control of Tank 2 level"),
            ("CASCADE", "Cascaded control (Level→Flow→Motor)")
        ]
        
        for i, (mode, desc) in enumerate(modes):
            radio = QRadioButton(f"{mode}")
            radio.setToolTip(desc)
            radio.toggled.connect(lambda checked, m=mode: self.set_mode(m) if checked else None)
            self.mode_buttons.addButton(radio, i)
            mode_layout.addWidget(radio)
        
        # NOTE: setChecked(True) is called AFTER manual_pwm_group is created below
        # to avoid AttributeError when the toggled signal fires.

        mode_group.setLayout(mode_layout)
        layout.addWidget(mode_group)

        # ── Manual PWM control (only active in MANUAL mode) ──────────────
        self.manual_pwm_group = QGroupBox("Manual Velocity (PWM)")
        pwm_layout = QVBoxLayout()

        # Slider row
        slider_row = QHBoxLayout()
        self.pwm_slider = QSlider(Qt.Orientation.Horizontal)
        self.pwm_slider.setRange(0, 255)
        self.pwm_slider.setValue(0)
        self.pwm_slider.setTickInterval(25)
        self.pwm_slider.setTickPosition(QSlider.TickPosition.TicksBelow)
        slider_row.addWidget(self.pwm_slider, stretch=1)

        self.pwm_spin = QSpinBox()
        self.pwm_spin.setRange(0, 255)
        self.pwm_spin.setValue(0)
        self.pwm_spin.setFixedWidth(70)
        slider_row.addWidget(self.pwm_spin)
        pwm_layout.addLayout(slider_row)

        # Percentage label
        self.pwm_pct_label = QLabel("0 / 255  —  0.0%")
        self.pwm_pct_label.setObjectName("caption")
        pwm_layout.addWidget(self.pwm_pct_label)

        # Send button
        send_pwm_btn = QPushButton("⚡ Send PWM")
        send_pwm_btn.clicked.connect(self.send_manual_pwm)
        pwm_layout.addWidget(send_pwm_btn)

        # Sync slider ↔ spinbox
        self.pwm_slider.valueChanged.connect(self._on_slider_changed)
        self.pwm_spin.valueChanged.connect(self._on_spin_changed)

        self.manual_pwm_group.setLayout(pwm_layout)
        layout.addWidget(self.manual_pwm_group)

        # Set MANUAL as default (after group exists so set_mode() doesn't crash)
        self.mode_buttons.button(0).setChecked(True)

        # Control actions group
        ctrl_group = QGroupBox("Control Actions")
        ctrl_layout = QVBoxLayout()
        
        btn_row = QHBoxLayout()
        
        start_btn = QPushButton("▶ Start Control")
        start_btn.clicked.connect(self.start_control)
        btn_row.addWidget(start_btn)
        
        stop_btn = QPushButton("⏹ Stop Control")
        stop_btn.setObjectName("secondaryButton")
        stop_btn.clicked.connect(self.stop_control)
        btn_row.addWidget(stop_btn)
        
        ctrl_layout.addLayout(btn_row)
        
        # Data logging toggle
        log_btn = QPushButton("📊 Toggle Data Logging")
        log_btn.setObjectName("secondaryButton")
        log_btn.clicked.connect(self.toggle_logging)
        ctrl_layout.addWidget(log_btn)
        
        # Metrics toggle
        metrics_btn = QPushButton("📈 Toggle Metrics")
        metrics_btn.setObjectName("secondaryButton")
        metrics_btn.clicked.connect(self.toggle_metrics)
        ctrl_layout.addWidget(metrics_btn)
        
        ctrl_group.setLayout(ctrl_layout)
        layout.addWidget(ctrl_group)
        
        layout.addStretch()
        return widget
    
    def create_pid_tuning_tab(self) -> QWidget:
        """Create PID parameter tuning tab."""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setSpacing(16)
        
        # PID Flow parameters
        flow_group = QGroupBox("PID Flow Controller")
        flow_layout = QFormLayout()
        
        self.flow_kp = QDoubleSpinBox()
        self.flow_kp.setRange(0, 200)
        self.flow_kp.setValue(50.0)
        self.flow_kp.setSingleStep(1.0)
        flow_layout.addRow("Kp (Proportional):", self.flow_kp)
        
        self.flow_ki = QDoubleSpinBox()
        self.flow_ki.setRange(0, 100)
        self.flow_ki.setValue(10.0)
        self.flow_ki.setSingleStep(0.5)
        flow_layout.addRow("Ki (Integral):", self.flow_ki)
        
        self.flow_kd = QDoubleSpinBox()
        self.flow_kd.setRange(0, 50)
        self.flow_kd.setValue(5.0)
        self.flow_kd.setSingleStep(0.5)
        flow_layout.addRow("Kd (Derivative):", self.flow_kd)
        
        apply_flow_btn = QPushButton("Apply Flow PID")
        apply_flow_btn.clicked.connect(self.apply_flow_pid)
        flow_layout.addRow("", apply_flow_btn)
        
        flow_group.setLayout(flow_layout)
        layout.addWidget(flow_group)
        
        # PID Level1 parameters
        level1_group = QGroupBox("PID Level 1 Controller")
        level1_layout = QFormLayout()
        
        self.level1_kp = QDoubleSpinBox()
        self.level1_kp.setRange(0, 200)
        self.level1_kp.setValue(30.0)
        self.level1_kp.setSingleStep(1.0)
        level1_layout.addRow("Kp:", self.level1_kp)
        
        self.level1_ki = QDoubleSpinBox()
        self.level1_ki.setRange(0, 100)
        self.level1_ki.setValue(5.0)
        self.level1_ki.setSingleStep(0.5)
        level1_layout.addRow("Ki:", self.level1_ki)
        
        self.level1_kd = QDoubleSpinBox()
        self.level1_kd.setRange(0, 50)
        self.level1_kd.setValue(3.0)
        self.level1_kd.setSingleStep(0.5)
        level1_layout.addRow("Kd:", self.level1_kd)
        
        apply_level1_btn = QPushButton("Apply Level 1 PID")
        apply_level1_btn.clicked.connect(self.apply_level1_pid)
        level1_layout.addRow("", apply_level1_btn)
        
        level1_group.setLayout(level1_layout)
        layout.addWidget(level1_group)
        
        layout.addStretch()
        return widget
    
    def create_reference_tab(self) -> QWidget:
        """Create reference trajectory tab."""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setSpacing(16)
        
        # Reference type selection
        type_group = QGroupBox("Reference Type")
        type_layout = QFormLayout()
        
        self.ref_combo = QComboBox()
        self.ref_combo.addItems(["STEP", "RAMP", "PARA (Parabolic)"])
        self.ref_combo.currentTextChanged.connect(self.on_ref_type_changed)
        type_layout.addRow("Type:", self.ref_combo)
        
        type_group.setLayout(type_layout)
        layout.addWidget(type_group)
        
        # Reference parameters
        params_group = QGroupBox("Parameters")
        params_layout = QFormLayout()
        
        self.ref_initial = QDoubleSpinBox()
        self.ref_initial.setRange(0, 150)
        self.ref_initial.setValue(0.0)
        self.ref_initial.setSingleStep(5.0)
        self.ref_initial.setDecimals(1)
        params_layout.addRow("Initial Value:", self.ref_initial)
        
        self.ref_final = QDoubleSpinBox()
        self.ref_final.setRange(0, 150)
        self.ref_final.setValue(60.0)
        self.ref_final.setSingleStep(5.0)
        self.ref_final.setDecimals(1)
        params_layout.addRow("Final Value:", self.ref_final)
        
        self.ref_duration = QDoubleSpinBox()
        self.ref_duration.setRange(0.0, 300)
        self.ref_duration.setValue(2.0)
        self.ref_duration.setSingleStep(1.0)
        self.ref_duration.setSuffix(" s")
        params_layout.addRow("Duration:", self.ref_duration)
        
        apply_ref_btn = QPushButton("Apply Reference")
        apply_ref_btn.clicked.connect(self.apply_reference)
        params_layout.addRow("", apply_ref_btn)
        
        params_group.setLayout(params_layout)
        layout.addWidget(params_group)
        
        layout.addStretch()
        return widget
    
    def create_experiments_tab(self) -> QWidget:
        """Create predefined experiments tab."""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setSpacing(16)
        
        exp_group = QGroupBox("Predefined Experiments")
        exp_layout = QVBoxLayout()
        
        experiments = [
            ("STEP_FLOW", "Step Response - Flow Control", 
             "Applies step change 0.5→1.5 L/min with automatic logging"),
            ("RAMP_LEVEL", "Ramp Tracking - Level Control", 
             "Ramp from 10→30 mm over 30 seconds"),
            ("DISTURBANCE", "Disturbance Rejection Test", 
             "Simulates disturbance by reducing PWM temporarily")
        ]
        
        for exp_name, exp_title, exp_desc in experiments:
            btn = QPushButton(f"🧪 {exp_title}")
            btn.setToolTip(exp_desc)
            btn.clicked.connect(lambda checked, name=exp_name: self.run_experiment(name))
            exp_layout.addWidget(btn)
        
        exp_group.setLayout(exp_layout)
        layout.addWidget(exp_group)
        
        # Info label
        info = QLabel("⚠ Experiments run automatically for 60 seconds.\n"
                     "Ensure system is connected and ready before starting.")
        info.setObjectName("caption")
        info.setWordWrap(True)
        layout.addWidget(info)
        
        layout.addStretch()
        return widget
    
    # ── PWM slider/spin sync ──────────────────────────────────────────────
    def _on_slider_changed(self, value: int):
        self.pwm_spin.blockSignals(True)
        self.pwm_spin.setValue(value)
        self.pwm_spin.blockSignals(False)
        self.pwm_pct_label.setText(f"{value} / 255  —  {value/255*100:.1f}%")
        self.send_manual_pwm()

    def _on_spin_changed(self, value: int):
        self.pwm_slider.blockSignals(True)
        self.pwm_slider.setValue(value)
        self.pwm_slider.blockSignals(False)
        self.pwm_pct_label.setText(f"{value} / 255  —  {value/255*100:.1f}%")
        self.send_manual_pwm()

    def send_manual_pwm(self):
        """Send SETPWM command to Arduino."""
        cmd = CommandBuilder.set_pwm(self.pwm_spin.value())
        self.serial_worker.send_command(cmd)

    # Command methods
    def set_mode(self, mode: str):
        """Send mode change command and enable/disable PWM panel."""
        is_manual = (mode == "MANUAL")
        self.manual_pwm_group.setEnabled(is_manual)
        cmd = CommandBuilder.set_mode(mode)
        self.serial_worker.send_command(cmd)
    
    def start_control(self):
        """Start automatic control ensuring a bumpless transfer."""
        if self.app_state.current_telemetry:
            telemetry = self.app_state.current_telemetry
            mode = self.app_state.current_mode
            
            # Auto-populate the initial value from current sensor readings for a smooth transition
            if mode in [ControlMode.CASCADE, ControlMode.AUTO_LEVEL1]:
                self.ref_initial.setValue(telemetry.level_tank1)
            elif mode == ControlMode.AUTO_LEVEL2:
                self.ref_initial.setValue(telemetry.level_tank2)
            elif mode == ControlMode.AUTO_FLOW:
                self.ref_initial.setValue(telemetry.flow_rate)

        # 1) Send the reference currently visible in the UI
        self.apply_reference()
        # 2) Let it process briefly (optional, usually serial buffer handles it)
        # 3) Start the control
        cmd = CommandBuilder.start_control()
        self.serial_worker.send_command(cmd)
    
    def stop_control(self):
        """Stop control."""
        cmd = CommandBuilder.stop_control()
        self.serial_worker.send_command(cmd)
    
    def toggle_logging(self):
        """Toggle data logging."""
        cmd = CommandBuilder.toggle_datalog()
        self.serial_worker.send_command(cmd)
    
    def toggle_metrics(self):
        """Toggle metrics measurement."""
        cmd = CommandBuilder.toggle_metrics()
        self.serial_worker.send_command(cmd)
    
    def apply_flow_pid(self):
        """Apply flow PID parameters (Arduino Sprint 5 uses global flow PID, not SETPID)."""
        # Note: Sprint 5 doesn't have SETPID for flow, only SETPID1/SETPID2 for levels
        # This would require custom implementation or use SETPID1 for level
        pass
    
    def apply_level1_pid(self):
        """Apply Level 1 PID parameters."""
        kp = self.level1_kp.value()
        ki = self.level1_ki.value()
        kd = self.level1_kd.value()
        cmd = CommandBuilder.set_pid(1, kp, ki, kd)
        self.serial_worker.send_command(cmd)
    
    def apply_reference(self):
        """Apply reference trajectory."""
        ref_type = self.ref_combo.currentText().split()[0]  # Get STEP/RAMP/PARA
        initial = self.ref_initial.value()
        final = self.ref_final.value()
        duration = self.ref_duration.value()
        
        cmd = CommandBuilder.set_reference(ref_type, initial, final, duration)
        self.serial_worker.send_command(cmd)
    
    def run_experiment(self, name: str):
        """Run predefined experiment."""
        cmd = CommandBuilder.run_experiment(name)
        self.serial_worker.send_command(cmd)
    
    def emergency_stop(self):
        """Emergency stop - immediately stop control and motor."""
        self.serial_worker.send_command(CommandBuilder.stop_control())
    
    def on_ref_type_changed(self, ref_type: str):
        """Update UI based on reference type."""
        # All types use the same parameters for now
        pass

    # =========================================================================
    # SERVO CONTROL TAB
    # =========================================================================

    def create_servo_control_tab(self) -> QWidget:
        """Create controls for Sprint 7 dual-servo firmware."""
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        inner = QWidget()
        layout = QVBoxLayout(inner)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(16)
        scroll.setWidget(inner)

        info = QLabel("Upload Sprint 7 firmware before using these controls: arduino/07_servo_control/07_servo_control.ino")
        info.setObjectName("caption")
        info.setWordWrap(True)
        layout.addWidget(info)

        layout.addWidget(self._create_servo_group(1, "Servo 1 - GPIO10"))
        layout.addWidget(self._create_servo_group(2, "Servo 2 - GPIO11"))

        both_group = QGroupBox("Both Servos")
        both_layout = QHBoxLayout(both_group)
        both_layout.addWidget(QLabel("Send current angles:"))
        both_btn = QPushButton("Send Both")
        both_btn.clicked.connect(self.send_both_servos)
        both_layout.addWidget(both_btn)
        both_layout.addStretch()
        layout.addWidget(both_group)

        layout.addStretch()
        return scroll

    def _create_servo_group(self, servo: int, title: str) -> QGroupBox:
        group = QGroupBox(title)
        layout = QVBoxLayout(group)
        layout.setSpacing(12)

        angle_row = QHBoxLayout()
        angle_row.addWidget(QLabel("Angle:"))

        slider = QSlider(Qt.Orientation.Horizontal)
        slider.setRange(0, 180)
        slider.setValue(90)
        slider.setTickInterval(15)
        slider.setTickPosition(QSlider.TickPosition.TicksBelow)
        angle_row.addWidget(slider, stretch=1)

        spin = QSpinBox()
        spin.setRange(0, 180)
        spin.setValue(90)
        spin.setSuffix("°")
        spin.setFixedWidth(80)
        angle_row.addWidget(spin)

        send_btn = QPushButton("Send Angle")
        send_btn.clicked.connect(lambda checked=False, s=servo: self.send_servo_angle(s))
        angle_row.addWidget(send_btn)
        layout.addLayout(angle_row)

        slider.valueChanged.connect(lambda value, s=servo: self._on_servo_slider_changed(s, value))
        spin.valueChanged.connect(lambda value, s=servo: self._on_servo_spin_changed(s, value))
        self.servo_angle_sliders[servo] = slider
        self.servo_angle_spins[servo] = spin

        position_grid = QGridLayout()
        for col, (label, direction) in enumerate([
            ("Left", "LEFT"),
            ("Center", "CENTER"),
            ("Right", "RIGHT"),
        ]):
            btn = QPushButton(label)
            btn.clicked.connect(lambda checked=False, s=servo, d=direction: self.send_servo_direction(s, d))
            position_grid.addWidget(btn, 0, col)

        step_left = QPushButton("Step Left")
        step_left.setObjectName("secondaryButton")
        step_left.clicked.connect(lambda checked=False, s=servo: self.send_servo_step(s, "LEFT"))
        position_grid.addWidget(step_left, 1, 0)

        step_right = QPushButton("Step Right")
        step_right.setObjectName("secondaryButton")
        step_right.clicked.connect(lambda checked=False, s=servo: self.send_servo_step(s, "RIGHT"))
        position_grid.addWidget(step_right, 1, 2)
        layout.addLayout(position_grid)

        continuous_row = QHBoxLayout()
        continuous_row.addWidget(QLabel("Continuous speed:"))
        speed_spin = QSpinBox()
        speed_spin.setRange(0, 100)
        speed_spin.setValue(60)
        speed_spin.setSuffix("%")
        continuous_row.addWidget(speed_spin)
        self.servo_speed_spins[servo] = speed_spin

        cw_btn = QPushButton("CW")
        cw_btn.clicked.connect(lambda checked=False, s=servo: self.send_servo_continuous(s, "CW"))
        continuous_row.addWidget(cw_btn)

        ccw_btn = QPushButton("CCW")
        ccw_btn.clicked.connect(lambda checked=False, s=servo: self.send_servo_continuous(s, "CCW"))
        continuous_row.addWidget(ccw_btn)

        stop_btn = QPushButton("Stop")
        stop_btn.setObjectName("secondaryButton")
        stop_btn.clicked.connect(lambda checked=False, s=servo: self.send_servo_stop(s))
        continuous_row.addWidget(stop_btn)

        disable_btn = QPushButton("Disable PWM")
        disable_btn.setObjectName("secondaryButton")
        disable_btn.clicked.connect(lambda checked=False, s=servo: self.send_servo_disable(s))
        continuous_row.addWidget(disable_btn)
        continuous_row.addStretch()
        layout.addLayout(continuous_row)

        return group

    def _on_servo_slider_changed(self, servo: int, value: int):
        spin = self.servo_angle_spins[servo]
        spin.blockSignals(True)
        spin.setValue(value)
        spin.blockSignals(False)

    def _on_servo_spin_changed(self, servo: int, value: int):
        slider = self.servo_angle_sliders[servo]
        slider.blockSignals(True)
        slider.setValue(value)
        slider.blockSignals(False)

    def send_servo_angle(self, servo: int):
        angle = self.servo_angle_spins[servo].value()
        self.serial_worker.send_command(CommandBuilder.servo_angle(servo, angle))

    def send_both_servos(self):
        angle1 = self.servo_angle_spins[1].value()
        angle2 = self.servo_angle_spins[2].value()
        self.serial_worker.send_command(CommandBuilder.servo_both(angle1, angle2))

    def send_servo_direction(self, servo: int, direction: str):
        if direction == "LEFT":
            self.servo_angle_spins[servo].setValue(45)
        elif direction == "CENTER":
            self.servo_angle_spins[servo].setValue(90)
        elif direction == "RIGHT":
            self.servo_angle_spins[servo].setValue(135)
        self.serial_worker.send_command(CommandBuilder.servo_direction(servo, direction))

    def send_servo_step(self, servo: int, direction: str):
        current = self.servo_angle_spins[servo].value()
        delta = -10 if direction == "LEFT" else 10
        self.servo_angle_spins[servo].setValue(max(0, min(180, current + delta)))
        self.serial_worker.send_command(CommandBuilder.servo_step(servo, direction))

    def send_servo_continuous(self, servo: int, direction: str):
        speed = self.servo_speed_spins[servo].value()
        self.serial_worker.send_command(CommandBuilder.servo_continuous(servo, direction, speed))

    def send_servo_stop(self, servo: int):
        self.serial_worker.send_command(CommandBuilder.servo_stop(servo))

    def send_servo_disable(self, servo: int):
        self.serial_worker.send_command(CommandBuilder.servo_disable(servo))

    # =========================================================================
    # CALIBRATION TAB
    # =========================================================================

    def create_calibration_tab(self) -> QWidget:
        """Create calibration / test mode tab."""
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        inner = QWidget()
        layout = QVBoxLayout(inner)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(16)
        scroll.setWidget(inner)

        # ---- Flow test group ------------------------------------------------
        flow_group = QGroupBox("⚡ Flow Test")
        flow_layout = QVBoxLayout(flow_group)
        flow_layout.setSpacing(10)

        pwm_row = QHBoxLayout()
        pwm_row.addWidget(QLabel("Pump PWM:"))
        self.cal_pwm_slider = QSlider(Qt.Orientation.Horizontal)
        self.cal_pwm_slider.setRange(0, 100)
        self.cal_pwm_slider.setValue(60)
        self.cal_pwm_spin = QSpinBox()
        self.cal_pwm_spin.setRange(0, 100)
        self.cal_pwm_spin.setValue(60)
        self.cal_pwm_spin.setSuffix("%")
        self.cal_pwm_label = QLabel("153 / 255 PWM")
        self.cal_pwm_label.setObjectName("statusLabel")
        self.cal_pwm_slider.valueChanged.connect(self._on_cal_pwm_slider_changed)
        self.cal_pwm_spin.valueChanged.connect(self._on_cal_pwm_spin_changed)
        pwm_row.addWidget(self.cal_pwm_slider, stretch=1)
        pwm_row.addWidget(self.cal_pwm_spin)
        pwm_row.addWidget(self.cal_pwm_label)
        flow_layout.addLayout(pwm_row)

        btn_row = QHBoxLayout()
        self.cal_start_btn = QPushButton("▶ Start Cal Flow")
        self.cal_start_btn.setObjectName("primaryButton")
        self.cal_start_btn.clicked.connect(self.start_cal_flow)
        self.cal_stop_btn = QPushButton("⏹ Stop")
        self.cal_stop_btn.setObjectName("dangerButton")
        self.cal_stop_btn.clicked.connect(self.stop_cal_flow)
        btn_row.addWidget(self.cal_start_btn)
        btn_row.addWidget(self.cal_stop_btn)
        flow_layout.addLayout(btn_row)
        layout.addWidget(flow_group)

        # ---- Live ADC readings group ----------------------------------------
        live_group = QGroupBox("📊 Live ADC Readings (streaming)")
        live_layout = QHBoxLayout(live_group)
        live_layout.setSpacing(20)

        self.cal_t1_adc_label = QLabel("T1 ADC: ---")
        self.cal_t1_adc_label.setObjectName("statusLabel")
        self.cal_t2_adc_label = QLabel("T2 ADC: ---")
        self.cal_t2_adc_label.setObjectName("statusLabel")
        live_layout.addWidget(self.cal_t1_adc_label, alignment=Qt.AlignmentFlag.AlignCenter)
        live_layout.addWidget(self.cal_t2_adc_label, alignment=Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(live_group)

        # ---- Tank 1 calibration group ---------------------------------------
        t1_group = QGroupBox("🚿 Tank 1 Calibration")
        t1_layout = QVBoxLayout(t1_group)
        t1_btn_row = QHBoxLayout()
        self.cal_t1_empty_btn = QPushButton("⬤ Mark T1 EMPTY")
        self.cal_t1_empty_btn.clicked.connect(lambda: self.record_tank_empty(1))
        self.cal_t1_full_btn = QPushButton("✅ Mark T1 FULL")
        self.cal_t1_full_btn.clicked.connect(lambda: self.record_tank_full(1))
        t1_btn_row.addWidget(self.cal_t1_empty_btn)
        t1_btn_row.addWidget(self.cal_t1_full_btn)
        t1_layout.addLayout(t1_btn_row)
        self.cal_t1_values_label = QLabel("Empty ADC: --   |   Full ADC: --")
        self.cal_t1_values_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        t1_layout.addWidget(self.cal_t1_values_label)
        layout.addWidget(t1_group)

        # ---- Tank 2 calibration group ---------------------------------------
        t2_group = QGroupBox("🚿 Tank 2 Calibration")
        t2_layout = QVBoxLayout(t2_group)
        t2_btn_row = QHBoxLayout()
        self.cal_t2_empty_btn = QPushButton("⬤ Mark T2 EMPTY")
        self.cal_t2_empty_btn.clicked.connect(lambda: self.record_tank_empty(2))
        self.cal_t2_full_btn = QPushButton("✅ Mark T2 FULL")
        self.cal_t2_full_btn.clicked.connect(lambda: self.record_tank_full(2))
        t2_btn_row.addWidget(self.cal_t2_empty_btn)
        t2_btn_row.addWidget(self.cal_t2_full_btn)
        t2_layout.addLayout(t2_btn_row)
        self.cal_t2_values_label = QLabel("Empty ADC: --   |   Full ADC: --")
        self.cal_t2_values_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        t2_layout.addWidget(self.cal_t2_values_label)
        layout.addWidget(t2_group)

        # ---- Apply calibration group ----------------------------------------
        apply_group = QGroupBox("🎯 Apply Calibration")
        apply_layout = QVBoxLayout(apply_group)

        apply_btn = QPushButton("🎯 Apply Calibration to Arduino")
        apply_btn.setObjectName("primaryButton")
        apply_btn.clicked.connect(self.apply_calibration)
        apply_layout.addWidget(apply_btn)

        getcal_btn = QPushButton("📝 Read Current Calibration from Arduino")
        getcal_btn.clicked.connect(self.get_calibration)
        apply_layout.addWidget(getcal_btn)

        self.cal_status_label = QLabel("Status: idle")
        self.cal_status_label.setObjectName("statusLabel")
        self.cal_status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        apply_layout.addWidget(self.cal_status_label)

        layout.addWidget(apply_group)
        layout.addStretch()
        return scroll

    # -- Calibration slots ----------------------------------------------------

    def _on_cal_pwm_slider_changed(self, value: int):
        self.cal_pwm_spin.blockSignals(True)
        self.cal_pwm_spin.setValue(value)
        self.cal_pwm_spin.blockSignals(False)
        raw = (value * 255) // 100
        self.cal_pwm_label.setText(f"{raw} / 255 PWM")
        self.start_cal_flow()

    def _on_cal_pwm_spin_changed(self, value: int):
        self.cal_pwm_slider.blockSignals(True)
        self.cal_pwm_slider.setValue(value)
        self.cal_pwm_slider.blockSignals(False)
        raw = (value * 255) // 100
        self.cal_pwm_label.setText(f"{raw} / 255 PWM")
        self.start_cal_flow()

    def start_cal_flow(self):
        """Enter calibration mode on Arduino with the selected PWM."""
        pct = self.cal_pwm_slider.value()
        cmd = CommandBuilder.cal_mode(pct)
        self.serial_worker.send_command(cmd)
        self.cal_status_label.setText(f"Status: calibration active @ {pct}%")

    def stop_cal_flow(self):
        """Exit calibration mode."""
        self.serial_worker.send_command(CommandBuilder.cal_stop())
        self.cal_status_label.setText("Status: stopped")

    def record_tank_empty(self, tank: int):
        """Record current ADC value as 'empty' for given tank."""
        if tank == 1:
            self.cal_t1_empty_adc = self._cur_t1_adc
            self._refresh_t1_labels()
        else:
            self.cal_t2_empty_adc = self._cur_t2_adc
            self._refresh_t2_labels()

    def record_tank_full(self, tank: int):
        """Record current ADC value as 'full' for given tank."""
        if tank == 1:
            self.cal_t1_full_adc = self._cur_t1_adc
            self._refresh_t1_labels()
        else:
            self.cal_t2_full_adc = self._cur_t2_adc
            self._refresh_t2_labels()

    def _refresh_t1_labels(self):
        self.cal_t1_values_label.setText(
            f"Empty ADC: {self.cal_t1_empty_adc:.0f}   |   Full ADC: {self.cal_t1_full_adc:.0f}"
        )

    def _refresh_t2_labels(self):
        self.cal_t2_values_label.setText(
            f"Empty ADC: {self.cal_t2_empty_adc:.0f}   |   Full ADC: {self.cal_t2_full_adc:.0f}"
        )

    def apply_calibration(self):
        """Send SETCAL commands to Arduino for both tanks."""
        if self.cal_t1_full_adc <= self.cal_t1_empty_adc:
            self.cal_status_label.setText("Error: T1 full ADC must be > empty ADC")
            return
        if self.cal_t2_full_adc <= self.cal_t2_empty_adc:
            self.cal_status_label.setText("Error: T2 full ADC must be > empty ADC")
            return
        cmd1 = CommandBuilder.set_calibration(1, self.cal_t1_empty_adc, self.cal_t1_full_adc)
        cmd2 = CommandBuilder.set_calibration(2, self.cal_t2_empty_adc, self.cal_t2_full_adc)
        self.serial_worker.send_command(cmd1)
        self.serial_worker.send_command(cmd2)
        self.cal_status_label.setText(
            f"Calibration sent — T1 [{self.cal_t1_empty_adc:.0f},{self.cal_t1_full_adc:.0f}]  T2 [{self.cal_t2_empty_adc:.0f},{self.cal_t2_full_adc:.0f}]"
        )

    def get_calibration(self):
        """Request current calibration from Arduino."""
        self.serial_worker.send_command(CommandBuilder.get_calibration())

    def _parse_cal_message(self, status_msg):
        """Parse [CAL] streaming messages and update live ADC labels."""
        if getattr(status_msg, 'level', '') != "CAL":
            return
        text: str = getattr(status_msg, 'message', '')
        # Format from Arduino: T1:1234 T2:2345 PWM:153
        m1 = re.search(r'T1:(\d+(?:\.\d+)?)', text)
        m2 = re.search(r'T2:(\d+(?:\.\d+)?)', text)
        if m1:
            self._cur_t1_adc = float(m1.group(1))
            self.cal_t1_adc_label.setText(f"T1 ADC: {self._cur_t1_adc:.0f}")
        if m2:
            self._cur_t2_adc = float(m2.group(1))
            self.cal_t2_adc_label.setText(f"T2 ADC: {self._cur_t2_adc:.0f}")
