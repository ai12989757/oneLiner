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
import os

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

class installWindow(QWidget):
    def __init__(self, parent=maya_main_window()):
        super(installWindow, self).__init__(parent)
        self.setWindowFlags(Qt.Window)
        self.setWindowTitle('设置快捷键')
        self.setWindowFlags(Qt.Window | Qt.WindowCloseButtonHint | Qt.WindowTitleHint)
        self.resize(240, 100)
        self.layout = QVBoxLayout()
        self.setLayout(self.layout)
        self.lineEdit = QLineEdit()
        self.lineEdit.setFixedHeight(40)
        self.lineEdit.setReadOnly(True)
        self.lineEdit.setFocus()
        self.layout.addWidget(self.lineEdit)
        self.buttonLayout = QHBoxLayout()
        self.layout.addLayout(self.buttonLayout)
        self.pushButton_2 = QPushButton('确定')
        self.pushButton = QPushButton('取消')
        self.buttonLayout.addWidget(self.pushButton_2)
        self.buttonLayout.addWidget(self.pushButton)
        self.pushButton_2.clicked.connect(self.on_click)
        self.pushButton.clicked.connect(self.close)
        self.setFixedSize(self.width(), self.height())

        self.kyes = []
        self.HotKeys = ''

    # 确定按钮
    def on_click(self):
        # 删除已经存在的热键
        hotkeyPath = cmds.internalVar(userHotkeyDir=True)
        for file in os.listdir(hotkeyPath):
            file = os.path.join(hotkeyPath, file)
            with open(file,'r') as f:
                lines = f.readlines()
                for line in lines:
                    if 'oneLinerNameCommand' in line:
                        mel.eval(line.replace('oneLinerNameCommand',''))
                        #print('删除热键成功: '+ line.replace('oneLinerNameCommand',''))
        cmds.savePrefs( hotkeys=True )

        # 获取输入的快捷键                
        keyShortcut = ''
        # + 拆分字符串
        if self.HotKeys.split('+'):
            if len(self.HotKeys.split('+')[-1]) == 1:
                keyShortcut = self.HotKeys.split('+')[-1].lower()
            else:
                keyShortcut = self.HotKeys.split('+')[-1]
        else:
            keyShortcut = self.HotKeys
   
        def setOneLinerHotkey(): # 设置热键
            oneLinerCommand = mel.eval('$TempString = $oneLinerCommand')  # 获取当前命令
            if cmds.runTimeCommand('oneLiner', q=True, ex=True):
                cmds.runTimeCommand('oneLiner',e=True,delete=True)
            
            for i in  range(cmds.assignCommand(q=True, numElements=True)):
                i += 1
                if cmds.assignCommand(i, q=True, n=True) == 'oneLinerNameCommand':
                    cmds.assignCommand(e=True,delete=i)

            cmds.runTimeCommand('oneLiner',annotation='renameTool',category='Custom Scripts',commandLanguage='python',command=oneLinerCommand)
            cmds.nameCommand('oneLinerNameCommand',annotation='renameTool',sourceType='mel',command='oneLiner')
            cmds.hotkey(name='oneLinerNameCommand',keyShortcut=keyShortcut,sht=("Shift" in self.HotKeys),ctl=("Control" in self.HotKeys),alt=("Alt" in self.HotKeys))
            cmds.savePrefs( hotkeys=True )
        # 检索是否已经存在热键
        key_commandName = cmds.hotkey(keyShortcut, q=True, n=True, sht=("Shift" in self.HotKeys),ctl=("Control" in self.HotKeys),alt=("Alt" in self.HotKeys))
        if key_commandName:
            if cmds.hotkeySet(q=True,current=True) == 'Maya_Default':
                cmds.hotkeySet('OneTools', current=True)
                setOneLinerHotkey()
            else:
                # 弹出警告窗口
                result = cmds.confirmDialog(title='Warning',message='热键已存在，是否覆盖此命令 '+key_commandName+' 的快捷键',button=[u'是',u'否'],defaultButton=u'是',cancelButton=u'否',dismissString=u'否')
                #cmds.warning('热键已存在，请手动在热键编辑器里设置快捷键')
                if result == u'是':
                    setOneLinerHotkey()
        else:
            setOneLinerHotkey()

        self.close()

    def keyPressEvent(self, keyevent):
        # 返回按键的真实名称
        if keyevent.key() == Qt.Key_Enter or keyevent.key() == Qt.Key_Return or keyevent.key() == Qt.Key_Escape:
            # 关闭窗口
            self.close()
        elif keyevent.key() == Qt.Key_Shift:
            self.kyes.append("Shift")
        elif keyevent.key() == Qt.Key_Control:
            self.kyes.append("Control")
        elif keyevent.key() == Qt.Key_Alt:
            self.kyes.append("Alt")
        else:
            keyname = QKeySequence(keyevent.key()).toString()
            self.kyes.append(keyname)

        # 将keys显示在输入框中,按键名称用+分割
        self.lineEdit.setText("+".join(self.kyes))
    
    def keyReleaseEvent(self, keyevent):
        self.kyes.clear()
        self.HotKeys = self.lineEdit.text()

if __name__ == '__main__':
    #app = QApplication(sys.argv)
    win = installWindow()
    win.show()
    #sys.exit(app.exec_())
