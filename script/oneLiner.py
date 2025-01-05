#!/usr/bin/python
# -*- coding: utf-8 -*-
#---------written by:----------------------
#-------Fauzan Syabana---------------------
#------zansyabana@gmail.com----------------
#Licensed under MIT License
#------------------------------------------
#----------change by:----------------------
#------------yibai-------------------------
#--------159689757@qq.com------------------
# Change Log: 
# - support python 3.x
# - create new UI
# - fix f:/fs:/fe: can't select all
# - - + don't rename shape name
# - fix - + can't rename shadingEngine node
#------------------------------------------
# 待修复有重名对象时的问题
# 优化/h模式下的选择，会排除掉 shape 下如果父级在 selection 中的对象

import maya.cmds as cmds

# region 撤回块
# 定义一个装饰器，用来存放撤回块
def undoBlock(func):
    def wrapper(*args, **kwargs):
        # 开始撤回块
        cmds.undoInfo(openChunk=True)
        # 执行函数
        func(*args, **kwargs)
        # 结束撤回块
        cmds.undoInfo(closeChunk=True)
    return wrapper

#selector
def selector(lookName):
    nameSplit = lookName.split(':')
    method = nameSplit[0]
    nameSel = nameSplit[1]
    if method == 'f':
        sel = cmds.ls("*{}*".format(nameSel), r=True)
    elif method == 'fe':
        sel = cmds.ls("*{}".format(nameSel), r=True)
    elif method == 'fs':
        sel = cmds.ls("{}*".format(nameSel), r=True)
    Temp = []
    for child in sel:
        if cmds.nodeType(child) != 'transform' and cmds.nodeType(child) != 'joint':
            if cmds.listRelatives(child, p=True) != None:
                for i in cmds.listRelatives(child, p=True):
                    if i in sel:
                        Temp.append(child)
                        continue
    for i in Temp:
        sel.remove(i)
    cmds.select(sel, ne=True)

def getNweName(nName, method='s'):
    changeName = []
    if nName == '/':
        pass
    # get selection method
    if ':' in nName:
        selector(nName)
    elif '/s' in nName:
        method = 's'
        nName = nName.replace('/s', '')
    elif '/h' in nName:
        method = 'h'
        nName = nName.replace('/h', '')
    elif '/a' in nName:
        if '>' in nName:
            method = 'a'
            #print(method)
        nName = nName.replace('/a', '')

    if method == 's':
        slt = cmds.ls(selection=True)
    elif method == 'h':
        sltH = []
        slt = cmds.ls(selection=True)
        for i in slt:
            sltH.append(i)
            relatives,fullname = cmds.listRelatives(i, ad=True, type='transform'), cmds.listRelatives(i, ad=True, fullPath=True, type='transform')
            relatives = list(reversed(relatives))
            fullname = list(reversed(fullname))
            if relatives:
                for index, child in enumerate(relatives):
                    if len(cmds.ls(child)) > 1:
                        child = fullname[index]
                    sltH.append(child)
        slt = sltH
    elif method == 'a':
        slt = cmds.ls()

    # find numbering replacement
    def numReplace(numName, idx, start=1):
        global padding
        alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        if numName.find('//') != -1:
            start = int(numName[numName.find('//') + 2:len(numName)])
            numName = numName.replace(numName[numName.find('//'):len(numName)], '')
            #print(start)
        number = idx + start
        if numName.find('#') != -1:
            padding = numName.count('#')
            hastag = "{0:#>{1}}".format("#", padding)  # get how many '#' is in the new name
            num = "{0:0>{1}d}".format(number, padding)  # get number
            numName = numName.replace(hastag, num)
        # alphabetical numbering
        if numName.find('@') != -1:
            padding = numName.count('@')
            aSym = "{0:@>{1}}".format("@", padding) # get how many '@' is in the new name
            alphaNum = "{}".format(alpha[idx])  # get alphabetical number
            numName = numName.replace(aSym, alphaNum)

        return numName

    if not nName:
        return  slt, slt
    if nName.find(':') == -1:
        for i in slt:  # for every object in selection list
            # check if there is '>' that represents the replacement method
            if i.find('|') !=-1:
                curName = i.split('|')[-1]
            else:
                curName = i
            if nName.find('>') != -1:
                wordSplit = nName.split('>')
                oldWord = wordSplit[0]
                newWord = numReplace(wordSplit[1],slt.index(i))
                newName = i.replace(oldWord,newWord)
            # check if the first character is '-' or '+', remove character method
            elif nName[0] == '-':
                charToRemove = int(nName[1:len(nName)])
                newName = curName[0:-charToRemove]
            elif nName[0] == '+':
                charToRemove = int(nName[1:len(nName)])
                newName = curName[charToRemove:len(curName)]
            else:
                newName = numReplace(nName, slt.index(i))
                # get current Name if '!' mentioned
                #print(i)
                if newName.find('!') != -1:
                    newName = newName.replace('!', curName)
            if newName:
                if newName == '/':
                    return slt, slt
                if '|' in newName:
                    newName = newName.split('|')[-1]
                if len(cmds.ls(newName)) > 1:
                    newName = renameAddDigit(newName)
            changeName.append(newName)

        if any('|' in i for i in slt) or any('|' in i for i in changeName):
            slt.reverse()
            changeName.reverse()
    return slt, changeName

@undoBlock
def oneLiner(nName, method='s'):
    slt, newNameList = getNweName(nName, method)
    for i, newName in zip(slt, newNameList):
        try:
            cmds.rename(i, newName)
        except:
            print("{} is not renamed".format(i))
        
def newNameView(nName, method='s'):
    return getNweName(nName, method)[1]
    
def renameAddDigit(name):
    if '|' in name:
        name = name.split('|')[-1]
    digit = ''
    if name[-1].isdigit():
        while name[-1].isdigit():
            digit = digit + name[-1]
            name = name[:-1]
    else:
        digit = 0
        
    if cmds.objExists(name):
        while cmds.objExists(name + str(digit + 1)):
            digit += 1
        return name + str(digit + 1)
    else:
        return None
    
@undoBlock
def renamePastedPrefix():
    while cmds.ls("pasted__*"):
        for i in cmds.ls("pasted__*"):
            if cmds.objExists(i):
                name = i
                if 'pasted__' in i:
                    name = i.replace("pasted__", "")
                cmds.rename(i, renameAddDigit(name))
                
                    