import rendering
from PySide2 import QtCore, QtWidgets
from gui_utils import LineEditSlider
from matplotlib_canvas import MatplotlibCanvas
from transfer_functions import RGBATransferFunction, HSVATransferFunction


class TransferFunctionPage:

    def __init__(self, window):
        self.window = window

        self.RGBA_transfer_function_tabs = TransferFunctionTabs(self.window, 'RGBA')
        self.HSVA_transfer_function_tabs = TransferFunctionTabs(self.window, 'HSVA')
        self.limit_controller = LimitController(self.window)


class TransferFunctionTabs:

    def __init__(self, window, transfer_function_type):
        self.window = window

        transfer_function_types = ['RGBA', 'HSVA']
        assert transfer_function_type in transfer_function_types

        transfer_function_classes = {'RGBA': RGBATransferFunction, 'HSVA': HSVATransferFunction}
        transfer_function_class = transfer_function_classes[transfer_function_type]

        self.widget = self.window.findChild(QtWidgets.QTabWidget, '{}TransferFunctionTabWidget'.format(transfer_function_type))

        self.tab_names = transfer_function_class.get_component_names()

        self.canvas_widgets = {name: MatplotlibCanvas(self.window) for name in self.tab_names}

        self.transfer_function = transfer_function_class(*(self.canvas_widgets[name].axes for name in self.tab_names))

        lower_slider_state = [(self.transfer_function.get_lower_value(idx),
                               self.transfer_function.get_value_lower_limit(idx),
                               self.transfer_function.get_value_upper_limit(idx))
                              for idx in range(len(self.tab_names))]

        upper_slider_state = [(self.transfer_function.get_upper_value(idx),
                               self.transfer_function.get_value_lower_limit(idx),
                               self.transfer_function.get_value_upper_limit(idx))
                              for idx in range(len(self.tab_names))]


        for idx, name in enumerate(self.tab_names):

            lower_line_edit_slider = LineEditSlider(*lower_slider_state[idx], tick_position=QtWidgets.QSlider.TicksRight,
                                                    value_update_callback=lambda value, idx=idx: self.transfer_function.set_lower_value(idx, value))

            upper_line_edit_slider = LineEditSlider(*upper_slider_state[idx], tick_position=QtWidgets.QSlider.TicksLeft,
                                                    value_update_callback=lambda value, idx=idx: self.transfer_function.set_upper_value(idx, value))

            self.widget.addTab(TransferFunctionTabLayout(self.canvas_widgets[name], lower_line_edit_slider, upper_line_edit_slider), name)


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
                                                           value_update_callback=lambda value: rendering.session.set_transfer_function_lower_limit(value))
        self.upper_limit_line_edit_slider = LineEditSlider(1, 0, 1, line_edit=upper_limit_line_edit, slider=upper_limit_slider,
                                                           value_update_callback=lambda value: rendering.session.set_transfer_function_upper_limit(value))
