"""
Real-time chart widget using PyQtGraph.
"""
import pyqtgraph as pg
from PyQt6.QtWidgets import QWidget, QVBoxLayout, QLabel
from PyQt6.QtCore import Qt
from collections import deque
import numpy as np


class RealtimeChart(QWidget):
    """High-performance real-time chart using PyQtGraph."""
    
    def __init__(self, title: str = "", y_label: str = "", max_points: int = 500, parent=None):
        super().__init__(parent)
        
        self.max_points = max_points
        self.data_buffers = {}  # {series_name: deque}
        self.plot_items = {}    # {series_name: PlotDataItem}
        
        # Layout
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(8)
        
        # Title
        if title:
            title_label = QLabel(title)
            title_label.setObjectName("heading3")
            layout.addWidget(title_label)
        
        # PyQtGraph widget
        pg.setConfigOptions(antialias=True)
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setBackground('#0a0e1a')
        self.plot_widget.showGrid(x=True, y=True, alpha=0.3)
        
        # Style axis
        axis_color = '#9aa0a6'
        self.plot_widget.getAxis('bottom').setPen(pg.mkPen(color=axis_color))
        self.plot_widget.getAxis('left').setPen(pg.mkPen(color=axis_color))
        self.plot_widget.getAxis('bottom').setTextPen(pg.mkPen(color=axis_color))
        self.plot_widget.getAxis('left').setTextPen(pg.mkPen(color=axis_color))
        
        # Labels
        if y_label:
            self.plot_widget.setLabel('left', y_label, color=axis_color)
        self.plot_widget.setLabel('bottom', 'Time (s)', color=axis_color)
        
        # Legend
        self.plot_widget.addLegend(offset=(10, 10))
        
        layout.addWidget(self.plot_widget)
    
    def add_series(self, name: str, color: str = '#00e5ff', width: int = 2):
        """Add a data series to the chart.
        
        Args:
            name: Series identifier
            color: Line color (hex)
            width: Line width in pixels
        """
        self.data_buffers[name] = {
            'x': deque(maxlen=self.max_points),
            'y': deque(maxlen=self.max_points)
        }
        
        pen = pg.mkPen(color=color, width=width)
        curve = self.plot_widget.plot([], [], name=name, pen=pen)
        self.plot_items[name] = curve
    
    def update_series(self, name: str, x_value: float, y_value: float):
        """Add data point to series.
        
        Args:
            name: Series identifier
            x_value: X-axis value (time)
            y_value: Y-axis value (measurement)
        """
        if name not in self.data_buffers:
            return
        
        self.data_buffers[name]['x'].append(x_value)
        self.data_buffers[name]['y'].append(y_value)
    
    def update_series_bulk(self, name: str, x_data: list, y_data: list):
        """Update series with multiple points at once.
        
        Args:
            name: Series identifier
            x_data: List of X values
            y_data: List of Y values
        """
        if name not in self.data_buffers:
            return
        
        # Clear and repopulate (more efficient for bulk updates)
        self.data_buffers[name]['x'].clear()
        self.data_buffers[name]['y'].clear()
        
        # Take last max_points
        if len(x_data) > self.max_points:
            x_data = x_data[-self.max_points:]
            y_data = y_data[-self.max_points:]
        
        self.data_buffers[name]['x'].extend(x_data)
        self.data_buffers[name]['y'].extend(y_data)
    
    def render(self):
        """Render all series to the chart (call this periodically, not on every data point)."""
        for name, curve in self.plot_items.items():
            if name in self.data_buffers:
                x_data = np.array(self.data_buffers[name]['x'])
                y_data = np.array(self.data_buffers[name]['y'])
                
                if len(x_data) > 0:
                    curve.setData(x_data, y_data)
    
    def clear_all(self):
        """Clear all data series."""
        for buffer in self.data_buffers.values():
            buffer['x'].clear()
            buffer['y'].clear()
        self.render()
    
    def set_x_range(self, min_x: float, max_x: float):
        """Set X-axis range."""
        self.plot_widget.setXRange(min_x, max_x, padding=0)
    
    def set_y_range(self, min_y: float, max_y: float):
        """Set Y-axis range."""
        self.plot_widget.setYRange(min_y, max_y, padding=0.1)
    
    def enable_auto_range(self, enable: bool = True):
        """Enable/disable auto-ranging."""
        self.plot_widget.enableAutoRange(enable=enable)


class StackedAreaChart(RealtimeChart):
    """Stacked area chart for PID components visualization."""
    
    def __init__(self, title: str = "PID Components", parent=None):
        super().__init__(title, "Output", parent=parent)
        
        # Add PID series with filled areas
        self.add_series("P", color='#00e5ff', width=0)  # No line, just fill
        self.add_series("I", color='#9d4edd', width=0)
        self.add_series("D", color='#00ff88', width=0)
        
        # Override plot items to be filled
        for name, color in [("P", '#00e5ff'), ("I", '#9d4edd'), ("D", '#00ff88')]:
            brush = pg.mkBrush(color=color + '80')  # Semi-transparent
            self.plot_items[name].setBrush(brush)
            self.plot_items[name].setFillLevel(0)
