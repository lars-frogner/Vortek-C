from PySide2 import QtCore, QtUiTools


class UiLoader(QtUiTools.QUiLoader):
    '''
    https://stackoverflow.com/questions/27603350/how-do-i-load-children-from-ui-file-in-pyside
    '''
    _baseinstance = None
    _parent_widget = None

    def createWidget(self, classname, parent=None, name=''):
        print(name, classname, parent)
        if parent is self._parent_widget and self._baseinstance is not None:
            widget = self._baseinstance
        else:
            widget = super().createWidget(classname, parent, name)
            if self._baseinstance is not None:
                setattr(self._baseinstance, name, widget)
        return widget

    def loadUi(self, uifile, baseinstance=None, parent_widget=None):
        self._baseinstance = baseinstance
        self._parent_widget = parent_widget
        widget = self.load(uifile, parentWidget=self._parent_widget)
        QtCore.QMetaObject.connectSlotsByName(widget)
        return widget
