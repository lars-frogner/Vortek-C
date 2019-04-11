import matplotlib
matplotlib.use('Qt5Agg')
from PySide2 import QtCore, QtWidgets
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure


class MatplotlibCanvas(FigureCanvas):

    def __init__(self, parent=None, width=6, height=4, dpi=100, edgecolor='k', linewidth=1.0, tight_layout=True):

        self.fig = Figure(figsize=(width, height), dpi=dpi, edgecolor=edgecolor, linewidth=linewidth, tight_layout=tight_layout)
        self.axes = self.fig.add_subplot(111)

        FigureCanvas.__init__(self, self.fig)
        self.setParent(parent)

        FigureCanvas.setSizePolicy(self, QtWidgets.QSizePolicy.Expanding,
                                         QtWidgets.QSizePolicy.Expanding)
        FigureCanvas.updateGeometry(self)
