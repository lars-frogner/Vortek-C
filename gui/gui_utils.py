from PySide2 import QtCore, QtWidgets, QtGui


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
