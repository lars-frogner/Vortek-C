import rendering
from PySide2 import QtCore, QtWidgets, QtGui
from ui_loader import UiLoader


class RenderingSettingsWindow(QtWidgets.QDialog):

    def __init__(self, parent_window):

        super().__init__(parent_window)
        UiLoader().loadUi('rendering_settings_window.ui', self, parent_widget=parent_window)
        #self.setParent(parent_window)


        #self.setup_generate_brick_boundaries_action()
        #self.setup_generate_sub_brick_boundaries_action()

    def setup_generate_brick_boundaries_action(self):

        self.generate_brick_boundaries_action = self.widget.addAction('Generate brick boundaries')
        self.generate_brick_boundaries_action.setCheckable(True)
        self.reset_generate_brick_boundaries_action()

        @QtCore.Slot()
        def toggle_generate_brick_boundaries():
            rendering.session.set_brick_boundary_indicator_creation(self.generate_brick_boundaries_action.isChecked())

        self.generate_brick_boundaries_action.triggered.connect(toggle_generate_brick_boundaries)

    def setup_generate_sub_brick_boundaries_action(self):

        self.generate_sub_brick_boundaries_action = self.widget.addAction('Generate sub brick boundaries')
        self.generate_sub_brick_boundaries_action.setCheckable(True)
        self.reset_generate_sub_brick_boundaries_action()

        @QtCore.Slot()
        def toggle_generate_sub_brick_boundaries():
            rendering.session.set_sub_brick_boundary_indicator_creation(self.generate_sub_brick_boundaries_action.isChecked())

        self.generate_sub_brick_boundaries_action.triggered.connect(toggle_generate_sub_brick_boundaries)

    def reset_generate_brick_boundaries_action(self):
        self.generate_brick_boundaries_action.setChecked(False)

    def reset_generate_sub_brick_boundaries_action(self):
        self.generate_sub_brick_boundaries_action.setChecked(False)
