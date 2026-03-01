"""
Main application window with sidebar navigation.
"""
from PyQt6.QtWidgets import (QMainWindow, QWidget, QHBoxLayout, QVBoxLayout,
                              QPushButton, QStackedWidget, QLabel, QSizePolicy)
from PyQt6.QtCore import Qt, QSize
from PyQt6.QtGui import QKeySequence, QShortcut

from core.serial_worker import SerialWorker
from viewmodels import AppState
from ui.pages import (DashboardPage, DevicePage, ControlsPage,
                      LogsPage, SettingsPage)
from ui.components.status_chip import StatusChip


class NavButton(QPushButton):
    """Checkable sidebar navigation button."""

    def __init__(self, icon: str, label: str, parent=None):
        super().__init__(parent)
        self.setCheckable(True)
        self.setText(f"  {icon}  {label}")
        self.setObjectName("navButton")
        self.setMinimumHeight(48)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)


class MainWindow(QMainWindow):
    """Main application window."""

    def __init__(self, app_state: AppState, serial_worker: SerialWorker, parent=None):
        super().__init__(parent)
        self.app_state = app_state
        self.serial_worker = serial_worker

        self.setWindowTitle("Hydraulic Control System – ESP32-S3")
        self.setMinimumSize(1280, 800)

        # ── Root widget ──────────────────────────────────────────────────────
        root = QWidget()
        self.setCentralWidget(root)
        root_layout = QHBoxLayout(root)
        root_layout.setContentsMargins(0, 0, 0, 0)
        root_layout.setSpacing(0)

        # ── Sidebar ──────────────────────────────────────────────────────────
        sidebar = QWidget()
        sidebar.setObjectName("sidebar")
        sidebar.setFixedWidth(220)
        sidebar_layout = QVBoxLayout(sidebar)
        sidebar_layout.setContentsMargins(0, 0, 0, 0)
        sidebar_layout.setSpacing(0)

        # Logo / app title
        logo_widget = QWidget()
        logo_widget.setObjectName("logoArea")
        logo_layout = QVBoxLayout(logo_widget)
        logo_layout.setContentsMargins(16, 20, 16, 20)

        app_title = QLabel("HYDRAULIC\nCONTROL SYSTEM")
        app_title.setObjectName("appTitle")
        app_title.setAlignment(Qt.AlignmentFlag.AlignLeft)
        logo_layout.addWidget(app_title)

        subtitle = QLabel("ESP32-S3  Lab 2")
        subtitle.setObjectName("caption")
        logo_layout.addWidget(subtitle)

        sidebar_layout.addWidget(logo_widget)

        # Status chip
        status_area = QWidget()
        status_area_layout = QHBoxLayout(status_area)
        status_area_layout.setContentsMargins(16, 8, 16, 8)
        self.status_chip = StatusChip()
        status_area_layout.addWidget(self.status_chip)
        status_area_layout.addStretch()
        sidebar_layout.addWidget(status_area)

        # Divider
        divider = QWidget()
        divider.setFixedHeight(1)
        divider.setObjectName("divider")
        sidebar_layout.addWidget(divider)

        # Nav buttons
        nav_items = [
            ("📊", "Dashboard"),
            ("🔌", "Device"),
            ("🎛", "Controls"),
            ("📋", "Logs"),
            ("⚙", "Settings"),
        ]
        self.nav_buttons: list[NavButton] = []
        nav_widget = QWidget()
        nav_layout = QVBoxLayout(nav_widget)
        nav_layout.setContentsMargins(8, 8, 8, 8)
        nav_layout.setSpacing(4)

        for icon, label in nav_items:
            btn = NavButton(icon, label)
            btn.clicked.connect(self._make_nav_handler(len(self.nav_buttons)))
            self.nav_buttons.append(btn)
            nav_layout.addWidget(btn)

        nav_layout.addStretch()
        sidebar_layout.addWidget(nav_widget)

        root_layout.addWidget(sidebar)

        # ── Content stack ────────────────────────────────────────────────────
        self.pages = QStackedWidget()
        self.pages.setObjectName("contentArea")

        self.dashboard_page  = DashboardPage(app_state)
        self.device_page     = DevicePage(app_state, serial_worker)
        self.controls_page   = ControlsPage(app_state, serial_worker)
        self.logs_page       = LogsPage(app_state)
        self.settings_page   = SettingsPage(app_state)

        for page in [self.dashboard_page, self.device_page, self.controls_page,
                     self.logs_page, self.settings_page]:
            self.pages.addWidget(page)

        root_layout.addWidget(self.pages, stretch=1)

        # ── Keyboard shortcuts ───────────────────────────────────────────────
        for i in range(5):
            sc = QShortcut(QKeySequence(f"Ctrl+{i+1}"), self)
            sc.activated.connect(self._make_nav_handler(i))

        QShortcut(QKeySequence("Ctrl+Q"), self).activated.connect(self.close)

        # ── Connect signals ──────────────────────────────────────────────────
        app_state.connection_changed.connect(self._on_connection_changed)
        # Forward status messages to logs page
        # (connected later from main.py after worker signals are wired)

        # Start on Dashboard
        self._navigate(0)

    # ── Navigation ────────────────────────────────────────────────────────────
    def _make_nav_handler(self, index: int):
        return lambda: self._navigate(index)

    def _navigate(self, index: int):
        self.pages.setCurrentIndex(index)
        for i, btn in enumerate(self.nav_buttons):
            btn.setChecked(i == index)

    # ── Slots ─────────────────────────────────────────────────────────────────
    def _on_connection_changed(self, connected: bool):
        self.status_chip.set_connected(connected)
