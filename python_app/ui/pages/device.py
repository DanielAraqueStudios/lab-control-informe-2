"""
Device connection and management page.
"""
from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton,
                              QComboBox, QGroupBox, QFormLayout, QLineEdit, QTextEdit,
                              QSpinBox)
from PyQt6.QtCore import Qt
from core import SerialWorker, CommandBuilder
from viewmodels import AppState


class DevicePage(QWidget):
    """Device connection and protocol testing page."""
    
    def __init__(self, app_state: AppState, serial_worker: SerialWorker, parent=None):
        super().__init__(parent)
        self.app_state = app_state
        self.serial_worker = serial_worker
        
        # Main layout
        layout = QVBoxLayout(self)
        layout.setContentsMargins(24, 24, 24, 24)
        layout.setSpacing(24)
        
        # Page title
        title = QLabel("Device Connection")
        title.setObjectName("heading1")
        layout.addWidget(title)
        
        # Connection settings group
        conn_group = QGroupBox("Serial Port Settings")
        conn_layout = QFormLayout()
        conn_layout.setSpacing(12)
        
        # Port selection
        port_row = QHBoxLayout()
        self.port_combo = QComboBox()
        self.port_combo.setMinimumWidth(200)
        port_row.addWidget(self.port_combo)
        
        refresh_btn = QPushButton("🔄 Refresh")
        refresh_btn.setObjectName("secondaryButton")
        refresh_btn.setMaximumWidth(100)
        refresh_btn.clicked.connect(self.refresh_ports)
        port_row.addWidget(refresh_btn)
        port_row.addStretch()
        
        conn_layout.addRow("COM Port:", port_row)
        
        # Baud rate
        self.baud_spin = QSpinBox()
        self.baud_spin.setRange(9600, 921600)
        self.baud_spin.setValue(115200)
        self.baud_spin.setSingleStep(9600)
        conn_layout.addRow("Baud Rate:", self.baud_spin)
        
        conn_group.setLayout(conn_layout)
        layout.addWidget(conn_group)
        
        # Connection buttons
        btn_layout = QHBoxLayout()
        
        self.connect_btn = QPushButton("🔌 Connect")
        self.connect_btn.clicked.connect(self.toggle_connection)
        self.connect_btn.setMinimumWidth(150)
        btn_layout.addWidget(self.connect_btn)
        
        self.status_btn = QPushButton("📊 Request Status")
        self.status_btn.setObjectName("secondaryButton")
        self.status_btn.clicked.connect(self.request_status)
        self.status_btn.setEnabled(False)
        btn_layout.addWidget(self.status_btn)
        
        self.help_btn = QPushButton("❓ Request Help")
        self.help_btn.setObjectName("secondaryButton")
        self.help_btn.clicked.connect(self.request_help)
        self.help_btn.setEnabled(False)
        btn_layout.addWidget(self.help_btn)
        
        btn_layout.addStretch()
        layout.addLayout(btn_layout)
        
        # Protocol test group
        test_group = QGroupBox("Protocol Test")
        test_layout = QVBoxLayout()
        
        # Custom command
        cmd_layout = QHBoxLayout()
        cmd_layout.addWidget(QLabel("Command:"))
        self.cmd_input = QLineEdit()
        self.cmd_input.setPlaceholderText("Enter command (e.g., STATUS)")
        self.cmd_input.returnPressed.connect(self.send_custom_command)
        cmd_layout.addWidget(self.cmd_input)
        
        send_btn = QPushButton("Send")
        send_btn.clicked.connect(self.send_custom_command)
        send_btn.setMaximumWidth(100)
        cmd_layout.addWidget(send_btn)
        
        test_layout.addLayout(cmd_layout)
        
        # Quick test buttons
        quick_layout = QHBoxLayout()
        quick_layout.addWidget(QLabel("Quick Tests:"))
        
        for cmd_name, cmd_func in [
            ("HELP", lambda: self.serial_worker.send_command(CommandBuilder.help_command())),
            ("STATUS", lambda: self.serial_worker.send_command(CommandBuilder.get_status())),
            ("DATALOG", lambda: self.serial_worker.send_command(CommandBuilder.toggle_datalog())),
        ]:
            btn = QPushButton(cmd_name)
            btn.setObjectName("secondaryButton")
            btn.setMaximumWidth(100)
            btn.clicked.connect(cmd_func)
            quick_layout.addWidget(btn)
        
        quick_layout.addStretch()
        test_layout.addLayout(quick_layout)
        
        test_group.setLayout(test_layout)
        layout.addWidget(test_group)
        
        # Device info
        info_group = QGroupBox("Device Information")
        info_layout = QFormLayout()
        
        self.fw_label = QLabel("Sprint 5 v2.0")
        self.uptime_label = QLabel("--")
        self.rx_count_label = QLabel("0")
        self.tx_count_label = QLabel("0")
        
        info_layout.addRow("Firmware:", self.fw_label)
        info_layout.addRow("Uptime:", self.uptime_label)
        info_layout.addRow("Messages RX:", self.rx_count_label)
        info_layout.addRow("Messages TX:", self.tx_count_label)
        
        info_group.setLayout(info_layout)
        layout.addWidget(info_group)
        
        layout.addStretch()
        
        # Connect signals
        self.serial_worker.connection_changed.connect(self.on_connection_changed)
        self.app_state.connection_changed.connect(self.on_connection_changed)
        
        # Initial port scan
        self.refresh_ports()
    
    def refresh_ports(self):
        """Scan and populate available COM ports."""
        self.port_combo.clear()
        ports = SerialWorker.list_available_ports()
        
        if ports:
            self.port_combo.addItems(ports)
        else:
            self.port_combo.addItem("No ports found")
            self.connect_btn.setEnabled(False)
    
    def toggle_connection(self):
        """Connect or disconnect from device."""
        if self.app_state.device.connected:
            # Disconnect
            self.serial_worker.disconnect()
        else:
            # Connect
            port = self.port_combo.currentText()
            baud = self.baud_spin.value()
            
            if port and port != "No ports found":
                self.serial_worker.connect(port, baud)
                self.app_state.set_connection(False, port)  # Will update on worker signal
    
    def on_connection_changed(self, connected: bool):
        """Update UI based on connection state."""
        if connected:
            self.connect_btn.setText("🔌 Disconnect")
            self.connect_btn.setObjectName("dangerButton")
            self.port_combo.setEnabled(False)
            self.baud_spin.setEnabled(False)
            self.status_btn.setEnabled(True)
            self.help_btn.setEnabled(True)
        else:
            self.connect_btn.setText("🔌 Connect")
            self.connect_btn.setObjectName("")
            self.port_combo.setEnabled(True)
            self.baud_spin.setEnabled(True)
            self.status_btn.setEnabled(False)
            self.help_btn.setEnabled(False)
        
        # Force style refresh
        self.connect_btn.style().unpolish(self.connect_btn)
        self.connect_btn.style().polish(self.connect_btn)
    
    def send_custom_command(self):
        """Send custom command from input field."""
        command = self.cmd_input.text().strip()
        if command:
            self.serial_worker.send_command(command + '\n')
            self.cmd_input.clear()
    
    def request_status(self):
        """Request status dump from Arduino."""
        self.serial_worker.send_command(CommandBuilder.get_status())
    
    def request_help(self):
        """Request help/command list from Arduino."""
        self.serial_worker.send_command(CommandBuilder.help_command())
