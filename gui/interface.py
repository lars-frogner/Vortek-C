from renderer import RenderingContext

# The rendering process must be forked before importing QT modules
rendering_context = RenderingContext()

import sys
import os
from PySide2 import QtCore, QtWidgets, QtUiTools, QtGui
from matplotlib_canvas import MatplotlibCanvas
from transfer_functions import RGBATransferFunction, HSVATransferFunction


class UiLoader(QtUiTools.QUiLoader):
    '''
    https://stackoverflow.com/questions/27603350/how-do-i-load-children-from-ui-file-in-pyside
    '''
    _baseinstance = None

    def createWidget(self, classname, parent=None, name=''):
        if parent is None and self._baseinstance is not None:
            widget = self._baseinstance
        else:
            widget = super(UiLoader, self).createWidget(classname, parent, name)
            if self._baseinstance is not None:
                setattr(self._baseinstance, name, widget)
        return widget

    def loadUi(self, uifile, baseinstance=None):
        self._baseinstance = baseinstance
        widget = self.load(uifile)
        QtCore.QMetaObject.connectSlotsByName(widget)
        return widget


class MainWindow(QtWidgets.QMainWindow):

    def __init__(self, parent=None):

        super().__init__(parent)
        UiLoader().loadUi('interface.ui', self)

        self.file_menu = FileMenu(self)
        self.context_stack = ContextStack(self)


class FileMenu:

    def __init__(self, window):

        self.window = window
        self.menubar = self.window.findChild(QtWidgets.QMenuBar, 'menubar')
        self.widget = self.menubar.addMenu('File')

        self.previous_path = QtCore.QDir.homePath()

        self.setup_load_bifrost_field_action()

    def setup_load_bifrost_field_action(self):

        self.load_bifrost_field_action = self.widget.addAction('Load Bifrost field')

        @QtCore.Slot()
        def load_bifrost_field():
            filename, _ = QtWidgets.QFileDialog.getOpenFileName(self.window,
                                                                self.load_bifrost_field_action.text(),
                                                                self.previous_path,
                                                                'Prepped Bifrost data file (*.raw)')

            if len(filename) > 0:
                self.previous_path = os.path.split(filename)[0]
                file_base_name = '.'.join(filename.split('.')[:-1])
                rendering_context.set_field_from_bifrost_file('bifrost_field', file_base_name)

        self.load_bifrost_field_action.triggered.connect(load_bifrost_field)


class ContextStack:

    def __init__(self, window):
        self.window = window
        self.widget = self.window.findChild(QtWidgets.QStackedWidget, 'contextStackedWidget')

        self.RGBA_transfer_function_tab = RGBATransferFunctionTab(self.window)
        self.HSVA_transfer_function_tab = HSVATransferFunctionTab(self.window)


class TransferFunctionTab:

    def __init__(self, window):
        self.window = window
        self.widget = None
        self.tab_names = []
        self.canvas_widgets = {}

    def setup_matplotlib_canvas_widgets(self):
        for name in self.tab_names:
            canvas = MatplotlibCanvas(self.window, width=5, height=4, dpi=100)
            self.widget.addTab(canvas, name)
            self.canvas_widgets[name] = canvas


class RGBATransferFunctionTab(TransferFunctionTab):

    def __init__(self, window):
        super().__init__(window)
        self.widget = self.window.findChild(QtWidgets.QTabWidget, 'RGBATransferFunctionTabWidget')
        self.tab_names = ['Red', 'Green', 'Blue', 'Alpha']
        self.setup_matplotlib_canvas_widgets()
        self.transfer_function = RGBATransferFunction(rendering_context,
                                                      *(self.canvas_widgets[name].axes for name in self.tab_names))


class HSVATransferFunctionTab(TransferFunctionTab):

    def __init__(self, window):
        super().__init__(window)
        self.widget = self.window.findChild(QtWidgets.QTabWidget, 'HSVATransferFunctionTabWidget')
        self.tab_names = ['Hue', 'Saturation', 'Value', 'Alpha']
        self.setup_matplotlib_canvas_widgets()
        self.transfer_function = HSVATransferFunction(rendering_context,
                                                      *(self.canvas_widgets[name].axes for name in self.tab_names))


if __name__ == '__main__':

    app = QtWidgets.QApplication(sys.argv)

    mainwindow = MainWindow()
    mainwindow.show()

    rendering_context.start_rendering()

    exit_status = app.exec_()

    rendering_context.stop_rendering()

    sys.exit(exit_status)
