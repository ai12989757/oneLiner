#!/usr/bin/python
# -*- coding: utf-8 -*-
try:
    from shiboken6 import wrapInstance
except ImportError:
    from shiboken2 import wrapInstance
try:
    from PySide6.QtCore import *
    from PySide6.QtGui import *
    from PySide6.QtWidgets import *
except ImportError:
    from PySide2.QtCore import *
    from PySide2.QtGui import *
    from PySide2.QtWidgets import *

import maya.OpenMayaUI as omui
import maya.cmds as cmds
import maya.mel as mel

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

class installWindow(QWidget):
    def __init__(self, parent=maya_main_window()):
        super(installWindow, self).__init__(parent)
        self.setWindowFlags(Qt.Window)
        self.setWindowTitle('设置快捷键')
        self.setWindowFlags(Qt.Window | Qt.WindowCloseButtonHint | Qt.WindowTitleHint)
        self.resize(240, 80)
        self.layout = QVBoxLayout()
        self.setLayout(self.layout)
        self.lineEdit = QLineEdit()
        self.lineEdit.setReadOnly(True)
        self.lineEdit.setFocus()
        self.layout.addWidget(self.lineEdit)
        self.buttonLayout = QHBoxLayout()
        self.layout.addLayout(self.buttonLayout)
        self.pushButton = QPushButton('取消')
        self.pushButton_2 = QPushButton('确定')
        self.buttonLayout.addWidget(self.pushButton)
        self.buttonLayout.addWidget(self.pushButton_2)
        self.pushButton_2.clicked.connect(self.on_click)
        self.pushButton.clicked.connect(self.off_click)
        self.setFixedSize(self.width(), self.height())

    # 确定按钮
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
  
        



        # 检索是否已经存在热键
        sht = ''
        ctl = ''
        alt = ''
        k = ''
        if "Shift" in HotKeys:
            sht = 'Shift'

        if "Control" in HotKeys:
            ctl = 'Ctrl+'

        if "Alt" in HotKeys:
            alt = 'Alt+'

        if sht == '' and ctl == '' and alt == '':\
            # 小写字母
            k = keyShortcut.lower()
        elif len(sht) > 0 and ctl == '' and alt == '':
            # 大写字母
            k = keyShortcut.upper()

        # # 如果快捷键已存在则删除
        # onelinerHotkey = ctl+alt+k
        # if cmds.hotkey( onelinerHotkey, query=True):
        #     evalString = 'removeHotkey("' + onelinerHotkey + '")'
        #     mel.eval(evalString)

        # 设置热键
        cmds.nameCommand('oneLinerNameCommand',annotation='renameTool',sourceType='mel',command='oneLiner')
        if cmds.hotkey(keyShortcut, q=True, n=True, sht=("Shift" in HotKeys),ctl=("Control" in HotKeys),alt=("Alt" in HotKeys)):
            cmds.warning('热键已存在，请手动在热键编辑器里设置快捷键')
        else:
            print(end='// 结果: 安装完成,快捷键为: '+HotKeys)
        cmds.hotkey(name='oneLinerNameCommand',keyShortcut=keyShortcut,sht=("Shift" in HotKeys),ctl=("Control" in HotKeys),alt=("Alt" in HotKeys))
        #cmds.savePrefs( hotkeys=True )

        # 关闭窗口
        self.close()
        

    # 取消按钮
    def off_click(self):
        self.close()
        print(end='// 提示: 请稍后在热键编辑器里设置快捷键')

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
