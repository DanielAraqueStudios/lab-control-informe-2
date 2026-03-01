"""
Logs page for viewing serial communication and status messages.
"""
from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton,
                              QTextEdit, QComboBox, QGroupBox, QCheckBox)
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QTextCursor, QColor, QTextCharFormat
from viewmodels import AppState
from core.models import StatusMessage


class LogsPage(QWidget):
    """Serial monitor and filtered logs page."""
    
    def __init__(self, app_state: AppState, parent=None):
        super().__init__(parent)
        self.app_state = app_state
        
        # Main layout
        layout = QVBoxLayout(self)
        layout.setContentsMargins(24, 24, 24, 24)
        layout.setSpacing(16)
        
        # Page title
        title_layout = QHBoxLayout()
        title = QLabel("System Logs")
        title.setObjectName("heading1")
        title_layout.addWidget(title)
        title_layout.addStretch()
        
        # Clear button
        clear_btn = QPushButton("🗑 Clear Logs")
        clear_btn.setObjectName("secondaryButton")
        clear_btn.clicked.connect(self.clear_logs)
        title_layout.addWidget(clear_btn)
        
        layout.addLayout(title_layout)
        
        # Filter controls
        filter_layout = QHBoxLayout()
        
        filter_layout.addWidget(QLabel("Filter by level:"))
        
        self.filter_combo = QComboBox()
        self.filter_combo.addItems(["All", "INFO", "CMD", "CTRL", "PID", "ERROR", "METRICS", "MODE"])
        self.filter_combo.currentTextChanged.connect(self.apply_filter)
        filter_layout.addWidget(self.filter_combo)
        
        self.autoscroll_check = QCheckBox("Auto-scroll")
        self.autoscroll_check.setChecked(True)
        filter_layout.addWidget(self.autoscroll_check)
        
        filter_layout.addStretch()
        
        layout.addLayout(filter_layout)
        
        # Log display (styled text edit for colored logs)
        self.log_view = QTextEdit()
        self.log_view.setReadOnly(True)
        self.log_view.setLineWrapMode(QTextEdit.LineWrapMode.NoWrap)
        layout.addWidget(self.log_view, stretch=1)
        
        # Stats bar
        stats_layout = QHBoxLayout()
        self.stats_label = QLabel("Messages: 0 | Errors: 0")
        self.stats_label.setObjectName("caption")
        stats_layout.addWidget(self.stats_label)
        stats_layout.addStretch()
        
        layout.addLayout(stats_layout)
        
        # Message counters
        self.msg_count = 0
        self.error_count = 0
        
        # Color scheme for different log levels
        self.level_colors = {
            "INFO": "#9aa0a6",
            "CMD": "#00e5ff",
            "CTRL": "#00ff88",
            "PID": "#9d4edd",
            "ERROR": "#ff006e",
            "METRICS": "#ffd60a",
            "MODE": "#00e5ff",
            "OK": "#00ff88",
            "READY": "#00ff88"
        }
    
    def add_log(self, status: StatusMessage):
        """Add log message to view."""
        # Update counters
        self.msg_count += 1
        if status.level == "ERROR":
            self.error_count += 1
        
        self.update_stats()
        
        # Apply filter
        current_filter = self.filter_combo.currentText()
        if current_filter != "All" and status.level != current_filter:
            return
        
        # Format message with color
        cursor = self.log_view.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.End)
        
        # Timestamp in gray
        fmt = QTextCharFormat()
        fmt.setForeground(QColor("#5f6368"))
        cursor.setCharFormat(fmt)
        cursor.insertText(f"[{status.timestamp.strftime('%H:%M:%S.%f')[:-3]}] ")
        
        # Level tag in color
        level_color = self.level_colors.get(status.level, "#9aa0a6")
        fmt.setForeground(QColor(level_color))
        cursor.setCharFormat(fmt)
        cursor.insertText(f"[{status.level}] ")
        
        # Message in white
        fmt.setForeground(QColor("#e8eaed"))
        cursor.setCharFormat(fmt)
        cursor.insertText(status.message + "\n")
        
        # Auto-scroll to bottom
        if self.autoscroll_check.isChecked():
            self.log_view.verticalScrollBar().setValue(
                self.log_view.verticalScrollBar().maximum()
            )
    
    def apply_filter(self):
        """Re-render logs with current filter."""
        self.log_view.clear()
        
        current_filter = self.filter_combo.currentText()
        
        for status in self.app_state.status_logs:
            if current_filter == "All" or status.level == current_filter:
                self.add_log(status)
    
    def clear_logs(self):
        """Clear all logs."""
        self.log_view.clear()
        self.app_state.clear_logs()
        self.msg_count = 0
        self.error_count = 0
        self.update_stats()
    
    def update_stats(self):
        """Update statistics label."""
        self.stats_label.setText(f"Messages: {self.msg_count} | Errors: {self.error_count}")
