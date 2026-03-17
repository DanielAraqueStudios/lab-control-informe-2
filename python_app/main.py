"""
Application entry point.
Usage: python main.py
"""
import sys
import os
from pathlib import Path

from PyQt6.QtWidgets import QApplication
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QFont

# Allow imports from python_app/ root
sys.path.insert(0, str(Path(__file__).parent))

from core.serial_worker import SerialWorker
from viewmodels.app_state import AppState
from ui.main_window import MainWindow


def load_stylesheet(app: QApplication) -> None:
    """Load QSS dark theme from resources/styles.qss."""
    qss_path = Path(__file__).parent / "resources" / "styles.qss"
    if qss_path.exists():
        with open(qss_path, "r", encoding="utf-8") as f:
            app.setStyleSheet(f.read())
    else:
        print(f"[WARN] Stylesheet not found at {qss_path}")


def main() -> int:
    # ── High-DPI ─────────────────────────────────────────────────────────────
    os.environ.setdefault("QT_AUTO_SCREEN_SCALE_FACTOR", "1")

    app = QApplication(sys.argv)
    app.setApplicationName("Hydraulic Control System")
    app.setOrganizationName("Lab2")
    app.setApplicationVersion("1.0.0")

    # Default font
    font = QFont("Segoe UI", 10)
    app.setFont(font)

    # Load dark theme
    load_stylesheet(app)

    # ── Core objects ─────────────────────────────────────────────────────────
    serial_worker = SerialWorker()
    app_state = AppState()

    # ── Wire signals: serial → app_state ────────────────────────────────────
    serial_worker.telemetry_received.connect(app_state.update_telemetry)
    serial_worker.status_received.connect(app_state.add_status)
    serial_worker.metrics_received.connect(app_. state.update_metrics)
    serial_worker.connection_changed.connect(app_state.set_connection)

    # ── Main window ──────────────────────────────────────────────────────────
    window = MainWindow(app_state, serial_worker)

    # Wire status messages also to logs page directly for real-time display
    serial_worker.status_received.connect(window.logs_page.add_log)

    window.show()

    result = app.exec()

    # ── Cleanup ──────────────────────────────────────────────────────────────
    if serial_worker.isRunning():
        serial_worker.disconnect()
        serial_worker.wait(3000)

    return result


if __name__ == "__main__":
    sys.exit(main())
