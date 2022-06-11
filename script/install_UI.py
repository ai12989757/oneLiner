#!/usr/bin/python
# -*- coding: utf-8 -*-
from PySide2.QtCore import *
from PySide2.QtGui import *
from PySide2.QtWidgets import *

from shiboken2 import wrapInstance
import maya.OpenMayaUI as omui
import maya.cmds as cmds
from install_main import Ui_install

def maya_main_window():
    """
    Return the Maya main window widget as a Python object
    """
    main_window_ptr = omui.MQtUtil.mainWindow()
    try:
        return wrapInstance(long(main_window_ptr), QWidget)
    except:
        return wrapInstance(int(main_window_ptr), QWidget)
    # python3.x版本已删除long() 函数, 用int替代

kyes = []
# 定义全局变量
global HotKeys

class installWindow(Ui_install, QWidget):
    def __init__(self, parent=maya_main_window()):
        super(installWindow, self).__init__(parent)
        self.setupUi(self)
        self.setWindowFlags(Qt.Window)
        self.lineEdit.setReadOnly(True)
        self.lineEdit.setFocus()
        self.pushButton_2.clicked.connect(self.on_click)
        self.pushButton.clicked.connect(self.close)
        self.setFixedSize(self.width(), self.height())
    # on_click()函数
    def on_click(self):
        global HotKeys

        # + 拆分字符串
        if HotKeys.split('+'):
            if len(HotKeys.split('+')[-1]) == 1:
                keyShortcut = HotKeys.split('+')[-1].lower()
            else:
                keyShortcut = HotKeys.split('+')[-1]
        else:
            keyShortcut = HotKeys
  
        cmds.nameCommand('oneLinerNameCommand',annotation='oneLiner',sourceType='mel',command='oneLiner')
        cmds.hotkey(name='oneLinerNameCommand',keyShortcut=keyShortcut,sht=("Shift" in HotKeys),ctl=("Control" in HotKeys),alt=("Alt" in HotKeys))
        print(end='// 结果: 安装完成,快捷键为: '+HotKeys)
        self.close()

    def keyPressEvent(self, keyevent):
        # 返回按键的真实名称
        if keyevent.key() == Qt.Key_Enter or keyevent.key() == Qt.Key_Return or keyevent.key() == Qt.Key_Escape:
            # 关闭窗口
            self.close()
        elif keyevent.key() == Qt.Key_Shift:
            kyes.append("Shift")
        elif keyevent.key() == Qt.Key_Control:
            kyes.append("Control")
        elif keyevent.key() == Qt.Key_Alt:
            kyes.append("Alt")
        else:
            keyname = QKeySequence(keyevent.key()).toString()
            kyes.append(keyname)

        # 将keys显示在输入框中,按键名称用+分割
        self.lineEdit.setText("+".join(kyes))
    
    def keyReleaseEvent(self, keyevent):
        global HotKeys
        kyes.clear()
        HotKeys = self.lineEdit.text()

if __name__ == '__main__':
    #app = QApplication(sys.argv)
    win = installWindow()
    win.show()
    #sys.exit(app.exec_())
