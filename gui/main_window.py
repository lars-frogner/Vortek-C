import sys
import os
import rendering

# Important: the rendering session must be started BEFORE any PySide2 modules are imported!
if __name__ == '__main__':
    rendering.start_session()

from PySide2 import QtCore, QtWidgets
from ui_loader import UiLoader
from rendering_settings_window import RenderingSettingsWindow
from transfer_function_widgets import TransferFunctionPage


class MainWindow(QtWidgets.QMainWindow):

    def __init__(self):

        super().__init__()
        UiLoader().loadUi('main_window.ui', self)

        self.file_menu = FileMenu(self)
        self.rendering_menu = RendererMenu(self)

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

                rendering.session.set_brick_boundary_indicator_creation(True)
                rendering.session.set_sub_brick_boundary_indicator_creation(True)

                self.previous_path = os.path.split(filename)[0]
                file_base_name = '.'.join(filename.split('.')[:-1])
                rendering.session.set_field_from_bifrost_file('bifrost_field', file_base_name)

        self.load_bifrost_field_action.triggered.connect(load_bifrost_field)


class RendererMenu:

    def __init__(self, window):

        self.window = window
        self.menubar = self.window.findChild(QtWidgets.QMenuBar, 'menubar')
        self.widget = self.menubar.addMenu('Rendering')

        self.rendering_settings_window = RenderingSettingsWindow(self.window)

        self.setup_launch_renderer_action()
        self.setup_stop_renderer_action()
        self.setup_open_settings_action()

    def setup_launch_renderer_action(self):

        self.launch_renderer_action = self.widget.addAction('Launch renderer')
        self.launch_renderer_action.setVisible(False)

        @QtCore.Slot()
        def launch_renderer():
            rendering.session.start_renderer()
            self.sync_launch_stop_actions_to_renderer_state(True)

        self.launch_renderer_action.triggered.connect(launch_renderer)

    def setup_stop_renderer_action(self):

        self.stop_renderer_action = self.widget.addAction('Stop renderer')
        self.stop_renderer_action.setVisible(True)

        @QtCore.Slot()
        def stop_renderer():
            rendering.session.stop_renderer()
            self.sync_launch_stop_actions_to_renderer_state(False)

        self.stop_renderer_action.triggered.connect(stop_renderer)

    def setup_open_settings_action(self):

        self.open_settings_action = self.widget.addAction('Rendering settings')

        @QtCore.Slot()
        def open_settings():
            self.rendering_settings_window.show()

        self.open_settings_action.triggered.connect(open_settings)

    def sync_launch_stop_actions_to_renderer_state(self, is_running):
        self.launch_renderer_action.setVisible(not is_running)
        self.stop_renderer_action.setVisible(is_running)


class ContextStack:

    def __init__(self, window):
        self.window = window
        self.widget = self.window.findChild(QtWidgets.QStackedWidget, 'contextStackedWidget')

        self.transfer_function_page = TransferFunctionPage(self.window)


if __name__ == '__main__':

    os.environ['QT_MAC_WANTS_LAYER'] = '1'
    QtCore.QCoreApplication.setAttribute(QtCore.Qt.AA_ShareOpenGLContexts)

    app = QtWidgets.QApplication(sys.argv)

    mainwindow = MainWindow()
    mainwindow.show()

    rendering.session.start_renderer()

    exit_status = app.exec_()

    rendering.end_session()

    sys.exit(exit_status)
