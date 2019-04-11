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
        self.renderer_menu = RendererMenu(self)

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


class RendererMenu:

    def __init__(self, window):

        self.window = window
        self.menubar = self.window.findChild(QtWidgets.QMenuBar, 'menubar')
        self.widget = self.menubar.addMenu('Renderer')

        self.setup_launch_renderer_action()
        self.setup_stop_renderer_action()
        self.setup_generate_brick_boundaries_action()
        self.setup_generate_sub_brick_boundaries_action()

    def setup_launch_renderer_action(self):

        self.launch_renderer_action = self.widget.addAction('Launch renderer')

        @QtCore.Slot()
        def launch_renderer():
            rendering_context.start_renderer()

        self.launch_renderer_action.triggered.connect(launch_renderer)

    def setup_stop_renderer_action(self):

        self.stop_renderer_action = self.widget.addAction('Stop renderer')

        @QtCore.Slot()
        def stop_renderer():
            rendering_context.stop_renderer()

        self.stop_renderer_action.triggered.connect(stop_renderer)

    def setup_generate_brick_boundaries_action(self):

        self.generate_brick_boundaries_action = self.widget.addAction('Generate brick boundaries')
        self.generate_brick_boundaries_action.setCheckable(True)
        self.generate_brick_boundaries_action.setChecked(False)

        @QtCore.Slot()
        def toggle_generate_brick_boundaries():
            rendering_context.set_brick_boundary_indicator_creation(self.generate_brick_boundaries_action.isChecked())

        self.generate_brick_boundaries_action.triggered.connect(toggle_generate_brick_boundaries)

    def setup_generate_sub_brick_boundaries_action(self):

        self.generate_sub_brick_boundaries_action = self.widget.addAction('Generate sub brick boundaries')
        self.generate_sub_brick_boundaries_action.setCheckable(True)
        self.generate_sub_brick_boundaries_action.setChecked(False)

        @QtCore.Slot()
        def toggle_generate_sub_brick_boundaries():
            rendering_context.set_sub_brick_boundary_indicator_creation(self.generate_sub_brick_boundaries_action.isChecked())

        self.generate_sub_brick_boundaries_action.triggered.connect(toggle_generate_sub_brick_boundaries)


class ContextStack:

    def __init__(self, window):
        self.window = window
        self.widget = self.window.findChild(QtWidgets.QStackedWidget, 'contextStackedWidget')

        self.transfer_function_page = TransferFunctionPage(self.window)


class TransferFunctionPage:

    def __init__(self, window):
        self.window = window

        self.RGBA_transfer_function_tabs = RGBATransferFunctionTabs(self.window)
        self.HSVA_transfer_function_tabs = HSVATransferFunctionTabs(self.window)
        self.limit_controller = LimitController(self.window)


class TransferFunctionTabs:

    def __init__(self, window):
        self.window = window
        self.widget = None
        self.tab_names = []
        self.canvas_widgets = {}

    def setup_matplotlib_canvas_widgets(self):
        for idx, name in enumerate(self.tab_names):
            canvas = MatplotlibCanvas(self.window)
            lower_line_edit_slider = LineEditSlider(0, 0, 1, tick_position=QtWidgets.QSlider.TicksRight,
                                                    value_update_callback=lambda value: rendering_context.update_transfer_function_lower_node_value(idx, value))

            upper_line_edit_slider = LineEditSlider(1, 0, 1, tick_position=QtWidgets.QSlider.TicksLeft,
                                                    value_update_callback=lambda value: rendering_context.update_transfer_function_upper_node_value(idx, value))
            self.widget.addTab(TransferFunctionTabLayout(canvas, lower_line_edit_slider, upper_line_edit_slider), name)
            self.canvas_widgets[name] = canvas


class RGBATransferFunctionTabs(TransferFunctionTabs):

    def __init__(self, window):
        super().__init__(window)
        self.widget = self.window.findChild(QtWidgets.QTabWidget, 'RGBATransferFunctionTabWidget')
        self.tab_names = ['Red', 'Green', 'Blue', 'Alpha']
        self.setup_matplotlib_canvas_widgets()
        self.transfer_function = RGBATransferFunction(rendering_context,
                                                      *(self.canvas_widgets[name].axes for name in self.tab_names))


class HSVATransferFunctionTabs(TransferFunctionTabs):

    def __init__(self, window):
        super().__init__(window)
        self.widget = self.window.findChild(QtWidgets.QTabWidget, 'HSVATransferFunctionTabWidget')
        self.tab_names = ['Hue', 'Saturation', 'Value', 'Alpha']
        self.setup_matplotlib_canvas_widgets()
        self.transfer_function = HSVATransferFunction(rendering_context,
                                                      *(self.canvas_widgets[name].axes for name in self.tab_names))


class TransferFunctionTabLayout(QtWidgets.QWidget):

    def __init__(self, canvas, lower_line_edit_slider, upper_line_edit_slider, *args, **kwargs):
        super().__init__(*args, **kwargs)

        layout = QtWidgets.QHBoxLayout()
        layout.addWidget(EdgeNodeValueControlLayout(lower_line_edit_slider))
        layout.addWidget(canvas)
        layout.addWidget(EdgeNodeValueControlLayout(upper_line_edit_slider))
        self.setLayout(layout)


class EdgeNodeValueControlLayout(QtWidgets.QWidget):

    def __init__(self, line_edit_slider, *args, **kwargs):
        super().__init__(*args, **kwargs)

        layout = QtWidgets.QVBoxLayout()
        slider = line_edit_slider.get_slider()
        line_edit = line_edit_slider.get_line_edit()
        layout.addWidget(slider)
        layout.addWidget(line_edit)
        layout.setAlignment(slider, QtCore.Qt.AlignHCenter)
        self.setLayout(layout)


class LimitController:

    def __init__(self, window):
        self.window = window

        lower_limit_line_edit = self.window.findChild(QtWidgets.QLineEdit, 'lowerLimitLineEdit')
        upper_limit_line_edit = self.window.findChild(QtWidgets.QLineEdit, 'upperLimitLineEdit')
        lower_limit_slider = self.window.findChild(QtWidgets.QSlider, 'lowerLimitSlider')
        upper_limit_slider = self.window.findChild(QtWidgets.QSlider, 'upperLimitSlider')

        self.lower_limit_line_edit_slider = LineEditSlider(0, 0, 1, line_edit=lower_limit_line_edit, slider=lower_limit_slider,
                                                           value_update_callback=lambda value: rendering_context.set_transfer_function_lower_limit(value))
        self.upper_limit_line_edit_slider = LineEditSlider(1, 0, 1, line_edit=upper_limit_line_edit, slider=upper_limit_slider,
                                                           value_update_callback=lambda value: rendering_context.set_transfer_function_upper_limit(value))


class LineEditSlider:

    def __init__(self,
                 initial_value,
                 min_value, max_value,
                 line_edit=None,
                 slider=None,
                 orientation=QtCore.Qt.Vertical,
                 tick_position=QtWidgets.QSlider.NoTicks,
                 tick_interval=100,
                 notation=QtGui.QDoubleValidator.ScientificNotation,
                 decimals=3,
                 slider_resolution=1000,
                 value_update_callback=lambda value: None):

        assert initial_value >= min_value and initial_value <= max_value

        self.min_value = min_value
        self.max_value = max_value
        self.decimals = decimals

        self.value_update_callback = value_update_callback

        self.min_slider_value = 1
        self.max_slider_value = slider_resolution

        self.locale = QtCore.QLocale.c()

        self.validator = QtGui.QDoubleValidator(self.min_value, self.max_value, decimals)
        self.validator.setNotation(notation)
        self.validator.setLocale(self.locale)

        self.line_edit = QtWidgets.QLineEdit() if line_edit is None else line_edit
        self.line_edit.setValidator(self.validator)

        self.slider = QtWidgets.QSlider(orientation) if slider is None else slider
        self.slider.setTickPosition(tick_position)
        self.slider.setTickInterval(tick_interval)
        self.slider.setMinimum(self.min_slider_value)
        self.slider.setMaximum(self.max_slider_value)

        self.set_value(initial_value)
        self.slider.setValue(self.slider_value_from_value(initial_value))
        self.last_valid_value = initial_value

        self.connect_update_slots()

    def set_value(self, value):
        self.line_edit.setText('{0:.{1}g}'.format(value, self.decimals))

    def set_value_range(self, min_value, max_value, decimals=3):
        self.min_value = min_value
        self.max_value = max_value
        self.validator.setRange(self.min_value, self.max_value, decimals)

    def get_line_edit(self):
        return self.line_edit

    def get_slider(self):
        return self.slider

    def get_value(self):
        value, ok = self.locale.toDouble(self.line_edit.text())
        return value if ok else None

    def update_value(self, value):
        if value != self.last_valid_value:
            self.value_update_callback(value)
        self.last_valid_value = value

    def connect_update_slots(self):

        @QtCore.Slot()
        def set_value():
            value = self.value_from_slider_value(self.slider.value())
            self.set_value(value)
            self.update_value(value)

        @QtCore.Slot()
        def set_slider_value():
            value = self.get_value()
            if value is not None:
                self.slider.setValue(self.slider_value_from_value(value))
                self.update_value(value)

        self.slider.sliderReleased.connect(set_value)
        self.line_edit.editingFinished.connect(set_slider_value)

    def to_fraction(self, value, minimum, maximum):
        return (value - minimum)/(maximum - minimum)

    def from_fraction(self, value, minimum, maximum):
        return minimum + (maximum - minimum)*value

    def slider_value_from_slider_fraction(self, slider_fraction):
        return int(round(self.from_fraction(slider_fraction, self.min_slider_value, self.max_slider_value)))

    def slider_fraction_from_slider_value(self, slider_value):
        return self.to_fraction(slider_value, self.min_slider_value, self.max_slider_value)

    def value_from_slider_fraction(self, slider_fraction):
        return self.from_fraction(slider_fraction, self.min_value, self.max_value)

    def slider_fraction_from_value(self, value):
        return self.to_fraction(value, self.min_value, self.max_value)

    def slider_value_from_value(self, value):
        return self.slider_value_from_slider_fraction(self.slider_fraction_from_value(value))

    def value_from_slider_value(self, slider_value):
        return self.value_from_slider_fraction(self.slider_fraction_from_slider_value(slider_value))


if __name__ == '__main__':

    app = QtWidgets.QApplication(sys.argv)

    mainwindow = MainWindow()
    mainwindow.show()

    rendering_context.start_renderer()

    exit_status = app.exec_()

    rendering_context.cleanup()

    sys.exit(exit_status)
