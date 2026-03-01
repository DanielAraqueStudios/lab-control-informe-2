"""
Settings page for application preferences.
"""
from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel,
                              QPushButton, QGroupBox, QCheckBox, QSlider,
                              QSpinBox, QLineEdit, QFileDialog, QFormLayout)
from PyQt6.QtCore import Qt
from viewmodels import AppState


class SettingsPage(QWidget):
    """Application settings and preferences."""

    def __init__(self, app_state: AppState, parent=None):
        super().__init__(parent)
        self.app_state = app_state

        layout = QVBoxLayout(self)
        layout.setContentsMargins(24, 24, 24, 24)
        layout.setSpacing(16)

        # Title
        title = QLabel("Settings")
        title.setObjectName("heading1")
        layout.addWidget(title)

        # ── UI Preferences ──────────────────────────────────────────────────
        ui_group = QGroupBox("UI Preferences")
        ui_form = QFormLayout(ui_group)

        self.reduce_anim_check = QCheckBox("Reduce animations")
        self.reduce_anim_check.setChecked(app_state.reduce_animations)
        self.reduce_anim_check.setToolTip("Disables card/value animations for better performance")
        ui_form.addRow("Animations:", self.reduce_anim_check)

        fps_layout = QHBoxLayout()
        self.fps_spin = QSpinBox()
        self.fps_spin.setRange(5, 60)
        self.fps_spin.setValue(app_state.chart_update_rate)
        self.fps_spin.setSuffix(" FPS")
        fps_layout.addWidget(self.fps_spin)
        fps_layout.addStretch()
        ui_form.addRow("Chart update rate:", fps_layout)

        layout.addWidget(ui_group)

        # ── Export Settings ─────────────────────────────────────────────────
        exp_group = QGroupBox("Data Export")
        exp_form = QFormLayout(exp_group)

        path_layout = QHBoxLayout()
        self.export_path_edit = QLineEdit(app_state.export_path or "")
        self.export_path_edit.setPlaceholderText("Default: current directory")
        path_layout.addWidget(self.export_path_edit, stretch=1)

        browse_btn = QPushButton("Browse…")
        browse_btn.setObjectName("secondaryButton")
        browse_btn.clicked.connect(self._browse_export_path)
        path_layout.addWidget(browse_btn)

        exp_form.addRow("Export folder:", path_layout)
        layout.addWidget(exp_group)

        # ── Apply ────────────────────────────────────────────────────────────
        btn_layout = QHBoxLayout()
        btn_layout.addStretch()

        apply_btn = QPushButton("Apply Settings")
        apply_btn.clicked.connect(self._apply)
        btn_layout.addWidget(apply_btn)

        layout.addLayout(btn_layout)
        layout.addStretch()

    # ── Slots ────────────────────────────────────────────────────────────────
    def _browse_export_path(self):
        folder = QFileDialog.getExistingDirectory(self, "Select export folder")
        if folder:
            self.export_path_edit.setText(folder)

    def _apply(self):
        self.app_state.reduce_animations = self.reduce_anim_check.isChecked()
        self.app_state.chart_update_rate = self.fps_spin.value()
        self.app_state.export_path = self.export_path_edit.text()
