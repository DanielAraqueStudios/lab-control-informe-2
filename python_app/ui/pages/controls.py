"""
Control panel page for sending commands and adjusting parameters.
"""
from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton,
                              QComboBox, QGroupBox, QFormLayout, QDoubleSpinBox, 
                              QSlider, QRadioButton, QButtonGroup, QTabWidget)
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
        
        # Set MANUAL as default
        self.mode_buttons.button(0).setChecked(True)
        
        mode_group.setLayout(mode_layout)
        layout.addWidget(mode_group)
        
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
        self.ref_initial.setRange(-100, 100)
        self.ref_initial.setValue(0.5)
        self.ref_initial.setSingleStep(0.1)
        self.ref_initial.setDecimals(2)
        params_layout.addRow("Initial Value:", self.ref_initial)
        
        self.ref_final = QDoubleSpinBox()
        self.ref_final.setRange(-100, 100)
        self.ref_final.setValue(1.5)
        self.ref_final.setSingleStep(0.1)
        self.ref_final.setDecimals(2)
        params_layout.addRow("Final Value:", self.ref_final)
        
        self.ref_duration = QDoubleSpinBox()
        self.ref_duration.setRange(0.1, 300)
        self.ref_duration.setValue(10.0)
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
    
    # Command methods
    def set_mode(self, mode: str):
        """Send mode change command."""
        cmd = CommandBuilder.set_mode(mode)
        self.serial_worker.send_command(cmd)
    
    def start_control(self):
        """Start automatic control."""
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
