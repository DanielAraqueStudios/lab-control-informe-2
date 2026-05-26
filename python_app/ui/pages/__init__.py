"""Pages package."""
from .dashboard import DashboardPage
from .device import DevicePage
from .controls import ControlsPage
from .logs import LogsPage
from .settings import SettingsPage
from .telemetry import TelemetryPage

__all__ = ["DashboardPage", "DevicePage", "ControlsPage", "LogsPage", "SettingsPage", "TelemetryPage"]
